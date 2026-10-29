---
schema: code
kind: UCLASS (UDataAsset)
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ItemActionInventoryDataAsset.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/ItemActionInventoryDataAsset.cpp
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/TurnBasedMechanicsSettings.h
reconciled: 2026-10-29
commit: 193c1c9
---

# UItemActionInventoryDataAsset

## What it is

The game's item actions and **how many of each a player can hold**. It's static tuning, the same in every match.
**Every item action has a cap:** an action this asset doesn't list can't be held. One asset per project, named in
Project Settings > Plugins > Turn Based Mechanics > **Item Inventory** (`UTurnBasedMechanicsSettings`).

## When you touch this

- Making an action usable as an item, or changing how many can be held.

## Entry points

- `MaxHeld`: `TMap<TSubclassOf<UTurnBasedPlayerAction>, int32>`, each at least 1.
- `GetMaxHeld(ActionClass)`: 0 for an unlisted action.
- static `Get()`: the project's asset, or null.
- `IsDataValid`: errors on an entry with no action.

## Collaborators

- [[UnrealTurnBasedMechanics/code/ATurnBasedPlayerState|ATurnBasedPlayerState]]:
  - `InitialiseInventory` clamps each count to the cap and drops an unlisted action with a warning;
  - `GrantItems` adds up to the cap and returns how many were added (0 for an unlisted action).

## Gotchas

- **No asset, or an unlisted action, means it can't be held.** Grants of it add nothing, and an inventory entry for it
  is dropped. This is deliberate: an item without a cap is a mistake.
- Caps aren't per match or per grant. The game's grant calls (`GrantItems(Class, Amount)`) never pass one.

## Cross-impact

- A game that saves inventories (ConnectIt's Adventure) reads `GetInventory()` back after a match. Lowering a cap
  clamps saved counts on the next seed.

## Changes

- 2026-10-24 -- created (player-actions plan, Stage 3 revision): caps moved off the inventory entries and the grant
  call into one static asset; an unlisted action can't be held (owner's option b).

## See also

- [[UnrealTurnBasedMechanics/code/ActionConfig|ActionConfig]] (`FItemActionEntry`, `FItemBudget`)
