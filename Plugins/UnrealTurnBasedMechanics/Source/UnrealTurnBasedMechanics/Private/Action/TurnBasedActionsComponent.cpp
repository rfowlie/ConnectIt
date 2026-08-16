// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/TurnBasedActionsComponent.h"
#include "UnrealTurnBasedMechanics.h"
#include "Action/TurnBasedAction.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"


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

void UTurnBasedActionsComponent::InitialiseFromLoadout(UActionLoadoutDataAsset* InLoadout)
{
    if (!IsValid(InLoadout))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: InitialiseFromLoadout "
                 "called with null loadout on %s"),
            *GetOwner()->GetName());
        return;
    }

    Loadout = InLoadout;
    CloneActionsFromLoadout();
    CreateSystemActions();
    bIsInitialised = true;

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: %s initialised — "
             "loadout '%s', %d runtime actions "
             "root: %s, idle: %s, spectator: %s, pause: %s, awaiting: %s"),
        *GetOwner()->GetName(),
        *InLoadout->LoadoutName,
        RuntimeActions.Num(),
        IsValid(RootAction)
            ? *RootAction->ActionTag.ToString() : TEXT("none"),
        IsValid(IdleViewerAction)
            ? *IdleViewerAction->ActionTag.ToString() : TEXT("none"),
        IsValid(SpectatorViewerAction)
            ? *SpectatorViewerAction->ActionTag.ToString() : TEXT("none"),
        IsValid(PauseViewerAction)
            ? *PauseViewerAction->ActionTag.ToString() : TEXT("none"),
        IsValid(AwaitingConfirmationAction)
            ? *AwaitingConfirmationAction->ActionTag.ToString() : TEXT("none"));
}

void UTurnBasedActionsComponent::CloneActionsFromLoadout()
{
    RuntimeActions.Empty();
    if (!IsValid(Loadout)) return;

    AController* Controller = GetOwningController();
    UEnhancedInputComponent* EIC = IsValid(Controller)
        ? Cast<UEnhancedInputComponent>(Controller->InputComponent.Get())
        : nullptr;

    /*
     * TODO: don't love this cast
     * do we need to be using AController instead of PlayerController
     * Will AI controller even need an actions component?
     */
    APlayerController* PC = Cast<APlayerController>(Controller);
    UEnhancedInputLocalPlayerSubsystem* EILP = (IsValid(PC) && IsValid(PC->GetLocalPlayer()))
        ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())
        : nullptr;

    for (UTurnBasedAction* Source : Loadout->GetPermittedActions())
    {
        if (!IsValid(Source)) continue;

        UTurnBasedAction* Clone = DuplicateObject<UTurnBasedAction>(Source, this);
        if (!IsValid(Clone)) continue;

        Clone->InitialiseAction(Controller, EIC, EILP);
        BindActionDelegates(Clone);
        RuntimeActions.Add(Clone);

        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedActionsComponent: Cloned '%s' "
                 "(Required: %s, Cancellable: %s, "
                 "AllowsOptionalInterrupt: %s)"),
            *Clone->ActionTag.ToString(),
            Clone->bIsRequired ? TEXT("Yes") : TEXT("No"),
            Clone->bIsCancellable ? TEXT("Yes") : TEXT("No"),
            Clone->bAllowsOptionalInterrupt ? TEXT("Yes") : TEXT("No"));
    }
}

void UTurnBasedActionsComponent::CreateSystemActions()
{
    if (!IsValid(Loadout))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: CreateSystemActions called "
                 "with no loadout on %s"),
            *GetOwner()->GetName());
        return;
    }

    AController* Controller = GetOwningController();
    UEnhancedInputComponent* EIC = IsValid(Controller)
        ? Cast<UEnhancedInputComponent>(Controller->InputComponent.Get())
        : nullptr;
    APlayerController* PC = Cast<APlayerController>(Controller);
    UEnhancedInputLocalPlayerSubsystem* EILP = (IsValid(PC) && IsValid(PC->GetLocalPlayer()))
        ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())
        : nullptr;

    // Root action -- mandatory
    RootAction = Loadout->GetRootAction(this);
    if (IsValid(RootAction))
    {
        RootAction->InitialiseAction(Controller, EIC, EILP);
        BindActionDelegates(RootAction);
    }
    else
    {
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedActionsComponent: RootActionClass not set "
                 "in loadout '%s' on %s. "
                 "Root action is mandatory."),
            *Loadout->LoadoutName,
            *GetOwner()->GetName());
    }

    // Viewer actions -- optional but warned in DataAsset validation
    IdleViewerAction           = Loadout->GetIdleViewerAction(this);
    SpectatorViewerAction      = Loadout->GetSpectatorAction(this);
    PauseViewerAction          = Loadout->GetPauseAction(this);
    AwaitingConfirmationAction = Loadout->GetAwaitingConfirmationAction(this);

    // Give each viewer action OwningController early (same point RootAction
    // gets it via InitialiseAction above), not deferred until Activate
    if (IsValid(IdleViewerAction))           IdleViewerAction->InitialiseAction(Controller);
    if (IsValid(SpectatorViewerAction))      SpectatorViewerAction->InitialiseAction(Controller);
    if (IsValid(PauseViewerAction))          PauseViewerAction->InitialiseAction(Controller);
    if (IsValid(AwaitingConfirmationAction)) AwaitingConfirmationAction->InitialiseAction(Controller);

    WarnIfViewerActionMissing(IdleViewerAction, TEXT("IdleViewerActionClass"),
        TEXT("Stack will remain unchanged on turn end."));

    WarnIfViewerActionMissing(SpectatorViewerAction, TEXT("SpectatorViewerActionClass"),
        TEXT("Stack will remain unchanged on opponent turn start."));

    WarnIfViewerActionMissing(AwaitingConfirmationAction, TEXT("AwaitingConfirmationActionClass"),
        TEXT("Board-change requests will still correctly wait for server "
             "confirmation before completing, but with no input/stack "
             "blocking or waiting UI in the meantime."));
}

void UTurnBasedActionsComponent::WarnIfViewerActionMissing(
    const UTurnBasedSpectatorAction* Action,
    const TCHAR* ClassPropertyName,
    const TCHAR* Consequence) const
{
    if (IsValid(Action)) return;

    UE_LOG(LogTurnBasedMechanics, Warning,
        TEXT("TurnBasedActionsComponent: No %s set in loadout '%s' on %s. %s"),
        ClassPropertyName,
        *Loadout->LoadoutName,
        *GetOwner()->GetName(),
        Consequence);
}

void UTurnBasedActionsComponent::BindActionDelegates(UTurnBasedAction* Action)
{
    if (!IsValid(Action)) return;

    Action->OnChangeRequested.AddDynamic(
        this, &UTurnBasedActionsComponent::HandleBoardChangeRequested);

    Action->OnActionCompleted.AddDynamic(
        this, &UTurnBasedActionsComponent::HandleActionCompleted);

    Action->OnActionCancelled.AddDynamic(
        this, &UTurnBasedActionsComponent::HandleActionCancelled);

    // Wire next action request -- component finds and pushes the action
    Action->OnNextActionRequested_Native.AddLambda(
        [this](TSubclassOf<UTurnBasedAction> NextClass)
        {
            HandleNextActionRequested(NextClass);
        });
}

void UTurnBasedActionsComponent::HandleNextActionRequested(TSubclassOf<UTurnBasedAction> NextClass)
{
    if (!NextClass) return;

    // Find matching runtime action by class
    UTurnBasedAction* const* Found = RuntimeActions.FindByPredicate(
        [NextClass](const UTurnBasedAction* Action)
        {
            return IsValid(Action) && Action->GetClass() == NextClass;
        });

    if (!Found || !IsValid(*Found))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: NextAction '%s' not found "
                 "in runtime actions on %s"),
            *NextClass->GetName(),
            *GetOwner()->GetName());
        return;
    }

    TryPushActionByRef(*Found);
}

// --- Turn Lifecycle ---

void UTurnBasedActionsComponent::NotifyTurnStarted(const FTurnStartContext& Context)
{
    CurrentTurnNumber = Context.TurnNumber;

    // Tick cooldowns -- it is our turn
    TickCooldowns(true);

    // Reset completions on all runtime actions
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Action->ResetTurnState();
    }

    // Fire designer hook -- default clears stack and pushes root
    OnTurnStarted(Context);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Turn %d started on %s "
             "(own turn #%d) — stack depth: %d"),
        CurrentTurnNumber,
        *GetOwner()->GetName(),
        Context.ActiveParticipant.TurnsTaken,
        ActionStack.Num());
}

void UTurnBasedActionsComponent::NotifyOpponentTurnStarted(const FTurnStartContext& Context)
{
    // Tick cooldowns -- not our turn
    TickCooldowns(false);

    // Fire designer hook -- default clears stack and pushes spectator
    OnOpponentTurnStarted(Context);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Opponent turn started on %s "
             "— turn %d, %s's own turn #%d, stack depth: %d"),
        *GetOwner()->GetName(),
        Context.TurnNumber,
        *Context.ActiveParticipant.GetDisplayName(),
        Context.ActiveParticipant.TurnsTaken,
        ActionStack.Num());
}

void UTurnBasedActionsComponent::NotifyTurnEnded()
{
    if (!IsValid(IdleViewerAction))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: NotifyTurnEnded called but "
                 "IdleViewerActionClass not set on %s"),
            *GetOwner()->GetName());
        return;
    }

    // Clear stack and push idle viewer
    // Idle is appropriate during Updating phase
    ClearAndPush(IdleViewerAction);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Turn ended on %s "
             "— idle viewer active"),
        *GetOwner()->GetName());
}

void UTurnBasedActionsComponent::NotifyPaused()
{
    if (!IsValid(PauseViewerAction))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: NotifyPaused called but "
                 "PauseViewerActionClass not set on %s — "
                 "no pause action will be pushed"),
            *GetOwner()->GetName());
        return;
    }

    // Push pause on top -- stack preserved below
    PushAction(PauseViewerAction);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Paused on %s "
             "— pause viewer pushed, stack depth: %d"),
        *GetOwner()->GetName(),
        ActionStack.Num());
}

void UTurnBasedActionsComponent::NotifyUnpaused()
{
    // Pop pause viewer -- whatever was below reactivates
    UTurnBasedActionBase* Popped = SafePopAction();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Unpaused on %s "
             "— popped '%s', stack depth: %d"),
        *GetOwner()->GetName(),
        IsValid(Popped) ? *Popped->ActionTag.ToString() : TEXT("null"),
        ActionStack.Num());
}

void UTurnBasedActionsComponent::NotifyMatchEnded()
{
    // Clear stack -- push idle as safe non-empty end state
    if (IsValid(IdleViewerAction))
    {
        ClearAndPush(IdleViewerAction);
    }
    else
    {
        // No idle set -- just clear everything
        ClearStack();
    }

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Match ended on %s "
             "— stack cleared"),
        *GetOwner()->GetName());
}

void UTurnBasedActionsComponent::TickCooldowns(bool bIsMyTurn)
{
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Action->TickCooldown(bIsMyTurn);
    }
}

// --- Stack Control ---

void UTurnBasedActionsComponent::PushAction(UTurnBasedActionBase* Action)
{
    if (bAwaitingRequestConfirmation)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: PushAction blocked -- "
                 "awaiting board-change request confirmation on %s"),
            *GetOwner()->GetName());
        return;
    }

    if (!IsValid(Action))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: PushAction called "
                 "with null action on %s"),
            *GetOwner()->GetName());
        return;
    }

    // Force deactivate current top without popping
    if (ActionStack.Num() > 0 && IsValid(ActionStack.Last()))
    {
        ActionStack.Last()->ForceDeactivate();
    }

    ActionStack.Add(Action);
    Action->Activate(GetOwningController());
    OnActionPushed.Broadcast(Action);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Pushed '%s' "
             "— stack depth: %d"),
        *Action->ActionTag.ToString(),
        ActionStack.Num());
}

UTurnBasedActionBase* UTurnBasedActionsComponent::SafePopAction()
{
    if (bAwaitingRequestConfirmation)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: SafePopAction blocked -- "
                 "awaiting board-change request confirmation on %s"),
            *GetOwner()->GetName());
        return nullptr;
    }

    if (ActionStack.Num() <= 1)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: SafePopAction blocked "
                 "— cannot pop last action in stack on %s"),
            *GetOwner()->GetName());
        return nullptr;
    }

    UTurnBasedActionBase* Popped = ActionStack.Last();

    if (IsValid(Popped))
    {
        Popped->ForceDeactivate();
    }

    OnActionPopped.Broadcast(Popped);

    // Reactivate new top
    if (ActionStack.Num() > 0 && IsValid(ActionStack.Last()))
    {
        ActionStack.Last()->Activate(GetOwningController());
    }

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Popped '%s' "
             "— stack depth: %d, new top: '%s'"),
        IsValid(Popped) ? *Popped->ActionTag.ToString() : TEXT("null"),
        ActionStack.Num(),
        (ActionStack.Num() > 0 && IsValid(ActionStack.Last()))
            ? *ActionStack.Last()->ActionTag.ToString()
            : TEXT("none"));

    return Popped;
}

void UTurnBasedActionsComponent::ClearStack()
{
    // Force deactivate all from top to bottom
    for (int32 i = ActionStack.Num() - 1; i >= 0; i--)
    {
        if (IsValid(ActionStack[i]))
        {
            ActionStack[i]->ForceDeactivate();
        }
    }

    ActionStack.Empty();
}

void UTurnBasedActionsComponent::ClearAndPush(UTurnBasedActionBase* NewRoot)
{
    if (bAwaitingRequestConfirmation)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: ClearAndPush blocked -- "
                 "awaiting board-change request confirmation on %s"),
            *GetOwner()->GetName());
        return;
    }

    if (!IsValid(NewRoot))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: ClearAndPush called "
                 "with null action on %s"),
            *GetOwner()->GetName());
        return;
    }

    ClearStack();

    // Push and activate new root
    ActionStack.Add(NewRoot);
    NewRoot->Activate(GetOwningController());
    OnActionPushed.Broadcast(NewRoot);
    
    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Stack cleared and '%s' pushed "
             "as new root on %s"),
        *NewRoot->ActionTag.ToString(),
        *GetOwner()->GetName());
}

// --- Action Control ---

bool UTurnBasedActionsComponent::TryPushAction(FGameplayTag ActionTag)
{
    return TryPushActionByRef(FindActionByTag(ActionTag));
}

bool UTurnBasedActionsComponent::TryPushActionByRef(UTurnBasedAction* Action)
{
    if (bAwaitingRequestConfirmation)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: TryPushActionByRef blocked -- "
                 "awaiting board-change request confirmation on %s"),
            *GetOwner()->GetName());
        return false;
    }

    if (!IsValid(Action))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: TryPushAction "
                 "called with null action on %s"),
            *GetOwner()->GetName());
        return false;
    }

    if (!Action->CanActivate())
    {
        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedActionsComponent: '%s' cannot activate "
                 "— completions: %d/%d, cooldown: %d"),
            *Action->ActionTag.ToString(),
            Action->CompletionsThisTurn,
            Action->MaxCompletionsPerTurn,
            Action->TurnsUntilAvailable);
        return false;
    }

    // Check optional interrupt permission on current top
    UTurnBasedActionBase* CurrentTop = GetTopAction();
    if (IsValid(CurrentTop) && !Action->bIsRequired)
    {
        if (UTurnBasedAction* TopAction =
            Cast<UTurnBasedAction>(CurrentTop))
        {
            if (TopAction->bIsRequired
                && !TopAction->bAllowsOptionalInterrupt)
            {
                UE_LOG(LogTurnBasedMechanics, Warning,
                    TEXT("TurnBasedActionsComponent: Optional '%s' "
                         "cannot interrupt required '%s' — "
                         "bAllowsOptionalInterrupt is false"),
                    *Action->ActionTag.ToString(),
                    *TopAction->ActionTag.ToString());
                return false;
            }
        }
    }

    PushAction(Action);
    return true;
}

void UTurnBasedActionsComponent::CancelTopAction()
{
    UTurnBasedAction* TopAction = Cast<UTurnBasedAction>(GetTopAction());

    if (!IsValid(TopAction))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: CancelTopAction — "
                 "top of stack is not a UTurnBasedAction on %s"),
            *GetOwner()->GetName());
        return;
    }

    if (!TopAction->bIsCancellable)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: CancelTopAction — "
                 "'%s' is not cancellable"),
            *TopAction->ActionTag.ToString());
        return;
    }

    // Cancel fires OnActionCancelled which calls HandleActionCancelled
    // which calls SafePopAction
    TopAction->Cancel();
}

bool UTurnBasedActionsComponent::CanEndTurn() const
{
    return CanAutoEndTurn();
}

void UTurnBasedActionsComponent::RequestTurnEnd()
{
    if (!CanEndTurn())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: RequestTurnEnd — "
                 "CanAutoEndTurn returned false on %s"),
            *GetOwner()->GetName());
        return;
    }

    OnTurnEndRequested.Broadcast();
    OnTurnEndRequested_Native.Broadcast();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: Turn end requested on %s"),
        *GetOwner()->GetName());
}

// --- Queries ---

UTurnBasedActionBase* UTurnBasedActionsComponent::GetTopAction() const
{
    if (ActionStack.IsEmpty()) return nullptr;
    return ActionStack.Last();
}

bool UTurnBasedActionsComponent::IsRootOnTop() const
{
    if (ActionStack.IsEmpty()) return false;
    return ActionStack.Last() == RootAction;
}

TArray<UTurnBasedAction*> UTurnBasedActionsComponent::GetAllRuntimeActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* A : RuntimeActions)
    {
        if (IsValid(A)) Out.Add(A);
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

// --- Designer Hooks ---

void UTurnBasedActionsComponent::OnTurnStarted_Implementation(
    const FTurnStartContext& Context)
{
    if (!IsValid(RootAction))
    {
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedActionsComponent: OnTurnStarted — "
                 "RootActionClass not set on %s. "
                 "Root action is mandatory. "
                 "Create a do-nothing action if needed."),
            *GetOwner()->GetName());
        return;
    }

    ClearAndPush(RootAction);
}

void UTurnBasedActionsComponent::OnOpponentTurnStarted_Implementation(
    const FTurnStartContext& Context)
{
    if (!IsValid(SpectatorViewerAction))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedActionsComponent: OnOpponentTurnStarted — "
                 "SpectatorViewerActionClass not set on %s. "
                 "Stack will remain unchanged."),
            *GetOwner()->GetName());
        return;
    }

    ClearAndPush(SpectatorViewerAction);
}

bool UTurnBasedActionsComponent::CanAutoEndTurn_Implementation() const
{
    // Default -- all required actions must have at least one completion
    for (const UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (Action->bIsRequired && Action->CompletionsThisTurn == 0)
        {
            return false;
        }
    }
    return true;
}

// --- Completion Handlers ---

void UTurnBasedActionsComponent::HandleActionCompleted(UTurnBasedAction* Action)
{
    LogActionRecord(Action, TEXT("Completed"));
    SafePopAction();
    OnActionCompleted.Broadcast(Action);
    CheckAutoEndTurn();
}

void UTurnBasedActionsComponent::HandleActionCancelled(UTurnBasedAction* Action)
{
    LogActionRecord(Action, TEXT("Cancelled"));
    SafePopAction();
    OnActionCancelled.Broadcast(Action);
}

void UTurnBasedActionsComponent::HandleBoardChangeRequested(const FTurnActionRequest& Request)
{
    if (bAwaitingRequestConfirmation)
    {
        // Shouldn't be reachable -- pushing AwaitingConfirmationAction
        // force-deactivates the requesting action, tearing down its
        // hover/selection bindings, so it can't legitimately fire a second
        // request while still waiting on the first. Kept as defense-in-depth
        // for a future action type that might not route through the same
        // Activate/Deactivate lifecycle.
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedActionsComponent: HandleBoardChangeRequested -- "
                 "already awaiting a request on %s, ignoring"),
            *GetOwner()->GetName());
        return;
    }

    bAwaitingRequestConfirmation = true;
    PendingRequest = Request;

    // Push before flipping the flag -- PushAction refuses to run while
    // bAwaitingRequestConfirmation is true
    if (IsValid(AwaitingConfirmationAction))
    {
        PushAction(AwaitingConfirmationAction);
    }    

    OnBoardChangeRequested.Broadcast(Request);
    OnBoardChangeRequested_Native.Broadcast(Request);
}

void UTurnBasedActionsComponent::NotifyBoardChangeOutcome(
    const FTurnActionRequest& Request, bool bSucceeded)
{
    if (!bAwaitingRequestConfirmation || Request != PendingRequest) return;

    bAwaitingRequestConfirmation = false;

    // Only pop if AwaitingConfirmationAction was actually pushed -- if the
    // slot wasn't configured (see CreateSystemActions' warning), the
    // requesting action is still the live top of stack, nothing to pop
    if (IsValid(AwaitingConfirmationAction) && GetTopAction() == AwaitingConfirmationAction)
    {
        SafePopAction();
    }

    if (bSucceeded)
    {
        if (UTurnBasedAction* Action = Cast<UTurnBasedAction>(GetTopAction()))
        {
            Action->Complete();
        }
    }
    // Failure: reactivation above (or having never been deactivated, in
    // the degraded no-awaiting-action case) already is "recommence" --
    // the requesting action is simply live again, ready to retry.
}

void UTurnBasedActionsComponent::CheckAutoEndTurn()
{
    if (!bAutoEndTurnOnAllRequiredActionsCompleted) return;
    if (!CanAutoEndTurn()) return;

    OnTurnEndReady.Broadcast();
    RequestTurnEnd();
}

// --- Logging ---

void UTurnBasedActionsComponent::LogActionRecord(UTurnBasedAction* Action, FString Note)
{
    if (!IsValid(Action)) return;

    FTurnBasedActionRecord Record;
    Record.ActionTag    = Action->ActionTag;
    Record.TurnNumber   = CurrentTurnNumber;
    Record.DebugNote    = Note;
    Record.Timestamp    = GetWorld()
        ? GetWorld()->GetTimeSeconds() : 0.f;

    ActionHistory.Add(Record);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedActionsComponent: [Turn %d] '%s' — %s"),
        Record.TurnNumber,
        *Record.ActionTag.ToString(),
        *Note);
}

// --- Helpers ---

AController* UTurnBasedActionsComponent::GetOwningController() const
{
    return GetOwner<AController>();
}