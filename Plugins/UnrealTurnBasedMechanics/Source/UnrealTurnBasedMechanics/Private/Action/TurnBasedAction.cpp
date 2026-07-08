// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedAction.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "TurnBasedMechanicsStructs.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"


void UTurnBasedAction::InitialiseAction(
    AController* InOwningController,
    UEnhancedInputComponent* InInputComponent)
{
    OwningController       = InOwningController;
    EnhancedInputComponent = InInputComponent;
}

void UTurnBasedAction::Activate()
{
    if (!CanActivate())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedAction: %s cannot activate — state: %s"),
            *ActionTag.ToString(),
            *UEnum::GetValueAsString(State));
        return;
    }

    State = ETurnBasedActionState::Active;
    UsesThisTurn++;

    if (bRequiresSelection)
    {
        BindSelectionInput();
    }

    OnActivated();
    OnActionActivated.Broadcast(this);
    OnActionActivated_Native.Broadcast(this);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedAction: %s activated"),
        *ActionTag.ToString());
}

void UTurnBasedAction::Cancel()
{
    if (!bIsCancellable)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedAction: %s is not cancellable"),
            *ActionTag.ToString());
        return;
    }

    if (State != ETurnBasedActionState::Active)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedAction: %s cancel called but not active"),
            *ActionTag.ToString());
        return;
    }

    UnbindSelectionInput();
    ClearSelectionState();

    // Refund the use -- cancellation should not consume the action
    UsesThisTurn = FMath::Max(0, UsesThisTurn - 1);
    State = ETurnBasedActionState::Cancelled;

    OnCancelled();
    OnActionCancelled.Broadcast(this);
    OnActionCancelled_Native.Broadcast(this);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedAction: %s cancelled"),
        *ActionTag.ToString());
}

void UTurnBasedAction::Complete()
{
    UnbindSelectionInput();
    ClearSelectionState();

    State = ETurnBasedActionState::Completed;

    if (CooldownTurns > 0)
    {
        TurnsUntilAvailable = CooldownTurns;
        State = ETurnBasedActionState::OnCooldown;
    }

    OnCompleted();
    OnActionCompleted.Broadcast(this);
    OnActionCompleted_Native.Broadcast(this);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedAction: %s completed"),
        *ActionTag.ToString());
}

bool UTurnBasedAction::CanActivate() const
{
    if (State == ETurnBasedActionState::Active)    return false;
    if (State == ETurnBasedActionState::OnCooldown) return false;
    if (TurnsUntilAvailable > 0)                   return false;
    if (MaxUsesPerTurn > 0 && UsesThisTurn >= MaxUsesPerTurn) return false;
    return true;
}

void UTurnBasedAction::TickCooldown(bool bIsOwningParticipantTurn)
{
    if (!ShouldTickCooldown(bIsOwningParticipantTurn)) return;
    if (TurnsUntilAvailable <= 0) return;

    TurnsUntilAvailable--;

    if (TurnsUntilAvailable <= 0)
    {
        if (State == ETurnBasedActionState::OnCooldown)
        {
            State = ETurnBasedActionState::Available;
        }

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedAction: %s cooldown expired — now available"),
            *ActionTag.ToString());
    }
}

void UTurnBasedAction::ResetTurnState()
{
    UsesThisTurn = 0;

    // Restore completed non-cooldown actions to available
    if (State == ETurnBasedActionState::Completed
        || State == ETurnBasedActionState::Cancelled)
    {
        if (CooldownTurns == 0)
        {
            State = ETurnBasedActionState::Available;
        }
    }
}

void UTurnBasedAction::RequestBoardChange(const FTurnActionRequest& Request)
{
    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedAction: %s fired invalid FTurnActionRequest"),
            *ActionTag.ToString());
        return;
    }

    OnChangeRequested.Broadcast(Request);
    OnChangeRequested_Native.Broadcast(Request);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedAction: %s requested board change — type: %s"),
        *ActionTag.ToString(),
        *Request.RequestType.ToString());
}

UWorld* UTurnBasedAction::GetWorld() const
{
    if (UActorComponent* Comp = GetTypedOuter<UActorComponent>())
        return Comp->GetWorld();
    if (AActor* Actor = GetTypedOuter<AActor>())
        return Actor->GetWorld();
    return nullptr;
}

UGridWorldSubsystem* UTurnBasedAction::GetGridSubsystem() const
{
    UWorld* World = GetWorld();
    return IsValid(World)
        ? World->GetSubsystem<UGridWorldSubsystem>()
        : nullptr;
}

// --- Input Binding ---

void UTurnBasedAction::BindSelectionInput()
{
    if (UGridWorldSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.AddDynamic(
            this, &UTurnBasedAction::OnGridTileHoverChanged);
    }

    if (IsValid(EnhancedInputComponent) && IsValid(SelectionInputAction))
    {
        EnhancedInputComponent->BindAction(
            SelectionInputAction,
            ETriggerEvent::Triggered,
            this,
            &UTurnBasedAction::OnSelectionInputTriggered);
    }
}

void UTurnBasedAction::UnbindSelectionInput()
{
    if (UGridWorldSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.RemoveDynamic(
            this, &UTurnBasedAction::OnGridTileHoverChanged);
    }

    if (IsValid(EnhancedInputComponent))
    {
        EnhancedInputComponent->ClearBindingsForObject(this);
    }

    CurrentHoveredTile = nullptr;
}

void UTurnBasedAction::OnGridTileHoverChanged(AGridTileBase* NewTile)
{
    AGridTileBase* Previous = CurrentHoveredTile.Get();

    // Clear previous hover state
    if (IsValid(Previous) && Previous != NewTile)
    {
        HandleHoverCleared(Previous);
    }

    CurrentHoveredTile = NewTile;

    if (IsValid(NewTile) && IsValidHoverTile(NewTile))
    {
        HandleValidHover(NewTile);
    }
    else if (IsValid(Previous))
    {
        HandleHoverCleared(Previous);
    }
}

void UTurnBasedAction::OnSelectionInputTriggered()
{
    AGridTileBase* Tile = CurrentHoveredTile.Get();
    if (!IsValid(Tile)) return;

    if (IsValidSelectionTile(Tile))
    {
        HandleValidSelection(Tile);
    }
}

// --- Default Virtual Implementations ---

void UTurnBasedAction::OnActivated_Implementation() {}
void UTurnBasedAction::OnCancelled_Implementation() {}
void UTurnBasedAction::OnCompleted_Implementation() {}

bool UTurnBasedAction::IsValidHoverTile_Implementation(
    AGridTileBase* Tile) const
{
    return IsValid(Tile);
}

bool UTurnBasedAction::IsValidSelectionTile_Implementation(
    AGridTileBase* Tile) const
{
    return IsValid(Tile);
}

void UTurnBasedAction::HandleValidHover_Implementation(
    AGridTileBase* Tile) {}

void UTurnBasedAction::HandleHoverCleared_Implementation(
    AGridTileBase* PreviousTile) {}

void UTurnBasedAction::HandleValidSelection_Implementation(
    AGridTileBase* Tile) {}

void UTurnBasedAction::ClearSelectionState_Implementation() {}

bool UTurnBasedAction::ShouldTickCooldown_Implementation(
    bool bIsOwningParticipantTurn) const
{
    // Default -- only tick on owning participant's turn
    return bIsOwningParticipantTurn;
}