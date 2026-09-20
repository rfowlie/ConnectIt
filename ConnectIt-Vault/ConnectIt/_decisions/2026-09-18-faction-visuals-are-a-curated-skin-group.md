---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - visuals
  - skins
  - adventure-mode
  - factions
---
## Decision

Faction visuals (colour, mesh, icon, label per faction slot) are **not a separate
mechanism**. They are a **curated skin group**: a skin whose payload is keyed by faction
slot, selected through the same layered skin system as everything else
([skin-system-layered-data-and-flow](2026-09-18-skin-system-layered-data-and-flow.md)).

**Skin groups are relevant to adventure mode**: a level can force or curate a look by
naming a skin group, via the level-config policy in that system's resolution order
(level-forced > player override > player preset > project default). In adventure mode
the designer decides the look; in online play the player's own selection applies.

## Why

The retired
[faction-visuals subsystem](2026-09-09-faction-visuals-subsystem-built-then-removed.md)
was built as a bespoke `UGameInstanceSubsystem` + palette data asset + developer-settings
page, then removed to stay white-box; its recorded trigger for revisiting was "the first
production UI element that needs faction colour, icon or label." Designing the general
skin system revealed that palette *is* just one skin category keyed by faction slot, so a
second, parallel owner would duplicate the resolver, the save data, and the options UI.
Folding it in keeps one mechanism and revives the retired decision's intent (GameInstance
owner, not replicated, not on `PlayerState`) without its bespoke subsystem.

## What Would Change It

If faction visuals ever need to be authoritative gameplay data (e.g. colour used by game
rules, or opponent-visible identity that must be consistent across clients), they'd move
out of the cosmetic skin path — that would also revisit the local-only rule in
[skins-local-only-and-data-on-fixed-actor-class](2026-09-18-skins-local-only-and-data-on-fixed-actor-class.md).
