# UnrealCodingUtils — high level

## Purpose

A deliberately tiny, dependency-free grab-bag for small generic C++ helpers that don't
belong to any gameplay system. It currently holds one class,
`UCodingUtilsComponentLibrary` (a `UBlueprintFunctionLibrary`), with a single static
`BlueprintPure` function `IsAuthoritative(const UActorComponent*)` — a gap-fill for the
fact that `HasAuthority()` exists on `AActor` but not on `UActorComponent` (it checks
component valid -> owner valid -> owner has authority). The plugin is expected to grow only
along this narrow axis.

## Status

Active — enabled in `ConnectIt.uproject`. Depends only on `Core`.

## Key modules

- `UnrealCodingUtils` (Runtime). Single sub-area: `Library/`.

## Public API surface

- `UCodingUtilsComponentLibrary::IsAuthoritative(const UActorComponent*) -> bool`.

## Consumers

Any plugin or the game module that needs a component-level authority check.

## In-repo reference

- [`../../Plugins/UnrealCodingUtils/Docs/README.md`](../../Plugins/UnrealCodingUtils/Docs/README.md)
- [`../../Plugins/UnrealCodingUtils/Docs/Systems.md`](../../Plugins/UnrealCodingUtils/Docs/Systems.md)

## Known rough edges

- `IsAuthoritative` is tagged `Category = "Chimera|Utils"` — leftover naming from an
  unrelated predecessor codebase; should be renamed to a ConnectIt-appropriate category.

_Verify against the in-repo `Docs/` before acting on this list — it reflects a past snapshot._
