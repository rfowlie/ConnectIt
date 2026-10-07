// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Library/CodingUtilsLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"


UConnectIt_BoardStateComponent::UConnectIt_BoardStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

// Each tile that implements IConnectIt_TileDataProvider supplies its own
// designer-set starting values; any other tile gets the defaults below.
void UConnectIt_BoardStateComponent::InitialiseBoardState(
    UGridTileRegistryBase* TileRegistry,
    UGridPieceRegistryBase* PieceRegistry,
    int32 NumFactions, float InitialMultiplier, float InTargetScore)
{
    check(UCodingUtilsLibrary::IsAuthoritativeComponent(this));
    check(NumFactions > 0);

    FConnectItBoardState InitialState;

    // Populate tile map from registered positions
    int32 TilesWithDesignerData = 0;
    int32 StartingPieces = 0;
    TArray<int32> StartingPiecesPerFaction;
    StartingPiecesPerFaction.Init(0, NumFactions);

    for (AGridTileBase* Tile : TileRegistry->GetAllTiles())
    {
        FConnectItTileData TileData;
        TileData.SetFactionPiece(-1);
        TileData.Multiplier   = InitialMultiplier;
        TileData.bIsActive    = true;

        if (IsValid(Tile) && Tile->Implements<UConnectIt_TileDataProvider>())
        {
            const FConnectItTileInitialData Designer =
                IConnectIt_TileDataProvider::Execute_GetInitialTileData(Tile);

            TileData.Multiplier = Designer.Multiplier;
            TileData.bIsActive  = Designer.bIsActive;
            TileData.bCanShift  = Designer.bCanShift;
            TilesWithDesignerData++;

            if (Designer.StartingFactionPiece >= 0)
            {
                if (Designer.StartingFactionPiece < NumFactions)
                {
                    TileData.SetFactionPiece(Designer.StartingFactionPiece);
                    StartingPieces++;
                    StartingPiecesPerFaction[Designer.StartingFactionPiece]++;
                }
                else
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("ConnectIt_BoardStateComponent: tile '%s' has "
                             "StartingFactionPiece %d but the board only has %d "
                             "factions -- starting empty"),
                        *Tile->GetName(), Designer.StartingFactionPiece, NumFactions);
                }
            }
        }

        InitialState.SetTileData(TileRegistry->GetPositionOfTile(Tile), TileData);
    }

    FString PerFactionSummary;
    for (int32 Faction = 0; Faction < StartingPiecesPerFaction.Num(); Faction++)
    {
        PerFactionSummary += FString::Printf(TEXT("%s%d:%d"),
            Faction > 0 ? TEXT(", ") : TEXT(""), Faction, StartingPiecesPerFaction[Faction]);
    }
    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardStateComponent: InitialiseBoardState -- %d tiles, "
             "%d with designer data, %d starting pieces (faction:count %s)"),
        InitialState.TilePositions.Num(), TilesWithDesignerData,
        StartingPieces, *PerFactionSummary);

    // Initialise scoreboard with one entry per faction
    InitialState.ScoreBoard.Init(0.f, NumFactions);
    InitialState.LastModifiedTurn = 0;
    InitialState.bGameOver        = false;
    InitialState.WinningFactionSlot = -1;
    InitialState.TargetScore      = InTargetScore;

    // Apply without capturing snapshot -- no previous state yet
    BoardSnapshot.PreviousState = InitialState;
    BoardSnapshot.CurrentState  = InitialState;

    // Mark this initial snapshot as the seed so visuals for any starting
    // pieces get created. It replicates with the snapshot, so clients see the
    // same flag and enqueue the same tag from OnRep_BoardSnapshot.
    // BroadcastChange() stays off: nothing changed relative to a previous
    // state, only the game-event tag is needed.
    BoardSnapshot.ChangeEvent = FConnectItBoardChangeEvent();
    BoardSnapshot.ChangeEvent.bBoardSeeded = true;
    EnqueueBoardEventTags();
}

void UConnectIt_BoardStateComponent::SetBoardState(
    const FConnectItBoardState& NewState, const FConnectItBoardChangeEvent& ChangeEvent)
{
    check(UCodingUtilsLibrary::IsAuthoritativeComponent(this));

    // Capture current as previous before overwriting
    // this calls the replication function which broadcasts change for clients
    BoardSnapshot = FConnectItBoardStateSnapshot(
        BoardSnapshot.CurrentState,
        NewState, ChangeEvent);

    // Fire on server immediately
    // Clients receive via OnRep_BoardSnapshot replication
    BroadcastChange();
    EnqueueBoardEventTags();
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

int32 UConnectIt_BoardStateComponent::GetFactionPieceCount(const int32 FactionId) const
{
    int32 Count = 0;
    for (auto TileData : GetCurrentState().TileDataArray)
    {
        if (TileData.FactionPiece == FactionId)
        {
            Count++;
        }
    }

    return Count;
}

TArray<FGridPosition> UConnectIt_BoardStateComponent::GetFactionPiecePositions(const int32 FactionId) const
{
    TArray<FGridPosition> Positions;
    const auto CurrentState = GetCurrentState();
    for (int32 Index = 0 ; Index < CurrentState.TilePositions.Num(); Index++)
    {
        if (CurrentState.GetTileDataAt(Index).FactionPiece == FactionId)
        {
            Positions.Add(CurrentState.TilePositions[Index]);
        }  
    }

    return Positions;
}

int32 UConnectIt_BoardStateComponent::GetPositionMultiplier(const FGridPosition GridPosition) const
{
    const FConnectItTileData* TileData = GetCurrentState().GetTileData(GridPosition);
    if (!TileData)
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UConnectIt_BoardStateComponent::GetPositionMultiplier - GridPosition (X:%d Y:%d) invalid"),
            GridPosition.X, GridPosition.Y);
        return 0;
    }

    return TileData->Multiplier;
}

int32 UConnectIt_BoardStateComponent::GetTileMultiplier(const AGridTileBase* GridTile) const
{
    const auto TileRegistry = UConnectIt_GameUtilityLibrary::GetTileRegistry(this);
    if (!TileRegistry)
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UConnectIt_BoardStateComponent::GetTileMultiplier - Tile Registry invalid"));
        return 0;
    }
    
    return GetPositionMultiplier(TileRegistry->GetPositionOfTile(GridTile));
}

void UConnectIt_BoardStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UConnectIt_BoardStateComponent, BoardSnapshot);
}

void UConnectIt_BoardStateComponent::OnRep_BoardSnapshot()
{
    // Previous and current both arrived atomically
    BroadcastChange();
    EnqueueBoardEventTags();
}

void UConnectIt_BoardStateComponent::EnqueueBoardEventTags() const
{
    UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;

    if (!IsValid(GameEventSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardStateComponent: EnqueueBoardEventTags — "
                 "no UGameEventTaskSubsystem in world"));
        return;
    }

    const FConnectItBoardChangeEvent& ChangeEvent = BoardSnapshot.ChangeEvent;

    // Fixed order -- each SetBoardState call represents exactly one kind of
    // concrete change, then conditionally Scored and/or PlayerWin. Each
    // call enqueues independently; UGameEventTaskSubsystem's own queue
    // serializes them so the next one doesn't start firing until the
    // previous is done.
    
    // initial board seed -- starting pieces from the level need visuals
    if (ChangeEvent.bBoardSeeded)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_BoardSeeded));
    }

    // concrete changes to board from player actions
    if (ChangeEvent.bPiecePlaced)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_PiecePlaced));
    }
    if (ChangeEvent.bPieceRemoved)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_PieceRemoved));
    }
    if (ChangeEvent.bPiecesSwapped)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_PiecesSwapped));
    }
    if (ChangeEvent.bPieceCaptured)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_PieceCaptured));
    }
    if (ChangeEvent.bTileMultiplierDestroyed)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_TileMultiplierDestroyed));
    }
    if (ChangeEvent.bBoardShifted)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_BoardShifted));
    }

    // knock on board changes from rules
    if (ChangeEvent.HasScored())
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_Scored));
    }    
    if (ChangeEvent.bGameWon)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_PlayerWin));
    }

    // New mutation types (see AConnectIt_BoardManager's Handle*Request
    // methods) -- each is its own disjoint kind of change, so no ordering
    // relationship between them
    
    if (ChangeEvent.bTileActiveToggled)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_TileActiveToggled));
    }
    
}