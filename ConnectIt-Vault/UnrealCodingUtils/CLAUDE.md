# CLAUDE.md — UnrealCodingUtils

A deliberately tiny, dependency-free grab-bag for small generic C++ helpers that don't
belong to any gameplay system. Today it holds one class, `UCodingUtilsLibrary` (a
`UBlueprintFunctionLibrary`), with two static `BlueprintPure` functions —
`IsAuthoritativeActor(const AActor*)` and `IsAuthoritativeComponent(const UActorComponent*)`
— filling the gap that `HasAuthority()` exists on `AActor` but not `UActorComponent`.
Expected to grow only along this narrow axis.

## Domain

- **Source:** `../Plugins/UnrealCodingUtils/Source/` — one Runtime module, single
  sub-area `Library/`. Depends only on `Core`.
- **Status:** active — enabled in `ConnectIt.uproject`.
- **Type:** game-agnostic plugin, independently extractable.
- **Consumed by:** any plugin or [[ConnectIt/CLAUDE|ConnectIt]] needing a component-level
  authority check.

## Sections

- **`code/`** — one type page + `code/__INDEX.md`. Governed by
  [[_core/_schema/_code|_core/_schema/_code.md]]. No `systems/` / `recipes/` — no flow or
  task to document yet.
- `logs/` — created on first use.

## Start here

[[UnrealCodingUtils/code/__INDEX|code/__INDEX.md]].

## Known rough edges

- Both functions are tagged `Category = "Chimera|Utils"` — leftover naming from an
  unrelated predecessor codebase; rename to a ConnectIt-appropriate category.
