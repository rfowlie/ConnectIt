// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Operations/ConnectIt_BoardOperations.h"
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
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvents) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
        OutTouched.Add(Position);
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
        if (!Tile.bIsOccupied) continue;
        if (Tile.FactionPiece == ForFaction) bHasOwn = true;
        else bHasOther = true;
        if (bHasOwn && bHasOther) return EConnectItMoveLegality::Legal;
    }
    return EConnectItMoveLegality::NoLegalMove;
}

void FConnectItBoardOperation_SwapPieces::Apply(
    FConnectItBoardState& Board,
    FConnectItTouchedPositions& OutTouched,
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

        // A first, then B -- the order the after-move step scores them in
        OutTouched.Add(PositionA);
        OutTouched.Add(PositionB);
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
    FConnectItTouchedPositions& OutTouched,
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

    // Any position in the line that now holds a piece might complete a line
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Data = Board.GetTileData(Position);
        if (Data && Data->bIsOccupied)
        {
            OutTouched.Add(Position);
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
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvents) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
        OutTouched.Add(Position);
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
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvents) const
{
    int32 PreviousFaction = INDEX_NONE;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        PreviousFaction = Tile->FactionPiece;
        Tile->SetFactionPiece(Faction);
        OutTouched.Add(Position);
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
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // No piece arrives anywhere, so nothing is reported in OutTouched
    int32 RemovedFaction = INDEX_NONE;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        RemovedFaction = Tile->FactionPiece;
        Tile->SetFactionPiece(-1);
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
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvents) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->Multiplier = 1.0f;
    }

    if (OutEvents)
    {
        FConnectItBoardEvent_TileMultiplierDestroyed Event;
        Event.Position = Position;
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
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvents) const
{
    bool bNowActive = false;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->bIsActive = !Tile->bIsActive;
        bNowActive = Tile->bIsActive;
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
    FConnectItTouchedPositions& OutTouched,
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
