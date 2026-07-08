// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/TurnBasedActionsComponent.h"
#include "Action/TurnBasedAction.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"


UTurnBasedActionsComponent::UTurnBasedActionsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UTurnBasedActionsComponent::BeginPlay()
{
    Super::BeginPlay();
}

// --- Setup ---

void UTurnBasedActionsComponent::InitialiseFromLoadout(UActionLoadOutDataAsset* InLoadout)
{
    if (!IsValid(InLoadout))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionsComponent: InitialiseFromLoadout "
                 "called with null loadout on %s"),
            *GetOwner()->GetName());
        return;
    }

    Loadout = InLoadout;
    CloneActionsFromLoadout();
    CreateRootAction();
    bIsInitialised = true;

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: %s initialised — "
             "loadout '%s', %d actions, root: %s, "
             "AutoActivate: %s, AutoEnd: %s"),
        *GetOwner()->GetName(),
        *InLoadout->LoadoutName,
        RuntimeActions.Num(),
        IsValid(RootAction) ? *RootAction->ActionTag.ToString() : TEXT("none"),
        bAutoActivateRequiredActions ? TEXT("On") : TEXT("Off"),
        bAutoEndTurnOnAllRequiredActionsCompleted ? TEXT("On") : TEXT("Off"));
}

void UTurnBasedActionsComponent::CloneActionsFromLoadout()
{
    RuntimeActions.Empty();
    if (!IsValid(Loadout)) return;

    AController* Controller = GetOwningController();
    UEnhancedInputComponent* EIC = IsValid(Controller)
        ? Cast<UEnhancedInputComponent>(Controller->InputComponent.Get())
        : nullptr;

    for (const UTurnBasedAction* Source : Loadout->GetPermittedActions())
    {
        if (!IsValid(Source)) continue;

        UTurnBasedAction* Clone = DuplicateObject<UTurnBasedAction>(
            Source, this);
        if (!IsValid(Clone)) continue;

        Clone->InitialiseAction(Controller, EIC);
        BindActionDelegates(Clone);
        RuntimeActions.Add(Clone);

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionsComponent: Cloned '%s' "
                 "(Required: %s, Cancellable: %s, "
                 "AllowsOptionalInterrupt: %s)"),
            *Clone->ActionTag.ToString(),
            Clone->bIsRequired ? TEXT("Yes") : TEXT("No"),
            Clone->bIsCancellable ? TEXT("Yes") : TEXT("No"),
            Clone->bAllowsOptionalInterrupt ? TEXT("Yes") : TEXT("No"));
    }
}

void UTurnBasedActionsComponent::CreateRootAction()
{
    RootAction = nullptr;
    if (!RootActionClass) return;

    AController* Controller = GetOwningController();
    UEnhancedInputComponent* EIC = IsValid(Controller)
        ? Cast<UEnhancedInputComponent>(Controller->InputComponent.Get())
        : nullptr;

    RootAction = NewObject<UTurnBasedAction>(this, RootActionClass);
    if (!IsValid(RootAction)) return;

    RootAction->InitialiseAction(Controller, EIC);
    BindActionDelegates(RootAction);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Root action created — '%s'"),
        *RootAction->ActionTag.ToString());
}

void UTurnBasedActionsComponent::BindActionDelegates(UTurnBasedAction* Action)
{
    if (!IsValid(Action)) return;

    Action->OnChangeRequested.AddDynamic(
        this, &UTurnBasedActionsComponent::HandleActionChangeRequested);
    Action->OnActionCompleted.AddDynamic(
        this, &UTurnBasedActionsComponent::HandleActionCompleted);
    Action->OnActionCancelled.AddDynamic(
        this, &UTurnBasedActionsComponent::HandleActionCancelled);
}

// --- Turn Lifecycle ---

void UTurnBasedActionsComponent::NotifyTurnStarted(int32 InTurnNumber)
{
    CurrentTurnNumber = InTurnNumber;
    ActiveAction      = nullptr;

    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Action->ResetTurnState();
    }

    if (IsValid(RootAction)) RootAction->ResetTurnState();

    // Build required queue first so CanEndTurn() is correct
    BuildRequiredQueue();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Turn %d started on %s "
             "— %d required actions queued"),
        CurrentTurnNumber,
        *GetOwner()->GetName(),
        PendingRequiredActions.Num());
}

void UTurnBasedActionsComponent::TickCooldowns(bool bIsMyTurn)
{
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Action->TickCooldown(bIsMyTurn);
    }
}

void UTurnBasedActionsComponent::NotifyTurnEnded()
{
    // Cancel active action if it is not the root
    if (IsValid(ActiveAction) && ActiveAction != RootAction)
    {
        if (ActiveAction->bIsCancellable) CancelActiveAction();
    }

    // Deactivate root cleanly
    if (IsValid(RootAction) && ActiveAction == RootAction)
    {
        RootAction->Cancel();
    }

    ActiveAction = nullptr;
    PendingRequiredActions.Empty();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Turn ended on %s"),
        *GetOwner()->GetName());
}

void UTurnBasedActionsComponent::NotifyTurnPaused()
{
    // Pause active non-root action
    if (IsValid(ActiveAction)
        && ActiveAction != RootAction
        && ActiveAction->bIsCancellable)
    {
        CancelActiveAction();
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Turn paused on %s"),
        *GetOwner()->GetName());
}

void UTurnBasedActionsComponent::NotifyTurnResumed()
{
    // Rebuild queue and return to appropriate state
    BuildRequiredQueue();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Turn resumed on %s"),
        *GetOwner()->GetName());
}

// --- Action Control ---

bool UTurnBasedActionsComponent::TryActivateActionByTag(FGameplayTag ActionTag)
{
    return TryActivateActionByRef(FindActionByTag(ActionTag));
}

bool UTurnBasedActionsComponent::TryActivateActionByRef(UTurnBasedAction* Action)
{
    if (!IsValid(Action)) return false;

    if (!Action->CanActivate())
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionsComponent: '%s' cannot activate "
                 "— state: %s, uses: %d/%d, cooldown: %d"),
            *Action->ActionTag.ToString(),
            *UEnum::GetValueAsString(Action->State),
            Action->UsesThisTurn,
            Action->MaxUsesPerTurn,
            Action->TurnsUntilAvailable);
        return false;
    }

    // Check optional interrupt permission on active required action
    const bool bActiveIsRequired = IsValid(ActiveAction)
        && ActiveAction->bIsRequired
        && ActiveAction != RootAction;

    if (!Action->bIsRequired && bActiveIsRequired)
    {
        if (!ActiveAction->bAllowsOptionalInterrupt)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("TurnBasedActionsComponent: Optional '%s' cannot "
                     "interrupt required '%s' — bAllowsOptionalInterrupt false"),
                *Action->ActionTag.ToString(),
                *ActiveAction->ActionTag.ToString());
            return false;
        }
    }

    // Cancel current if possible
    // Root action is always interruptible by TryActivateAction
    if (IsValid(ActiveAction))
    {
        const bool bIsRoot = (ActiveAction == RootAction);
        if (!bIsRoot && !ActiveAction->bIsCancellable)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("TurnBasedActionsComponent: Cannot activate '%s' "
                     "— '%s' is active and not cancellable"),
                *Action->ActionTag.ToString(),
                *ActiveAction->ActionTag.ToString());
            return false;
        }

        // Interrupt root or cancel active
        if (bIsRoot)
        {
            // Interrupt root -- do not fire cancel delegate
            // root will resume when this action completes
            ActiveAction = nullptr;
        }
        else
        {
            CancelActiveAction();
        }
    }

    ActiveAction = Action;
    Action->Activate();
    OnActionActivated.Broadcast(Action);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Activated '%s'"),
        *Action->ActionTag.ToString());

    return true;
}

void UTurnBasedActionsComponent::CancelActiveAction()
{
    if (!IsValid(ActiveAction)) return;
    if (ActiveAction == RootAction) return; // Root cannot be cancelled by caller

    ActiveAction->Cancel();
    // ActiveAction cleared in HandleActionCancelled
    // which then calls ReturnToRoot
}

bool UTurnBasedActionsComponent::CanEndTurn() const
{
    if (!bIsInitialised) return false;

    // No required actions -- always can end
    const bool bHasRequired = RuntimeActions.ContainsByPredicate(
        [](const UTurnBasedAction* A)
        {
            return IsValid(A) && A->bIsRequired;
        });

    if (!bHasRequired) return true;

    // All required must be complete
    for (const UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (Action->bIsRequired && !Action->IsComplete())
            return false;
    }

    return true;
}

void UTurnBasedActionsComponent::RequestTurnEnd()
{
    if (!CanEndTurn())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionsComponent: RequestTurnEnd — "
                 "required actions not complete on %s"),
            *GetOwner()->GetName());
        return;
    }

    OnTurnEndRequested.Broadcast();
    OnTurnEndRequested_Native.Broadcast();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Turn end requested on %s"),
        *GetOwner()->GetName());
}

// --- Queries ---

TArray<UTurnBasedAction*> UTurnBasedActionsComponent::GetAllActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A)) Out.Add(A);
    }
    return Out;
}

TArray<UTurnBasedAction*> UTurnBasedActionsComponent::GetAvailableActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A) && A->CanActivate()) Out.Add(A);
    }
    return Out;
}

TArray<UTurnBasedAction*> UTurnBasedActionsComponent::GetRequiredActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A) && A->bIsRequired) Out.Add(A);
    }
    return Out;
}

UTurnBasedAction* UTurnBasedActionsComponent::FindActionByTag(FGameplayTag Tag) const
{
    // Check runtime actions
    UTurnBasedAction* const* Found = RuntimeActions.FindByPredicate(
        [Tag](const UTurnBasedAction* A)
        {
            return IsValid(A) && A->ActionTag == Tag;
        });

    if (Found) return *Found;

    // Check root action
    if (IsValid(RootAction) && RootAction->ActionTag == Tag)
        return RootAction;

    return nullptr;
}

// --- Internal ---

void UTurnBasedActionsComponent::ReturnToRoot()
{
    if (!IsValid(RootAction))
    {
        ActiveAction = nullptr;
        return;
    }

    if (!RootAction->CanActivate())
    {
        // Root might be exhausted or on cooldown in edge cases
        // Reset it explicitly since root should always be available
        RootAction->ResetTurnState();
    }

    ActiveAction = RootAction;
    RootAction->Activate();
    OnActionActivated.Broadcast(RootAction);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Returned to root action '%s'"),
        *RootAction->ActionTag.ToString());
}

void UTurnBasedActionsComponent::BuildRequiredQueue()
{
    PendingRequiredActions.Empty();

    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (!Action->bIsRequired) continue;
        if (Action->IsComplete()) continue;
        PendingRequiredActions.Add(Action);
    }

    if (bAutoActivateRequiredActions && !PendingRequiredActions.IsEmpty())
    {
        AdvanceRequiredQueue();
    }
    else
    {
        // No required actions to auto-activate -- return to root
        ReturnToRoot();
    }
}

void UTurnBasedActionsComponent::AdvanceRequiredQueue()
{
    // Clean stale entries
    while (!PendingRequiredActions.IsEmpty())
    {
        UTurnBasedAction* Next = PendingRequiredActions[0];
        if (!IsValid(Next) || Next->IsComplete())
        {
            PendingRequiredActions.RemoveAt(0);
            continue;
        }
        break;
    }

    if (PendingRequiredActions.IsEmpty())
    {
        HandleAllRequiredComplete();
        return;
    }

    UTurnBasedAction* Next = PendingRequiredActions[0];

    // Active non-cancellable non-root action is blocking
    if (IsValid(ActiveAction)
        && ActiveAction != RootAction
        && !ActiveAction->bIsCancellable)
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionsComponent: Deferring required '%s' "
                 "— '%s' is active and not cancellable"),
            *Next->ActionTag.ToString(),
            *ActiveAction->ActionTag.ToString());
        return;
    }

    // Interrupt root or cancel active non-root
    if (IsValid(ActiveAction))
    {
        if (ActiveAction == RootAction)
        {
            ActiveAction = nullptr; // Interrupt root silently
        }
        else if (ActiveAction->bIsCancellable)
        {
            CancelActiveAction();
        }
    }

    if (!Next->CanActivate())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionsComponent: Required '%s' cannot "
                 "activate — state: %s"),
            *Next->ActionTag.ToString(),
            *UEnum::GetValueAsString(Next->State));
        ReturnToRoot();
        return;
    }

    ActiveAction = Next;
    Next->Activate();
    OnActionActivated.Broadcast(Next);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: Auto-activated required '%s' "
             "(%d remaining)"),
        *Next->ActionTag.ToString(),
        PendingRequiredActions.Num() - 1);
}

void UTurnBasedActionsComponent::HandleAllRequiredComplete()
{
    OnTurnEndReady.Broadcast();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: All required complete on %s"),
        *GetOwner()->GetName());

    if (bAutoEndTurnOnAllRequiredActionsCompleted)
    {
        RequestTurnEnd();
    }
    else
    {
        // Return to root -- player decides when to end
        ReturnToRoot();
    }
}

void UTurnBasedActionsComponent::CheckRequiredCompletion()
{
    if (PendingRequiredActions.IsEmpty())
    {
        HandleAllRequiredComplete();
    }
    else
    {
        ReturnToRoot();
    }
}

// --- Action Delegate Handlers ---

void UTurnBasedActionsComponent::HandleActionChangeRequested(
    const FTurnActionRequest& Request)
{
    OnBoardChangeRequested.Broadcast(Request);
    OnBoardChangeRequested_Native.Broadcast(Request);
}

void UTurnBasedActionsComponent::HandleActionCompleted(
    UTurnBasedAction* Action)
{
    // Root action completing is unusual -- it should never complete
    // naturally but handle it defensively
    if (Action == RootAction)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionsComponent: Root action completed "
                 "unexpectedly on %s"),
            *GetOwner()->GetName());
        ActiveAction = nullptr;
        return;
    }

    if (ActiveAction == Action) ActiveAction = nullptr;

    LogActionRecord(Action, ETurnBasedActionState::Completed);
    OnActionCompleted.Broadcast(Action);

    if (Action->bIsRequired)
    {
        PendingRequiredActions.Remove(Action);

        if (bAutoActivateRequiredActions)
        {
            AdvanceRequiredQueue();
        }
        else
        {
            CheckRequiredCompletion();
        }
    }
    else
    {
        // Optional completed -- return to root or advance required queue
        if (bAutoActivateRequiredActions && !PendingRequiredActions.IsEmpty())
        {
            AdvanceRequiredQueue();
        }
        else
        {
            ReturnToRoot();
        }
    }
}

void UTurnBasedActionsComponent::HandleActionCancelled(
    UTurnBasedAction* Action)
{
    if (Action == RootAction)
    {
        // Root was cancelled by system (turn end, pause)
        // Do not return to root
        if (ActiveAction == Action) ActiveAction = nullptr;
        return;
    }

    if (ActiveAction == Action) ActiveAction = nullptr;

    LogActionRecord(Action, ETurnBasedActionState::Cancelled,
        TEXT("Cancelled"));
    OnActionCancelled.Broadcast(Action);

    // Always return to root after any non-root cancellation
    ReturnToRoot();
}

// --- Logging ---

void UTurnBasedActionsComponent::LogActionRecord(
    UTurnBasedAction* Action,
    ETurnBasedActionState OutcomeState,
    const FString& Note)
{
    if (!IsValid(Action)) return;

    FTurnBasedActionRecord Record;
    Record.ActionTag    = Action->ActionTag;
    Record.OutcomeState = OutcomeState;
    Record.TurnNumber   = CurrentTurnNumber;
    Record.DebugNote    = Note;
    Record.Timestamp    = GetWorld()
        ? GetWorld()->GetTimeSeconds() : 0.f;

    ActionHistory.Add(Record);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionsComponent: [Turn %d] '%s' -> %s%s"),
        Record.TurnNumber,
        *Record.ActionTag.ToString(),
        *UEnum::GetValueAsString(OutcomeState),
        Note.IsEmpty()
            ? TEXT("")
            : *FString::Printf(TEXT(" (%s)"), *Note));
}

// --- Helpers ---

AController* UTurnBasedActionsComponent::GetOwningController() const
{
    return GetOwner<AController>();
}
