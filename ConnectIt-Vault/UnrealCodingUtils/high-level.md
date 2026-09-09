# UnrealCodingUtils — high level

## Purpose

A deliberately tiny, dependency-free grab-bag for small generic C++ helpers that don't
belong to any gameplay system. It currently holds one class, `UCodingUtilsLibrary` (a
`UBlueprintFunctionLibrary`), with two static `BlueprintPure` functions —
`IsAuthoritativeActor(const AActor*)` and `IsAuthoritativeComponent(const UActorComponent*)`
— a gap-fill for the fact that `HasAuthority()` exists on `AActor` but not on
`UActorComponent`. The plugin is expected to grow only along this narrow axis.

## Status

Active — enabled in `ConnectIt.uproject`. Depends only on `Core`.

## Key modules

- `UnrealCodingUtils` (Runtime). Single sub-area: `Library/`.

## Public API surface

- `UCodingUtilsLibrary::IsAuthoritativeActor(const AActor*) -> bool`
- `UCodingUtilsLibrary::IsAuthoritativeComponent(const UActorComponent*) -> bool`

## Consumers

Any plugin or the game module that needs a component-level authority check.

## In-repo reference

- [`../../old/Plugins/UnrealCodingUtils/Docs/README.md`](../../old/Plugins/UnrealCodingUtils/Docs/README.md)
- [`../../old/Plugins/UnrealCodingUtils/Docs/Systems.md`](../../old/Plugins/UnrealCodingUtils/Docs/Systems.md)

## Vault code docs

- [[UnrealCodingUtils/code/index|code/index.md]] ([[_schema/code|schema]]) — one page,
  `UCodingUtilsLibrary`.

## Known rough edges

- Both functions are tagged `Category = "Chimera|Utils"` — leftover naming from an
  unrelated predecessor codebase; should be renamed to a ConnectIt-appropriate category.

_Verify against the in-repo `old/Plugins/UnrealCodingUtils/Docs/` before acting on this list — it reflects a past snapshot._
