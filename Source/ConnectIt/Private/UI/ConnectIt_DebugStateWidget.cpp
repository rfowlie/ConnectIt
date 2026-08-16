// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ConnectIt_DebugStateWidget.h"

#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"
#include "Action/TurnBasedActionsComponent.h"
#include "Action/TurnBasedActionBase.h"
#include "Action/TurnBasedAction.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"


void UConnectIt_DebugStateWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindAll();
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::NativeDestruct()
{
    UnbindAll();
    Super::NativeDestruct();
}

void UConnectIt_DebugStateWidget::ForceRefreshAll()
{
    RefreshBoardState();
    RefreshTurnState();
    RefreshActionState();
    RefreshEventQueueState();
    OnDebugStateUpdated();
}

// --- Binding ---

void UConnectIt_DebugStateWidget::BindAll()
{
    if (bBound) return;

    // Board manager -- may not have registered with the subsystem yet
    // (level-placed actor, BeginPlay-ordering dependent). Try the direct
    // route first; fall back to the documented ready-signal pattern instead
    // of polling. See Workflows/BoardManagerSubsystem_Workflow.txt.
    ResolvedBoardManagerSubsystem = GetWorld()
        ? GetWorld()->GetSubsystem<UConnectIt_BoardManagerSubsystem>()
        : nullptr;

    if (IsValid(ResolvedBoardManagerSubsystem))
    {
        if (AConnectIt_BoardManager* BoardManager =
            ResolvedBoardManagerSubsystem->GetBoardManager())
        {
            BindBoardManager(BoardManager);
        }
        else
        {
            ResolvedBoardManagerSubsystem->OnBoardManagerReady.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleBoardManagerReady);
        }
    }

    // Game state / participant manager -- both exist from very early
    // (GameState is engine-spawned on connect, ParticipantManager is its
    // default subobject), so no ready-signal dance needed here in practice.
    ResolvedGameState = UConnectIt_GameUtilityLibrary::GetConnectItGameState(this);
    if (IsValid(ResolvedGameState))
    {
        ResolvedGameState->OnMatchPhaseChanged.AddDynamic(
            this, &UConnectIt_DebugStateWidget::HandleMatchPhaseChanged);
        ResolvedGameState->OnMatchResultUpdated.AddDynamic(
            this, &UConnectIt_DebugStateWidget::HandleMatchResultUpdated);

        ResolvedParticipantManager = ResolvedGameState->ParticipantManager;
        if (IsValid(ResolvedParticipantManager))
        {
            ResolvedParticipantManager->OnTurnPhaseChanged.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleTurnPhaseChanged);
            ResolvedParticipantManager->OnActiveControllerChanged.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleActiveControllerChanged);
            ResolvedParticipantManager->OnParticipantForfeited.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleParticipantForfeited);
            ResolvedParticipantManager->OnAllParticipantsReady.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleAllParticipantsReady);
            ResolvedParticipantManager->OnGameOver.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleGameOver);
        }
    }

    // Local player's action stack -- only the owning player's own actions
    // are meaningful to read this way (ActionsComponent is per-controller)
    if (APlayerController* OwningPC = GetOwningPlayer())
    {
        ResolvedActionsComponent = OwningPC->FindComponentByClass<UTurnBasedActionsComponent>();
        if (IsValid(ResolvedActionsComponent))
        {
            ResolvedActionsComponent->OnActionPushed.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleActionStackChanged);
            ResolvedActionsComponent->OnActionPopped.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleActionStackChanged);
            ResolvedActionsComponent->OnActionCompleted.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleActionResolved);
            ResolvedActionsComponent->OnActionCancelled.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleActionResolved);
            ResolvedActionsComponent->OnBoardChangeRequested.AddDynamic(
                this, &UConnectIt_DebugStateWidget::HandleBoardChangeRequested);
        }
    }

    // Event-sequencing queue state -- UGameEventTaskSubsystem is a
    // UWorldSubsystem, always exists for the world's lifetime
    ResolvedGameEventSubsystem = GetWorld()
        ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>()
        : nullptr;

    if (IsValid(ResolvedGameEventSubsystem))
    {
        ResolvedGameEventSubsystem->OnActiveManagerTagsChanged.AddDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActiveManagerTagsChanged);
    }

    bBound = true;
}

void UConnectIt_DebugStateWidget::BindBoardManager(AConnectIt_BoardManager* InBoardManager)
{
    if (!IsValid(InBoardManager)) return;

    ResolvedBoardManager = InBoardManager;
    ResolvedBoardStateComponent = ResolvedBoardManager->GetBoardStateComponent();

    if (IsValid(ResolvedBoardStateComponent))
    {
        ResolvedBoardStateComponent->OnBoardStateChanged.AddDynamic(
            this, &UConnectIt_DebugStateWidget::HandleBoardStateChanged);
    }
}

void UConnectIt_DebugStateWidget::UnbindAll()
{
    if (!bBound) return;

    if (IsValid(ResolvedBoardManagerSubsystem))
    {
        ResolvedBoardManagerSubsystem->OnBoardManagerReady.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleBoardManagerReady);
    }

    if (IsValid(ResolvedBoardStateComponent))
    {
        ResolvedBoardStateComponent->OnBoardStateChanged.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleBoardStateChanged);
    }

    if (IsValid(ResolvedGameState))
    {
        ResolvedGameState->OnMatchPhaseChanged.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleMatchPhaseChanged);
        ResolvedGameState->OnMatchResultUpdated.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleMatchResultUpdated);
    }

    if (IsValid(ResolvedParticipantManager))
    {
        ResolvedParticipantManager->OnTurnPhaseChanged.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleTurnPhaseChanged);
        ResolvedParticipantManager->OnActiveControllerChanged.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActiveControllerChanged);
        ResolvedParticipantManager->OnParticipantForfeited.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleParticipantForfeited);
        ResolvedParticipantManager->OnAllParticipantsReady.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleAllParticipantsReady);
        ResolvedParticipantManager->OnGameOver.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleGameOver);
    }

    if (IsValid(ResolvedActionsComponent))
    {
        ResolvedActionsComponent->OnActionPushed.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActionStackChanged);
        ResolvedActionsComponent->OnActionPopped.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActionStackChanged);
        ResolvedActionsComponent->OnActionCompleted.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActionResolved);
        ResolvedActionsComponent->OnActionCancelled.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActionResolved);
        ResolvedActionsComponent->OnBoardChangeRequested.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleBoardChangeRequested);
    }

    if (IsValid(ResolvedGameEventSubsystem))
    {
        ResolvedGameEventSubsystem->OnActiveManagerTagsChanged.RemoveDynamic(
            this, &UConnectIt_DebugStateWidget::HandleActiveManagerTagsChanged);
    }

    bBound = false;
}

// --- Cache refresh ---

void UConnectIt_DebugStateWidget::RefreshBoardState()
{
    CachedBoardState.bValid = IsValid(ResolvedBoardStateComponent);
    if (!CachedBoardState.bValid) return;

    CachedBoardState.CurrentState = ResolvedBoardStateComponent->GetCurrentState();
    CachedBoardState.LastChangeEvent = ResolvedBoardStateComponent->GetChangeEvent();
}

void UConnectIt_DebugStateWidget::RefreshTurnState()
{
    CachedTurnState.bValid = IsValid(ResolvedGameState) && IsValid(ResolvedParticipantManager);
    if (!CachedTurnState.bValid) return;

    CachedTurnState.TurnPhase = ResolvedGameState->GetCurrentTurnPhase();
    CachedTurnState.MatchPhase = ResolvedGameState->GetMatchPhase();
    CachedTurnState.ActiveParticipantIndex = ResolvedGameState->GetActiveParticipantIndex();
    CachedTurnState.TurnNumber = ResolvedGameState->GetActiveTurnNumber();
    CachedTurnState.Participants = ResolvedGameState->GetParticipants();
    CachedTurnState.MatchResult = ResolvedGameState->MatchResult;
}

void UConnectIt_DebugStateWidget::RefreshActionState()
{
    CachedActionState.bValid = IsValid(ResolvedActionsComponent);
    if (!CachedActionState.bValid) return;

    // ActionTag lives on UTurnBasedActionBase itself -- no cast needed.
    // Cache the tag only, never the returned pointer (see header comment).
    if (UTurnBasedActionBase* Top = ResolvedActionsComponent->GetTopAction())
    {
        CachedActionState.TopActionTag = Top->ActionTag;
    }
    else
    {
        CachedActionState.TopActionTag = FGameplayTag();
    }

    if (UTurnBasedActionBase* Root = ResolvedActionsComponent->GetRootAction())
    {
        CachedActionState.RootActionTag = Root->ActionTag;
    }
    else
    {
        CachedActionState.RootActionTag = FGameplayTag();
    }

    CachedActionState.StackDepth = ResolvedActionsComponent->GetStackDepth();
    CachedActionState.bAwaitingRequestConfirmation =
        ResolvedActionsComponent->IsAwaitingRequestConfirmation();
}

void UConnectIt_DebugStateWidget::RefreshEventQueueState()
{
    CachedActiveEventTags = IsValid(ResolvedGameEventSubsystem)
        ? ResolvedGameEventSubsystem->GetTagsInQueue()
        : TArray<FGameplayTag>();
}

// --- Delegate handlers ---

void UConnectIt_DebugStateWidget::HandleBoardManagerReady(AConnectIt_BoardManager* InBoardManager)
{
    BindBoardManager(InBoardManager);
    RefreshBoardState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleBoardStateChanged()
{
    RefreshBoardState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleMatchResultUpdated(const FConnectItMatchResult& Result)
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleTurnPhaseChanged(ETurnPhase NewPhase)
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleActiveControllerChanged(AController* NewActiveController)
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo)
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleAllParticipantsReady()
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleGameOver()
{
    RefreshTurnState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleActionStackChanged(UTurnBasedActionBase* Action)
{
    RefreshActionState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleActionResolved(UTurnBasedAction* Action)
{
    RefreshActionState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleBoardChangeRequested(const FTurnActionRequest& Request)
{
    RefreshActionState();
    OnDebugStateUpdated();
}

void UConnectIt_DebugStateWidget::HandleActiveManagerTagsChanged()
{
    RefreshEventQueueState();
    OnDebugStateUpdated();
}
