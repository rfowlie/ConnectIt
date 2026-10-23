// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/UObjectIterator.h"


namespace
{
    FString PositionText(const FGridPosition& Position)
    {
        return FString::Printf(TEXT("(%d,%d)"), Position.X, Position.Y);
    }

    void SetWhyNot(FString* OutWhyNot, const FString& Reason)
    {
        if (OutWhyNot) *OutWhyNot = Reason;
    }
}

namespace
{
    // Every kind of board operation: its request type and a default instance
    // to ask type-level questions of (HasAnyLegalInstance)
    struct FOperationCatalog
    {
        TArray<FGameplayTag> RequestTypes;
        TMap<FGameplayTag, FInstancedStruct> Defaults;
    };

    const FOperationCatalog& GetOperationCatalog()
    {
        // Built on first use (game thread, after modules have loaded -- the
        // GameMode asks at match setup, before any background search).
        // Blueprint structs can't derive from a C++ struct, so the set never
        // changes at runtime; afterwards it is only read, from any thread.
        static const FOperationCatalog Catalog = []()
        {
            FOperationCatalog Found;
            const UScriptStruct* Base = FConnectItBoardOperation::StaticStruct();
            for (TObjectIterator<UScriptStruct> It; It; ++It)
            {
                if (*It == Base || !It->IsChildOf(Base)) continue;

                FInstancedStruct Instance;
                Instance.InitializeAs(*It);
                const FGameplayTag RequestType = Instance.Get<FConnectItBoardOperation>().GetRequestType();
                if (RequestType.IsValid() && !Found.Defaults.Contains(RequestType))
                {
                    Found.RequestTypes.Add(RequestType);
                    Found.Defaults.Add(RequestType, MoveTemp(Instance));
                }
            }
            return Found;
        }();
        return Catalog;
    }
}

const TArray<FGameplayTag>& FConnectItBoardOperation::GetAllRequestTypes()
{
    return GetOperationCatalog().RequestTypes;
}

EConnectItMoveLegality FConnectItBoardOperation::GetLegality(
    FGameplayTag RequestType, const FConnectItBoardState& Board, int32 ForFaction)
{
    const FInstancedStruct* Default = GetOperationCatalog().Defaults.Find(RequestType);
    const FConnectItBoardOperation* Operation = Default ? Default->GetPtr<FConnectItBoardOperation>() : nullptr;
    return Operation ? Operation->HasAnyLegalInstance(Board, ForFaction) : EConnectItMoveLegality::Unknown;
}

// ---------------------------------------------------------------------------
// Place Piece
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_PlacePiece::GetRequestType() const
{
    return ConnectIt_Game_PlacePiece;
}

bool FConnectItBoardOperation_PlacePiece::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (!IsTilePlaceableAt(Board, Position))
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s invalid for placement"), *PositionText(Position)));
        return false;
    }
    return true;
}

EConnectItMoveLegality FConnectItBoardOperation_PlacePiece::HasAnyLegalInstance(
    const FConnectItBoardState& Board, int32 ForFaction) const
{
    for (int32 TileIndex = 0; TileIndex < Board.NumTiles(); TileIndex++)
    {
        if (IsTilePlaceable(Board, TileIndex)) return EConnectItMoveLegality::Legal;
    }
    return EConnectItMoveLegality::NoLegalMove;
}

void FConnectItBoardOperation_PlacePiece::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
        OutChanges.AddEntered(Position, Faction, ArrivingPiecesSurviveScoring());
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_PiecePlaced Event;
        Event.Position = Position;
        Event.Faction  = Faction;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_PlacePiece::Describe() const
{
    return PositionText(Position);
}

// ---------------------------------------------------------------------------
// Swap Pieces
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_SwapPieces::GetRequestType() const
{
    return ConnectIt_Game_SwapPieces;
}

bool FConnectItBoardOperation_SwapPieces::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    const FConnectItTileData* DataA = Board.GetTileData(PositionA);
    const FConnectItTileData* DataB = Board.GetTileData(PositionB);

    if (!DataA || !DataB || !DataA->bIsOccupied || !DataB->bIsOccupied)
    {
        SetWhyNot(OutWhyNot, TEXT("both positions must be occupied"));
        return false;
    }

    // A trade, not an arbitrary reposition -- exactly one side must belong
    // to the requesting faction.
    const bool bOwnsA = DataA->FactionPiece == Faction;
    const bool bOwnsB = DataB->FactionPiece == Faction;
    if (bOwnsA == bOwnsB)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("faction %d must own exactly one of the two pieces"), Faction));
        return false;
    }

    return true;
}

EConnectItMoveLegality FConnectItBoardOperation_SwapPieces::HasAnyLegalInstance(
    const FConnectItBoardState& Board, int32 ForFaction) const
{
    // CanApply's rule: both tiles occupied, exactly one of them the
    // faction's -- so a piece of its own and any other occupied tile
    bool bHasOwn = false;
    bool bHasOther = false;
    for (const FConnectItTileData& Tile : Board.TileDataArray)
    {
        // A locked tile's piece can't be swapped
        if (!Tile.bIsOccupied || Tile.bIsLocked) continue;
        if (Tile.FactionPiece == ForFaction) bHasOwn = true;
        else bHasOther = true;
        if (bHasOwn && bHasOther) return EConnectItMoveLegality::Legal;
    }
    return EConnectItMoveLegality::NoLegalMove;
}

void FConnectItBoardOperation_SwapPieces::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    FConnectItTileData* DataA = Board.GetTileDataMutable(PositionA);
    FConnectItTileData* DataB = Board.GetTileDataMutable(PositionB);
    if (DataA && DataB)
    {
        const int32 PieceA = DataA->FactionPiece;
        const int32 PieceB = DataB->FactionPiece;
        DataA->SetFactionPiece(PieceB);
        DataB->SetFactionPiece(PieceA);

        // Each tile's piece left and the other's arrived. A first, then B --
        // the order the after-move step scores them in.
        OutChanges.AddExited(PositionA, PieceA);
        OutChanges.AddExited(PositionB, PieceB);
        OutChanges.AddEntered(PositionA, PieceB, ArrivingPiecesSurviveScoring());
        OutChanges.AddEntered(PositionB, PieceA, ArrivingPiecesSurviveScoring());
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_PiecesSwapped Event;
        Event.PositionA = PositionA;
        Event.PositionB = PositionB;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_SwapPieces::Describe() const
{
    return FString::Printf(TEXT("%s <-> %s"), *PositionText(PositionA), *PositionText(PositionB));
}

// ---------------------------------------------------------------------------
// Shift
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_Shift::GetRequestType() const
{
    return ConnectIt_Game_Shift;
}

bool FConnectItBoardOperation_Shift::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (Positions.Num() < 2)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("need at least 2 positions to shift, got %d"), Positions.Num()));
        return false;
    }

    // Positions is client-computed -- every one must be a tile on the
    // server's own board
    int32 NumShiftable = 0;
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Data = Board.GetTileData(Position);
        if (!Data)
        {
            SetWhyNot(OutWhyNot, FString::Printf(
                TEXT("position %s is not a registered tile"), *PositionText(Position)));
            return false;
        }
        if (Data->bCanShift) NumShiftable++;
    }

    if (NumShiftable < 2)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("fewer than 2 shiftable tiles in the line (%d of %d positions can shift)"),
            NumShiftable, Positions.Num()));
        return false;
    }

    if (Steps < 1)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("Steps must be at least 1, got %d"), Steps));
        return false;
    }

    // A whole number of laps puts every tile back where it started
    if (Steps % NumShiftable == 0)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("shifting %d step(s) along %d shiftable tile(s) is a full lap -- nothing would move"),
            Steps, NumShiftable));
        return false;
    }

    return true;
}

void FConnectItBoardOperation_Shift::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // A tile with bCanShift false is skipped -- it keeps its own data
    // untouched and takes no part in the rotation. Snapshot the shiftable
    // subset (still in line order) before writing anything, since the
    // rotation reads every entry before writing any of them.
    TArray<FGridPosition> ShiftablePositions;
    TArray<FConnectItTileData> ShiftableOldData;
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Data = Board.GetTileData(Position);
        if (Data && Data->bCanShift)
        {
            ShiftablePositions.Add(Position);
            ShiftableOldData.Add(*Data);
        }
    }

    // Rotate whole tile data (FactionPiece, Multiplier, bIsActive, bCanShift
    // -- the tile itself, not just the piece on it) Steps places along the
    // shiftable subset: each shiftable position takes on the old data of the
    // shiftable position Steps places BEFORE it, the far end's data wrapping
    // around to the near end. Since Positions is sorted by increasing
    // distance along Direction from the selected tile, this moves every
    // shiftable tile exactly Steps shiftable places further in that
    // direction. Nothing outside Positions is ever touched.
    // ShiftStartPositions/ShiftEndPositions record the same pairing the
    // rotation itself uses, for visual listeners -- a tile goes straight from
    // its start to its end, however many places that is.
    const int32 ShiftableNum = ShiftablePositions.Num();
    TArray<FGridPosition> ShiftStartPositions;
    TArray<FGridPosition> ShiftEndPositions;
    ShiftStartPositions.Reserve(ShiftableNum);
    ShiftEndPositions.Reserve(ShiftableNum);
    for (int32 Index = 0; Index < ShiftableNum; Index++)
    {
        const int32 FromIndex = ((Index - Steps) % ShiftableNum + ShiftableNum) % ShiftableNum;

        Board.SetTileData(ShiftablePositions[Index], ShiftableOldData[FromIndex]);
        ShiftStartPositions.Add(ShiftablePositions[FromIndex]);
        ShiftEndPositions.Add(ShiftablePositions[Index]);
    }

    // Any position in the line that now holds a piece might complete a line.
    // Moved, not entered: each piece travelled with its tile (and the tile's
    // power, if it has one), so no piece arrived on a tile.
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Data = Board.GetTileData(Position);
        if (Data && Data->bIsOccupied)
        {
            OutChanges.AddMoved(Position, Data->FactionPiece, ArrivingPiecesSurviveScoring());
        }
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_BoardShifted Event;
        Event.Direction      = Direction;
        Event.AnchorPosition = Positions.IsEmpty() ? FGridPosition() : Positions[0];
        Event.Steps          = Steps;
        Event.StartPositions = MoveTemp(ShiftStartPositions);
        Event.EndPositions   = MoveTemp(ShiftEndPositions);
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_Shift::Describe() const
{
    return FString::Printf(TEXT("%d tile(s) from %s, %d step(s)"),
        Positions.Num(), Positions.IsEmpty() ? TEXT("(?)") : *PositionText(Positions[0]), Steps);
}

// ---------------------------------------------------------------------------
// Force Place Piece
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_ForcePlacePiece::GetRequestType() const
{
    return ConnectIt_Game_ForcePlacePiece;
}

bool FConnectItBoardOperation_ForcePlacePiece::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    // Only requires the position to exist -- deliberately skips the Place
    // Piece check (active/unoccupied). Existence is still checked so a
    // garbage position can't silently grow the tile arrays.
    if (!Board.GetTileData(Position))
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s not registered"), *PositionText(Position)));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_ForcePlacePiece::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
        OutChanges.AddEntered(Position, Faction, ArrivingPiecesSurviveScoring());
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_PiecePlaced Event;
        Event.Position = Position;
        Event.Faction  = Faction;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_ForcePlacePiece::Describe() const
{
    return PositionText(Position);
}

// ---------------------------------------------------------------------------
// Capture Piece
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_CapturePiece::GetRequestType() const
{
    return ConnectIt_Game_CapturePiece;
}

bool FConnectItBoardOperation_CapturePiece::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    const FConnectItTileData* Existing = Board.GetTileData(Position);
    if (!Existing || !Existing->bIsOccupied)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s is not occupied"), *PositionText(Position)));
        return false;
    }

    if (Existing->FactionPiece == Faction)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s already belongs to faction %d"), *PositionText(Position), Faction));
        return false;
    }

    return true;
}

void FConnectItBoardOperation_CapturePiece::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    int32 PreviousFaction = INDEX_NONE;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        PreviousFaction = Tile->FactionPiece;
        Tile->SetFactionPiece(Faction);
        OutChanges.AddPieceChanged(Position, Faction, PreviousFaction, ArrivingPiecesSurviveScoring());
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_PieceCaptured Event;
        Event.Position         = Position;
        Event.CapturingFaction = Faction;
        Event.PreviousFaction  = PreviousFaction;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_CapturePiece::Describe() const
{
    return PositionText(Position);
}

// ---------------------------------------------------------------------------
// Remove Piece
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_RemovePiece::GetRequestType() const
{
    return ConnectIt_Game_RemovePiece;
}

bool FConnectItBoardOperation_RemovePiece::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (DelayTurns > 0)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("DelayTurns %d not supported yet (no per-turn scheduling mechanism "
                 "exists), only immediate (0) removal is handled"), DelayTurns));
        return false;
    }

    const FConnectItTileData* Existing = Board.GetTileData(Position);
    if (!Existing || !Existing->bIsOccupied)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s is not occupied"), *PositionText(Position)));
        return false;
    }

    return true;
}

void FConnectItBoardOperation_RemovePiece::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // A piece leaves; nothing arrives, so nothing scores
    int32 RemovedFaction = INDEX_NONE;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        RemovedFaction = Tile->FactionPiece;
        Tile->SetFactionPiece(-1);
        OutChanges.AddExited(Position, RemovedFaction);
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_PieceRemoved Event;
        Event.Position       = Position;
        Event.RemovedFaction = RemovedFaction;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_RemovePiece::Describe() const
{
    return PositionText(Position);
}

// ---------------------------------------------------------------------------
// Destroy Tile Multiplier
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_DestroyTileMultiplier::GetRequestType() const
{
    return ConnectIt_Game_DestroyTileMultiplier;
}

bool FConnectItBoardOperation_DestroyTileMultiplier::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    const FConnectItTileData* Existing = Board.GetTileData(Position);
    if (!Existing)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s not registered"), *PositionText(Position)));
        return false;
    }

    if (Existing->Multiplier == 1.0f)
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s has no multiplier to destroy"), *PositionText(Position)));
        return false;
    }

    return true;
}

void FConnectItBoardOperation_DestroyTileMultiplier::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    float PreviousMultiplier = 1.0f;
    float NewMultiplier = 1.0f;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        PreviousMultiplier = Tile->Multiplier;
        NewMultiplier = OutChanges.ClampMultiplier(*Tile, 1.0f);
        Tile->Multiplier = NewMultiplier;
        OutChanges.AddTileChanged(Position, EConnectItTileChange::Multiplier, Faction);
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_TileMultiplierDestroyed Event;
        Event.Position           = Position;
        Event.PreviousMultiplier = PreviousMultiplier;
        Event.NewMultiplier      = NewMultiplier;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_DestroyTileMultiplier::Describe() const
{
    return PositionText(Position);
}

// ---------------------------------------------------------------------------
// Toggle Tile Active
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_ToggleTileActive::GetRequestType() const
{
    return ConnectIt_Game_ToggleTileActive;
}

bool FConnectItBoardOperation_ToggleTileActive::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (!Board.GetTileData(Position))
    {
        SetWhyNot(OutWhyNot, FString::Printf(
            TEXT("position %s not registered"), *PositionText(Position)));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_ToggleTileActive::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    bool bNowActive = false;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->bIsActive = !Tile->bIsActive;
        bNowActive = Tile->bIsActive;
        OutChanges.AddTileChanged(Position, EConnectItTileChange::Active, Faction);
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_TileActiveToggled Event;
        Event.Position   = Position;
        Event.bNowActive = bNowActive;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_ToggleTileActive::Describe() const
{
    return PositionText(Position);
}

// ---------------------------------------------------------------------------
// Change Goals
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_ChangeGoals::GetRequestType() const
{
    return ConnectIt_Game_ChangeGoals;
}

bool FConnectItBoardOperation_ChangeGoals::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (Factions.IsEmpty())
    {
        SetWhyNot(OutWhyNot, TEXT("no factions to change the goals of"));
        return false;
    }
    for (const int32 GoalFaction : Factions)
    {
        if (!Board.ScoreBoard.IsValidIndex(GoalFaction))
        {
            SetWhyNot(OutWhyNot, FString::Printf(
                TEXT("faction %d is not on this board (%d factions)"), GoalFaction, Board.ScoreBoard.Num()));
            return false;
        }
    }
    return true;
}

void FConnectItBoardOperation_ChangeGoals::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // New win goals start from the first; lose conditions keep no progress
    if (bWinGoals)
    {
        Board.GroupsCompleted.SetNumZeroed(Board.ScoreBoard.Num());
        for (const int32 GoalFaction : Factions)
        {
            if (Board.GroupsCompleted.IsValidIndex(GoalFaction))
            {
                Board.GroupsCompleted[GoalFaction] = 0;
            }
        }
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_GoalsChanged Event;
        Event.Factions   = Factions;
        Event.bWinGoals  = bWinGoals;
        Event.GroupNames = GroupNames;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_ChangeGoals::Describe() const
{
    FString FactionText;
    for (const int32 GoalFaction : Factions)
    {
        FactionText += FString::Printf(TEXT("%s%d"), FactionText.IsEmpty() ? TEXT("") : TEXT(","), GoalFaction);
    }
    FString Names;
    for (const FName& Name : GroupNames)
    {
        Names += (Names.IsEmpty() ? TEXT("") : TEXT(", ")) + Name.ToString();
    }
    return FString::Printf(TEXT("%s for faction(s) %s: %s"),
        bWinGoals ? TEXT("win goals") : TEXT("lose conditions"), *FactionText,
        Names.IsEmpty() ? TEXT("none") : *Names);
}

// ---------------------------------------------------------------------------
// Begin Phase
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_BeginPhase::GetRequestType() const
{
    return ConnectIt_Game_BeginPhase;
}

bool FConnectItBoardOperation_BeginPhase::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (!Phase.IsValid())
    {
        SetWhyNot(OutWhyNot, TEXT("no phase given"));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_BeginPhase::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    const FGameplayTag Previous = Board.CurrentPhase;
    Board.CurrentPhase = Phase;

    if (OutEvents)
    {
        FConnectItBoardEvent_PhaseChanged Event;
        Event.PreviousPhase = Previous;
        Event.Phase         = Phase;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_BeginPhase::Describe() const
{
    return Phase.ToString();
}

// ---------------------------------------------------------------------------
// Remove Tiles / Add Tiles / Change Layout
// ---------------------------------------------------------------------------

namespace
{
    // Whether every position is given only once; OutRepeat gets the first one that isn't
    bool AllDifferent(TConstArrayView<FGridPosition> Positions, FGridPosition& OutRepeat)
    {
        TSet<FGridPosition> Seen;
        for (const FGridPosition& Position : Positions)
        {
            bool bSeen = false;
            Seen.Add(Position, &bSeen);
            if (bSeen)
            {
                OutRepeat = Position;
                return false;
            }
        }
        return true;
    }

    TArray<FGridPosition> CellPositions(TConstArrayView<FConnectItBoardLayoutCell> Cells)
    {
        TArray<FGridPosition> Positions;
        Positions.Reserve(Cells.Num());
        for (const FConnectItBoardLayoutCell& Cell : Cells)
        {
            Positions.Add(Cell.Position);
        }
        return Positions;
    }

    // Removal checks: every position is a tile, given once
    bool CanRemoveTiles(const FConnectItBoardState& Board, TConstArrayView<FGridPosition> Positions, FString* OutWhyNot)
    {
        FGridPosition Repeat;
        if (!AllDifferent(Positions, Repeat))
        {
            SetWhyNot(OutWhyNot, FString::Printf(TEXT("position %s is removed twice"), *PositionText(Repeat)));
            return false;
        }
        for (const FGridPosition& Position : Positions)
        {
            if (!Board.GetTileData(Position))
            {
                SetWhyNot(OutWhyNot, FString::Printf(TEXT("position %s is not a tile"), *PositionText(Position)));
                return false;
            }
        }
        return true;
    }

    // Addition checks: every cell's position is free (or about to be freed by
    // Removing), given once
    bool CanAddTiles(const FConnectItBoardState& Board, TConstArrayView<FConnectItBoardLayoutCell> Cells,
        TConstArrayView<FGridPosition> Removing, FString* OutWhyNot)
    {
        FGridPosition Repeat;
        if (!AllDifferent(CellPositions(Cells), Repeat))
        {
            SetWhyNot(OutWhyNot, FString::Printf(TEXT("position %s is added twice"), *PositionText(Repeat)));
            return false;
        }
        for (const FConnectItBoardLayoutCell& Cell : Cells)
        {
            if (Board.GetTileData(Cell.Position) && !Removing.Contains(Cell.Position))
            {
                SetWhyNot(OutWhyNot, FString::Printf(TEXT("position %s is already a tile"), *PositionText(Cell.Position)));
                return false;
            }
        }
        return true;
    }

    // Takes the tiles off, recording any piece that went with each
    void RemoveTilesFrom(FConnectItBoardState& Board, TConstArrayView<FGridPosition> Positions,
        FConnectItBoardEvent_TilesRemoved& OutEvent)
    {
        for (const FGridPosition& Position : Positions)
        {
            const FConnectItTileData* Tile = Board.GetTileData(Position);
            const int32 PieceFaction = Tile ? Tile->FactionPiece : INDEX_NONE;
            if (Board.RemoveTileData(Position))
            {
                OutEvent.Positions.Add(Position);
                OutEvent.PieceFactions.Add(PieceFaction);
            }
        }
    }

    // Puts the tiles on, each with a new tile id and its cell's power tag (if any).
    // A starting piece is scored (reported as moved: a tile
    // appearing sets off no Piece Enter) and counts as one of its faction's
    // starting pieces, like a designer-placed piece at the start of the match
    // (piece-count goals measure from those).
    void AddTilesTo(FConnectItBoardState& Board, TConstArrayView<FConnectItBoardLayoutCell> Cells,
        FConnectItBoardChanges& OutChanges, FConnectItBoardEvent_TilesAdded& OutEvent)
    {
        const int32 NumFactions = Board.ScoreBoard.Num();
        for (const FConnectItBoardLayoutCell& Cell : Cells)
        {
            if (Board.GetTileData(Cell.Position)) continue;

            // Its starting multiplier within its type's limits -- and the event
            // carries the clamped value, so replaying it adds the same tile
            FConnectItBoardLayoutCell Added = Cell;
            FConnectItTileData TileData = Cell.TileData.ToTileData(NumFactions);
            TileData.Multiplier = OutChanges.ClampMultiplier(TileData, TileData.Multiplier);
            Added.TileData.Multiplier = TileData.Multiplier;

            // A Turn Start power's countdown starts now
            const FConnectItTilePower* Power = OutChanges.Rules ? OutChanges.Rules->FindTilePower(TileData.TilePowerTag) : nullptr;
            if (Power && Power->HasTrigger(EConnectItPowerTrigger::TurnStart))
            {
                TileData.PowerReadyTurn = Board.TurnNumber + Power->CooldownTurns;
            }

            const int32 TileId = Board.AddTile(Cell.Position, TileData);
            if (Cell.TileData.HasStartingPiece(NumFactions))
            {
                OutChanges.AddMoved(Cell.Position, Cell.TileData.StartingFactionPiece, true);
                if (Board.StartingPieces.IsValidIndex(Cell.TileData.StartingFactionPiece))
                {
                    Board.StartingPieces[Cell.TileData.StartingFactionPiece]++;
                }
            }
            OutEvent.Cells.Add(Added);
            OutEvent.TileIds.Add(TileId);
            OutEvent.PowerReadyTurns.Add(TileData.PowerReadyTurn);
        }

        // Partners once every cell is a tile (a partner can be one added alongside)
        for (const FConnectItBoardLayoutCell& Cell : OutEvent.Cells)
        {
            if (Cell.TileData.bHasChainPartner)
            {
                Board.LinkChainPartner(Cell.Position, true, Cell.TileData.ChainPartner);
            }
        }
    }

    FString PositionsText(TConstArrayView<FGridPosition> Positions)
    {
        FString Text;
        for (const FGridPosition& Position : Positions)
        {
            Text += (Text.IsEmpty() ? TEXT("") : TEXT(" ")) + PositionText(Position);
        }
        return Text.IsEmpty() ? TEXT("none") : Text;
    }
}

FGameplayTag FConnectItBoardOperation_RemoveTiles::GetRequestType() const
{
    return ConnectIt_Game_RemoveTiles;
}

bool FConnectItBoardOperation_RemoveTiles::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (Positions.IsEmpty())
    {
        SetWhyNot(OutWhyNot, TEXT("no tiles to remove"));
        return false;
    }
    return CanRemoveTiles(Board, Positions, OutWhyNot);
}

void FConnectItBoardOperation_RemoveTiles::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    FConnectItBoardEvent_TilesRemoved Event;
    RemoveTilesFrom(Board, Positions, Event);
    if (OutEvents)
    {
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_RemoveTiles::Describe() const
{
    return PositionsText(Positions);
}

FGameplayTag FConnectItBoardOperation_AddTiles::GetRequestType() const
{
    return ConnectIt_Game_AddTiles;
}

bool FConnectItBoardOperation_AddTiles::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (Cells.IsEmpty())
    {
        SetWhyNot(OutWhyNot, TEXT("no tiles to add"));
        return false;
    }
    return CanAddTiles(Board, Cells, {}, OutWhyNot);
}

void FConnectItBoardOperation_AddTiles::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    FConnectItBoardEvent_TilesAdded Event;
    AddTilesTo(Board, Cells, OutChanges, Event);
    if (OutEvents)
    {
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_AddTiles::Describe() const
{
    return PositionsText(CellPositions(Cells));
}

void FConnectItBoardOperation_ChangeLayout::SetDiff(const FConnectItBoardLayoutDiff& Diff)
{
    RemovePositions = Diff.Removed;
    AddCells = Diff.Added;
}

FGameplayTag FConnectItBoardOperation_ChangeLayout::GetRequestType() const
{
    return ConnectIt_Game_ChangeLayout;
}

bool FConnectItBoardOperation_ChangeLayout::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    // An empty change is allowed: the layout may only move the camera
    return CanRemoveTiles(Board, RemovePositions, OutWhyNot)
        && CanAddTiles(Board, AddCells, RemovePositions, OutWhyNot);
}

void FConnectItBoardOperation_ChangeLayout::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    FConnectItBoardEvent_TilesRemoved Removed;
    RemoveTilesFrom(Board, RemovePositions, Removed);

    FConnectItBoardEvent_TilesAdded Added;
    AddTilesTo(Board, AddCells, OutChanges, Added);

    if (OutEvents)
    {
        if (!Removed.Positions.IsEmpty()) OutEvents->Add(Removed);
        if (!Added.Cells.IsEmpty()) OutEvents->Add(Added);

        FConnectItBoardEvent_LayoutChanged Changed;
        Changed.LayoutName      = LayoutName;
        Changed.bHasCamera      = bHasCamera;
        Changed.CameraTransform = CameraTransform;
        OutEvents->Add(Changed);
    }
}

FString FConnectItBoardOperation_ChangeLayout::Describe() const
{
    return FString::Printf(TEXT("'%s': -%d tile(s), +%d tile(s)"),
        *LayoutName.ToString(), RemovePositions.Num(), AddCells.Num());
}

// ---------------------------------------------------------------------------
// Set Tile Locked
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_SetTileLocked::GetRequestType() const
{
    return ConnectIt_Game_SetTileLocked;
}

bool FConnectItBoardOperation_SetTileLocked::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    const FConnectItTileData* Tile = Board.GetTileData(Position);
    if (!Tile)
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("position %s not registered"), *PositionText(Position)));
        return false;
    }
    if (Tile->bIsLocked == bLocked)
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("tile %s is already %s"), *PositionText(Position),
            bLocked ? TEXT("locked") : TEXT("unlocked")));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_SetTileLocked::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->bIsLocked = bLocked;
        OutChanges.AddTileChanged(Position, EConnectItTileChange::Lock, Faction);
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_TileLockChanged Event;
        Event.Position = Position;
        Event.bLocked  = bLocked;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_SetTileLocked::Describe() const
{
    return FString::Printf(TEXT("%s %s"), *PositionText(Position), bLocked ? TEXT("locked") : TEXT("unlocked"));
}

// ---------------------------------------------------------------------------
// Set Chain Partner
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_SetChainPartner::GetRequestType() const
{
    return ConnectIt_Game_SetChainPartner;
}

bool FConnectItBoardOperation_SetChainPartner::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    const FConnectItTileData* Tile = Board.GetTileData(Position);
    if (!Tile || Tile->TilePowerTag != ConnectIt_TilePower_Chained)
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("%s is not a Chained tile"), *PositionText(Position)));
        return false;
    }
    if (bHasPartner && (PartnerPosition == Position || !Board.GetTileData(PartnerPosition)))
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("partner %s is not another tile"), *PositionText(PartnerPosition)));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_SetChainPartner::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    Board.LinkChainPartner(Position, bHasPartner, PartnerPosition);
    const FConnectItTileData* Tile = Board.GetTileData(Position);
    OutChanges.AddTileChanged(Position, EConnectItTileChange::Power, Faction);

    if (OutEvents)
    {
        FConnectItBoardEvent_ChainPartnerChanged Event;
        Event.Position        = Position;
        Event.bHasPartner     = bHasPartner;
        Event.PartnerPosition = PartnerPosition;
        Event.PartnerId       = Tile ? Tile->ChainPartnerId : INDEX_NONE;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_SetChainPartner::Describe() const
{
    return bHasPartner
        ? FString::Printf(TEXT("%s -> %s"), *PositionText(Position), *PositionText(PartnerPosition))
        : FString::Printf(TEXT("%s -> no partner"), *PositionText(Position));
}

// ---------------------------------------------------------------------------
// Move Piece
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_MovePiece::GetRequestType() const
{
    return ConnectIt_Game_MovePiece;
}

bool FConnectItBoardOperation_MovePiece::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    const FConnectItTileData* FromTile = Board.GetTileData(From);
    if (!FromTile || !FromTile->bIsOccupied)
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("%s has no piece to move"), *PositionText(From)));
        return false;
    }
    const FConnectItTileData* ToTile = Board.GetTileData(To);
    if (!ToTile || From == To || !ToTile->bIsActive || ToTile->bIsOccupied)
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("%s isn't an empty, active tile to move to"), *PositionText(To)));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_MovePiece::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    FConnectItTileData* FromTile = Board.GetTileDataMutable(From);
    FConnectItTileData* ToTile = Board.GetTileDataMutable(To);
    if (!FromTile || !ToTile) return;

    // The piece keeps its owner
    const int32 PieceFaction = FromTile->FactionPiece;
    FromTile->SetFactionPiece(-1);
    ToTile->SetFactionPiece(PieceFaction);
    OutChanges.AddExited(From, PieceFaction);
    OutChanges.AddEntered(To, PieceFaction, ArrivingPiecesSurviveScoring());

    if (OutEvents)
    {
        FConnectItBoardEvent_PieceMoved Event;
        Event.From    = From;
        Event.To      = To;
        Event.Faction = PieceFaction;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_MovePiece::Describe() const
{
    return FString::Printf(TEXT("%s -> %s"), *PositionText(From), *PositionText(To));
}

// ---------------------------------------------------------------------------
// Adjust Multiplier
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_AdjustMultiplier::GetRequestType() const
{
    return ConnectIt_Game_AdjustMultiplier;
}

bool FConnectItBoardOperation_AdjustMultiplier::CanApply(const FConnectItBoardState& Board, FString* OutWhyNot) const
{
    if (!Board.GetTileData(Position))
    {
        SetWhyNot(OutWhyNot, FString::Printf(TEXT("position %s not registered"), *PositionText(Position)));
        return false;
    }
    if (FMath::IsNearlyZero(Delta))
    {
        SetWhyNot(OutWhyNot, TEXT("a change of 0"));
        return false;
    }
    return true;
}

void FConnectItBoardOperation_AdjustMultiplier::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    FConnectItTileData* Tile = Board.GetTileDataMutable(Position);
    if (!Tile) return;

    // Within the tile type's limits (it may end up unchanged)
    const float Previous = Tile->Multiplier;
    Tile->Multiplier = OutChanges.ClampMultiplier(*Tile, Previous + Delta);
    if (Tile->Multiplier != Previous)
    {
        OutChanges.AddTileChanged(Position, EConnectItTileChange::Multiplier, Faction);
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_MultiplierChanged Event;
        Event.Position           = Position;
        Event.PreviousMultiplier = Previous;
        Event.NewMultiplier      = Tile->Multiplier;
        OutEvents->Add(Event);
    }
}

FString FConnectItBoardOperation_AdjustMultiplier::Describe() const
{
    return FString::Printf(TEXT("%s %+.1f"), *PositionText(Position), Delta);
}

// ---------------------------------------------------------------------------
// Turn Start
// ---------------------------------------------------------------------------

FGameplayTag FConnectItBoardOperation_TurnStart::GetRequestType() const
{
    return ConnectIt_Game_TurnStart;
}

void FConnectItBoardOperation_TurnStart::Apply(
    FConnectItBoardState& Board,
    FConnectItBoardChanges& OutChanges,
    FConnectItBoardChangeEvent* OutEvents) const
{
    OutChanges.bTurnStarted = true;
}
