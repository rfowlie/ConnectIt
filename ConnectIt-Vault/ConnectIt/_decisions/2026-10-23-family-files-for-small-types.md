---
Date: 2026-10-23
status: Active
superseded by:
tags:
  - code-organisation
  - conventions
summary: 'Small same-shaped struct families (operations, events, power effects, match conditions) live one family per file beside a separate base header; systems with real logic keep their own files; a family with several files gets a subfolder (Board/Rules/Conditions/)'
---
## Decision

ConnectIt's small, same-shaped struct families live **one family per file**, next to a separate header for their base:

- **Families in one file:**
  - board operations (`Board/Operations/ConnectIt_BoardOperations.h`, base `ConnectIt_BoardOperation.h`);
  - board events (`Board/Events/ConnectIt_BoardEvents.h`);
  - tile power effects (`Board/Powers/ConnectIt_TilePowerEffects.h`, base in `ConnectIt_TilePower.h`);
  - match conditions (`Board/Rules/Conditions/ConnectIt_MatchConditions.h`, base `ConnectIt_MatchCondition.h`).
- **The base type stays in its own header**, so code that only needs the interface doesn't pull in every subtype.
- **Types with real logic or their own dependencies keep their own files:** the rule set, the Lines scoring rule, the
  power chain, the tile catalog, the visualizer, the MinMax rules.
- **A family whose `.cpp` grows too long** may split its `.cpp` by theme, keeping the one header.
- **A family gets a subfolder when it has several files**, for visibility: `Board/Rules/Conditions/` holds the base,
  the family, the group and the asset. The owner followed the same rule the same day: `Board/Rules/Scoring/` (the
  scoring rule base and Lines) and `Board/Actor/` (the tile, builder tile and piece actors).

## Why

- The owner's preference (2026-10-23), after noticing two competing setups. The four match conditions each had their
  own header, while operations, events and power effects were one file each.
- The types are a few fields and an override or two each. One file shows the whole vocabulary at a glance, and adding
  one means appending a struct.
- Nothing included the condition headers one by one: Unreal finds the subtypes by reflection.
- Moving a struct between headers is free for assets: its path is `/Script/ConnectIt.<StructName>`, whatever the
  header, so nothing saved breaks and no redirects are needed.

## What Would Change It

- A family member growing real logic or its own dependencies: it moves to its own file.
- Compile times, if one family header becomes a hot include that changes often.

The merge itself: the log `_logs/2026-10-23-0220.md`.
