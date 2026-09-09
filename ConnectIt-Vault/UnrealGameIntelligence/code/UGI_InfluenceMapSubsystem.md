---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_InfluenceMapSubsystem.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Private/InfluenceMap/GI_InfluenceMapSubsystem.cpp
reconciled: 2026-09-06
commit: 32e3e7c
---

# UGI_InfluenceMapSubsystem

`UWorldSubsystem`. The registry + orchestrator for influence-map **debug visualisers**:
holds an ordered registry, switches the single active visualiser, broadcasts selection,
and owns a debug UI widget's lifetime. **Intentionally unaware of influence maps and data
shapes** — it operates purely on `IGI_InfluenceMapVisualiser`; the project wires maps to
visualisers before registering.

## When you touch this

- Adding a debug overlay for some per-cell score (threat, reachability, resource).
- Standing up / showing / hiding the visualiser picker UI.

## Entry points

- **Registration:** `RegisterVisualiser(TScriptInterface<IGI_InfluenceMapVisualiser>)`
  (tag must be valid + unique), `UnregisterVisualiser(FGameplayTag)`.
- **Selection:** `SelectVisualiser(Tag)` / `SelectVisualiserByIndex(int32)` (deactivates
  current, activates chosen, broadcasts), `ClearSelection()`.
- **Queries:** `GetRegisteredTags()`, `GetRegisteredVisualiserCount()`, `GetActiveTag()`,
  `IsDebugUICreated()`.
- **UI lifetime:** `CreateDebugUI(TSubclassOf<UGI_InfluenceMapWidget>, APlayerController*)`
  (adds collapsed), `ShowDebugUI()` / `HideDebugUI()` (keeps instance),
  `DestroyDebugUI()`.
- **Delegate:** `OnVisualiserSelected` (`FGameplayTag Tag`, `int32 Index`).

## Collaborators

- Registry of `TScriptInterface<IGI_InfluenceMapVisualiser>` (insertion order = UI button
  order).
- Owns a
  [[UnrealGameIntelligence/code/IGI_InfluenceMapVisualiser|UGI_InfluenceMapWidget]]
  instance; binds its `OnSelectionChanged` (→ `OnWidgetSelectionChanged`) when shown;
  calls `SetupButtons` / `RefreshWidgetButtons` on registry changes while visible.
- Consumer: the `ConnectIt` game module implements the interfaces; the MinMax half of
  this plugin is unrelated.

## Gotchas

- **One active visualiser at a time.** `Activate()` is only called when the visualiser's
  `IsActive()` is false, `Deactivate()` only when true — honour that contract in
  implementations or you double-toggle.
- `CreateDebugUI` **replaces** any existing widget (destroys first).
- `HideDebugUI` keeps the widget alive; only `DestroyDebugUI` frees it.
- The subsystem never sees your data — a broken overlay is almost always the
  visualiser/provider wiring, not this class.

## Cross-impact

Changing the registration/selection API touches every project visualiser and the debug
widget subclass. The `OnVisualiserSelected` signature is bound by project UI.

## See also

- In-repo: `Docs/README.md` → *InfluenceMap*; `InfluenceMap_README.md` (composite-map
  design guide — note its `IGI_InfluenceMap` references are phantom).
- [[UnrealGameIntelligence/systems/influence-map-visualisation|systems/influence-map-visualisation]] ·
  [[UnrealGameIntelligence/recipes/add-an-influence-map-visualiser|recipes/add-an-influence-map-visualiser]]
