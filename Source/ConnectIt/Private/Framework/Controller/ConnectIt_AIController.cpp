// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Controller/ConnectIt_AIController.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/GameMode/TurnBasedGameMode.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedActionsComponent.h"


AConnectIt_AIController::AConnectIt_AIController()
{
    AIDisplayName = TEXT("Opponent");
}

void AConnectIt_AIController::BeginPlay()
{
    // Base creates PlayerState and binds participant delegates
    Super::BeginPlay();

    if (!HasAuthority()) return;

    InitialiseFromBoardManager();
}

void AConnectIt_AIController::InitialiseFromBoardManager()
{
    BoardManager = UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: No board manager found"));
        return;
    }

    if (UActionLoadoutDataAsset* LoadOut = BoardManager->GetEnemyLoadout())
    {
        ActionsComponent->InitialiseFromLoadout(LoadOut);
    }

    // Register with participant manager
    // PlayerState already exists thanks to base class
    if (const ATurnBasedGameMode* GameMode =
        Cast<ATurnBasedGameMode>(GetWorld()->GetAuthGameMode()))
    {
        GameMode->RegisterAIParticipant(this);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: GameMode is not "
                 "ATurnBasedGameMode"));
    }
}

void AConnectIt_AIController::HandleTurnNotification_Implementation(
    const FTurnNotification& Notification)
{
    // Base handles action component lifecycle
    Super::HandleTurnNotification_Implementation(Notification);

    // Add ConnectIt AI logic on turn start
    if (Notification.Phase == ETurnPhase::TurnStart
        || Notification.Phase == ETurnPhase::TurnActive)
    {
        if (CheckAndApplyForcedMove()) return;

        // No forced move -- let subclass run its intelligence
        BeginAIDecisionMaking();
    }
}

bool AConnectIt_AIController::CheckAndApplyForcedMove()
{
    // Blackboard forced move logic here
    // Returns true if a forced move was applied
    return false;
}