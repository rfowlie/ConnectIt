---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealCodingUtils/Source/UnrealCodingUtils/Public/Library/CodingUtilsLibrary.h
  - Plugins/UnrealCodingUtils/Source/UnrealCodingUtils/Private/Library/CodingUtilsLibrary.cpp
reconciled: 2026-09-10
commit: 3086271
---

# UCodingUtilsLibrary

`UBlueprintFunctionLibrary`. The plugin's only class: a `HasAuthority()` gap-fill, since
`HasAuthority()` exists on `AActor` but not on `UActorComponent`.

## When you touch this

- You need a server-authority check from a `UActorComponent` (or from Blueprint on a
  component).

## Entry points

- `static bool IsAuthoritativeActor(const AActor* Actor)` — valid actor → its authority.
- `static bool IsAuthoritativeComponent(const UActorComponent* Component)` — valid
  component → valid owner → owner has authority.

Both `BlueprintPure`.

## Collaborators

None. Pure static helpers.

## Gotchas

- Both are tagged `Category = "Chimera|Utils"` — leftover naming from an unrelated
  predecessor codebase; rename to a ConnectIt-appropriate category when convenient.
- The vault's `high-level.md` previously named this `UCodingUtilsComponentLibrary` with a
  single `IsAuthoritative` — the current header is `UCodingUtilsLibrary` with the two
  functions above. This page is the accurate one.

## Cross-impact

Self-contained. Renaming the class or functions is a find-and-replace across callers in
the plugins and game module.

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealCodingUtils/CLAUDE|UnrealCodingUtils overview]].
