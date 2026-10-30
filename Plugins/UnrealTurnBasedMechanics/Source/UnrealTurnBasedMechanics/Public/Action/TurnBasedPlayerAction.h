// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "TurnBasedMechanicsStructs.h"
#include "Action/TurnBasedActionBase.h"
#include "Input/InputTagBinder.h"
#include "Action/ActionConfig.h"
#include "TurnBasedPlayerAction.generated.h"

class UTurnBasedPlayerAction;
class ATurnBasedPlayerState;
class AGridTileBase;
class UGridHoverSubsystem;
class UActorPoolSubsystem;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;


// Fired when this action wants to chain into another action,
// Action component listens to this and calls TryPushAction(Class)
// Example: a menu action fires this when the player selects an option
// Example: an action-selection menu fires this to push a targeting action
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNextActionRequested, TSubclassOf<UTurnBasedPlayerAction>, NextActionClass);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNextActionRequested_Native, TSubclassOf<UTurnBasedPlayerAction>);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnActionEvent, UTurnBasedPlayerAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnActionRequested, const FTurnActionRequest&, Request);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnBasedActionEvent_Native, UTurnBasedPlayerAction*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnActionRequested_Native, const FTurnActionRequest&);


// A Player Action: something a participant chooses to do on their turn (place
// a piece, swap two...). It builds a request for the server and plays its
// part of the turn on the client. How many times it may be used is not the
// action's to say: the loadout or the game sets that. Spectator actions
// (UTurnBasedSpectatorAction) share the base but aren't player actions.
// (Was UTurnBasedAction; a class redirect keeps old assets loading.)
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UNREALTURNBASEDMECHANICS_API UTurnBasedPlayerAction : public UTurnBasedActionBase
{
    GENERATED_BODY()

public:

    // --- Designer Config ---
    // Budgets, per-turn caps, cooldowns and turn-end contribution are NOT
    // configured here -- they live in the permanent action list the player was
    // seeded from (FPermanentActionConfig: the loadout's PermanentActions, or
    // a list the game builds) and the loadout's TurnEndRequirements tree, and
    // their live state on ATurnBasedPlayerState.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bIsCancellable = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Config")
    bool bRequiresSelection = false;

    // --- Presentation ---
    // Purely for UI -- the action itself never reads these. Without them an
    // action bar has nothing to label a button with (ActionTag is an
    // identifier, not a display string).

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Presentation")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Presentation",
        meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Presentation")
    FSlateBrush Icon;

    // The tag an action class identifies itself with, read from its default
    // object -- lets config reference an action by class alone instead of a
    // separately typed tag. Invalid tag if the class is null or its
    // GetActionTag isn't implemented.
    static FGameplayTag GetTagForClass(TSubclassOf<UTurnBasedPlayerAction> ActionClass);

    // Whether this action is allowed to produce a request of this type.
    // The server checks it against a request's client-supplied ActionTag so
    // one action can't spend its budget while sending another action's
    // request. Default true (unrestricted); project actions override it.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action")
    bool ProducesRequestType(FGameplayTag RequestType) const;
    virtual bool ProducesRequestType_Implementation(FGameplayTag RequestType) const { return true; }

    // --- Lifecycle ---

    void InitialiseAction(
        AController* InOwningController,
        UEnhancedInputComponent* InInputComponent,
        UEnhancedInputLocalPlayerSubsystem* InLocalPlayerSubsystem);

    // Natural completion. Uses are counted by the server on the player's
    // PlayerState when the request commits, not here.
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Complete();

    // Player cancel
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Cancel();

    // System interrupt -- inherited from base
    // ForceDeactivate() also unbinds selection input

    // Client-side convenience for UI/selection: asks the owner's PlayerState
    // whether this action's uses, per-turn cap and cooldown allow it -- or,
    // for an item use, whether an item is held and the item budget has a use
    // left. The server re-checks every request regardless (see the Mediator
    // gate).
    UFUNCTION(BlueprintPure, Category = "Action")
    bool CanActivate() const;

    // Whether this activation is an item use: set by the actions component's
    // ActivateItemAction, until the action completes or is cancelled (a
    // refused request retried stays one). Its request is marked bIsItemUse,
    // so it spends an item and the item budget, never its permanent uses.
    UFUNCTION(BlueprintPure, Category = "Action")
    bool IsItemUse() const { return bIsItemUse; }

    // The actions component's (see IsItemUse)
    void SetItemUse(bool bInIsItemUse) { bIsItemUse = bInIsItemUse; }

    // --- Runtime state (read-only) ---
    // The action instance holds NO budget/cap/cooldown state of its own -- it
    // lives on the owner's PlayerState and is read from there on every call, so
    // it can never disagree with the authoritative copy. Each returns 0/false
    // if the owner has no PlayerState or no entry for this action's class.

    // Copy of this action's state (false if the player doesn't have it)
    UFUNCTION(BlueprintPure, Category = "Action|State")
    bool GetPermanentRuntimeState(FPermanentActionRuntimeState& OutState) const;

    UFUNCTION(BlueprintPure, Category = "Action|State")
    int32 GetUsesThisTurn() const;

    // Current effective per-turn cap (at least 1; 0 if unknown)
    UFUNCTION(BlueprintPure, Category = "Action|State")
    int32 GetMaxUsesPerTurn() const;

    // Uses left in the match: -1 = unlimited, 0 = none left (or unknown)
    UFUNCTION(BlueprintPure, Category = "Action|State")
    int32 GetMatchUsesRemaining() const;

    // --- Board Change ---

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionRequested OnChangeRequested;

    FOnTurnActionRequested_Native OnChangeRequested_Native;

    // --- Action Events ---
    // Separate from base OnActivated/OnDeactivated
    // Carry UTurnBasedPlayerAction* for typed access

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionEvent OnActionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnTurnActionEvent OnActionCancelled;

    FOnTurnBasedActionEvent_Native OnActionCompleted_Native;
    FOnTurnBasedActionEvent_Native OnActionCancelled_Native;
    
    UPROPERTY(BlueprintAssignable, Category = "Action|Delegates")
    FOnNextActionRequested OnNextActionRequested;

    FOnNextActionRequested_Native OnNextActionRequested_Native;

    // Helper -- subclasses call this to request chaining
    // Component receives and pushes the named action onto the stack
    UFUNCTION(BlueprintCallable, Category = "Action")
    void RequestNextAction(TSubclassOf<UTurnBasedPlayerAction> NextActionClass);

    // --- Cursors ---
    // Each player action is its own cursor manager: it takes cursor actors
    // from the actor pool, shows / moves / hides them, and sets tiles' cursor
    // states, however its situation needs. Tiles never choose a cursor.
    // When the action ends -- complete, cancel or force-deactivate -- every
    // cursor it holds goes back to the pool and every tile it touched goes
    // back to neutral, so a forgotten cursor or tile state can't leak.
    // (An override of ForceDeactivate_Internal must call Super for that.)
    // Every cursor class must implement IGridCursor (Cursor/GridCursorInterface.h):
    // one that doesn't is refused with an error. Give cursor class properties
    // meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor").

    // Where a cursor sits relative to its tile
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cursor")
    FVector CursorOffset = FVector(0.f, 0.f, 5.f);

    // A cursor of CursorClass from the actor pool (activated in the pool, then
    // hidden until shown), held by this action until released. Null -- and an
    // error -- if CursorClass doesn't implement IGridCursor; null with no class
    // or no pool.
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    AActor* AcquireActionCursor(TSubclassOf<AActor> CursorClass);

    // Hides Cursor and returns it to the pool (only one this action holds)
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void ReleaseActionCursor(AActor* Cursor);

    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void ReleaseActionCursors();

    // Whether this action still holds Cursor (it isn't once released)
    UFUNCTION(BlueprintPure, Category = "Action|Cursor")
    bool HoldsActionCursor(const AActor* Cursor) const;

    // Moves a held Cursor onto Tile (MoveCursor, at the tile plus
    // CursorOffset) and shows it. bReactivate: replay its arrival effects.
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void ShowCursorAt(AActor* Cursor, AGridTileBase* Tile, bool bReactivate = false);

    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void HideCursor(AActor* Cursor);

    // The follow cursor: shows a cursor of CursorClass on Tile, swapping the
    // current one if its class differs (so the class can be chosen per tile);
    // reactivated whenever it lands on a different tile. No class = hidden.
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void ShowHoverCursor(TSubclassOf<AActor> CursorClass, AGridTileBase* Tile);

    // Hides the follow cursor (still held, for the next hover)
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void HideHoverCursor();

    UFUNCTION(BlueprintPure, Category = "Action|Cursor")
    AActor* GetHoverCursor() const { return HoverCursor; }

    // Sets Tile's cursor state (no tag = neutral) and remembers the tile, so
    // it goes back to neutral when the action ends
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void SetTileCursorState(AGridTileBase* Tile, FGameplayTag State);

    // Every tile this action touched back to neutral
    UFUNCTION(BlueprintCallable, Category = "Action|Cursor")
    void ClearTileCursorStates();
    
protected:

    // The owner's PlayerState (null if there is no owner/PlayerState yet)
    ATurnBasedPlayerState* GetOwnerPlayerState() const;

    // --- Virtual Lifecycle Hooks ---

    virtual void Activate_Internal_Implementation() override;
    virtual void Deactivate_Internal_Implementation() override;
    virtual void ForceDeactivate_Internal_Implementation() override;

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnCancelled();

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void OnCompleted();

    // --- Input ---

    // Populated by ConstructInputBindings() -- each entry's InputActionDelegate
    // is bound directly to whatever this action wants that input to do, no
    // shared tag-switch consumer needed. Bound/unbound alongside selection
    // input by BindInput/UnbindInput, dispatched through InputTagBinder.
    UPROPERTY(BlueprintReadWrite, Category = "Action|Config")
    TArray<FInputTagBinding> InputBindings;

    // Set the values in InputBindings
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Config")
    void ConstructInputBindings();
    
    // --- Selection Hooks ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    bool IsValidHoverTile(AGridTileBase* Tile) const;
    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    bool IsValidSelectionTile(AGridTileBase* Tile) const;
    virtual bool IsValidSelectionTile_Implementation(AGridTileBase* Tile) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleValidHover(AGridTileBase* Tile);
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleHoverCleared(AGridTileBase* PreviousTile);
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile);

    // The cursor moved onto a tile this action can't use (IsValidHoverTile
    // false), after the previous tile's HandleHoverCleared. Default: nothing.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleInvalidHover(AGridTileBase* Tile);
    virtual void HandleInvalidHover_Implementation(AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleValidSelection(AGridTileBase* Tile);
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile);

    // A board tile was clicked (UGridHoverSubsystem::OnGridTileClicked, while
    // the action is active). Default: nothing -- the selection input
    // (HandleValidSelection) still selects; override for click-only handling.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void HandleTileClicked(AGridTileBase* Tile);
    virtual void HandleTileClicked_Implementation(AGridTileBase* Tile);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Selection")
    void ClearSelectionState();
    virtual void ClearSelectionState_Implementation();

    // Asks the server for a board change. Payload is the project's own struct
    // describing the change; it is wrapped in a FTurnActionRequest stamped
    // with this action's tag and sent on via OnChangeRequested. Subclasses
    // call it from HandleValidSelection.
    void RequestBoardChange(const FInstancedStruct& Payload);

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AGridTileBase> CurrentHoveredTile = nullptr;

    UGridHoverSubsystem* GetGridSubsystem() const;
    
    // Bind/unbind grid-tile hover + optional Enhanced Input selection.
    // Called automatically from Activate_Internal_Implementation when
    // bRequiresSelection is true. Exposed to subclasses so an action that
    // defers selection to a later internal state (bRequiresSelection left
    // false) can call these directly once it's ready -- see
    // ConnectIt_TutorialShiftIntroAction for an example.
    void BindInput();
    void UnbindInput();

    void BindGridSubsystem();
    void UnbindGridSubsystem();

    // --- Action Helpers ---
    // TODO: create helper functions for default action items (grid selection, piece selection, etc.)
    // default function for activating selection
    UFUNCTION(BlueprintCallable)
    void OnSelectionInputTriggered();

private:

    // See IsItemUse
    bool bIsItemUse = false;


    UPROPERTY()
    TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UEnhancedInputLocalPlayerSubsystem> LocalPlayerSubsystem = nullptr;

    // Owns InputBindings' mapping context + per-binding dispatch -- see
    // UnrealGameMechanics/Input/InputTagBinder.h.
    UPROPERTY()
    TObjectPtr<UInputTagBinder> InputTagBinder = nullptr;

    // Shared tail of Complete()/Cancel() -- runs Deactivate_Internal,
    // releases cursors and tile states, clears the active flag, and
    // broadcasts OnDeactivated
    void FinishAction();

    // --- Cursors (see the public section) ---

    UPROPERTY()
    TArray<TObjectPtr<AActor>> ActionCursors;

    // The held cursors currently shown (activated in the pool)
    UPROPERTY()
    TSet<TObjectPtr<AActor>> ShownCursors;

    UPROPERTY()
    TObjectPtr<AActor> HoverCursor = nullptr;

    // The tile the follow cursor was last shown on
    TWeakObjectPtr<AGridTileBase> HoverCursorTile;

    TArray<TWeakObjectPtr<AGridTileBase>> TouchedTiles;

    UActorPoolSubsystem* GetActorPool() const;

    UFUNCTION()
    void OnGridTileHoverChanged(AGridTileBase* NewTile);

    UFUNCTION()
    void OnGridTileHoverEnded(AGridTileBase* Tile);

    UFUNCTION()
    void OnGridTileClicked(AGridTileBase* Tile);
};