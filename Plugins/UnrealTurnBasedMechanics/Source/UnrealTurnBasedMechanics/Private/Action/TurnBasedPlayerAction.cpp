// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnBasedPlayerAction.h"
#include "UnrealTurnBasedMechanics.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Subsystem/GridHoverSubsystem.h"
#include "Tile/GridTileBase.h"


FGameplayTag UTurnBasedPlayerAction::GetTagForClass(TSubclassOf<UTurnBasedPlayerAction> ActionClass)
{
    if (!ActionClass) return FGameplayTag();

    const UTurnBasedPlayerAction* DefaultAction = ActionClass->GetDefaultObject<UTurnBasedPlayerAction>();
    return IsValid(DefaultAction) ? DefaultAction->GetActionTag() : FGameplayTag();
}

void UTurnBasedPlayerAction::InitialiseAction(
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

void UTurnBasedPlayerAction::Complete()
{
    if (!IsActive())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedPlayerAction: '%s' Complete called but not active"),
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
        TEXT("TurnBasedPlayerAction: '%s' completed"),
        *GetActionTag().ToString());
}

void UTurnBasedPlayerAction::Cancel()
{
    if (!IsActive()) return;
    if (!bIsCancellable)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedPlayerAction: '%s' Cancel called "
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
        TEXT("TurnBasedPlayerAction: '%s' cancelled"),
        *GetActionTag().ToString());
}

void UTurnBasedPlayerAction::FinishAction()
{
    Deactivate_Internal();
    SetIsActive(false);
    OnDeactivated.Broadcast(this);
    OnDeactivated_Native.Broadcast(this);
}

bool UTurnBasedPlayerAction::CanActivate() const
{
    if (IsActive()) return false;

    // The player's authoritative runtime state decides (match uses left,
    // per-turn cap, cooldown). If it isn't available yet (e.g. a client before the
    // PlayerState has replicated) don't block here -- this is only a
    // convenience; the server gate is the real check.
    if (const ATurnBasedPlayerState* PS = GetOwnerPlayerState())
    {
        if (PS->HasActionConfig())
        {
            return PS->CanUseAction(GetClass());
        }
    }

    return true;
}

ATurnBasedPlayerState* UTurnBasedPlayerAction::GetOwnerPlayerState() const
{
    const AController* Controller = OwningController;
    return IsValid(Controller) ? Controller->GetPlayerState<ATurnBasedPlayerState>() : nullptr;
}

bool UTurnBasedPlayerAction::GetPermanentRuntimeState(FPermanentActionRuntimeState& OutState) const
{
    const ATurnBasedPlayerState* PS = GetOwnerPlayerState();
    return IsValid(PS) && PS->GetPermanentRuntimeState(GetClass(), OutState);
}

int32 UTurnBasedPlayerAction::GetUsesThisTurn() const
{
    const ATurnBasedPlayerState* PS = GetOwnerPlayerState();
    return IsValid(PS) ? PS->GetActionUsesThisTurn(GetClass()) : 0;
}

int32 UTurnBasedPlayerAction::GetMaxUsesPerTurn() const
{
    const ATurnBasedPlayerState* PS = GetOwnerPlayerState();
    return IsValid(PS) ? PS->GetActionMaxUsesPerTurn(GetClass()) : 0;
}

int32 UTurnBasedPlayerAction::GetMatchUsesRemaining() const
{
    const ATurnBasedPlayerState* PS = GetOwnerPlayerState();
    return IsValid(PS) ? PS->GetMatchUsesRemaining(GetClass()) : 0;
}

void UTurnBasedPlayerAction::RequestBoardChange(const FInstancedStruct& Payload)
{
    if (!Payload.IsValid())
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedPlayerAction: '%s' requested a board change with no payload"),
            *GetActionTag().ToString());
        return;
    }

    // Stamp the sending action's identity so the server can resolve which
    // config/runtime-state entry this request belongs to.
    FTurnActionRequest Request;
    Request.ActionTag = GetActionTag();
    Request.Payload = Payload;

    OnChangeRequested.Broadcast(Request);
    OnChangeRequested_Native.Broadcast(Request);
}

UGridHoverSubsystem* UTurnBasedPlayerAction::GetGridSubsystem() const
{
    UWorld* World = GetWorld();
    return IsValid(World)
        ? World->GetSubsystem<UGridHoverSubsystem>()
        : nullptr;
}

// --- Base overrides ---

void UTurnBasedPlayerAction::RequestNextAction(TSubclassOf<UTurnBasedPlayerAction> NextActionClass)
{
    if (!NextActionClass)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedPlayerAction: '%s' RequestNextAction called "
                 "with null class"),
            *GetActionTag().ToString());
        return;
    }

    OnNextActionRequested.Broadcast(NextActionClass);
    OnNextActionRequested_Native.Broadcast(NextActionClass);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedPlayerAction: '%s' requested next action '%s'"),
        *GetActionTag().ToString(),
        *NextActionClass->GetName());
}

void UTurnBasedPlayerAction::Activate_Internal_Implementation()
{
    if (bRequiresSelection)
    {
        BindInput();
        BindGridSubsystem();
    }
}

void UTurnBasedPlayerAction::Deactivate_Internal_Implementation()
{
    UnbindInput();
    UnbindGridSubsystem();
    ClearSelectionState();
}

void UTurnBasedPlayerAction::ForceDeactivate_Internal_Implementation()
{
    // Force deactivate -- no completion increment
    // Clean up selection state same as cancel
    UnbindInput();
    UnbindGridSubsystem();
    ClearSelectionState();
}

// --- Input Binding ---

void UTurnBasedPlayerAction::BindInput()
{
    if (IsValid(InputTagBinder))
    {
        InputTagBinder->BindAll();
    }
}

void UTurnBasedPlayerAction::UnbindInput()
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

void UTurnBasedPlayerAction::BindGridSubsystem()
{
    if (UGridHoverSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.AddDynamic(
            this, &UTurnBasedPlayerAction::OnGridTileHoverChanged);
    }
}

void UTurnBasedPlayerAction::UnbindGridSubsystem()
{
    if (UGridHoverSubsystem* GridSub = GetGridSubsystem())
    {
        GridSub->OnGridTileHoverChanged.RemoveDynamic(
            this, &UTurnBasedPlayerAction::OnGridTileHoverChanged);
    }
}

// --- Action Helpers ---

void UTurnBasedPlayerAction::OnSelectionInputTriggered()
{
    AGridTileBase* Tile = CurrentHoveredTile.Get();
    if (!IsValid(Tile)) return;
    if (IsValidSelectionTile(Tile)) { HandleValidSelection(Tile); }
}

void UTurnBasedPlayerAction::OnGridTileHoverChanged(AGridTileBase* NewTile)
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

void UTurnBasedPlayerAction::OnCancelled_Implementation() {}
void UTurnBasedPlayerAction::OnCompleted_Implementation() {}
void UTurnBasedPlayerAction::ConstructInputBindings_Implementation() {}
bool UTurnBasedPlayerAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const { return IsValid(Tile); }
bool UTurnBasedPlayerAction::IsValidSelectionTile_Implementation(AGridTileBase* Tile) const { return IsValid(Tile); }
void UTurnBasedPlayerAction::HandleValidHover_Implementation(AGridTileBase*) {}
void UTurnBasedPlayerAction::HandleHoverCleared_Implementation(AGridTileBase*) {}
void UTurnBasedPlayerAction::HandleValidSelection_Implementation(AGridTileBase*) {}
void UTurnBasedPlayerAction::ClearSelectionState_Implementation() {}
