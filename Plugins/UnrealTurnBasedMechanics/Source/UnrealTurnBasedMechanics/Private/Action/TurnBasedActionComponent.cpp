// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedActionComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UTurnBasedActionComponent::UTurnBasedActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    // Action component does not replicate
    // All server communication goes through ServerRPC
    SetIsReplicatedByDefault(false);
}

void UTurnBasedActionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Bind to participant component's OnAnyParticipantTurnStarted
    // so cooldowns tick on every turn start regardless of who is active
    if (UTurnBasedParticipantComponent* ParticipantComp =
        GetParticipantComponent())
    {
        ParticipantComp->OnAnyParticipantTurnStarted_Native.AddLambda(
            [this](bool bIsMyTurn)
            {
                OnAnyParticipantTurnStarted(bIsMyTurn);
            });
    }
}

// --- Setup ---

void UTurnBasedActionComponent::InitialiseFromLoadout(UActionLoadOutDataAsset* InLoadOut)
{
    if (!IsValid(InLoadOut))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: InitialiseFromLoadout "
                 "called with null loadout on %s"),
            *GetOwner()->GetName());
        return;
    }

    Loadout = InLoadOut;
    CloneActionsFromLoadOut();
    bIsInitialised = true;

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: %s initialised with loadout '%s' "
             "— %d actions (%d required, %d optional)"),
        *GetOwner()->GetName(),
        *InLoadOut->LoadoutName,
        RuntimeActions.Num(),
        GetRequiredActions().Num(),
        GetAvailableActions().Num() - GetRequiredActions().Num());
}

void UTurnBasedActionComponent::CloneActionsFromLoadOut()
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
                 "found on %s — selection input will not bind"),
            *GetOwner()->GetName());
    }

    for (UTurnBasedAction* SourceAction : Loadout->GetPermittedActions())
    {
        if (!IsValid(SourceAction)) continue;

        // Clone from asset -- runtime instance is independent
        // Each controller has its own copy with independent state
        UTurnBasedAction* Clone = DuplicateObject<UTurnBasedAction>(
            SourceAction, this);

        if (!IsValid(Clone)) continue;

        Clone->InitialiseAction(Controller, EIC);
        BindActionDelegates(Clone);
        RuntimeActions.Add(Clone);

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Cloned action '%s' "
                 "(Required: %s, Cancellable: %s)"),
            *Clone->ActionTag.ToString(),
            Clone->bIsRequired ? TEXT("Yes") : TEXT("No"),
            Clone->bIsCancellable ? TEXT("Yes") : TEXT("No"));
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

void UTurnBasedActionComponent::OnTurnBegan(int32 InTurnNumber)
{
    CurrentTurnNumber = InTurnNumber;
    ActiveAction      = nullptr;

    // Reset per-turn state on all actions
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action))
        {
            Action->ResetTurnState();
        }
    }

    // Auto-activate required actions immediately
    ActivateRequiredActions();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn %d began on %s "
             "— %d actions available"),
        CurrentTurnNumber,
        *GetOwner()->GetName(),
        GetAvailableActions().Num());
}

void UTurnBasedActionComponent::OnAnyParticipantTurnStarted(bool bIsMyTurn)
{
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action))
        {
            Action->TickCooldown(bIsMyTurn);
        }
    }
}

// --- Action Control ---

bool UTurnBasedActionComponent::TryActivateAction(FGameplayTag ActionTag)
{
    UTurnBasedAction* Action = FindActionByTag(ActionTag);
    return TryActivateActionByRef(Action);
}

bool UTurnBasedActionComponent::TryActivateActionByRef(UTurnBasedAction* Action)
{
    if (!IsValid(Action))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: TryActivateAction "
                 "called with null action"));
        return false;
    }

    if (!Action->CanActivate())
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Action '%s' cannot activate "
                 "— state: %s, uses: %d/%d, cooldown: %d"),
            *Action->ActionTag.ToString(),
            *UEnum::GetValueAsString(Action->State),
            Action->UsesThisTurn,
            Action->MaxUsesPerTurn,
            Action->TurnsUntilAvailable);
        return false;
    }

    // Cancel current active action if cancellable
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

    for (const UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (Action->bIsRequired && !Action->IsComplete())
        {
            return false;
        }
    }

    return true;
}

void UTurnBasedActionComponent::RequestTurnEnd()
{
    if (!CanEndTurn())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: RequestTurnEnd called "
                 "but required actions not complete on %s"),
            *GetOwner()->GetName());
        return;
    }

    if (UTurnBasedParticipantComponent* ParticipantComp =
        GetParticipantComponent())
    {
        ParticipantComp->ServerSubmitTurnEnd();

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Turn end submitted on %s"),
            *GetOwner()->GetName());
    }
}

// --- Queries ---

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetAllActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action)) Out.Add(Action);
    }
    return Out;
}

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetAvailableActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action) && Action->CanActivate()) Out.Add(Action);
    }
    return Out;
}

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetRequiredActions() const
{
    TArray<UTurnBasedAction*> Out;
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (IsValid(Action) && Action->bIsRequired) Out.Add(Action);
    }
    return Out;
}

UTurnBasedAction* UTurnBasedActionComponent::FindActionByTag(FGameplayTag Tag) const
{
    UTurnBasedAction* const* Found = RuntimeActions.FindByPredicate(
        [Tag](const UTurnBasedAction* Action)
        {
            return IsValid(Action) && Action->ActionTag == Tag;
        });

    return Found ? *Found : nullptr;
}

// --- Turn Flow ---

void UTurnBasedActionComponent::ActivateRequiredActions()
{
    for (UTurnBasedAction* Action : RuntimeActions)
    {
        if (!IsValid(Action)) continue;
        if (!Action->bIsRequired) continue;
        if (!Action->CanActivate()) continue;

        TryActivateActionByRef(Action);

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Auto-activated required "
                 "action '%s'"),
            *Action->ActionTag.ToString());
    }
}

void UTurnBasedActionComponent::CheckAutoTurnEnd()
{
    if (!CanEndTurn()) return;

    OnTurnEndReady.Broadcast();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: All required actions complete "
             "on %s — auto-ending turn"),
        *GetOwner()->GetName());

    RequestTurnEnd();
}

// --- Action Delegate Handlers ---

void UTurnBasedActionComponent::HandleActionChangeRequested(
    const FTurnActionRequest& Request)
{
    // Route to server via RPC
    // Component has zero knowledge of what processes this on the server
    // Project-specific wiring connects the server side to the board manager
    ServerRouteBoardChangeRequest(Request);
}

void UTurnBasedActionComponent::HandleActionCompleted(UTurnBasedAction* Action)
{
    if (ActiveAction == Action)
    {
        ActiveAction = nullptr;
    }

    LogActionRecord(Action, ETurnBasedActionState::Completed);
    OnActionCompleted.Broadcast(Action);

    if (!Action->bIsRequired)
    {
        // Optional action completed
        // Reactivate required actions so player can proceed
        ActivateRequiredActions();
    }
    else
    {
        // Required action completed
        // Check if all required actions are done
        CheckAutoTurnEnd();
    }
}

void UTurnBasedActionComponent::HandleActionCancelled(UTurnBasedAction* Action)
{
    if (ActiveAction == Action)
    {
        ActiveAction = nullptr;
    }

    LogActionRecord(Action, ETurnBasedActionState::Cancelled,
        TEXT("Cancelled by player or system"));

    OnActionCancelled.Broadcast(Action);
}

// --- ServerRPC ---

void UTurnBasedActionComponent::ServerRouteBoardChangeRequest_Implementation(
    FTurnActionRequest Request)
{
    // Server side -- validate request is from active participant
    if (UTurnBasedParticipantComponent* ParticipantComp = GetParticipantComponent())
    {
        if (!ParticipantComp->IsMyTurn())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("TurnBasedActionComponent: Board change request "
                     "rejected — not this participant's turn on %s"),
                *GetOwner()->GetName());
            return;
        }
    }

    // Stamp faction ID from cached slot index if not set by action
    if (Request.FactionID < 0)
    {
        if (UTurnBasedParticipantComponent* ParticipantComp = GetParticipantComponent())
        {
            Request.FactionID = ParticipantComp->CachedSlotIndex;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Routing board change request "
             "'%s' from faction %d"),
        *Request.RequestType.ToString(),
        Request.FactionID);

    // Fire passthrough delegate -- project wires this to board manager
    OnBoardChangeRequested.Broadcast(Request);
    OnBoardChangeRequested_Native.Broadcast(Request);
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
        ? GetWorld()->GetTimeSeconds()
        : 0.f;

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

UTurnBasedParticipantComponent*
UTurnBasedActionComponent::GetParticipantComponent() const
{
    AController* Controller = GetOwningController();
    if (!IsValid(Controller)) return nullptr;
    return Controller->FindComponentByClass<UTurnBasedParticipantComponent>();
}