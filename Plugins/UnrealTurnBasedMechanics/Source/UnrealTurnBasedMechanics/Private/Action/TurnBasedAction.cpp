// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/TurnBasedAction.h"
#include "EnhancedInputComponent.h"
#include "TurnBasedMechanicsEnums.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"


void UTurnBasedAction::InitialiseAction(
    AController* InOwningController,
    UInputComponent* InInputComponent)
{
    OwningController = InOwningController;
    InputComponent   = InInputComponent;
}

void UTurnBasedAction::Activate()
{
    if (!CanActivate()) return;

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
    if (!bIsCancellable || State != ETurnBasedActionState::Active) return;

    UnbindSelectionInput();
    ClearVisuals();

    // Refund use — cancellation should not consume the action
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
    ClearVisuals();

    State = ETurnBasedActionState::Completed;

    if (CooldownTurns > 0)
    {
        TurnsUntilAvailable = CooldownTurns;
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
    if (State == ETurnBasedActionState::Active)   return false;
    if (State == ETurnBasedActionState::OnCooldown) return false;
    if (TurnsUntilAvailable > 0)                  return false;
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
    if (State == ETurnBasedActionState::Completed && CooldownTurns == 0)
    {
        State = ETurnBasedActionState::Available;
    }
}

UWorld* UTurnBasedAction::GetWorld() const
{
    if (UActorComponent* Comp = GetTypedOuter<UActorComponent>())
    {
        return Comp->GetWorld();
    }
    if (AActor* Actor = GetTypedOuter<AActor>())
    {
        return Actor->GetWorld();
    }
    return nullptr;
}

UGridWorldSubsystem* UTurnBasedAction::GetGridSubsystem() const
{
    UWorld* World = GetWorld();
    return IsValid(World) ? World->GetSubsystem<UGridWorldSubsystem>() : nullptr;
}

void UTurnBasedAction::BindSelectionInput()
{
    // Bind to subsystem hover updates
    if (UGridWorldSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.AddDynamic(
            this, &UTurnBasedAction::OnGridTileHoverChanged);
    }

    // Bind selection input via Enhanced Input
    if (UEnhancedInputComponent* EIC =
        Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (IsValid(SelectionInputAction))
        {
            EIC->BindAction(SelectionInputAction,
                ETriggerEvent::Triggered,
                this,
                &UTurnBasedAction::OnSelectionInputTriggered);
        }
    }
}

void UTurnBasedAction::UnbindSelectionInput()
{
    if (UGridWorldSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.RemoveDynamic(
            this, &UTurnBasedAction::OnGridTileHoverChanged);
    }

    if (UEnhancedInputComponent* EIC =
        Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (IsValid(SelectionInputAction))
        {
            EIC->ClearBindingsForObject(this);
        }
    }
}

void UTurnBasedAction::OnGridTileHoverChanged(AGridTileBase* NewTile)
{
    AGridTileBase* Previous = CurrentHoveredTile.Get();

    // Clear previous hover visuals
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

// --- Default virtual implementations ---

void UTurnBasedAction::OnActivated_Implementation() {}
void UTurnBasedAction::OnCancelled_Implementation() {}
void UTurnBasedAction::OnCompleted_Implementation() {}

bool UTurnBasedAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const
{
    return IsValid(Tile);
}

bool UTurnBasedAction::IsValidSelectionTile_Implementation(AGridTileBase* Tile) const
{
    return IsValid(Tile);
}

void UTurnBasedAction::HandleValidHover_Implementation(AGridTileBase* Tile) {}
void UTurnBasedAction::HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) {}
void UTurnBasedAction::HandleValidSelection_Implementation(AGridTileBase* Tile) {}
void UTurnBasedAction::ClearVisuals_Implementation() {}

bool UTurnBasedAction::ShouldTickCooldown_Implementation(
    bool bIsOwningParticipantTurn) const
{
    // Default — only tick on owning participant's turn
    return bIsOwningParticipantTurn;
}
