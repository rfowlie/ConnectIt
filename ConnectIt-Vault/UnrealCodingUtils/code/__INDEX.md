---
schema: code
reconciled: 2026-09-10
commit: 3086271
---

# UnrealCodingUtils — code index

Inventory + module map. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
Domain overview: [[UnrealCodingUtils/CLAUDE|CLAUDE.md]].

## Inventory

| Type | Kind | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|
| [[UnrealCodingUtils/code/UCodingUtilsLibrary\|UCodingUtilsLibrary]] | UCLASS (UBlueprintFunctionLibrary) | primary | `Library/CodingUtilsLibrary.{h,cpp}` | current |
| FUnrealCodingUtilsModule | class (IModuleInterface) | internal | `UnrealCodingUtils.h` | stub |

## Map

`Plugins/UnrealCodingUtils/Source/UnrealCodingUtils/` — one Runtime module, `Core` only,
one sub-area `Library/`. `UCodingUtilsLibrary` holds two static `BlueprintPure` authority
checks (`IsAuthoritativeActor`, `IsAuthoritativeComponent`) filling the gap that
`HasAuthority()` isn't on `UActorComponent`. No `systems/` or `recipes/` — no flow or
task to document.

**Start at:** [[UnrealCodingUtils/code/UCodingUtilsLibrary|UCodingUtilsLibrary]].
