# UnrealCodingUtils

## Purpose

UnrealCodingUtils is a tiny grab-bag of general-purpose C++ helper utilities that aren't specific to any gameplay system. It exists as its own plugin — rather than living inside a consuming game module or one of the gameplay-mechanics plugins — so that small, broadly-useful, dependency-free helpers have a home that any other module can pull in without inheriting gameplay-specific dependencies. It currently contains a single static helper function and is expected to grow as more small, broadly-useful utilities are identified across this plugin suite.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealCodingUtils` | Runtime | Default | Single module; entry point `FUnrealCodingUtilsModule` (`Public/UnrealCodingUtils.h`). |

## Key Dependencies

- Depends on: `Core` (Public only); `CoreUObject`, `Engine`, `Slate`, `SlateCore` (Private). No plugin-level dependencies declared in the `.uplugin`. This is intentionally the lightest-dependency first-party plugin in the repo.
- Depended on by: the consuming game module (not named here, to keep this plugin's documentation project-agnostic), as a Private-only dependency.

## When to Use It

- Adding a small, generic C++ helper function that doesn't belong to any particular gameplay system and would otherwise have nowhere natural to live.
- Utility functions that need to be broadly reusable and kept free of gameplay-mechanics dependencies (e.g. authority/networking helper checks not directly exposed by the engine on a given class).

## When NOT to Use It / Scope Boundaries

- This plugin should stay small and dependency-free: single-purpose, broadly-applicable helpers only.
- Anything gameplay-system-specific belongs in one of the other plugins instead: turn/event/pooling scaffolding in `UnrealGameMechanics`, AI tree-search/influence-maps in `UnrealGameIntelligence`, utility-AI scoring in `UnrealAIMechanics`.
- Do not let this plugin accumulate a dependency on `GameplayTags`, `UMG`, or other gameplay-facing modules — if a helper needs those, it likely belongs in a more specific plugin instead.

## Notable Design Patterns

- Static Blueprint Function Library pattern: helpers are exposed as static functions on a `UBlueprintFunctionLibrary` rather than as instantiable objects, matching the "stateless utility" scope of this plugin.

## Classes

### Library

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UCodingUtilsComponentLibrary` | `UBlueprintFunctionLibrary` | Component-related helpers; currently one function to check server authority on a `UActorComponent` (since `HasAuthority()` isn't directly available there). | `Public/Library/CodingUtilsComponentLibrary.h`. Key API: `static bool IsAuthoritative(const UActorComponent*)`. This function is tagged `Category = "Chimera|Utils"` — "Chimera" is leftover naming from an unrelated predecessor codebase, not this plugin's own name. See Known Discrepancies. |

## Known Discrepancies / Issues in This Plugin

- `UCodingUtilsComponentLibrary::IsAuthoritative` uses `Category = "Chimera|Utils"`, a naming residue from a predecessor codebase. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md#discrepancies) #1 ("Chimera naming residue").
