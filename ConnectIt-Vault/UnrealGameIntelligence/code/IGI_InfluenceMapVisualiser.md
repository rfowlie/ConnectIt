---
schema: code
kind: UINTERFACE
role: primary
source:
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_InfluenceMapVisualiserInterface.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_FloatGridDataProviderInterface.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_InfluenceMapWidget.h
reconciled: 2026-09-06
commit: 32e3e7c
---

# IGI_InfluenceMapVisualiser (+ data provider + widget)

The two contracts the project implements to get a debug overlay, plus the abstract debug
widget.

## When you touch this

- Writing a new visualiser type, or a new **data-provider shape** (float per cell is the
  shipped example; you can define tag/vector/enum shapes the same way).

## Entry points

### `IGI_InfluenceMapVisualiser` (implement on your visualiser)

- `GetTag() const` — unique registry tag (match the map it renders).
- `GetDisplayName() const` — UI button label.
- `Activate()` / `Deactivate()` — start/stop rendering; **`Activate` only called when
  `IsActive()==false`, `Deactivate` only when `true`**.
- `IsActive() const`.

### `IGI_FloatGridDataProviderInterface` (implement on your influence map)

- `virtual float GetData(FIntPoint) const = 0` — the score per cell; return `0.0f`
  outside range. Uses `FIntPoint` to avoid depending on a project grid type.

### `UGI_InfluenceMapWidget` (abstract; subclass a UMG widget)

- `SetupButtons(const TArray<FGameplayTag>&)` (`BlueprintImplementableEvent`) — build the
  button list; called on show and on registry change while visible.
- `OnSelectionChanged` (`FGameplayTag`, `int32`) — broadcast on button press; the
  subsystem listens.

## Collaborators

- The visualiser stores a `TScriptInterface<IGI_FloatGridDataProviderInterface>` (via a
  **project-specific** setter you add), so it renders **any** map of that data shape
  without casting.
- [[UnrealGameIntelligence/code/UGI_InfluenceMapSubsystem|UGI_InfluenceMapSubsystem]]
  registers, selects, and drives `Activate`/`Deactivate`; it never sees the data
  provider.
- The widget holds **no** subsystem reference — communication is outward via
  `OnSelectionChanged`.

## Gotchas

- **`IGI_InfluenceMap` in the header comments is a phantom** — it doesn't exist. Your map
  implements `IGI_FloatGridDataProviderInterface` (+ whatever project interface), not
  that.
- Honour the `Activate`/`Deactivate` call-only-on-state-change contract or you leak debug
  draws.
- `GetTag()` must be unique in the registry — a clash makes `RegisterVisualiser` fail.
- The data-provider setter is yours to add on the concrete visualiser; the interface
  doesn't declare it.

## Cross-impact

A new data shape = a new provider interface (mirroring
`IGI_FloatGridDataProviderInterface`) + visualiser types that consume it. The subsystem
and widget are untouched.

## See also

- In-repo: `InfluenceMap_README.md` (design guide), `Docs/README.md` → *InfluenceMap*.
- [[UnrealGameIntelligence/recipes/add-an-influence-map-visualiser|recipes/add-an-influence-map-visualiser]]
