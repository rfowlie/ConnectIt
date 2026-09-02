// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "TurnBasedMechanicsEnums.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Framework/Subsystem/ConnectIt_BlackboardSubsystem.h"
#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "GameFramework/PlayerController.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"


AConnectIt_BoardManager* UConnectIt_GameUtilityLibrary::GetBoardManager(const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    UConnectIt_BoardManagerSubsystem* Subsystem =
        World->GetSubsystem<UConnectIt_BoardManagerSubsystem>();

    if (!IsValid(Subsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameUtilityLibrary: "
                 "UConnectIt_BoardManagerSubsystem not found in world"));
        return nullptr;
    }

    AConnectIt_BoardManager* Cached = Subsystem->GetBoardManager();
    if (!IsValid(Cached))
    {
        // Should not happen in normal operation -- AConnectIt_BoardManager::
        // BeginPlay registers itself with the subsystem on both server and
        // client. A miss here means that registration never ran (or ran on
        // a different world) and is treated as a hard failure, not a race
        // to silently paper over with a world scan.
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameUtilityLibrary: "
                 "No AConnectIt_BoardManager registered with "
                 "UConnectIt_BoardManagerSubsystem"));
        return nullptr;
    }

    return Cached;
}

UConnectIt_BoardStateComponent* UConnectIt_GameUtilityLibrary::GetBoardStateComponent(
    const UObject* WorldContextObject)
{
    const AConnectIt_BoardManager* BoardManager = GetBoardManager(WorldContextObject);
    if (!IsValid(BoardManager)) return nullptr;
    return BoardManager->GetBoardStateComponent();
}

bool UConnectIt_GameUtilityLibrary::GetGridPositionForTile(
    const UObject* WorldContextObject,
    const AGridTileBase* Tile,
    FGridPosition& OutPosition)
{
    AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    if (!IsValid(BM)) return false;
    OutPosition = BM->GetTileRegistry()->GetPositionOfTile(Tile);
    return true;
}

TArray<AGridTileBase*> UConnectIt_GameUtilityLibrary::GetAllGridTiles(
    const UObject* WorldContextObject)
{
    const AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    if (!IsValid(BM) || !IsValid(BM->GetTileRegistry())) return {};

    return BM->GetTileRegistry()->GetAllTiles();
}

TArray<AGridTileBase*> UConnectIt_GameUtilityLibrary::GetEmptyGridTiles(
    const UObject* WorldContextObject)
{
    const AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(BM) || !IsValid(BM->GetTileRegistry()) || !IsValid(BoardState))
        return {};

    TArray<AGridTileBase*> EmptyTiles;
    for (AGridTileBase* Tile : BM->GetTileRegistry()->GetAllTiles())
    {
        const FGridPosition Position =
            BM->GetTileRegistry()->GetPositionOfTile(Tile);

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
    const AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(BM) || !IsValid(BM->GetTileRegistry()) || !IsValid(BoardState) || !IsValid(Tile))
        return false;

    const FGridPosition Position = BM->GetTileRegistry()->GetPositionOfTile(Tile);
    return !BoardState->GetCurrentState().IsTileOccupied(Position);
}

TArray<AGridTileBase*> UConnectIt_GameUtilityLibrary::GetGridTilesWithFactionPieces(
    const UObject* WorldContextObject,
    int32 FactionSlot)
{
    const AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    const UConnectIt_BoardStateComponent* BoardState =
        GetBoardStateComponent(WorldContextObject);

    if (!IsValid(BM) || !IsValid(BM->GetTileRegistry()) || !IsValid(BoardState))
        return {};

    TArray<AGridTileBase*> FactionTiles;
    for (AGridTileBase* Tile : BM->GetTileRegistry()->GetAllTiles())
    {
        const FGridPosition Position =
            BM->GetTileRegistry()->GetPositionOfTile(Tile);

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
    const AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    if (!IsValid(BM) || !IsValid(BM->GetTileRegistry())) return nullptr;

    return BM->GetTileRegistry()->GetTileAtPosition(Position);
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