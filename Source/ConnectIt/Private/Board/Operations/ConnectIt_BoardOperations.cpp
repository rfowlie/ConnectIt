// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "ConnectIt_GameplayTags.h"


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

void FConnectItBoardOperation_PlacePiece::Apply(
    FConnectItBoardState& Board,
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvent) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
        OutTouched.Add(Position);
    }

    if (OutEvent)
    {
        OutEvent->bPiecePlaced       = true;
        OutEvent->PlacedPosition     = Position;
        OutEvent->PlacingFactionSlot = Faction;
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

void FConnectItBoardOperation_SwapPieces::Apply(
    FConnectItBoardState& Board,
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvent) const
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

    if (OutEvent)
    {
        OutEvent->bPiecesSwapped = true;
        OutEvent->SwapPositionA  = PositionA;
        OutEvent->SwapPositionB  = PositionB;
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

    return true;
}

void FConnectItBoardOperation_Shift::Apply(
    FConnectItBoardState& Board,
    FConnectItTouchedPositions& OutTouched,
    FConnectItBoardChangeEvent* OutEvent) const
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
    // -- the tile itself, not just the piece on it) one step along the
    // shiftable subset: each shiftable position takes on the PREVIOUS
    // shiftable position's old data, with the last one's data wrapping around
    // to the first. Since Positions is sorted by increasing distance along
    // Direction from the selected tile, this moves every shiftable tile
    // exactly one step further in that direction. Nothing outside Positions
    // is ever touched. ShiftStartPositions/ShiftEndPositions record the same
    // pairing the rotation itself uses, for visual listeners.
    const int32 ShiftableNum = ShiftablePositions.Num();
    TArray<FGridPosition> ShiftStartPositions;
    TArray<FGridPosition> ShiftEndPositions;
    ShiftStartPositions.Reserve(ShiftableNum);
    ShiftEndPositions.Reserve(ShiftableNum);
    for (int32 Index = 0; Index < ShiftableNum; Index++)
    {
        const int32 FromIndex = (Index - 1 + ShiftableNum) % ShiftableNum;

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

    if (OutEvent)
    {
        OutEvent->bBoardShifted       = true;
        OutEvent->ShiftDirection      = Direction;
        OutEvent->ShiftAnchorPosition = Positions.IsEmpty() ? FGridPosition() : Positions[0];
        OutEvent->ShiftStartPositions = MoveTemp(ShiftStartPositions);
        OutEvent->ShiftEndPositions   = MoveTemp(ShiftEndPositions);
    }
}

FString FConnectItBoardOperation_Shift::Describe() const
{
    return FString::Printf(TEXT("%d tile(s) from %s"),
        Positions.Num(), Positions.IsEmpty() ? TEXT("(?)") : *PositionText(Positions[0]));
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
    FConnectItBoardChangeEvent* OutEvent) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
        OutTouched.Add(Position);
    }

    if (OutEvent)
    {
        OutEvent->bPiecePlaced       = true;
        OutEvent->PlacedPosition     = Position;
        OutEvent->PlacingFactionSlot = Faction;
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
    FConnectItBoardChangeEvent* OutEvent) const
{
    int32 PreviousFaction = INDEX_NONE;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        PreviousFaction = Tile->FactionPiece;
        Tile->SetFactionPiece(Faction);
        OutTouched.Add(Position);
    }

    if (OutEvent)
    {
        OutEvent->bPieceCaptured       = true;
        OutEvent->CapturedPosition     = Position;
        OutEvent->CapturingFactionSlot = Faction;
        OutEvent->PreviousFactionSlot  = PreviousFaction;
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
    FConnectItBoardChangeEvent* OutEvent) const
{
    // No piece arrives anywhere, so nothing is reported in OutTouched
    int32 RemovedFaction = INDEX_NONE;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        RemovedFaction = Tile->FactionPiece;
        Tile->SetFactionPiece(-1);
    }

    if (OutEvent)
    {
        OutEvent->bPieceRemoved      = true;
        OutEvent->RemovedPosition    = Position;
        OutEvent->RemovedFactionSlot = RemovedFaction;
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
    FConnectItBoardChangeEvent* OutEvent) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->Multiplier = 1.0f;
    }

    if (OutEvent)
    {
        OutEvent->bTileMultiplierDestroyed    = true;
        OutEvent->MultiplierDestroyedPosition = Position;
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
    FConnectItBoardChangeEvent* OutEvent) const
{
    bool bNowActive = false;
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->bIsActive = !Tile->bIsActive;
        bNowActive = Tile->bIsActive;
    }

    if (OutEvent)
    {
        OutEvent->bTileActiveToggled        = true;
        OutEvent->ToggledPosition           = Position;
        OutEvent->bToggledPositionNowActive = bNowActive;
    }
}

FString FConnectItBoardOperation_ToggleTileActive::Describe() const
{
    return PositionText(Position);
}
