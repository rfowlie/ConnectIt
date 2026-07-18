// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "EngineUtils.h"
#include "TurnBasedMechanicsEnums.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Framework/Subsystem/ConnectIt_BlackboardSubsystem.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "GameFramework/PlayerController.h"
#include "Tile/GridTileRegistryComponent.h"


AConnectIt_BoardManager* UConnectIt_GameUtilityLibrary::GetBoardManager(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    for (TActorIterator<AConnectIt_BoardManager> It(World); It; ++It)
    {
        return *It;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectIt_GameUtilityLibrary: "
             "No AConnectIt_BoardManager found in world"));

    return nullptr;
}

UConnectIt_BoardStateComponent* UConnectIt_GameUtilityLibrary::GetBoardStateComponent(
    const UObject* WorldContextObject)
{
    AConnectIt_BoardManager* BM = GetBoardManager(WorldContextObject);
    if (!IsValid(BM)) return nullptr;
    return Cast<UConnectIt_BoardStateComponent>(BM->GetBoardState());
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
        ? ParticipantComp->CachedSlotIndex
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

UGridWorldSubsystem* UConnectIt_GameUtilityLibrary::GetGridSubsystem(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    return World->GetSubsystem<UGridWorldSubsystem>();
}