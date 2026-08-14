// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedAction.h"
#include "UnrealTurnBasedMechanics.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"


void UTurnBasedAction::InitialiseAction(
    AController* InOwningController,
    UEnhancedInputComponent* InInputComponent)
{
    OwningController       = InOwningController;
    EnhancedInputComponent = InInputComponent;

    PostInitialiseAction();
}

void UTurnBasedAction::Complete()
{
    if (!IsActive())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' Complete called but not active"),
            *ActionTag.ToString());
        return;
    }

    UnbindSelectionInput();
    ClearSelectionState();

    // Increment completions -- only Complete() does this
    CompletionsThisTurn++;

    if (CooldownTurns > 0)
    {
        TurnsUntilAvailable = CooldownTurns;
    }

    OnCompleted();
    OnActionCompleted.Broadcast(this);
    OnActionCompleted_Native.Broadcast(this);

    FinishAction();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedAction: '%s' completed "
             "(%d completions this turn)"),
        *ActionTag.ToString(),
        CompletionsThisTurn);
}

void UTurnBasedAction::Cancel()
{
    if (!IsActive()) return;
    if (!bIsCancellable)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' Cancel called "
                 "but bIsCancellable is false"),
            *ActionTag.ToString());
        return;
    }

    UnbindSelectionInput();
    ClearSelectionState();

    // No CompletionsThisTurn increment on cancel

    OnCancelled();
    OnActionCancelled.Broadcast(this);
    OnActionCancelled_Native.Broadcast(this);

    FinishAction();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedAction: '%s' cancelled"),
        *ActionTag.ToString());
}

void UTurnBasedAction::FinishAction()
{
    Deactivate_Internal();
    SetIsActive(false);
    OnDeactivated.Broadcast(this);
    OnDeactivated_Native.Broadcast(this);
}

bool UTurnBasedAction::CanActivate() const
{
    if (IsActive()) return false;
    if (TurnsUntilAvailable > 0) return false;
    if (MaxCompletionsPerTurn > 0
        && CompletionsThisTurn >= MaxCompletionsPerTurn) return false;
    return true;
}

void UTurnBasedAction::TickCooldown(const bool bIsMyTurn)
{
    if (!ShouldTickCooldown(bIsMyTurn)) return;
    if (TurnsUntilAvailable <= 0) return;

    TurnsUntilAvailable--;

    if (TurnsUntilAvailable <= 0)
    {
        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedAction: '%s' cooldown expired"),
            *ActionTag.ToString());
    }
}

void UTurnBasedAction::ResetTurnState()
{
    CompletionsThisTurn = 0;
}

void UTurnBasedAction::RequestBoardChange(const FTurnActionRequest& Request)
{
    if (!Request.IsValid())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' fired invalid FTurnActionRequest"),
            *ActionTag.ToString());
        return;
    }

    OnChangeRequested.Broadcast(Request);
    OnChangeRequested_Native.Broadcast(Request);
}

UGridWorldSubsystem* UTurnBasedAction::GetGridSubsystem() const
{
    UWorld* World = GetWorld();
    return IsValid(World)
        ? World->GetSubsystem<UGridWorldSubsystem>()
        : nullptr;
}

// --- Base overrides ---

void UTurnBasedAction::RequestNextAction(TSubclassOf<UTurnBasedAction> NextActionClass)
{
    if (!NextActionClass)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' RequestNextAction called "
                 "with null class"),
            *ActionTag.ToString());
        return;
    }

    OnNextActionRequested.Broadcast(NextActionClass);
    OnNextActionRequested_Native.Broadcast(NextActionClass);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedAction: '%s' requested next action '%s'"),
        *ActionTag.ToString(),
        *NextActionClass->GetName());
}

void UTurnBasedAction::Activate_Internal_Implementation()
{
    if (bRequiresSelection) BindSelectionInput();
}

void UTurnBasedAction::Deactivate_Internal_Implementation()
{
    UnbindSelectionInput();
    ClearSelectionState();
}

void UTurnBasedAction::ForceDeactivate_Internal_Implementation()
{
    // Force deactivate -- no completion increment
    // Clean up selection state same as cancel
    UnbindSelectionInput();
    ClearSelectionState();
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
    if (IsValidSelectionTile(Tile)) { HandleValidSelection(Tile); }
}

// --- Default Virtual Implementations ---

void UTurnBasedAction::OnCancelled_Implementation() {}
void UTurnBasedAction::OnCompleted_Implementation() {}
bool UTurnBasedAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const { return IsValid(Tile); }
bool UTurnBasedAction::IsValidSelectionTile_Implementation(AGridTileBase* Tile) const { return IsValid(Tile); }
void UTurnBasedAction::HandleValidHover_Implementation(AGridTileBase*) {}
void UTurnBasedAction::HandleHoverCleared_Implementation(AGridTileBase*) {}
void UTurnBasedAction::HandleValidSelection_Implementation(AGridTileBase*) {}
void UTurnBasedAction::ClearSelectionState_Implementation() {}
bool UTurnBasedAction::ShouldTickCooldown_Implementation(bool bIsMyTurn) const { return bIsMyTurn; }