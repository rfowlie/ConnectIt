// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Controller/ConnectIt_AIController.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/GameMode/TurnBasedGameMode.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardManagerComponent.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Framework/Subsystem/ConnectIt_BlackboardSubsystem.h"
#include "Tile/GridTileBase.h"


AConnectIt_AIController::AConnectIt_AIController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionComponent =
        CreateDefaultSubobject<UTurnBasedActionsComponent>(
            TEXT("ActionComponent"));
}

void AConnectIt_AIController::BeginPlay()
{
    Super::BeginPlay();

    // AI controller only exists on server
    if (!HasAuthority()) return;

    // Cache slot index -- may be -1 until registered
    MySlotIndex = ParticipantComponent->CachedSlotIndex;

    // Bind turn notification -- Blueprint subclass implements logic
    ParticipantComponent->OnTurnNotificationReceived.AddDynamic(
        this, &AConnectIt_AIController::HandleTurnNotification);

    // Bind opponent turn -- notify action component
    ParticipantComponent->OnOpponentTurnStarted.AddDynamic(
        this, &AConnectIt_AIController::HandleOpponentTurnStarted);

    InitialiseFromBoardActor();
}

void AConnectIt_AIController::InitialiseFromBoardActor()
{
    BoardActor = UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(BoardActor))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: No AConnectIt_BoardManager "
                 "found in world"));
        return;
    }

    // Initialise action component from enemy loadout
    UActionLoadOutDataAsset* LoadOut = BoardActor->GetEnemyLoadout();
    if (IsValid(LoadOut))
    {
        ActionComponent->InitialiseFromLoadout(LoadOut);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: No enemy loadout set "
                 "on board manager config"));
    }

    // Register as AI participant with game mode
    if (ATurnBasedGameMode* GameMode = Cast<ATurnBasedGameMode>(
        GetWorld()->GetAuthGameMode()))
    {
        GameMode->RegisterAIParticipant(this);

        // Update slot index after registration
        MySlotIndex = ParticipantComponent->CachedSlotIndex;

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_AIController: Registered as AI participant "
                 "at slot %d"),
            MySlotIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: GameMode is not ATurnBasedGameMode"));
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: Initialised from board manager"));
}

void AConnectIt_AIController::HandleOpponentTurnStarted(
    int32 ActiveParticipantSlotIndex)
{
    ActionComponent->NotifyOpponentTurnStarted();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: Opponent turn started "
             "— active slot: %d"),
        ActiveParticipantSlotIndex);
}

bool AConnectIt_AIController::CheckAndApplyForcedMove()
{
    UConnectIt_BlackboardSubsystem* Blackboard =
        UConnectIt_GameUtilityLibrary::GetBlackboardSubsystem(this);

    if (!IsValid(Blackboard)) return false;
    if (!Blackboard->HasModifier(MySlotIndex)) return false;

    const FTurnModifier Modifier = Blackboard->GetModifier(MySlotIndex);
    Blackboard->ClearModifier(MySlotIndex);

    if (!IsValid(Modifier.TargetTile))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: Forced move modifier "
                 "has null TargetTile — ignoring"));
        return false;
    }

    FGridPosition ForcedPosition;
    UConnectIt_GameUtilityLibrary::GetGridPositionForTile(
        this, Modifier.TargetTile, ForcedPosition);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: Applying forced move at (%d,%d) "
             "from shard '%s'"),
        ForcedPosition.X,
        ForcedPosition.Y,
        *Modifier.SourceTag.ToString());

    SubmitMove(ForcedPosition);
    return true;
}

void AConnectIt_AIController::SubmitMove(const FGridPosition Position)
{
    // AI is server side -- submit directly to board manager
    // No ServerRPC needed
    AConnectIt_BoardManager* BoardManager =
        UConnectIt_GameUtilityLibrary::GetBoardManager(this);

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: SubmitMove failed "
                 "— board manager component not found"));
        return;
    }

    FTurnActionRequest Request;
    Request.RequestType = FGameplayTag::RequestGameplayTag("ConnectIt.Board.PlacePiece");
    Request.Positions.Add(Position);
    Request.FactionID = MySlotIndex;

    BoardManager->ProcessRequest(Request);

    // Notify action component turn ended
    // Cast away const since action component state needs updating
    if (UTurnBasedActionsComponent* AC =
        const_cast<AConnectIt_AIController*>(this)->ActionComponent.Get())
    {
        AC->NotifyTurnEnded();
    }

    // Submit turn end
    ParticipantComponent->ServerSubmitTurnEnd();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: Move submitted at (%d,%d) "
             "faction %d"),
        Position.X,
        Position.Y,
        MySlotIndex);
}