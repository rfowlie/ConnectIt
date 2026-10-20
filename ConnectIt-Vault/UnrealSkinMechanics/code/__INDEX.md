---
schema: code
reconciled: 2026-09-18
commit: uncommitted
---

# UnrealSkinMechanics — code index

Inventory + module map. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
Domain overview: [[UnrealSkinMechanics/CLAUDE|CLAUDE.md]].

`status`: **stub** = no page yet. The plugin is brand new and uncommitted, so there is no
`commit:` anchor to record yet — set one once it lands in git.

## Inventory

| Type | Kind | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|
| USkinSettings | UCLASS (UDeveloperSettings) | primary | `Settings/SkinSettings.h` | stub |
| USkinSubsystem | UCLASS (UGameInstanceSubsystem) | primary | `Subsystem/SkinSubsystem.{h,cpp}` | stub |
| USkinSaveGame | UCLASS (USaveGame) | primary | `Save/SkinSaveGame.h` | stub |
| FSkinPresetRowBase | USTRUCT (FTableRowBase) | primary (only plugin row struct; projects derive) | `SkinTypes.h` | stub |
| ESkinSource / FSkinResolution | UENUM / USTRUCT | primary | `SkinTypes.h` | stub |
| USkinDebugWidget | UCLASS (UUserWidget) | primary (debug panel) | `Debug/SkinDebugWidget.h` | stub |
| USkinDebugRow | UCLASS (UUserWidget) | internal | `Debug/SkinDebugRow.h` | stub |
| FUnrealSkinMechanicsModule (+ `LogSkinMechanics`) | class (IModuleInterface) | internal | `UnrealSkinMechanics.h` | stub |

## Map

`Plugins/UnrealSkinMechanics/Source/UnrealSkinMechanics/` — one Runtime module. Sub-areas:
`Settings/` (project-settings entry point), `Subsystem/` (the resolver), `Save/` (saved selection), and the top-level `SkinTypes.h` (the preset base row). The plugin defines no skin/catalog row shapes — those are project-owned.

**Start at:** [`ConnectIt/design/skin-system.md`](../../ConnectIt/design/skin-system.md).
