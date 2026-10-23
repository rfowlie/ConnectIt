// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_GameplayTags.h"


FGameplayTag FConnectItBoardEvent_BoardSeeded::GetEventTag() const
{
    return ConnectIt_Event_BoardSeeded;
}

FGameplayTag FConnectItBoardEvent_PiecePlaced::GetEventTag() const
{
    return ConnectIt_Event_PiecePlaced;
}

FGameplayTag FConnectItBoardEvent_PiecesSwapped::GetEventTag() const
{
    return ConnectIt_Event_PiecesSwapped;
}

FGameplayTag FConnectItBoardEvent_BoardShifted::GetEventTag() const
{
    return ConnectIt_Event_BoardShifted;
}

FGameplayTag FConnectItBoardEvent_PieceCaptured::GetEventTag() const
{
    return ConnectIt_Event_PieceCaptured;
}

FGameplayTag FConnectItBoardEvent_PieceRemoved::GetEventTag() const
{
    return ConnectIt_Event_PieceRemoved;
}

FGameplayTag FConnectItBoardEvent_TileMultiplierDestroyed::GetEventTag() const
{
    return ConnectIt_Event_TileMultiplierDestroyed;
}

FGameplayTag FConnectItBoardEvent_TileActiveToggled::GetEventTag() const
{
    return ConnectIt_Event_TileActiveToggled;
}

FGameplayTag FConnectItBoardEvent_GoalsChanged::GetEventTag() const
{
    return ConnectIt_Event_GoalsChanged;
}

FGameplayTag FConnectItBoardEvent_TilesRemoved::GetEventTag() const
{
    return ConnectIt_Event_TilesRemoved;
}

FGameplayTag FConnectItBoardEvent_TilesAdded::GetEventTag() const
{
    return ConnectIt_Event_TilesAdded;
}

FGameplayTag FConnectItBoardEvent_PhaseChanged::GetEventTag() const
{
    return ConnectIt_Event_PhaseChanged;
}

FGameplayTag FConnectItBoardEvent_LayoutChanged::GetEventTag() const
{
    return ConnectIt_Event_LayoutChanged;
}

FGameplayTag FConnectItBoardEvent_Scored::GetEventTag() const
{
    return ConnectIt_Event_Scored;
}

FGameplayTag FConnectItBoardEvent_WinCheckpointReached::GetEventTag() const
{
    return ConnectIt_Event_WinCheckpointReached;
}

FGameplayTag FConnectItBoardEvent_LoseConditionMet::GetEventTag() const
{
    return ConnectIt_Event_LoseConditionMet;
}

FGameplayTag FConnectItBoardEvent_GameWon::GetEventTag() const
{
    return ConnectIt_Event_PlayerWin;
}

// ---------------------------------------------------------------------------
// Applying events -- each makes its own change from its own data
// ---------------------------------------------------------------------------

void FConnectItBoardEvent::ApplyAll(const FConnectItBoardChangeEvent& Events, FConnectItBoardState& Board)
{
    for (const FInstancedStruct& Entry : Events.Events)
    {
        if (const FConnectItBoardEvent* Event = Entry.GetPtr<FConnectItBoardEvent>())
        {
            Event->ApplyTo(Board);
        }
    }
}

void FConnectItBoardEvent_BoardSeeded::ApplyTo(FConnectItBoardState& Board) const
{
    // The seed is the starting board itself: nothing to change
}

void FConnectItBoardEvent_PiecePlaced::ApplyTo(FConnectItBoardState& Board) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(Faction);
    }
}

void FConnectItBoardEvent_PiecesSwapped::ApplyTo(FConnectItBoardState& Board) const
{
    FConnectItTileData* TileA = Board.GetTileDataMutable(PositionA);
    FConnectItTileData* TileB = Board.GetTileDataMutable(PositionB);
    if (TileA && TileB)
    {
        const int32 PieceA = TileA->FactionPiece;
        TileA->SetFactionPiece(TileB->FactionPiece);
        TileB->SetFactionPiece(PieceA);
    }
}

void FConnectItBoardEvent_BoardShifted::ApplyTo(FConnectItBoardState& Board) const
{
    // A rotation: read every moving tile before writing any
    TArray<FConnectItTileData> Moving;
    Moving.Reserve(StartPositions.Num());
    for (const FGridPosition& Start : StartPositions)
    {
        const FConnectItTileData* Tile = Board.GetTileData(Start);
        Moving.Add(Tile ? *Tile : FConnectItTileData());
    }
    for (int32 Index = 0; Index < EndPositions.Num() && Index < Moving.Num(); Index++)
    {
        Board.SetTileData(EndPositions[Index], Moving[Index]);
    }
}

void FConnectItBoardEvent_PieceCaptured::ApplyTo(FConnectItBoardState& Board) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(CapturingFaction);
    }
}

void FConnectItBoardEvent_PieceRemoved::ApplyTo(FConnectItBoardState& Board) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->SetFactionPiece(-1);
    }
}

void FConnectItBoardEvent_TileMultiplierDestroyed::ApplyTo(FConnectItBoardState& Board) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->Multiplier = 1.f;
    }
}

void FConnectItBoardEvent_TileActiveToggled::ApplyTo(FConnectItBoardState& Board) const
{
    if (FConnectItTileData* Tile = Board.GetTileDataMutable(Position))
    {
        Tile->bIsActive = bNowActive;
    }
}

void FConnectItBoardEvent_GoalsChanged::ApplyTo(FConnectItBoardState& Board) const
{
    if (!bWinGoals) return;

    Board.GroupsCompleted.SetNumZeroed(Board.ScoreBoard.Num());
    for (const int32 GoalFaction : Factions)
    {
        if (Board.GroupsCompleted.IsValidIndex(GoalFaction))
        {
            Board.GroupsCompleted[GoalFaction] = 0;
        }
    }
}

void FConnectItBoardEvent_TilesRemoved::ApplyTo(FConnectItBoardState& Board) const
{
    for (const FGridPosition& Position : Positions)
    {
        Board.RemoveTileData(Position);
    }
}

void FConnectItBoardEvent_TilesAdded::ApplyTo(FConnectItBoardState& Board) const
{
    const int32 NumFactions = Board.ScoreBoard.Num();
    for (int32 Index = 0; Index < Cells.Num(); Index++)
    {
        const FConnectItBoardLayoutCell& Cell = Cells[Index];
        if (Board.GetTileData(Cell.Position)) continue;

        // The id it was given (a board that hasn't got that far gives the next)
        if (TileIds.IsValidIndex(Index))
        {
            FConnectItTileData Tile = Cell.ToTileData(NumFactions);
            Tile.TileId = TileIds[Index];
            Board.SetTileData(Cell.Position, Tile);
            Board.NextTileId = FMath::Max(Board.NextTileId, Tile.TileId + 1);
        }
        else
        {
            Board.AddTile(Cell.Position, Cell.ToTileData(NumFactions));
        }
        if (Cell.TileData.HasStartingPiece(NumFactions)
            && Board.StartingPieces.IsValidIndex(Cell.TileData.StartingFactionPiece))
        {
            Board.StartingPieces[Cell.TileData.StartingFactionPiece]++;
        }
    }
}

void FConnectItBoardEvent_PhaseChanged::ApplyTo(FConnectItBoardState& Board) const
{
    Board.CurrentPhase = Phase;
}

void FConnectItBoardEvent_LayoutChanged::ApplyTo(FConnectItBoardState& Board) const
{
    // Marks the new layout (its tiles came in Tiles Removed / Tiles Added)
}

void FConnectItBoardEvent_Scored::ApplyTo(FConnectItBoardState& Board) const
{
    if (Board.ScoreBoard.IsValidIndex(Faction))
    {
        Board.ScoreBoard[Faction] += Points;
    }
    for (int32 Index = 0; Index < Positions.Num() && Index < NewMultipliers.Num(); Index++)
    {
        if (FConnectItTileData* Tile = Board.GetTileDataMutable(Positions[Index]))
        {
            Tile->Multiplier = NewMultipliers[Index];
        }
    }
    for (const FGridPosition& Cleared : ClearedPositions)
    {
        if (FConnectItTileData* Tile = Board.GetTileDataMutable(Cleared))
        {
            Tile->SetFactionPiece(-1);
        }
    }
}

void FConnectItBoardEvent_WinCheckpointReached::ApplyTo(FConnectItBoardState& Board) const
{
    Board.GroupsCompleted.SetNumZeroed(FMath::Max(Board.GroupsCompleted.Num(), Faction + 1));
    if (Board.GroupsCompleted.IsValidIndex(Faction))
    {
        Board.GroupsCompleted[Faction] = FMath::Max(Board.GroupsCompleted[Faction], GroupIndex + 1);
    }
}

void FConnectItBoardEvent_LoseConditionMet::ApplyTo(FConnectItBoardState& Board) const
{
    Board.LosingFactionSlot = Faction;
}

void FConnectItBoardEvent_GameWon::ApplyTo(FConnectItBoardState& Board) const
{
    Board.bGameOver = true;
    Board.WinningFactionSlot = WinningFaction;
}
