# Path Tracer

**Status:** goals only (2026-10-30). Reviewed from the to-do notes and the plugin's content; not opened in the editor and not run.

## Summary

A Blueprint path tracer from an Unreal demo project: a core actor (`BP_PathTracer`, and a coloured variant) lays path marks
along a route, with a path-type enum, info widgets, a camera and a set of showcase maps. Its source is the content-only plugin
[UnrealPathMechanics](../UnrealPathMechanics/CLAUDE.md) (about 250 assets).

## What's in the plugin

| Area | What it is | Notes |
|---|---|---|
| Core actor | `BP_PathTracer`, `BP_PathTracer_Colored`, `E_PT_PathType` | The core is a Blueprint with no C++ parent. |
| Path marks | `BP_PathMark_01` to `_31`, `_Master`, two shadow variants | One Blueprint per material. See the findings. |
| UI | `WB_Info_Widget`, `WB_Info_Widget_Slot` | Info display. |
| Materials and textures | About 30 materials and 27 textures, including animated icon and effect materials | Worth keeping. |
| Meshes | Path mark and dot meshes, cylinder line and corner | |
| Showcase | 32 showcase Blueprints, 68 materials, 5 maps | Demo content, probably not needed. |
| Demo | Third-person mannequin, animations, scene geometry | From the source demo; not ours. |

## Goals

1. **Convert the path tracer from Blueprint to C++.** The core actor, its path-mark behaviour and the path logic. The
   owner's reason: everything is Blueprint, which is slow; C++ should give a large speed-up.
2. **Find each part a home in our framework.** Decide per part, not for the whole tool. Candidates are a generic path plugin,
   [UnrealGridMechanics](../UnrealGridMechanics/CLAUDE.md) for route and grid logic, or ConnectIt for game-specific use. No home is
   chosen yet.
3. **Break the monolith apart.** The core actor and the player controller do too much and serve every map mode. Split them so
   each mode and each piece of behaviour can be a base class, and the valuable modes survive.
4. **One path-mark class with a material slot.** Replace the per-material Blueprints with one class that exposes the material
   slot, so behaviour is generic and the look is data.
5. **Data assets for the data-only subclasses.** The subclasses of the core Blueprint are only data. Make them data assets that
   slot into the tracer as defaults.
6. **Make the grid generic.** The demo's grid is built on the plane idea and is not flexible like ours. Only the grid parts that
   matter to a path need to be kept (see phase 2).

## Phases

1. **Extract the path tracer.** It appears to work on its own, so convert and place it first, with no grid dependency.
2. **Look for grid value.** Then look at the grid section for anything worth exporting for other projects. Most of it (grid,
   blockers) is expected to be of no use; the question is how the tracer runs on **our** grid.

## Findings from the original to-do notes

- It is an engine project and needs porting to a plugin.
- The camera setup is decent and could be converted or reused, but it does not use Enhanced Input.
- The player controller is monolithic and supports all map modes. Each mode may be valuable, so it should be broken apart.
- The animated icon and effect materials are already good and worth keeping.
- The path marks use a Blueprint per material rather than exposing the material slot.
- The subclasses of the core Blueprint are data Blueprints. A data asset does the same job more cleanly.
- Everything is Blueprint, which is inefficient. The A* pathing has a lot of room for speed-up in C++.
- The demo grid is built on the plane idea and needs breaking apart before it can be generic.

## Open questions

- Which home does the converted tracer land in: a new generic plugin, or one of the existing ones?
- Can the tracer take its route from any grid through an interface, rather than from our grid directly?
- Do we keep the showcase and demo content, or leave it in the source demo?
- Is the path tracer intended for ConnectIt at all, or only as a reusable tool?
- Is this a C++ version? (https://www.fab.com/listings/15d5f5c7-72ce-42e3-9e8f-bf0f4c5726b60)
