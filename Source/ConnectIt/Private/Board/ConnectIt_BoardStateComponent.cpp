// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardStateComponent.h"

#include "Library/CodingUtilsComponentLibrary.h"
#include "Net/UnrealNetwork.h"


UConnectIt_BoardStateComponent::UConnectIt_BoardStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}


void UConnectIt_BoardStateComponent::InitialiseBoardState(
    const TArray<FGridPosition>& TilePositions,
    int32 NumFactions,
    float InitialMultiplier)
{
    check(UCodingUtilsComponentLibrary::IsAuthoritative(this));
    check(NumFactions > 0);

    FConnectItBoardState InitialState;

    // Populate tile map from registered positions
    for (const FGridPosition& Position : TilePositions)
    {
        FConnectItTileData TileData;
        TileData.FactionPiece = -1;
        TileData.Multiplier   = InitialMultiplier;
        TileData.bIsActive    = true;
        InitialState.SetTileData(Position, TileData);
    }

    // Initialise scoreboard with one entry per faction
    InitialState.ScoreBoard.Init(0.f, NumFactions);
    InitialState.LastModifiedTurn = 0;
    InitialState.bGameOver        = false;
    InitialState.WinningFactionSlot = -1;

    // Apply without capturing snapshot -- no previous state yet
    BoardSnapshot.PreviousState = InitialState;
    BoardSnapshot.CurrentState  = InitialState;

    /*
     * TODO: in theory this will likely not be necessary
     * each level should have a starting position where tiles and pieces are set
     * broadcasting a change is technically wrong
     */ 
    // Broadcast so interpreters can initialise their visual state
    // BroadcastChange();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardStateComponent: Initialised — "
             "%d tiles, %d factions"),
        TilePositions.Num(), NumFactions);
}

void UConnectIt_BoardStateComponent::SetBoardState(
    const FConnectItBoardState& NewState,
    const FConnectItBoardChangeEvent& ChangeEvent)
{
    check(UCodingUtilsComponentLibrary::IsAuthoritative(this));

    // Capture current as previous before overwriting
    // this calls the replication function which broadcasts change for clients
    BoardSnapshot = FConnectItBoardStateSnapshot(
        BoardSnapshot.CurrentState, NewState, ChangeEvent);

    // Fire on server immediately
    // Clients receive via OnRep_BoardSnapshot replication
    BroadcastChange();
}

const FConnectItBoardStateSnapshot* UConnectIt_BoardStateComponent::GetBoardSnapshot() const
{
    return &BoardSnapshot;
}

const FConnectItBoardState* UConnectIt_BoardStateComponent::GetBoardSnapShotCurrent() const
{
    return &BoardSnapshot.CurrentState;
}

const FConnectItBoardState* UConnectIt_BoardStateComponent::GetBoardSnapShotPrevious() const
{
    return &BoardSnapshot.PreviousState;
}

void UConnectIt_BoardStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UConnectIt_BoardStateComponent, BoardSnapshot);
}

void UConnectIt_BoardStateComponent::OnRep_BoardSnapshot()
{
    // Previous and current both arrived atomically
    // Fire void delegate -- all bound interpreters respond
    BroadcastChange();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardStateComponent: OnRep fired — "
             "broadcasting to interpreters"));
}