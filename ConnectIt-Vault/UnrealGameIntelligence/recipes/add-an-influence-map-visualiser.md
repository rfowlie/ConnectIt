---
schema: recipes
task: Add a debug overlay that renders a per-cell float score (threat, reachability, etc.) selectable from the influence-map debug UI.
touches:
  - a new influence-map class implementing IGI_FloatGridDataProviderInterface
  - a new visualiser class implementing IGI_InfluenceMapVisualiser (+ a project data-provider setter)
  - a UGI_InfluenceMapWidget subclass (once, if not already present)
  - project setup code that constructs + wires + registers
reconciled: 2026-09-06
commit: 32e3e7c
---

# Recipe: add an influence-map visualiser

## Goal

A new debug overlay showing one float per grid cell, appearing as a button in the
influence-map debug widget and toggled on/off through it.

## Prerequisites

- `UGI_InfluenceMapSubsystem` (always present — `UWorldSubsystem`).
- A `UGI_InfluenceMapWidget` subclass wired once: implement `SetupButtons(Tags)` to build
  a button per tag; each button broadcasts `OnSelectionChanged(Tag, Index)`.
- A gameplay tag for the new visualiser.

## Steps

1. **The map.** A `UObject` implementing `IGI_FloatGridDataProviderInterface`:
   ```cpp
   float UMyGame_ThreatMap::GetData(FIntPoint P) const; // 0.0f outside range
   ```
   Convert `FIntPoint` to your own grid coord internally.
2. **The visualiser.** A `UObject` implementing `IGI_InfluenceMapVisualiser`:
   - `GetTag()` → your unique tag; `GetDisplayName()` → button label.
   - Add your own setter, e.g.
     `SetDataProvider(TScriptInterface<IGI_FloatGridDataProviderInterface>)`, storing it.
   - `Activate()` — iterate cells, `Provider->GetData(P)`, draw (debug lines / decals /
     text). Set an `bIsActive` flag; `IsActive()` returns it.
   - `Deactivate()` — clear every draw you made; clear the flag.
3. **Wire & register (project setup):**
   ```cpp
   auto* Map = NewObject<UMyGame_ThreatMap>(this);
   auto* Vis = NewObject<UMyGame_ThreatVisualiser>(this);
   Vis->SetDataProvider(Map);
   GetWorld()->GetSubsystem<UGI_InfluenceMapSubsystem>()->RegisterVisualiser(Vis);
   ```
4. **Show the UI** (once): `Subsystem->CreateDebugUI(MyWidgetClass, PC); Subsystem->ShowDebugUI();`

## Verify

- The new button appears in the widget (`GetRegisteredTags()` includes your tag).
- Selecting it: the previous overlay disappears, yours draws; `GetActiveTag()` is your
  tag; `OnVisualiserSelected` fires.
- Selecting another / `ClearSelection()`: your `Deactivate()` runs and all your draws are
  gone.
- Registering with a tag already in use returns `false` from `RegisterVisualiser`.

## Pitfalls

- **`IsActive()` wrong** → the subsystem skips `Activate`/`Deactivate` (called only on a
  real state change) and your overlay gets stuck or double-drawn.
- **`Deactivate()` doesn't fully clear** persistent debug lines / decals → ghost overlays.
- Following the header example's `IGI_InfluenceMap` — it doesn't exist; only implement
  `IGI_FloatGridDataProviderInterface` (+ any project interface).
- Forgetting the data-provider setter is *your* addition — the interface doesn't declare
  it, so the subsystem can't wire the map for you.

## See also

- [[UnrealGameIntelligence/code/IGI_InfluenceMapVisualiser|IGI_InfluenceMapVisualiser]] ·
  [[UnrealGameIntelligence/code/UGI_InfluenceMapSubsystem|UGI_InfluenceMapSubsystem]]
- [[UnrealGameIntelligence/systems/influence-map-visualisation|systems/influence-map-visualisation]]
- In-repo: `old/Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/README/InfluenceMap_README.md`.
