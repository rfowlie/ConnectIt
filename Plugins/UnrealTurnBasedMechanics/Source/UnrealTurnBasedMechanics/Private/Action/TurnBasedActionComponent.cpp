// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedActionComponent.h"
#include "Action/TurnBasedAction.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"


UTurnBasedActionComponent::UTurnBasedActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UTurnBasedActionComponent::BeginPlay()
{
    Super::BeginPlay();
    // No internal bindings -- caller wires externally
}

// --- Setup ---

void UTurnBasedActionComponent::InitialiseFromLoadout(
    UActionLoadOutDataAsset* InLoadout)
{
    if (!IsValid(InLoadout))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: InitialiseFromLoadout "
                 "called with null loadout on %s"),
            *GetOwner()->GetName());
        return;
    }

    Loadout = InLoadout;
    CloneActionsFromLoadout();
    bIsInitialised = true;

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: %s initialised — "
             "loadout '%s', %d actions, "
             "AutoActivate: %s, AutoEnd: %s"),
        *GetOwner()->GetName(),
        *InLoadout->LoadoutName,
        RuntimeActions.Num(),
        bAutoActivateRequiredActions ? TEXT("On") : TEXT("Off"),
        bAutoEndTurnOnAllRequiredActionsCompleted ? TEXT("On") : TEXT("Off"));
}

void UTurnBasedActionComponent::CloneActionsFromLoadout()
{
    RuntimeActions.Empty();
    if (!IsValid(Loadout)) return;

    AController* Controller = GetOwningController();
    UEnhancedInputComponent* EIC = IsValid(Controller)
        ? Cast<UEnhancedInputComponent>(Controller->InputComponent.Get())
        : nullptr;

    if (!IsValid(EIC))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: No UEnhancedInputComponent "
                 "on %s — selection input will not bind"),
            *GetOwner()->GetName());
    }

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
            TEXT("TurnBasedActionComponent: Cloned '%s' "
                 "(Required: %s, Cancellable: %s, AllowsOptionalInterrupt: %s)"),
            *Clone->ActionTag.ToString(),
            Clone->bIsRequired ? TEXT("Yes") : TEXT("No"),
            Clone->bIsCancellable ? TEXT("Yes") : TEXT("No"),
            Clone->bAllowsOptionalInterrupt ? TEXT("Yes") : TEXT("No"));
    }
}

void UTurnBasedActionComponent::BindActionDelegates(UTurnBasedAction* Action)
{
    if (!IsValid(Action)) return;

    Action->OnChangeRequested.AddDynamic(
        this, &UTurnBasedActionComponent::HandleActionChangeRequested);
    Action->OnActionCompleted.AddDynamic(
        this, &UTurnBasedActionComponent::HandleActionCompleted);
    Action->OnActionCancelled.AddDynamic(
        this, &UTurnBasedActionComponent::HandleActionCancelled);
}

// --- Turn Lifecycle ---

void UTurnBasedActionComponent::NotifyTurnStarted(const int32 InTurnNumber)
{
    CurrentTurnNumber = InTurnNumber;
    ActiveAction      = nullptr;

    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Action->ResetTurnState();
    }

    // Always build the queue so CanEndTurn() works correctly
    // even when bAutoActivateRequiredActions is false
    BuildRequiredQueue();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn %d started on %s "
             "— %d required actions queued"),
        CurrentTurnNumber,
        *GetOwner()->GetName(),
        PendingRequiredActions.Num());
}

void UTurnBasedActionComponent::TickCooldowns(bool bIsMyTurn)
{
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Action->TickCooldown(bIsMyTurn);
    }
}

void UTurnBasedActionComponent::NotifyTurnEnded()
{
    if (IsValid(ActiveAction) && ActiveAction->bIsCancellable)
    {
        CancelActiveAction();
    }
    ActiveAction = nullptr;
    PendingRequiredActions.Empty();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn ended on %s"),
        *GetOwner()->GetName());
}

void UTurnBasedActionComponent::NotifyTurnPaused()
{
    if (IsValid(ActiveAction) && ActiveAction->bIsCancellable)
    {
        CancelActiveAction();
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn paused on %s"),
        *GetOwner()->GetName());
}

void UTurnBasedActionComponent::NotifyTurnResumed()
{
    // Rebuild queue from remaining incomplete required actions
    // and advance if auto-activate is on
    BuildRequiredQueue();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn resumed on %s"),
        *GetOwner()->GetName());
}

// --- Action Control ---

bool UTurnBasedActionComponent::TryActivateAction(FGameplayTag ActionTag)
{
    return TryActivateActionByRef(FindActionByTag(ActionTag));
}

bool UTurnBasedActionComponent::TryActivateActionByRef(UTurnBasedAction* Action)
{
    if (!IsValid(Action)) return false;

    if (!Action->CanActivate())
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: '%s' cannot activate "
                 "— state: %s, uses: %d/%d, cooldown: %d"),
            *Action->ActionTag.ToString(),
            *UEnum::GetValueAsString(Action->State),
            Action->UsesThisTurn,
            Action->MaxUsesPerTurn,
            Action->TurnsUntilAvailable);
        return false;
    }

    // If this is an optional action check whether the active required
    // action allows interruption
    if (!Action->bIsRequired && IsValid(ActiveAction) && ActiveAction->bIsRequired)
    {
        if (!ActiveAction->bAllowsOptionalInterrupt)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("TurnBasedActionComponent: Optional '%s' cannot "
                     "interrupt required '%s' — bAllowsOptionalInterrupt is false"),
                *Action->ActionTag.ToString(),
                *ActiveAction->ActionTag.ToString());
            return false;
        }
    }

    if (IsValid(ActiveAction))
    {
        if (ActiveAction->bIsCancellable)
        {
            CancelActiveAction();
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("TurnBasedActionComponent: Cannot activate '%s' "
                     "— '%s' is active and not cancellable"),
                *Action->ActionTag.ToString(),
                *ActiveAction->ActionTag.ToString());
            return false;
        }
    }

    ActiveAction = Action;
    Action->Activate();
    OnActionActivated.Broadcast(Action);
    return true;
}

void UTurnBasedActionComponent::CancelActiveAction()
{
    if (!IsValid(ActiveAction)) return;
    ActiveAction->Cancel();
    // ActiveAction cleared in HandleActionCancelled
}

bool UTurnBasedActionComponent::CanEndTurn() const
{
    if (!bIsInitialised) return false;

    // No required actions in loadout -- always can end
    const bool bHasRequiredActions = RuntimeActions.ContainsByPredicate(
        [](const UTurnBasedAction* A)
        {
            return IsValid(A) && A->bIsRequired;
        });

    if (!bHasRequiredActions) return true;

    // Has required actions -- all must be complete
    for (const UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (Action->bIsRequired && !Action->IsComplete())
            return false;
    }

    return true;
}

void UTurnBasedActionComponent::RequestTurnEnd()
{
    if (!CanEndTurn())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: RequestTurnEnd — "
                 "required actions not complete on %s"),
            *GetOwner()->GetName());
        return;
    }

    OnTurnEndRequested.Broadcast();
    OnTurnEndRequested_Native.Broadcast();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn end requested on %s"),
        *GetOwner()->GetName());
}

// --- Queries ---

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetAllActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A)) Out.Add(A);
    }
    return Out;
}

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetAvailableActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A) && A->CanActivate()) Out.Add(A);
    }
    return Out;
}

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetRequiredActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A) && A->bIsRequired) Out.Add(A);
    }
    return Out;
}

UTurnBasedAction* UTurnBasedActionComponent::FindActionByTag(
    FGameplayTag Tag) const
{
    UTurnBasedAction* const* Found = RuntimeActions.FindByPredicate(
        [Tag](const UTurnBasedAction* A)
        {
            return IsValid(A) && A->ActionTag == Tag;
        });
    return Found ? *Found : nullptr;
}

// --- Internal Turn Flow ---

void UTurnBasedActionComponent::BuildRequiredQueue()
{
    PendingRequiredActions.Empty();

    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (!Action->bIsRequired) continue;
        if (Action->IsComplete()) continue;
        PendingRequiredActions.Add(Action);
    }

    if (bAutoActivateRequiredActions)
    {
        AdvanceRequiredQueue();
    }
}

void UTurnBasedActionComponent::AdvanceRequiredQueue()
{
    // Clean stale entries from front of queue
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

    // Active non-cancellable action is blocking -- defer
    if (IsValid(ActiveAction) && !ActiveAction->bIsCancellable)
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Deferring required '%s' "
                 "— '%s' is active and not cancellable"),
            *Next->ActionTag.ToString(),
            *ActiveAction->ActionTag.ToString());
        return;
    }

    if (IsValid(ActiveAction) && ActiveAction->bIsCancellable)
    {
        CancelActiveAction();
    }

    if (!Next->CanActivate())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: Required '%s' cannot "
                 "activate — state: %s"),
            *Next->ActionTag.ToString(),
            *UEnum::GetValueAsString(Next->State));
        return;
    }

    ActiveAction = Next;
    Next->Activate();
    OnActionActivated.Broadcast(Next);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Auto-activated required '%s' "
             "(%d remaining in queue)"),
        *Next->ActionTag.ToString(),
        PendingRequiredActions.Num() - 1);
}

void UTurnBasedActionComponent::HandleAllRequiredComplete()
{
    OnTurnEndReady.Broadcast();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: All required actions complete on %s"),
        *GetOwner()->GetName());

    if (bAutoEndTurnOnAllRequiredActionsCompleted)
    {
        RequestTurnEnd();
    }
}

void UTurnBasedActionComponent::CheckAutoTurnEnd()
{
    if (PendingRequiredActions.IsEmpty())
    {
        HandleAllRequiredComplete();
    }
}

// --- Action Delegate Handlers ---

void UTurnBasedActionComponent::HandleActionChangeRequested(
    const FTurnActionRequest& Request)
{
    OnBoardChangeRequested.Broadcast(Request);
    OnBoardChangeRequested_Native.Broadcast(Request);
}

void UTurnBasedActionComponent::HandleActionCompleted(UTurnBasedAction* Action)
{
    if (ActiveAction == Action) ActiveAction = nullptr;

    LogActionRecord(Action, ETurnBasedActionState::Completed);
    OnActionCompleted.Broadcast(Action);

    if (Action->bIsRequired)
    {
        PendingRequiredActions.Remove(Action);

        if (bAutoActivateRequiredActions)
        {
            // Advance to next required or fire all-complete
            AdvanceRequiredQueue();
        }
        else
        {
            // Not auto-activating -- just check if all done
            CheckAutoTurnEnd();
        }
    }
    else
    {
        // Optional completed -- resume required queue if auto-activate on
        if (bAutoActivateRequiredActions)
        {
            AdvanceRequiredQueue();
        }
    }
}

void UTurnBasedActionComponent::HandleActionCancelled(UTurnBasedAction* Action)
{
    if (ActiveAction == Action) ActiveAction = nullptr;

    LogActionRecord(Action, ETurnBasedActionState::Cancelled,
        TEXT("Cancelled by player or system"));
    OnActionCancelled.Broadcast(Action);
}

// --- Logging ---

void UTurnBasedActionComponent::LogActionRecord(
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
        TEXT("TurnBasedActionComponent: [Turn %d] '%s' -> %s%s"),
        Record.TurnNumber,
        *Record.ActionTag.ToString(),
        *UEnum::GetValueAsString(OutcomeState),
        Note.IsEmpty()
            ? TEXT("")
            : *FString::Printf(TEXT(" (%s)"), *Note));
}

// --- Helpers ---

AController* UTurnBasedActionComponent::GetOwningController() const
{
    return GetOwner<AController>();
}