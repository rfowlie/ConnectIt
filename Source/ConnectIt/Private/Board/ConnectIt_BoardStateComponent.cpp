// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Library/CodingUtilsLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Board/Powers/ConnectIt_TileCatalog.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"


UConnectIt_BoardStateComponent::UConnectIt_BoardStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

FConnectItTileData UConnectIt_BoardStateComponent::MakeTileData(
    const AGridTileBase* Tile, int32 NumFactions, float InitialMultiplier, const UConnectIt_TileCatalog* Catalog)
{
    FConnectItTileData TileData;
    TileData.SetFactionPiece(-1);
    TileData.Multiplier   = InitialMultiplier;
    TileData.bIsActive    = true;
    if (!IsValid(Tile)) return TileData;

    if (Tile->Implements<UConnectIt_TileDataProvider>())
    {
        const FConnectItTileInitialData Designer =
            IConnectIt_TileDataProvider::Execute_GetInitialTileData(Tile);

        TileData = Designer.ToTileData(NumFactions);
        if (Designer.StartingFactionPiece >= 0 && !Designer.HasStartingPiece(NumFactions))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_BoardStateComponent: tile '%s' has "
                     "StartingFactionPiece %d but the board only has %d "
                     "factions -- starting empty"),
                *Tile->GetName(), Designer.StartingFactionPiece, NumFactions);
        }
    }

    if (Catalog && TileData.TilePowerTag.IsValid() && !Catalog->Defines(TileData.TilePowerTag))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardStateComponent: tile '%s' names power %s, which tile catalog '%s' doesn't "
                 "define -- it has no power"),
            *Tile->GetName(), *TileData.TilePowerTag.ToString(), *Catalog->GetName());
        TileData.TilePowerTag = FGameplayTag();
    }
    return TileData;
}

// Each tile that implements IConnectIt_TileDataProvider supplies its own
// designer-set starting values; any other tile gets the defaults (see
// MakeTileData). Each tile gets its board id in registry order.
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

    const UConnectIt_TileCatalog* Catalog = UConnectIt_TileCatalog::FindFor(this);
    int32 PoweredTiles = 0;
    for (AGridTileBase* Tile : TileRegistry->GetAllTiles())
    {
        const FConnectItTileData TileData = MakeTileData(Tile, NumFactions, InitialMultiplier, Catalog);

        if (IsValid(Tile) && Tile->Implements<UConnectIt_TileDataProvider>())
        {
            TilesWithDesignerData++;
        }
        if (StartingPiecesPerFaction.IsValidIndex(TileData.FactionPiece))
        {
            StartingPieces++;
            StartingPiecesPerFaction[TileData.FactionPiece]++;
        }
        if (TileData.TilePowerTag.IsValid())
        {
            PoweredTiles++;
        }

        InitialState.AddTile(TileRegistry->GetPositionOfTile(Tile), TileData);
    }

    FString PerFactionSummary;
    for (int32 Faction = 0; Faction < StartingPiecesPerFaction.Num(); Faction++)
    {
        PerFactionSummary += FString::Printf(TEXT("%s%d:%d"),
            Faction > 0 ? TEXT(", ") : TEXT(""), Faction, StartingPiecesPerFaction[Faction]);
    }
    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardStateComponent: InitialiseBoardState -- %d tiles, "
             "%d with designer data, %d with a power, %d starting pieces (faction:count %s)"),
        InitialState.TilePositions.Num(), TilesWithDesignerData, PoweredTiles,
        StartingPieces, *PerFactionSummary);

    // Initialise scoreboard with one entry per faction
    InitialState.ScoreBoard.Init(0.f, NumFactions);
    InitialState.TurnNumber       = 0;
    InitialState.bGameOver        = false;
    InitialState.WinningFactionSlot = -1;
    InitialState.TargetScore      = InTargetScore;
    InitialState.StartingPieces   = StartingPiecesPerFaction;
    InitialState.GroupsCompleted.Init(0, NumFactions);

    // Apply without capturing snapshot -- no previous state yet
    BoardSnapshot.PreviousState = InitialState;
    BoardSnapshot.CurrentState  = InitialState;

    DisplayBoard = InitialState;
    bDisplayBoardStarted = true;

    // The board is built from the builder tiles: now they become the tile
    // catalog's actors, before anything plays on them
    if (UConnectIt_TileRegistry* ConnectItTiles = Cast<UConnectIt_TileRegistry>(TileRegistry))
    {
        ConnectItTiles->SwapBuilderTiles();
    }

    // Mark this initial snapshot as the seed so visuals for any starting
    // pieces get created. It replicates with the snapshot, so clients see the
    // same flag and enqueue the same tag from OnRep_BoardSnapshot.
    // BroadcastChange() stays off: nothing changed relative to a previous
    // state, only the game-event tag is needed.
    BoardSnapshot.ChangeEvent = FConnectItBoardChangeEvent();
    BoardSnapshot.ChangeEvent.Add(FConnectItBoardEvent_BoardSeeded());
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

    // The change is resolving until its events have played out: say so
    // first, then queue the marker behind them. (With no visuals registered
    // the marker completes inside this call, so the order matters.)
    OnBoardChangeCommitted.Broadcast();
    if (UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr)
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(ConnectIt_Event_BoardChangeComplete));
    }
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

void UConnectIt_BoardStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr)
    {
        GameEventSubsystem->OnAnyTagComplete.RemoveDynamic(
            this, &UConnectIt_BoardStateComponent::HandleQueueEntryComplete);
    }
    Super::EndPlay(EndPlayReason);
}

void UConnectIt_BoardStateComponent::OnRep_BoardSnapshot()
{
    // A client's display board starts from the board before its first
    // snapshot's change (the seed itself, unless it joined mid-match)
    if (!bDisplayBoardStarted)
    {
        DisplayBoard = BoardSnapshot.PreviousState;
        bDisplayBoardStarted = true;

        // ...and its builder tiles become the tile catalog's actors (tile
        // actors are local: each machine swaps its own)
        if (UConnectIt_TileRegistry* ConnectItTiles =
            Cast<UConnectIt_TileRegistry>(UConnectIt_GameUtilityLibrary::GetTileRegistry(this)))
        {
            ConnectItTiles->SwapBuilderTiles();
        }
    }

    // Previous and current both arrived atomically
    BroadcastChange();
    EnqueueBoardEventTags();
}

void UConnectIt_BoardStateComponent::EnqueueBoardEventTags()
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

    // The display board steps along as these complete
    GameEventSubsystem->OnAnyTagComplete.AddUniqueDynamic(
        this, &UConnectIt_BoardStateComponent::HandleQueueEntryComplete);

    // Only entries that are board events with a tag are queued
    TArray<const FInstancedStruct*, TInlineAllocator<16>> ToQueue;
    for (const FInstancedStruct& Entry : BoardSnapshot.ChangeEvent.Events)
    {
        const FConnectItBoardEvent* Event = Entry.GetPtr<FConnectItBoardEvent>();
        if (!Event || !Event->GetEventTag().IsValid())
        {
            UE_LOG(LogTemp, Error,
                TEXT("ConnectIt_BoardStateComponent: EnqueueBoardEventTags — "
                     "skipping an entry that isn't a board event with a tag (%s)"),
                *GetNameSafe(Entry.GetScriptStruct()));
            continue;
        }
        ToQueue.Add(&Entry);
    }

    // Counted before any is queued: with no visuals bound an entry completes
    // inside its own queue call, and the display board mustn't catch up to
    // Current until the last one has applied
    PendingDisplayEvents += ToQueue.Num();
    if (PendingDisplayEvents == 0)
    {
        CatchDisplayBoardUp();
        return;
    }

    // One queue entry per event, in the order they happened. Each carries
    // its own data as the entry's payload, so a listener always reads the
    // event that is actually playing (UConnectIt_BoardEventLibrary) -- even if this
    // snapshot has been replaced by a newer one by then.
    for (const FInstancedStruct* Entry : ToQueue)
    {
        const FGameplayTag EventTag = Entry->Get<FConnectItBoardEvent>().GetEventTag();
        GameEventSubsystem->QueueTagContainerWithPayload(FGameplayTagContainer(EventTag), *Entry);
    }
}

void UConnectIt_BoardStateComponent::HandleQueueEntryComplete(FGameplayTag Tag)
{
    if (PendingDisplayEvents <= 0) return;

    const UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;
    if (!GameEventSubsystem) return;

    // Still the completing entry's payload (the queue clears it after this);
    // anything else completing (the change-complete marker, other game
    // events) carries no board event
    const FConnectItBoardEvent* Event = GameEventSubsystem->GetActivePayload().GetPtr<FConnectItBoardEvent>();
    if (!Event) return;

    Event->ApplyTo(DisplayBoard);
    PendingDisplayEvents--;

    if (PendingDisplayEvents == 0)
    {
        CatchDisplayBoardUp();
        return;
    }
    OnDisplayBoardChanged.Broadcast();
}

void UConnectIt_BoardStateComponent::CatchDisplayBoardUp()
{
    if (!DisplayBoard.HasSameTilesAndScores(BoardSnapshot.CurrentState))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardStateComponent: the display board's tiles or scores don't match the current "
                 "board after its events played -- snapping to it (a snapshot was missed, or an event doesn't "
                 "carry its whole change)"));
    }
    DisplayBoard = BoardSnapshot.CurrentState;
    OnDisplayBoardChanged.Broadcast();
}
