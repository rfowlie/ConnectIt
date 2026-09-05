// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "TurnBasedMechanicsEnums.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Framework/Controller/ConnectIt_PlayerController.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_LevelConfigSettings.h"
#include "Framework/Subsystem/ConnectIt_BlackboardSubsystem.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "GameFramework/PlayerController.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"
#include "Tile/GridTileRegistryComponent.h"


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

    // Fast path -- the common client-side case, exactly one meaningful
    // controller.
    if (const AConnectIt_PlayerController* LocalPC =
        Cast<AConnectIt_PlayerController>(World->GetFirstPlayerController()))
    {
        if (IsValid(LocalPC->GetTileRegistry()))
        {
            return LocalPC->GetTileRegistry();
        }
    }

    // Fall back to scanning every connected controller -- needed
    // server-side (e.g. AI move generation), where "the first/local
    // player controller" isn't the right concept (may not exist at all on
    // a dedicated server). Every connected controller's TileRegistry
    // necessarily agrees -- tile layout is level-authored and
    // deterministic -- so which one answers doesn't matter.
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (const AConnectIt_PlayerController* PC = Cast<AConnectIt_PlayerController>(It->Get()))
        {
            if (IsValid(PC->GetTileRegistry()))
            {
                return PC->GetTileRegistry();
            }
        }
    }

    return nullptr;
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
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameUtilityLibrary: No ConnectIt_LevelConfigDataAsset "
                 "registered for level '%s' in ConnectIt_LevelConfigSettings"),
            *LevelName.ToString());
        return nullptr;
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