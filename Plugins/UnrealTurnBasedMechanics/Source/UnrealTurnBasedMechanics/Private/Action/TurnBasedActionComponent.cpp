// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/TurnBasedActionComponent.h"
#include "GameFramework/Controller.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UTurnBasedActionComponent::UTurnBasedActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTurnBasedActionComponent::BeginPlay()
{
    Super::BeginPlay();
    
}

void UTurnBasedActionComponent::InitialiseFromLoadOut(UActionLoadOutDataAsset* InDataAsset)
{
    if (!IsValid(InDataAsset))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: InitialiseFromLoadout called with null loadout"));
        return;
    }

    Loadout = InDataAsset;
    CloneActionsFromLoadOut();

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Initialised with load out %s — %d actions"),
        *InDataAsset->LoadoutName,
        ActiveActions.Num());
}

void UTurnBasedActionComponent::CloneActionsFromLoadOut()
{
    ActiveActions.Empty();

    if (!IsValid(Loadout)) return;

    AController* OwningController = GetOwner<AController>();
    UInputComponent* IC = IsValid(OwningController)
        ? OwningController->InputComponent.Get()
        : nullptr;

    for (UTurnBasedAction* SourceAction : Loadout->GetPermittedActions())
    {
        if (!IsValid(SourceAction)) continue;

        // Clone from asset — runtime instance is independent
        UTurnBasedAction* Clone = DuplicateObject<UTurnBasedAction>(
            SourceAction, this);

        if (!IsValid(Clone)) continue;

        Clone->InitialiseAction(OwningController, IC);
        BindActionDelegates(Clone);
        ActiveActions.Add(Clone);

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Cloned action %s"),
            *Clone->ActionTag.ToString());
    }
}

void UTurnBasedActionComponent::BindActionDelegates(UTurnBasedAction* Action)
{
    if (!IsValid(Action)) return;

    Action->OnActionCompleted.AddDynamic(
        this, &UTurnBasedActionComponent::HandleActionCompleted);
    Action->OnActionCancelled.AddDynamic(
        this, &UTurnBasedActionComponent::HandleActionCancelled);
}

void UTurnBasedActionComponent::OnTurnBegan()
{
    CurrentTurnNumber++;
    ActiveAction = nullptr;

    // Reset per-turn state on all actions
    for (UTurnBasedAction* Action : ActiveActions)
    {
        if (IsValid(Action))
        {
            Action->ResetTurnState();
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: Turn began — %d actions available"),
        GetAvailableActions().Num());
}

void UTurnBasedActionComponent::OnAnyParticipantTurnStarted(bool bIsMyTurn)
{
    // Tick cooldowns on all actions
    for (UTurnBasedAction* Action : ActiveActions)
    {
        if (IsValid(Action))
        {
            Action->TickCooldown(bIsMyTurn);
        }
    }
}

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
            TEXT("TurnBasedActionComponent: TryActivateAction called with null action"));
        return false;
    }

    if (!Action->CanActivate())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: Action %s cannot activate — state: %s"),
            *Action->ActionTag.ToString(),
            *UEnum::GetValueAsString(Action->State));
        return false;
    }

    // Cancel any currently active action if it is cancellable
    if (IsValid(ActiveAction) && ActiveAction->bIsCancellable)
    {
        CancelActiveAction();
    }
    else if (IsValid(ActiveAction))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedActionComponent: Cannot activate %s — %s is active and not cancellable"),
            *Action->ActionTag.ToString(),
            *ActiveAction->ActionTag.ToString());
        return false;
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
}

bool UTurnBasedActionComponent::CanEndTurn() const
{
    for (const UTurnBasedAction* Action : ActiveActions)
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
            TEXT("TurnBasedActionComponent: RequestTurnEnd called but required actions not complete"));
        return;
    }

    // Delegate to the participant component to submit to server
    if (UTurnBasedParticipantComponent* ParticipantComp =
        GetOwner()->FindComponentByClass<UTurnBasedParticipantComponent>())
    {
        ParticipantComp->ServerSubmitTurnEnd();

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedActionComponent: Turn end submitted"));
    }
}

TArray<UTurnBasedAction*> UTurnBasedActionComponent::GetAvailableActions() const
{
    TArray<UTurnBasedAction*> Available;
    for (UTurnBasedAction* Action : ActiveActions)
    {
        if (IsValid(Action) && Action->CanActivate())
        {
            Available.Add(Action);
        }
    }
    return Available;
}

UTurnBasedAction* UTurnBasedActionComponent::FindActionByTag(
    FGameplayTag Tag) const
{
    TObjectPtr<UTurnBasedAction> const* Found = ActiveActions.FindByPredicate(
        [Tag](const UTurnBasedAction* Action)
        {
            return IsValid(Action) && Action->ActionTag == Tag;
        });

    return Found ? *Found : nullptr;
}

void UTurnBasedActionComponent::HandleActionCompleted(UTurnBasedAction* Action)
{
    if (ActiveAction == Action)
    {
        ActiveAction = nullptr;
    }

    LogActionRecord(Action, ETurnBasedActionState::Completed);
    OnActionCompleted.Broadcast(Action);

    // Check if this completion means the turn can now end
    CheckAutoTurnEnd();
}

void UTurnBasedActionComponent::HandleActionCancelled(UTurnBasedAction* Action)
{
    if (ActiveAction == Action)
    {
        ActiveAction = nullptr;
    }

    LogActionRecord(Action, ETurnBasedActionState::Cancelled, TEXT("Cancelled by player"));
    OnActionCancelled.Broadcast(Action);
}

void UTurnBasedActionComponent::CheckAutoTurnEnd()
{
    if (!CanEndTurn())
    {
        // depending on setup we might want to auto initialize and action or UI
        // automatically when another action ends, this can be hooked up here
        OnActionRequired.Broadcast();
        return;
    }

    OnTurnEndReady.Broadcast();

    // If the required action was the last one, auto-submit
    // Optional actions still available — let UI decide to auto-end
    // For ConnectIt: PlacePiece is required and there are no post-placement
    // optional actions, so auto-end is safe here
    RequestTurnEnd();
}

void UTurnBasedActionComponent::LogActionRecord(
    UTurnBasedAction* Action,
    ETurnBasedActionState OutcomeState,
    const FString& Note)
{
    if (!IsValid(Action)) return;

    FTurnBasedActionRecord Record;
    Record.ActionTag     = Action->ActionTag;
    Record.OutcomeState  = OutcomeState;
    Record.TurnNumber    = CurrentTurnNumber;
    Record.DebugNote     = Note;
    Record.Timestamp     = GetWorld()
        ? GetWorld()->GetTimeSeconds()
        : 0.f;

    ActionHistory.Add(Record);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedActionComponent: [Turn %d] Action %s -> %s %s"),
        Record.TurnNumber,
        *Record.ActionTag.ToString(),
        *UEnum::GetValueAsString(OutcomeState),
        Note.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("(%s)"), *Note));
}
