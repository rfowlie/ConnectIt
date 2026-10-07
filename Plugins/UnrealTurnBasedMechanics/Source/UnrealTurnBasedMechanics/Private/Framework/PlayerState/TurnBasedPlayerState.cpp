// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedAction.h"
#include "Action/TurnEndRequirement.h"
#include "Net/UnrealNetwork.h"

namespace
{
    template <typename TEntry>
    TEntry* FindEntry(TArray<TEntry>& Entries, const TSubclassOf<UTurnBasedAction>& ActionClass)
    {
        return Entries.FindByPredicate(
            [&ActionClass](const TEntry& E) { return E.ActionClass == ActionClass; });
    }

    template <typename TEntry>
    const TEntry* FindEntry(const TArray<TEntry>& Entries, const TSubclassOf<UTurnBasedAction>& ActionClass)
    {
        return Entries.FindByPredicate(
            [&ActionClass](const TEntry& E) { return E.ActionClass == ActionClass; });
    }

    template <typename TConfig>
    const TConfig* FindConfig(const TArray<TConfig>& Configs, const TSubclassOf<UTurnBasedAction>& ActionClass)
    {
        return Configs.FindByPredicate(
            [&ActionClass](const TConfig& C) { return C.ActionClass == ActionClass; });
    }
}


ATurnBasedPlayerState::ATurnBasedPlayerState()
{
	bReplicates = true;
}

void ATurnBasedPlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATurnBasedPlayerState, SlotIndex);
	DOREPLIFETIME(ATurnBasedPlayerState, ParticipantType);
	DOREPLIFETIME(ATurnBasedPlayerState, TurnsMissed);
	DOREPLIFETIME(ATurnBasedPlayerState, bForfeited);
	DOREPLIFETIME(ATurnBasedPlayerState, bIsReady);
	DOREPLIFETIME(ATurnBasedPlayerState, Loadout);
	DOREPLIFETIME(ATurnBasedPlayerState, PermanentActionState);
	DOREPLIFETIME(ATurnBasedPlayerState, NumberedActionState);
	DOREPLIFETIME(ATurnBasedPlayerState, ActionStateRevision);
}

// --- Action State ---

int32 ATurnBasedPlayerState::GetActionUsesThisTurn(TSubclassOf<UTurnBasedAction> ActionClass) const
{
	if (const FPermanentActionRuntimeEntry* P = FindEntry(PermanentActionState, ActionClass))
	{
		return P->State.UsesThisTurn;
	}
	if (const FNumberedActionRuntimeEntry* N = FindEntry(NumberedActionState, ActionClass))
	{
		return N->State.UsesThisTurn;
	}
	return 0;
}

int32 ATurnBasedPlayerState::GetActionMaxUsesPerTurn(TSubclassOf<UTurnBasedAction> ActionClass) const
{
	if (const FPermanentActionRuntimeEntry* P = FindEntry(PermanentActionState, ActionClass))
	{
		return P->State.MaxUsesThisTurn;
	}
	if (const FNumberedActionRuntimeEntry* N = FindEntry(NumberedActionState, ActionClass))
	{
		return N->State.MaxUsesThisTurn;
	}
	return 0;
}

bool ATurnBasedPlayerState::AreTurnEndRequirementsMet(const UActionLoadoutDataAsset* InLoadout) const
{
    if (!IsValid(InLoadout) || !InLoadout->HasTurnEndRequirements())
    {
        return true;
    }

    // Each leaf compares its action's uses this turn against its own
    // required count.
    return InLoadout->TurnEndRequirements->IsSatisfied(
        [this](const FGameplayTag& ActionTag) -> int32
        {
            return GetActionUsesThisTurnByTag(ActionTag);
        });
}

int32 ATurnBasedPlayerState::GetActionUsesThisTurnByTag(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid()) return 0;

	for (const FPermanentActionRuntimeEntry& E : PermanentActionState)
	{
		if (UTurnBasedAction::GetTagForClass(E.ActionClass) == ActionTag) return E.State.UsesThisTurn;
	}
	for (const FNumberedActionRuntimeEntry& E : NumberedActionState)
	{
		if (UTurnBasedAction::GetTagForClass(E.ActionClass) == ActionTag) return E.State.UsesThisTurn;
	}
	return 0;
}

bool ATurnBasedPlayerState::GetPermanentRuntimeState(
	TSubclassOf<UTurnBasedAction> ActionClass, FPermanentActionRuntimeState& OutState) const
{
	if (const FPermanentActionRuntimeEntry* P = FindEntry(PermanentActionState, ActionClass))
	{
		OutState = P->State;
		return true;
	}
	return false;
}

bool ATurnBasedPlayerState::GetNumberedRuntimeState(
	TSubclassOf<UTurnBasedAction> ActionClass, FNumberedActionRuntimeState& OutState) const
{
	if (const FNumberedActionRuntimeEntry* N = FindEntry(NumberedActionState, ActionClass))
	{
		OutState = N->State;
		return true;
	}
	return false;
}

int32 ATurnBasedPlayerState::GetNumberedActionUsesRemaining(TSubclassOf<UTurnBasedAction> ActionClass) const
{
	const FNumberedActionRuntimeEntry* N = FindEntry(NumberedActionState, ActionClass);
	return N ? N->State.UsesRemaining : 0;
}

bool ATurnBasedPlayerState::CanUseAction(TSubclassOf<UTurnBasedAction> ActionClass) const
{
	if (!IsValid(Loadout) || !ActionClass) return false;

	// The per-turn cap comes from the runtime entry (the current effective
	// value), not the config, so temporary changes are honoured. The config
	// lookup is only "is this action in the loadout at all".
	if (FindConfig(Loadout->PermanentActions, ActionClass))
	{
		const FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass);
		if (!Entry) return false;
		if (Entry->State.CooldownTurnsRemaining > 0) return false;
		if (Entry->State.MaxUsesThisTurn > 0 && Entry->State.UsesThisTurn >= Entry->State.MaxUsesThisTurn) return false;
		return true;
	}

	if (FindConfig(Loadout->NumberedActions, ActionClass))
	{
		const FNumberedActionRuntimeEntry* Entry = FindEntry(NumberedActionState, ActionClass);
		if (!Entry) return false;
		if (Entry->State.UsesRemaining <= 0) return false;
		if (Entry->State.CooldownTurnsRemaining > 0) return false;
		if (Entry->State.MaxUsesThisTurn > 0 && Entry->State.UsesThisTurn >= Entry->State.MaxUsesThisTurn) return false;
		return true;
	}

	// Not in this loadout's config arrays
	return false;
}

void ATurnBasedPlayerState::InitialiseActionState(UActionLoadoutDataAsset* InLoadout)
{
	if (!HasAuthority()) return;

	Loadout = InLoadout;
	PermanentActionState.Reset();
	NumberedActionState.Reset();

	if (IsValid(Loadout))
	{
		for (const FPermanentActionConfig& Config : Loadout->PermanentActions)
		{
			if (!Config.ActionClass) continue;

			FPermanentActionRuntimeEntry Entry;
			Entry.ActionClass = Config.ActionClass;
			Entry.State.MaxUsesThisTurn = Config.MaxUsesPerTurn;
			PermanentActionState.Add(Entry);
		}

		for (const FNumberedActionConfig& Config : Loadout->NumberedActions)
		{
			if (!Config.ActionClass) continue;

			FNumberedActionRuntimeEntry Entry;
			Entry.ActionClass = Config.ActionClass;
			Entry.State.MaxUsesThisTurn = Config.MaxUsesPerTurn;
			Entry.State.UsesRemaining = Config.MaxHeldUses > 0
				? FMath::Min(Config.StartingMatchUses, Config.MaxHeldUses)
				: Config.StartingMatchUses;
			NumberedActionState.Add(Entry);
		}
	}

	MarkActionStateChanged();
	OnLoadoutChanged.Broadcast();
}

void ATurnBasedPlayerState::OnRep_Loadout()
{
	OnLoadoutChanged.Broadcast();
}

bool ATurnBasedPlayerState::ConsumeActionUse(TSubclassOf<UTurnBasedAction> ActionClass)
{
	if (!HasAuthority()) return false;
	if (!CanUseAction(ActionClass)) return false;

	if (FPermanentActionRuntimeEntry* P = FindEntry(PermanentActionState, ActionClass))
	{
		const FPermanentActionConfig* Config = FindConfig(Loadout->PermanentActions, ActionClass);
		P->State.UsesThisTurn++;
		if (Config && Config->CooldownTurns > 0) P->State.CooldownTurnsRemaining = Config->CooldownTurns;
	}
	else if (FNumberedActionRuntimeEntry* N = FindEntry(NumberedActionState, ActionClass))
	{
		const FNumberedActionConfig* Config = FindConfig(Loadout->NumberedActions, ActionClass);
		N->State.UsesThisTurn++;
		N->State.UsesRemaining--;
		if (Config && Config->CooldownTurns > 0) N->State.CooldownTurnsRemaining = Config->CooldownTurns;
	}
	else
	{
		return false;
	}

	MarkActionStateChanged();
	return true;
}

int32 ATurnBasedPlayerState::GrantActionUses(TSubclassOf<UTurnBasedAction> ActionClass, int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || !IsValid(Loadout)) return 0;

	FNumberedActionRuntimeEntry* Entry = FindEntry(NumberedActionState, ActionClass);
	const FNumberedActionConfig* Config = FindConfig(Loadout->NumberedActions, ActionClass);
	if (!Entry || !Config) return 0;

	int32 ToAdd = Amount;
	if (Config->MaxHeldUses > 0)
	{
		ToAdd = FMath::Clamp(Config->MaxHeldUses - Entry->State.UsesRemaining, 0, Amount);
	}
	if (ToAdd <= 0) return 0;

	Entry->State.UsesRemaining += ToAdd;
	MarkActionStateChanged();
	return ToAdd;
}

void ATurnBasedPlayerState::ResetActionTurnCounters()
{
	if (!HasAuthority()) return;

	// Only the per-turn use counters reset. The effective cap
	// (MaxUsesThisTurn) is deliberately left alone so a temporary change to it
	// survives turn boundaries.
	for (FPermanentActionRuntimeEntry& E : PermanentActionState)
	{
		E.State.UsesThisTurn = 0;
	}
	for (FNumberedActionRuntimeEntry& E : NumberedActionState)
	{
		E.State.UsesThisTurn = 0;
	}

	MarkActionStateChanged();
}

void ATurnBasedPlayerState::TickActionCooldowns()
{
	if (!HasAuthority()) return;

	for (FPermanentActionRuntimeEntry& E : PermanentActionState)
	{
		if (E.State.CooldownTurnsRemaining > 0) E.State.CooldownTurnsRemaining--;
	}
	for (FNumberedActionRuntimeEntry& E : NumberedActionState)
	{
		if (E.State.CooldownTurnsRemaining > 0) E.State.CooldownTurnsRemaining--;
	}

	MarkActionStateChanged();
}

void ATurnBasedPlayerState::OnRep_ActionState()
{
	// Clients: the revision arrived with the state -- just notify
	OnActionRuntimeStateUpdated.Broadcast();
}

void ATurnBasedPlayerState::MarkActionStateChanged()
{
	ActionStateRevision++;
	OnActionRuntimeStateUpdated.Broadcast();
}

bool ATurnBasedPlayerState::HasActionConfig() const
{
	return IsValid(Loadout)
		&& (!Loadout->PermanentActions.IsEmpty() || !Loadout->NumberedActions.IsEmpty());
}

TSubclassOf<UTurnBasedAction> ATurnBasedPlayerState::FindActionClassByTag(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid()) return nullptr;

	for (const FPermanentActionRuntimeEntry& E : PermanentActionState)
	{
		if (UTurnBasedAction::GetTagForClass(E.ActionClass) == ActionTag) return E.ActionClass;
	}
	for (const FNumberedActionRuntimeEntry& E : NumberedActionState)
	{
		if (UTurnBasedAction::GetTagForClass(E.ActionClass) == ActionTag) return E.ActionClass;
	}
	return nullptr;
}

// --- Manager-only setters ---

void ATurnBasedPlayerState::SetSlotIndex(int32 InSlotIndex)
{
	if (!HasAuthority()) return;
	SlotIndex = InSlotIndex;
}

void ATurnBasedPlayerState::SetParticipantType(EParticipantType InType)
{
	if (!HasAuthority()) return;
	ParticipantType = InType;
}

void ATurnBasedPlayerState::IncrementTurnsMissed()
{
	if (!HasAuthority()) return;

	TurnsMissed++;
	OnTurnsMissedChanged.Broadcast(TurnsMissed);
}

void ATurnBasedPlayerState::SetForfeited(bool bInForfeited)
{
	if (!HasAuthority()) return;
	if (bForfeited == bInForfeited) return;

	bForfeited = bInForfeited;
	if (bForfeited) OnForfeited.Broadcast();
}

void ATurnBasedPlayerState::SetReady(bool bInReady)
{
	if (!HasAuthority()) return;
	if (bIsReady == bInReady) return;

	bIsReady = bInReady;
	OnReadyChanged.Broadcast(bIsReady);
}

void ATurnBasedPlayerState::ResetTurnState()
{
	if (!HasAuthority()) return;

	TurnsMissed = 0;
	bForfeited  = false;
	bIsReady    = false;
}

// --- RepNotify ---

void ATurnBasedPlayerState::OnRep_TurnsMissed()
{
	OnTurnsMissedChanged.Broadcast(TurnsMissed);
}

void ATurnBasedPlayerState::OnRep_Forfeited()
{
	if (bForfeited) OnForfeited.Broadcast();
}

void ATurnBasedPlayerState::OnRep_Ready()
{
	OnReadyChanged.Broadcast(bIsReady);
}