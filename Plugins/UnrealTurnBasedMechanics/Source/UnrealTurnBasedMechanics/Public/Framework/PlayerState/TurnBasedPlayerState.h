// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TurnBasedMechanicsEnums.h"
#include "GameplayTagContainer.h"
#include "Action/ActionConfig.h"
#include "TurnBasedPlayerState.generated.h"

class UActionLoadoutDataAsset;
class UTurnBasedAction;

// Fires whenever any per-action runtime state changes -- on the server as it
// happens, on clients from the replication notify. The actions component
// waits on this to leave its post-completion limbo and check turn end.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionRuntimeStateUpdated);

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
    // Per-action budgets/cooldowns for this player, seeded from the loadout's
    // config arrays. Mutated only on the server; clients read the replicated
    // copy. The one source of truth for uses -- see the 2026-09-18 decisions.

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    UActionLoadoutDataAsset* GetLoadout() const { return Loadout; }

    // True when the loadout uses the config arrays (the new action system).
    // A loadout still on the legacy Instanced Actions array returns false,
    // and every new-system code path stays out of its way.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool HasActionConfig() const;

    // The config class whose GetActionTag matches (null if none)
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TSubclassOf<UTurnBasedAction> FindActionClassByTag(FGameplayTag ActionTag) const;

    // Bumped on every state mutation. Lets a waiting component tell "the
    // update I'm waiting for has landed" from "nothing has changed yet",
    // regardless of whether it arrived before or after its outcome RPC.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionStateRevision() const { return ActionStateRevision; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<FPermanentActionRuntimeEntry> GetPermanentActionState() const { return PermanentActionState; }

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    TArray<FNumberedActionRuntimeEntry> GetNumberedActionState() const { return NumberedActionState; }

    // How many times this action has been used this turn (0 if unknown)
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionUsesThisTurn(TSubclassOf<UTurnBasedAction> ActionClass) const;

    // The action's current effective per-turn cap (0 = unlimited, or unknown
    // action) -- read from runtime state, so it reflects temporary changes.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionMaxUsesPerTurn(TSubclassOf<UTurnBasedAction> ActionClass) const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetActionUsesThisTurnByTag(FGameplayTag ActionTag) const;

    // Copies of one action's whole runtime state. Return false (and leave Out
    // untouched) if the action has no entry of that kind on this player.
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool GetPermanentRuntimeState(TSubclassOf<UTurnBasedAction> ActionClass,
        FPermanentActionRuntimeState& OutState) const;

    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool GetNumberedRuntimeState(TSubclassOf<UTurnBasedAction> ActionClass,
        FNumberedActionRuntimeState& OutState) const;

    // Uses left in the match for a numbered action (0 if unknown/permanent)
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    int32 GetNumberedActionUsesRemaining(TSubclassOf<UTurnBasedAction> ActionClass) const;

    // Cooldown clear, per-turn cap not reached, and (numbered) uses left
    UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")
    bool CanUseAction(TSubclassOf<UTurnBasedAction> ActionClass) const;

    // Server only. Sets the loadout and seeds every entry from its config
    // arrays (numbered actions start at StartingMatchUses).
    void InitialiseActionState(UActionLoadoutDataAsset* InLoadout);

    // Server only. Spends one use: per-turn count up, cooldown started,
    // numbered uses down. False (and no change) if CanUseAction fails.
    bool ConsumeActionUse(TSubclassOf<UTurnBasedAction> ActionClass);

    // Server only. Adds uses to a numbered action, clamped to its
    // MaxHeldUses. Returns how many were actually added.
    int32 GrantActionUses(TSubclassOf<UTurnBasedAction> ActionClass, int32 Amount);

    // Server only. Turn boundary hooks.
    void ResetActionTurnCounters();
    void TickActionCooldowns();

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Turn Based|Actions")
    FOnActionRuntimeStateUpdated OnActionRuntimeStateUpdated;

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
    UPROPERTY(Replicated)
    TObjectPtr<UActionLoadoutDataAsset> Loadout = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_ActionState)
    TArray<FPermanentActionRuntimeEntry> PermanentActionState;

    UPROPERTY(ReplicatedUsing = OnRep_ActionState)
    TArray<FNumberedActionRuntimeEntry> NumberedActionState;

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