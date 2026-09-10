---
schema: systems
spans:
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_InfluenceMapSubsystem.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Private/InfluenceMap/GI_InfluenceMapSubsystem.cpp
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_InfluenceMapVisualiserInterface.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_FloatGridDataProviderInterface.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/InfluenceMap/GI_InfluenceMapWidget.h
reconciled: 2026-09-10
commit: 32e3e7c
---

# Influence-map visualisation

## What happens

The project builds an **influence map** (implements `IGI_FloatGridDataProviderInterface`:
`float GetData(FIntPoint)`) and a **visualiser** (implements
`IGI_InfluenceMapVisualiser`), wiring the map into the visualiser through a
project-specific setter. It registers the visualiser with
[[UnrealGameIntelligence/code/UGI_InfluenceMapSubsystem|UGI_InfluenceMapSubsystem]] by a
unique tag. A debug widget (`UGI_InfluenceMapWidget` subclass) shows one button per
registered tag; pressing one broadcasts `OnSelectionChanged` → the subsystem deactivates
the current visualiser and activates the chosen one, which renders by querying its stored
data provider. The subsystem never sees the map or the data shape.

## Diagram

```mermaid
sequenceDiagram
    participant P as Project
    participant Map as Influence map (IGI_FloatGridDataProviderInterface)
    participant Vis as Visualiser (IGI_InfluenceMapVisualiser)
    participant S as UGI_InfluenceMapSubsystem
    participant W as UGI_InfluenceMapWidget (subclass)

    P->>Map: NewObject()
    P->>Vis: NewObject(); Vis->SetDataProvider(Map)  %% project-specific setter
    P->>S: RegisterVisualiser(Vis)   (unique GetTag())
    P->>S: CreateDebugUI(WidgetClass, PC) ; ShowDebugUI()
    S->>W: SetupButtons(GetRegisteredTags())
    Note over S,W: subsystem binds W.OnSelectionChanged
    W-->>S: OnSelectionChanged(Tag, Index)   (user clicks a button)
    S->>Vis: (prev) Deactivate()   %% only if IsActive()
    S->>Vis: Activate()            %% only if !IsActive()
    Vis->>Map: GetData(FIntPoint) per cell → render
    S-->>P: OnVisualiserSelected(Tag, Index)
```

## Steps

1. **Build & wire (project):** map `NewObject`; visualiser `NewObject`;
   `Vis->SetDataProvider(Map)` (your setter).
2. **Register:** `Subsystem->RegisterVisualiser(Vis)` — fails if `GetTag()` is invalid or
   already registered.
3. **UI:** `CreateDebugUI(WidgetClass, OwningPlayer)` (adds collapsed) → `ShowDebugUI()`.
   The subsystem calls `SetupButtons(GetRegisteredTags())` and binds
   `OnSelectionChanged`.
4. **Select:** button press → `UGI_InfluenceMapWidget::OnSelectionChanged(Tag, Index)` →
   subsystem `SelectVisualiser` → `Deactivate()` old (if active), `Activate()` new (if
   not).
5. **Render:** the active visualiser loops its cells calling `Map->GetData(FIntPoint)` and
   draws; `OnVisualiserSelected` fires for project listeners.
6. **Teardown:** `ClearSelection()` / `HideDebugUI()` (keeps widget) / `DestroyDebugUI()`.

## Gotchas

- **`Activate`/`Deactivate` are called only on a state change** — implement `IsActive()`
  correctly or you double-toggle and leak debug draws.
- `RegisterVisualiser` returns `false` on a tag clash / invalid tag — check it.
- The data-provider setter is **not** on the interface; each concrete visualiser adds its
  own.
- `IGI_InfluenceMap` in the example comments doesn't exist — ignore it.
- `CreateDebugUI` destroys any existing widget first.

## Cross-impact

A new data shape ⇒ a new provider interface + visualisers; subsystem/widget unchanged.
Changing `OnVisualiserSelected` / `OnSelectionChanged` signatures touches project UI and
the widget subclass.

## See also

- In-repo: [[UnrealGameIntelligence/CLAUDE|UnrealGameIntelligence overview]] → *InfluenceMap*.
- [[UnrealGameIntelligence/code/recipes/add-an-influence-map-visualiser|recipes/add-an-influence-map-visualiser]]
