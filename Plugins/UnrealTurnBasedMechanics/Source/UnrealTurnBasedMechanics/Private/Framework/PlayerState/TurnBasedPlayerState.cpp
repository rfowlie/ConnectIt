// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/ItemActionInventoryDataAsset.h"
#include "UnrealTurnBasedMechanics.h"
#include "Action/TurnBasedPlayerAction.h"
#include "Action/TurnEndRequirement.h"
#include "Net/UnrealNetwork.h"

namespace
{
    template <typename TEntry>
    TEntry* FindEntry(TArray<TEntry>& Entries, const TSubclassOf<UTurnBasedPlayerAction>& ActionClass)
    {
        return Entries.FindByPredicate(
            [&ActionClass](const TEntry& E) { return E.ActionClass == ActionClass; });
    }

    template <typename TEntry>
    const TEntry* FindEntry(const TArray<TEntry>& Entries, const TSubclassOf<UTurnBasedPlayerAction>& ActionClass)
    {
        return Entries.FindByPredicate(
            [&ActionClass](const TEntry& E) { return E.ActionClass == ActionClass; });
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
	DOREPLIFETIME(ATurnBasedPlayerState, ActionStateRevision);
	DOREPLIFETIME(ATurnBasedPlayerState, Inventory);
	DOREPLIFETIME(ATurnBasedPlayerState, ItemBudget);
	DOREPLIFETIME(ATurnBasedPlayerState, ItemUsesThisMatch);
	DOREPLIFETIME(ATurnBasedPlayerState, ItemUsesThisTurn);
}

// --- Action State ---

int32 ATurnBasedPlayerState::GetActionUsesThisTurn(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
	const FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass);
	return Entry ? Entry->State.UsesThisTurn : 0;
}

int32 ATurnBasedPlayerState::GetActionMaxUsesPerTurn(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
	const FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass);
	return Entry ? Entry->State.MaxUsesThisTurn : 0;
}

bool ATurnBasedPlayerState::AreTurnEndRequirementsMet(const UActionLoadoutDataAsset* InLoadout) const
{
    if (!IsValid(InLoadout) || !InLoadout->HasTurnEndRequirements())
    {
        return true;
    }

    // Each Action Used leaf compares its action's permanent uses this turn
    // against its own required count; each Item Used leaf, the item uses
    // A named lambda: FTurnEndUses only references it (TFunctionRef), so it
    // must outlive the evaluation -- a temporary would dangle
    const auto ActionUsesThisTurn = [this](const FGameplayTag& ActionTag) -> int32
    {
        return GetActionUsesThisTurnByTag(ActionTag);
    };
    const FTurnEndUses Uses{ ActionUsesThisTurn, ItemUsesThisTurn };
    return InLoadout->TurnEndRequirements->IsSatisfied(Uses);
}

int32 ATurnBasedPlayerState::GetActionUsesThisTurnByTag(FGameplayTag ActionTag) const
{
	return GetActionUsesThisTurn(FindActionClassByTag(ActionTag));
}

bool ATurnBasedPlayerState::GetPermanentRuntimeState(
	TSubclassOf<UTurnBasedPlayerAction> ActionClass, FPermanentActionRuntimeState& OutState) const
{
	if (const FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass))
	{
		OutState = Entry->State;
		return true;
	}
	return false;
}

int32 ATurnBasedPlayerState::GetMatchUsesRemaining(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
	const FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass);
	return Entry ? Entry->State.MatchUsesRemaining : 0;
}

bool ATurnBasedPlayerState::CanUseAction(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
	if (!ActionClass) return false;

	// Everything comes from the runtime entry (the current effective values),
	// not a config, so temporary changes are honoured and a list the game
	// seeded works like the loadout's
	const FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass);
	if (!Entry) return false;

	const FPermanentActionRuntimeState& State = Entry->State;
	if (State.CooldownTurnsRemaining > 0) return false;
	if (State.UsesThisTurn >= State.MaxUsesThisTurn) return false;
	if (State.MatchUsesRemaining == 0) return false;
	return true;
}

void ATurnBasedPlayerState::InitialiseActionState(UActionLoadoutDataAsset* InLoadout)
{
	static const TArray<FPermanentActionConfig> None;
	InitialiseActionState(InLoadout, IsValid(InLoadout) ? InLoadout->PermanentActions : None);
}

void ATurnBasedPlayerState::InitialiseActionState(UActionLoadoutDataAsset* InLoadout,
	const TArray<FPermanentActionConfig>& PermanentActions)
{
	if (!HasAuthority()) return;

	Loadout = InLoadout;
	PermanentActionState.Reset();

	for (const FPermanentActionConfig& Config : PermanentActions)
	{
		if (!Config.ActionClass || FindEntry(PermanentActionState, Config.ActionClass)) continue;

		FPermanentActionRuntimeEntry Entry;
		Entry.ActionClass = Config.ActionClass;
		Entry.State.MatchUsesRemaining = Config.HasUnlimitedMatchUses() ? -1 : Config.MatchUses;
		// A turn always has a limit (see FPermanentActionConfig::TurnUses)
		Entry.State.MaxUsesThisTurn = FMath::Max(Config.TurnUses, 1);
		Entry.State.CooldownTurns = FMath::Max(Config.CooldownTurns, 0);
		PermanentActionState.Add(Entry);
	}

	MarkActionStateChanged();
	OnLoadoutChanged.Broadcast();
}

void ATurnBasedPlayerState::OnRep_Loadout()
{
	OnLoadoutChanged.Broadcast();
}

bool ATurnBasedPlayerState::ConsumeActionUse(TSubclassOf<UTurnBasedPlayerAction> ActionClass)
{
	if (!HasAuthority()) return false;
	if (!CanUseAction(ActionClass)) return false;

	FPermanentActionRuntimeEntry* Entry = FindEntry(PermanentActionState, ActionClass);
	if (!Entry) return false;

	FPermanentActionRuntimeState& State = Entry->State;
	State.UsesThisTurn++;
	if (State.MatchUsesRemaining > 0) State.MatchUsesRemaining--;
	if (State.CooldownTurns > 0) State.CooldownTurnsRemaining = State.CooldownTurns;

	MarkActionStateChanged();
	return true;
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
	if (ItemUsesThisTurn != 0)
	{
		ItemUsesThisTurn = 0;
		OnInventoryChanged.Broadcast();
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
	return !PermanentActionState.IsEmpty();
}

TSubclassOf<UTurnBasedPlayerAction> ATurnBasedPlayerState::FindActionClassByTag(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid()) return nullptr;

	for (const FPermanentActionRuntimeEntry& E : PermanentActionState)
	{
		if (UTurnBasedPlayerAction::GetTagForClass(E.ActionClass) == ActionTag) return E.ActionClass;
	}
	return nullptr;
}

// --- Item actions ---

int32 ATurnBasedPlayerState::GetItemCount(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
	const FItemActionEntry* Entry = FindEntry(Inventory, ActionClass);
	return Entry ? Entry->Count : 0;
}

TSubclassOf<UTurnBasedPlayerAction> ATurnBasedPlayerState::FindItemActionClassByTag(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid()) return nullptr;

	for (const FItemActionEntry& E : Inventory)
	{
		if (UTurnBasedPlayerAction::GetTagForClass(E.ActionClass) == ActionTag) return E.ActionClass;
	}
	return nullptr;
}

bool ATurnBasedPlayerState::HasItemBudgetLeft() const
{
	if (ItemUsesThisTurn >= ItemBudget.TurnUses) return false;
	return ItemBudget.MatchUses < 0 || ItemUsesThisMatch < ItemBudget.MatchUses;
}

bool ATurnBasedPlayerState::CanUseItem(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
	return ActionClass && GetItemCount(ActionClass) > 0 && HasItemBudgetLeft();
}

namespace
{
	// The most of ActionClass a player can hold, from the game's item
	// inventory asset; 0 (with a warning) when it can't be held
	int32 GetItemCap(const TSubclassOf<UTurnBasedPlayerAction>& ActionClass)
	{
		const UItemActionInventoryDataAsset* Caps = UItemActionInventoryDataAsset::Get();
		const int32 Cap = Caps ? Caps->GetMaxHeld(ActionClass) : 0;
		if (Cap <= 0)
		{
			UE_LOG(LogTurnBasedMechanics, Warning,
				TEXT("TurnBasedPlayerState: item action '%s' can't be held -- %s"),
				ActionClass ? *ActionClass->GetName() : TEXT("(none)"),
				Caps ? TEXT("the item inventory asset doesn't list it")
				     : TEXT("no item inventory asset in Project Settings > Turn Based Mechanics"));
		}
		return Cap;
	}
}

void ATurnBasedPlayerState::InitialiseInventory(const TArray<FItemActionEntry>& Entries)
{
	if (!HasAuthority()) return;

	Inventory.Reset();
	for (const FItemActionEntry& Source : Entries)
	{
		if (!Source.ActionClass || Source.Count <= 0 || FindEntry(Inventory, Source.ActionClass)) continue;

		const int32 Cap = GetItemCap(Source.ActionClass);
		if (Cap <= 0) continue;

		FItemActionEntry Entry = Source;
		Entry.Count = FMath::Min(Entry.Count, Cap);
		Inventory.Add(Entry);
	}

	MarkInventoryChanged();
}

void ATurnBasedPlayerState::SetItemBudget(const FItemBudget& InBudget)
{
	if (!HasAuthority()) return;

	ItemBudget = InBudget;
	ItemBudget.TurnUses = FMath::Max(ItemBudget.TurnUses, 0);
	ItemUsesThisMatch = 0;
	ItemUsesThisTurn = 0;

	MarkInventoryChanged();
}

bool ATurnBasedPlayerState::ConsumeItemUse(TSubclassOf<UTurnBasedPlayerAction> ActionClass)
{
	if (!HasAuthority()) return false;
	if (!CanUseItem(ActionClass)) return false;

	FItemActionEntry* Entry = FindEntry(Inventory, ActionClass);
	if (!Entry) return false;

	Entry->Count--;
	ItemUsesThisMatch++;
	ItemUsesThisTurn++;
	// None left: the action leaves the inventory (only held items are kept)
	if (Entry->Count <= 0)
	{
		Inventory.RemoveAll([&ActionClass](const FItemActionEntry& E) { return E.ActionClass == ActionClass; });
	}

	MarkInventoryChanged();
	return true;
}

int32 ATurnBasedPlayerState::GrantItems(TSubclassOf<UTurnBasedPlayerAction> ActionClass, int32 Amount)
{
	if (!HasAuthority() || !ActionClass || Amount <= 0) return 0;

	const int32 Cap = GetItemCap(ActionClass);
	const int32 Held = GetItemCount(ActionClass);
	const int32 Added = FMath::Clamp(Cap - Held, 0, Amount);
	if (Added <= 0) return 0;

	if (FItemActionEntry* Entry = FindEntry(Inventory, ActionClass))
	{
		Entry->Count += Added;
	}
	else
	{
		FItemActionEntry NewEntry;
		NewEntry.ActionClass = ActionClass;
		NewEntry.Count = Added;
		Inventory.Add(NewEntry);
	}

	MarkInventoryChanged();
	return Added;
}

void ATurnBasedPlayerState::OnRep_Inventory()
{
	OnInventoryChanged.Broadcast();
}

void ATurnBasedPlayerState::MarkInventoryChanged()
{
	OnInventoryChanged.Broadcast();
	MarkActionStateChanged();
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