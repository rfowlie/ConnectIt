// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedAction.h"
#include "UnrealTurnBasedMechanics.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Tile/GridTileBase.h"


FGameplayTag UTurnBasedAction::GetTagForClass(TSubclassOf<UTurnBasedAction> ActionClass)
{
    if (!ActionClass) return FGameplayTag();

    const UTurnBasedAction* DefaultAction = ActionClass->GetDefaultObject<UTurnBasedAction>();
    return IsValid(DefaultAction) ? DefaultAction->GetActionTag() : FGameplayTag();
}

void UTurnBasedAction::InitialiseAction(
    AController* InOwningController,
    UEnhancedInputComponent* InInputComponent,
    UEnhancedInputLocalPlayerSubsystem* InLocalPlayerSubsystem)
{
    OwningController = InOwningController;
    EnhancedInputComponent = InInputComponent;
    LocalPlayerSubsystem = InLocalPlayerSubsystem;

    ConstructInputBindings();

    // TODO: check if there are duplicate Input Actions in bindings

    InputTagBinder = NewObject<UInputTagBinder>(this);
    InputTagBinder->Initialise(EnhancedInputComponent, LocalPlayerSubsystem, InputBindings);

    PostInitialiseAction();
}

void UTurnBasedAction::Complete()
{
    if (!IsActive())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' Complete called but not active"),
            *GetActionTag().ToString());
        return;
    }

    UnbindInput();
    UnbindGridSubsystem();
    ClearSelectionState();

    OnCompleted();
    OnActionCompleted.Broadcast(this);
    OnActionCompleted_Native.Broadcast(this);

    FinishAction();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedAction: '%s' completed"),
        *GetActionTag().ToString());
}

void UTurnBasedAction::Cancel()
{
    if (!IsActive()) return;
    if (!bIsCancellable)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' Cancel called "
                 "but bIsCancellable is false"),
            *GetActionTag().ToString());
        return;
    }

    UnbindInput();
    UnbindGridSubsystem();
    ClearSelectionState();

    OnCancelled();
    OnActionCancelled.Broadcast(this);
    OnActionCancelled_Native.Broadcast(this);

    FinishAction();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedAction: '%s' cancelled"),
        *GetActionTag().ToString());
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

    // The player's authoritative runtime state decides (uses left, per-turn
    // cap, cooldown). If it isn't available yet (e.g. a client before the
    // PlayerState has replicated) don't block here -- this is only a
    // convenience; the server gate is the real check.
    if (const AController* Controller = OwningController)
    {
        if (const ATurnBasedPlayerState* PS = Controller->GetPlayerState<ATurnBasedPlayerState>())
        {
            if (PS->HasActionConfig())
            {
                return PS->CanUseAction(GetClass());
            }
        }
    }

    return true;
}

void UTurnBasedAction::RequestBoardChange(const FTurnActionRequest& Request)
{
    if (!Request.IsValid())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedAction: '%s' fired invalid FTurnActionRequest"),
            *GetActionTag().ToString());
        return;
    }

    // Stamp the sending action's identity so the server can resolve which
    // config/runtime-state entry this request belongs to.
    FTurnActionRequest StampedRequest = Request;
    StampedRequest.ActionTag = GetActionTag();

    OnChangeRequested.Broadcast(StampedRequest);
    OnChangeRequested_Native.Broadcast(StampedRequest);
}

UGridHoverSubsystem* UTurnBasedAction::GetGridSubsystem() const
{
    UWorld* World = GetWorld();
    return IsValid(World)
        ? World->GetSubsystem<UGridHoverSubsystem>()
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
            *GetActionTag().ToString());
        return;
    }

    OnNextActionRequested.Broadcast(NextActionClass);
    OnNextActionRequested_Native.Broadcast(NextActionClass);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedAction: '%s' requested next action '%s'"),
        *GetActionTag().ToString(),
        *NextActionClass->GetName());
}

void UTurnBasedAction::Activate_Internal_Implementation()
{
    if (bRequiresSelection)
    {
        BindInput();
        BindGridSubsystem();
    }
}

void UTurnBasedAction::Deactivate_Internal_Implementation()
{
    UnbindInput();
    UnbindGridSubsystem();
    ClearSelectionState();
}

void UTurnBasedAction::ForceDeactivate_Internal_Implementation()
{
    // Force deactivate -- no completion increment
    // Clean up selection state same as cancel
    UnbindInput();
    UnbindGridSubsystem();
    ClearSelectionState();
}

// --- Input Binding ---

void UTurnBasedAction::BindInput()
{
    if (IsValid(InputTagBinder))
    {
        InputTagBinder->BindAll();
    }
}

void UTurnBasedAction::UnbindInput()
{
    if (IsValid(EnhancedInputComponent))
    {
        EnhancedInputComponent->ClearBindingsForObject(this);
    }

    if (IsValid(InputTagBinder))
    {
        InputTagBinder->UnbindAll();
    }
}

void UTurnBasedAction::BindGridSubsystem()
{
    if (UGridHoverSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.AddDynamic(
            this, &UTurnBasedAction::OnGridTileHoverChanged);
    }
}

void UTurnBasedAction::UnbindGridSubsystem()
{
    if (UGridHoverSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.RemoveDynamic(
            this, &UTurnBasedAction::OnGridTileHoverChanged);
    }
}

// --- Action Helpers ---

void UTurnBasedAction::OnSelectionInputTriggered()
{
    AGridTileBase* Tile = CurrentHoveredTile.Get();
    if (!IsValid(Tile)) return;
    if (IsValidSelectionTile(Tile)) { HandleValidSelection(Tile); }
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

// --- Default Virtual Implementations ---

void UTurnBasedAction::OnCancelled_Implementation() {}
void UTurnBasedAction::OnCompleted_Implementation() {}
void UTurnBasedAction::ConstructInputBindings_Implementation() {}
bool UTurnBasedAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const { return IsValid(Tile); }
bool UTurnBasedAction::IsValidSelectionTile_Implementation(AGridTileBase* Tile) const { return IsValid(Tile); }
void UTurnBasedAction::HandleValidHover_Implementation(AGridTileBase*) {}
void UTurnBasedAction::HandleHoverCleared_Implementation(AGridTileBase*) {}
void UTurnBasedAction::HandleValidSelection_Implementation(AGridTileBase*) {}
void UTurnBasedAction::ClearSelectionState_Implementation() {}
