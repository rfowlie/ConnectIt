// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/TurnBasedControllerCoordinatorComponent.h"
#include "GameFramework/Controller.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Action/TurnBasedActionsComponent.h"


UTurnBasedControllerCoordinatorComponent::
    UTurnBasedControllerCoordinatorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UTurnBasedControllerCoordinatorComponent::BeginPlay()
{
    Super::BeginPlay();
    WireComponents();
}

void UTurnBasedControllerCoordinatorComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    UnbindAll();
    Super::EndPlay(EndPlayReason);
}

// --- Wiring ---

void UTurnBasedControllerCoordinatorComponent::WireComponents()
{
    if (bIsWired)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("TurnBasedCoordinator: %s already wired"),
            *GetOwner()->GetName());
        return;
    }

    if (!ResolveComponents())
    {
        UE_LOG(LogTemp, Error,
            TEXT("TurnBasedCoordinator: %s failed to resolve required "
                 "components. Ensure UTurnBasedParticipantComponent and "
                 "UTurnBasedActionsComponent exist on this controller."),
            *GetOwner()->GetName());
        return;
    }

    BindParticipantToActions();
    BindActionsToParticipant();

    if (bRouteMatchPhaseChanges)
    {
        BindGameStateToActions();
    }

    bIsWired = true;

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedCoordinator: %s wired successfully"),
        *GetOwner()->GetName());
}

bool UTurnBasedControllerCoordinatorComponent::ResolveComponents()
{
    AController* Owner = GetOwner<AController>();
    if (!IsValid(Owner))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TurnBasedCoordinator: Owner is not an AController"));
        return false;
    }

    ParticipantComponent =
        Owner->FindComponentByClass<UTurnBasedParticipantComponent>();

    ActionsComponent =
        Owner->FindComponentByClass<UTurnBasedActionsComponent>();

    return IsValid(ParticipantComponent) && IsValid(ActionsComponent);
}

void UTurnBasedControllerCoordinatorComponent::BindParticipantToActions()
{
    ParticipantComponent->OnTurnNotificationReceived.AddDynamic(
        this,
        &UTurnBasedControllerCoordinatorComponent::HandleTurnNotification);

    ParticipantComponent->OnOpponentTurnStarted.AddDynamic(
        this,
        &UTurnBasedControllerCoordinatorComponent::HandleOpponentTurnStarted);
}

void UTurnBasedControllerCoordinatorComponent::BindActionsToParticipant()
{
    ActionsComponent->OnTurnEndRequested.AddDynamic(
        this,
        &UTurnBasedControllerCoordinatorComponent::HandleTurnEndRequested);
}

void UTurnBasedControllerCoordinatorComponent::BindGameStateToActions()
{
    ATurnBasedGameState* GS = GetTurnBasedGameState();
    if (!IsValid(GS))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedCoordinator: %s could not find "
                 "ATurnBasedGameState — match phase routing disabled. "
                 "This can happen on listen server if coordinator "
                 "BeginPlay fires before GameState exists."),
            *GetOwner()->GetName());
        return;
    }

    GS->OnMatchPhaseChanged.AddDynamic(
        this,
        &UTurnBasedControllerCoordinatorComponent::HandleMatchPhaseChanged);
}

void UTurnBasedControllerCoordinatorComponent::UnbindAll()
{
    if (IsValid(ParticipantComponent))
    {
        ParticipantComponent->OnTurnNotificationReceived.RemoveDynamic(
            this,
            &UTurnBasedControllerCoordinatorComponent::HandleTurnNotification);

        ParticipantComponent->OnOpponentTurnStarted.RemoveDynamic(
            this,
            &UTurnBasedControllerCoordinatorComponent::HandleOpponentTurnStarted);
    }

    if (IsValid(ActionsComponent))
    {
        ActionsComponent->OnTurnEndRequested.RemoveDynamic(
            this,
            &UTurnBasedControllerCoordinatorComponent::HandleTurnEndRequested);
    }

    if (ATurnBasedGameState* GS = GetTurnBasedGameState())
    {
        GS->OnMatchPhaseChanged.RemoveDynamic(
            this,
            &UTurnBasedControllerCoordinatorComponent::HandleMatchPhaseChanged);
    }

    bIsWired = false;
}

// --- Participant -> Actions ---

void UTurnBasedControllerCoordinatorComponent::HandleTurnNotification(
    const FTurnNotification& Notification)
{
    if (!IsValid(ActionsComponent)) return;

    switch (Notification.Phase)
    {
        case ETurnPhase::TurnStart:
        case ETurnPhase::TurnActive:
        {
            // Notification.ParticipantInfo already carries TurnsTaken --
            // BuildNotification copies the full FTurnParticipantInfo by value
            FTurnStartContext Context;
            Context.TurnNumber       = Notification.TurnNumber;
            Context.ActiveParticipant = Notification.ParticipantInfo;
            ActionsComponent->NotifyTurnStarted(Context);
            break;
        }

        case ETurnPhase::TurnEnd:
        case ETurnPhase::TurnTimeout:
        case ETurnPhase::TurnSkipped:
            ActionsComponent->NotifyTurnEnded();
            break;

        default:
            break;
    }
}

void UTurnBasedControllerCoordinatorComponent::HandleOpponentTurnStarted(
    int32 ActiveParticipantSlotIndex)
{
    if (!IsValid(ActionsComponent)) return;
    if (!IsValid(ParticipantComponent)) return;

    // Participants/TurnNumber already replicate independently on the
    // manager component (GameState-resident, reachable on both server and
    // clients) -- no need to widen the ClientReceiveOpponentTurnStarted RPC
    // itself, just read them here once the RPC arrives.
    UTurnBasedParticipantManagerComponent* Manager =
        ParticipantComponent->GetParticipantManager();

    FTurnStartContext Context;
    if (IsValid(Manager))
    {
        Context.TurnNumber = Manager->TurnNumber;

        const FTurnParticipantInfo* Found = Manager->Participants.FindByPredicate(
            [ActiveParticipantSlotIndex](const FTurnParticipantInfo& Info)
            {
                return Info.SlotIndex == ActiveParticipantSlotIndex;
            });

        if (Found)
        {
            Context.ActiveParticipant = *Found;
        }
    }

    ActionsComponent->NotifyOpponentTurnStarted(Context);
}

// --- Actions -> Participant ---

void UTurnBasedControllerCoordinatorComponent::HandleTurnEndRequested()
{
    if (!IsValid(ParticipantComponent)) return;
    ParticipantComponent->ServerSubmitTurnEnd();
}

// --- GameState -> Actions ---

void UTurnBasedControllerCoordinatorComponent::HandleMatchPhaseChanged(
    EMatchPhase NewPhase)
{
    if (!IsValid(ActionsComponent)) return;

    switch (NewPhase)
    {
        case EMatchPhase::Paused:
            bIsPaused = true;
            ActionsComponent->NotifyPaused();
            break;

        case EMatchPhase::InProgress:
            // Only unpause if we were actually paused
            // InProgress also fires at the start of every turn
            if (bIsPaused)
            {
                bIsPaused = false;
                ActionsComponent->NotifyUnpaused();
            }
            break;

        case EMatchPhase::GameOver:
            ActionsComponent->NotifyMatchEnded();
            break;

        default:
            break;
    }
}

// --- Helpers ---

ATurnBasedGameState*
UTurnBasedControllerCoordinatorComponent::GetTurnBasedGameState() const
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return nullptr;
    return World->GetGameState<ATurnBasedGameState>();
}