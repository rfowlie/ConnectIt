// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "TurnBasedMechanicsEnums.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Framework/Data/ConnectIt_LevelConfigSettings.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Subsystem/ConnectIt_BlackboardSubsystem.h"
#include "Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/ConnectIt_TileRegistry.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"


UConnectIt_BoardStateComponent* UConnectIt_GameUtilityLibrary::GetBoardStateComponent(
    const UObject* WorldContextObject)
{
    // Board state lives on AConnectIt_GameState now -- single source of
    // truth, replicated, reachable via the engine's own GetGameState<>()
    // on both server and every client with no need for the board-manager
    // subsystem cache at all.
    const AConnectIt_GameState* GameState = GetConnectItGameState(WorldContextObject);
    return IsValid(GameState) ? GameState->GetBoardStateComponent() : nullptr;
}

UGridTileRegistryBase* UConnectIt_GameUtilityLibrary::GetTileRegistry(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    // TileRegistry lives on UConnectIt_BoardRegistrySubsystem now -- one
    // canonical instance per world, resolved identically on server and
    // every client, no controller-existence dependency.
    const UConnectIt_BoardRegistrySubsystem* Subsystem =
        World->GetSubsystem<UConnectIt_BoardRegistrySubsystem>();
    return IsValid(Subsystem) ? Subsystem->GetTileRegistry() : nullptr;
}

UGridPieceRegistryBase* UConnectIt_GameUtilityLibrary::GetPieceRegistry(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    const UConnectIt_BoardRegistrySubsystem* Subsystem =
        World->GetSubsystem<UConnectIt_BoardRegistrySubsystem>();
    return IsValid(Subsystem) ? Subsystem->GetPieceRegistry() : nullptr;
}

bool UConnectIt_GameUtilityLibrary::GetGridPositionForTile(
    const UObject* WorldContextObject,
    const AGridTileBase* Tile,
    FGridPosition& OutPosition)
{
    UGridTileRegistryBase* TileRegistry = GetTileRegistry(WorldContextObject);
    if (!IsValid(TileRegistry)) return false;
    OutPosition = TileRegistry->GetPositionOfTile(Tile);
    return true;
}

TArray<AGridTileBase*> UConnectIt_GameUtilityLibrary::GetAllGridTiles(
    const UObject* WorldContextObject)
{
    UGridTileRegistryBase* TileRegistry = GetTileRegistry(WorldContextObject);
    if (!IsValid(TileRegistry)) return {};

    return TileRegistry->GetAllTiles();
}

TArray<AGridTileBase*> UConnectIt_GameUtilityLibrary::GetEmptyGridTiles(
    const UObject* WorldContextObject)
{
    UGridTileRegistryBase* TileRegistry = GetTileRegistry(WorldContextObject);
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(TileRegistry) || !IsValid(BoardState))
        return {};

    TArray<AGridTileBase*> EmptyTiles;
    for (AGridTileBase* Tile : TileRegistry->GetAllTiles())
    {
        const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);

        if (BoardState->GetCurrentState().IsTileValidForPlacement(Position))
        {
            EmptyTiles.Add(Tile);
        }
    }

    return EmptyTiles;
}

bool UConnectIt_GameUtilityLibrary::IsTileEmpty(
    const UObject* WorldContextObject,
    const AGridTileBase* Tile)
{
    UGridTileRegistryBase* TileRegistry = GetTileRegistry(WorldContextObject);
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(TileRegistry) || !IsValid(BoardState) || !IsValid(Tile))
        return false;

    const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);
    return !BoardState->GetCurrentState().IsTileOccupied(Position);
}

TArray<AGridTileBase*> UConnectIt_GameUtilityLibrary::GetGridTilesWithFactionPieces(
    const UObject* WorldContextObject,
    int32 FactionSlot)
{
    UGridTileRegistryBase* TileRegistry = GetTileRegistry(WorldContextObject);
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(TileRegistry) || !IsValid(BoardState))
        return {};

    TArray<AGridTileBase*> FactionTiles;
    for (AGridTileBase* Tile : TileRegistry->GetAllTiles())
    {
        const FGridPosition Position = TileRegistry->GetPositionOfTile(Tile);

        const FConnectItTileData* TileData =
            BoardState->GetCurrentState().GetTileData(Position);

        if (TileData && TileData->FactionPiece == FactionSlot)
        {
            FactionTiles.Add(Tile);
        }
    }

    return FactionTiles;
}

AGridTileBase* UConnectIt_GameUtilityLibrary::GetTileAtPosition(
    const UObject* WorldContextObject,
    FGridPosition Position)
{
    UGridTileRegistryBase* TileRegistry = GetTileRegistry(WorldContextObject);
    if (!IsValid(TileRegistry)) return nullptr;

    return TileRegistry->GetTileAtPosition(Position);
}

AGridTileBase* UConnectIt_GameUtilityLibrary::GetRandomEmptyGridTile(
    const UObject* WorldContextObject)
{
    const TArray<AGridTileBase*> EmptyTiles = GetEmptyGridTiles(WorldContextObject);
    if (EmptyTiles.Num() == 0) return nullptr;

    return EmptyTiles[FMath::RandHelper(EmptyTiles.Num())];
}

bool UConnectIt_GameUtilityLibrary::IsGameBoardFull(
    const UObject* WorldContextObject)
{
    return GetEmptyGridTiles(WorldContextObject).Num() == 0;
}

bool UConnectIt_GameUtilityLibrary::HasFactionWon(
    const UObject* WorldContextObject,
    int32 FactionSlot)
{
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(BoardState)) return false;

    const FConnectItBoardState& CurrentState = BoardState->GetCurrentState();
    return CurrentState.bGameOver && CurrentState.WinningFactionSlot == FactionSlot;
}

UConnectIt_LevelConfigDataAsset* UConnectIt_GameUtilityLibrary::GetLevelConfig(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    const UConnectIt_LevelConfigSettings* Settings = GetDefault<UConnectIt_LevelConfigSettings>();
    if (!IsValid(Settings)) return nullptr;

    const FName LevelName(*UGameplayStatics::GetCurrentLevelName(WorldContextObject, /*bRemovePrefixString=*/true));

    const TSoftObjectPtr<UConnectIt_LevelConfigDataAsset>* Entry = Settings->LevelConfigs.Find(LevelName);
    if (!Entry)
    {
        // Missing per-level registration is expected to happen (a new/
        // duplicated level nobody's added to ConnectIt_LevelConfigSettings
        // yet) -- fall back to DefaultLevelConfig with a loud Warning
        // instead of silently returning null. A null return here used to
        // cascade into every level-config-dependent system (tile registry,
        // action loadout) being silently unset, with nothing pointing back
        // at "you forgot to register this level" -- see
        // ConnectIt/_decisions/2026-09-14-level-config-default-fallback.md.
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameUtilityLibrary: No ConnectIt_LevelConfigDataAsset "
                 "registered for level '%s' in ConnectIt_LevelConfigSettings -- "
                 "falling back to DefaultLevelConfig. Add an entry for this level "
                 "to silence this and use level-specific rules/loadouts."),
            *LevelName.ToString());

        if (Settings->DefaultLevelConfig.IsNull())
        {
            UE_LOG(LogTemp, Error,
                TEXT("ConnectIt_GameUtilityLibrary: ...and DefaultLevelConfig is "
                     "also unset in ConnectIt_LevelConfigSettings -- no config "
                     "available for level '%s'"),
                *LevelName.ToString());
            return nullptr;
        }

        return Settings->DefaultLevelConfig.LoadSynchronous();
    }

    return Entry->LoadSynchronous();
}

AConnectIt_GameState* UConnectIt_GameUtilityLibrary::GetConnectItGameState(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    return World->GetGameState<AConnectIt_GameState>();
}

void UConnectIt_GameUtilityLibrary::GetAllConnectItPlayerStates(
    TArray<AConnectIt_PlayerState*>& OutPlayerStates, 
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return;

    OutPlayerStates.Empty();
    for (int32 index = 0; index < UGameplayStatics::GetNumPlayerStates(WorldContextObject); index++)
    {
        if (AConnectIt_PlayerState* PlayerState = Cast<AConnectIt_PlayerState>(
            UGameplayStatics::GetPlayerState(WorldContextObject, index)))
        {
            OutPlayerStates.Add(PlayerState);
        }
    }
}

AConnectIt_PlayerState* UConnectIt_GameUtilityLibrary::GetLocalConnectItPlayerState(const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!IsValid(PC)) return nullptr;

    return Cast<AConnectIt_PlayerState>(PC->PlayerState);
}

AConnectIt_PlayerState* UConnectIt_GameUtilityLibrary::GetPlayerStateForFaction(
    const UObject* WorldContextObject, int32 FactionID)
{
    TArray<AConnectIt_PlayerState*> PlayerStates;
    GetAllConnectItPlayerStates(PlayerStates, WorldContextObject);

    for (AConnectIt_PlayerState* PlayerState : PlayerStates)
    {
        if (IsValid(PlayerState) && PlayerState->GetSlotIndex() == FactionID)
        {
            return PlayerState;
        }
    }

    return nullptr;
}

UTurnBasedParticipantManagerComponent* UConnectIt_GameUtilityLibrary::GetParticipantManager(
    const UObject* WorldContextObject)
{
    AConnectIt_GameState* GS = GetConnectItGameState(WorldContextObject);
    return IsValid(GS) ? GS->ParticipantManager : nullptr;
}

ETurnPhase UConnectIt_GameUtilityLibrary::GetCurrentTurnPhase(
    const UObject* WorldContextObject)
{
    const UTurnBasedParticipantManagerComponent* PM =
        GetParticipantManager(WorldContextObject);

    return IsValid(PM)
        ? PM->CurrentPhase
        : ETurnPhase::WaitingForParticipants;
}

int32 UConnectIt_GameUtilityLibrary::GetLocalPlayerSlotIndex(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return -1;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return -1;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!IsValid(PC)) return -1;

    UTurnBasedParticipantComponent* ParticipantComp =
        PC->FindComponentByClass<UTurnBasedParticipantComponent>();

    return IsValid(ParticipantComp)
        ? ParticipantComp->GetActiveParticipantSlotIndex()
        : -1;
}

bool UConnectIt_GameUtilityLibrary::IsLocalPlayerTurn(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return false;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return false;

    // Get local player slot
    const int32 LocalSlot = GetLocalPlayerSlotIndex(WorldContextObject);
    if (LocalSlot < 0) return false;

    // Get active participant slot from manager
    UTurnBasedParticipantManagerComponent* PM =
        GetParticipantManager(WorldContextObject);
    if (!IsValid(PM)) return false;

    const TArray<FTurnParticipantInfo>& Participants = PM->Participants;
    if (!Participants.IsValidIndex(PM->ActiveParticipantIndex))
        return false;

    return Participants[PM->ActiveParticipantIndex].SlotIndex == LocalSlot;
}

UConnectIt_BlackboardSubsystem* UConnectIt_GameUtilityLibrary::GetBlackboardSubsystem(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    const UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    UConnectIt_BlackboardSubsystem* Subsystem = World->GetSubsystem<UConnectIt_BlackboardSubsystem>();
    return Subsystem;
}

UGridHoverSubsystem* UConnectIt_GameUtilityLibrary::GetGridSubsystem(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    return World->GetSubsystem<UGridHoverSubsystem>();
}