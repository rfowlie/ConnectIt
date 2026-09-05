// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ConnectIt_DebugStateWidget.h"

#include "Board/ConnectIt_BoardStateComponent.h"
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

    // Board state -- lives on AConnectIt_GameState now, reachable directly
    // with no "wait for a level-placed actor to register" dance needed.
    ResolvedBoardStateComponent = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (IsValid(ResolvedBoardStateComponent))
    {
        ResolvedBoardStateComponent->OnBoardStateChanged.AddDynamic(
            this, &UConnectIt_DebugStateWidget::HandleBoardStateChanged);
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

void UConnectIt_DebugStateWidget::UnbindAll()
{
    if (!bBound) return;

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

// Every handler below refreshes ALL categories (not just the one its own
// delegate name suggests) via ForceRefreshAll, rather than a per-category
// Refresh*()+OnDebugStateUpdated() pair. This is deliberate, not laziness --
// see Docs/RuntimeStateAccess.md's Turn-End & Resolution Sequencing section:
// several of the turn-state delegates (OnActiveControllerChanged,
// OnRep_ActiveParticipantIndex) are currently dead stubs on the plugin side,
// and OnTurnPhaseChanged can go quiet on rapid turn cycling if CurrentPhase
// settles back on its previously-replicated value within one tick even
// though TurnNumber (no OnRep) still changed. Meanwhile something *does*
// reliably fire every turn (the action-stack delegates, via the reliable
// ClientReceiveTurnNotification RPC) -- so every handler just refreshes
// everything. Debug-only tool; the extra cost is irrelevant, and it means
// no category can go silently stale behind a delegate that doesn't fire
// reliably.

void UConnectIt_DebugStateWidget::HandleBoardStateChanged()
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleMatchResultUpdated(const FConnectItMatchResult& Result)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleTurnPhaseChanged(ETurnPhase NewPhase)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleActiveControllerChanged(AController* NewActiveController)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleAllParticipantsReady()
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleGameOver()
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleActionStackChanged(UTurnBasedActionBase* Action)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleActionResolved(UTurnBasedAction* Action)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleBoardChangeRequested(const FTurnActionRequest& Request)
{
    ForceRefreshAll();
}

void UConnectIt_DebugStateWidget::HandleActiveManagerTagsChanged(const FGameplayTagContainer& TagContainer)
{
    ForceRefreshAll();
}