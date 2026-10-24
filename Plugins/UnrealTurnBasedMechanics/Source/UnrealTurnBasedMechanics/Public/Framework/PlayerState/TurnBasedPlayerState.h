// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TurnBasedMechanicsEnums.h"
#include "GameplayTagContainer.h"
#include "Action/ActionConfig.h"
#include "TurnBasedPlayerState.generated.h"

class UActionLoadoutDataAsset;
class UTurnBasedPlayerAction;

// Fires whenever any per-action runtime state changes -- on the server as it
// happens, on clients from the replication notify. The actions component
// waits on this to leave its post-completion limbo and check turn end.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionRuntimeStateUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadoutChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnsMissedChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnForfeited);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyChanged, bool, bIsReady);

UCLASS()
class UNREALTURNBASEDMECHANICS_API ATurnBasedPlayerState : public APlayerState
{
    GENERATED_BODY()

    // Manager writes these -- nobody else should
    friend class UTurnBasedParticipantManagerComponent;

public:

    ATurnBasedPlayerState();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    int32 GetTurnsMissed() const { return TurnsMissed; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsForfeited() const { return bForfeited; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    bool IsReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "Turn Based")
    EParticipantType GetParticipantType() const { return ParticipantType; }

    // --- Action State (authoritative) ---
    // Per-action uses and cooldowns for this player's permanent actions,
    // seeded from a permanent action list (the loadout's, or one the game
    // builds). Mutated only on the server; clients read the replicated copy.
    // The one source of truth for uses -- see the 2026-09-18 decisions.

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UActionLoadoutDataAsset* GetLoadout() const { return Loadout; }

    // True once this player's action state has been seeded with at least one
    // permanent action (on a client: once it has replicated)
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool HasActionConfig() const;

    // The config class whose GetActionTag matches (null if none)
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TSubclassOf<UTurnBasedPlayerAction> FindActionClassByTag(FGameplayTag ActionTag) const;

    // Bumped on every state mutation. Lets a waiting component tell "the
    // update I'm waiting for has landed" from "nothing has changed yet",
    // regardless of whether it arrived before or after its outcome RPC.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionStateRevision() const { return ActionStateRevision; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<FPermanentActionRuntimeEntry> GetPermanentActionState() const { return PermanentActionState; }

    // How many times this action has been used this turn (0 if unknown)
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionUsesThisTurn(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const;

    // The action's current effective per-turn cap (at least 1; 0 for an
    // action this player doesn't have) -- read from runtime state, so it
    // reflects temporary changes.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionMaxUsesPerTurn(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionUsesThisTurnByTag(FGameplayTag ActionTag) const;

    // A copy of one action's whole runtime state. Returns false (and leaves
    // Out untouched) if this player doesn't have the action.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool GetPermanentRuntimeState(TSubclassOf<UTurnBasedPlayerAction> ActionClass,
        FPermanentActionRuntimeState& OutState) const;

    // Uses left in the match: -1 = unlimited; 0 when none are left, or for an
    // action this player doesn't have
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetMatchUsesRemaining(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const;

    // The player has the action, its cooldown is clear, this turn's cap isn't
    // reached and the match has uses left
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanUseAction(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const;

    // Whether InLoadout's turn-end requirement tree is satisfied by this
    // player's uses this turn. No loadout or no tree = nothing is required,
    // so true. The one place turn-end requirements are evaluated -- the human
    // actions component (UTurnBasedActionsComponent::CanAutoEndTurn) and AI
    // controllers both call it.
    bool AreTurnEndRequirementsMet(const UActionLoadoutDataAsset* InLoadout) const;

    // AreTurnEndRequirementsMet against this player's own (seeded) loadout
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanEndTurn() const { return AreTurnEndRequirementsMet(Loadout); }

    // Server only. Sets the loadout (its system actions and turn-end tree)
    // and seeds the action state from its PermanentActions.
    void InitialiseActionState(UActionLoadoutDataAsset* InLoadout);

    // Server only. Sets the loadout and seeds the action state from
    // PermanentActions instead of the loadout's own list -- for a game whose
    // uses come from elsewhere (e.g. a save). An action appears once; a later
    // entry for the same class is ignored.
    void InitialiseActionState(UActionLoadoutDataAsset* InLoadout,
        const TArray<FPermanentActionConfig>& PermanentActions);

    // Server only. Spends one use: this turn's count up, the match's down
    // (unless unlimited), cooldown started. False (and no change) if
    // CanUseAction fails.
    bool ConsumeActionUse(TSubclassOf<UTurnBasedPlayerAction> ActionClass);

    // Server only. Turn boundary hooks.
    void ResetActionTurnCounters();
    void TickActionCooldowns();

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionRuntimeStateUpdated OnActionRuntimeStateUpdated;

    // This player's loadout was set or replaced -- fires on the server when
    // InitialiseActionState runs and on clients when the loadout replicates.
    // The owning client's controller builds its action stack from GetLoadout()
    // in response, so a player's actions always come from here, never from
    // static level data.
    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnLoadoutChanged OnLoadoutChanged;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnTurnsMissedChanged OnTurnsMissedChanged;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnForfeited OnForfeited;

    UPROPERTY(BlueprintAssignable, Category = "Turn Based")
    FOnReadyChanged OnReadyChanged;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    // --- Manager-only setters (friend access) ---

    void SetSlotIndex(int32 InSlotIndex);
    void SetParticipantType(EParticipantType InType);
    void IncrementTurnsMissed();
    void SetForfeited(bool bInForfeited);
    void SetReady(bool bInReady);
    void ResetTurnState();

private:

    // Stable for lifetime of match -- doubles as FactionID
    UPROPERTY(Replicated)
    int32 SlotIndex = -1;

    UPROPERTY(Replicated)
    EParticipantType ParticipantType = EParticipantType::Human;

    UPROPERTY(ReplicatedUsing = OnRep_TurnsMissed)
    int32 TurnsMissed = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Forfeited)
    bool bForfeited = false;

    UPROPERTY(ReplicatedUsing = OnRep_Ready)
    bool bIsReady = false;

    // Static data asset -- replicates as an asset reference
    UPROPERTY(ReplicatedUsing = OnRep_Loadout)
    TObjectPtr<UActionLoadoutDataAsset> Loadout = nullptr;

    UFUNCTION()
    void OnRep_Loadout();

    UPROPERTY(ReplicatedUsing = OnRep_ActionState)
    TArray<FPermanentActionRuntimeEntry> PermanentActionState;

    UPROPERTY(ReplicatedUsing = OnRep_ActionState)
    int32 ActionStateRevision = 0;

    UFUNCTION()
    void OnRep_ActionState();

    // Server: bump the revision, then notify
    void MarkActionStateChanged();

    // --- RepNotify ---
    
    UFUNCTION()
    void OnRep_TurnsMissed();

    UFUNCTION()
    void OnRep_Forfeited();

    UFUNCTION()
    void OnRep_Ready();
};