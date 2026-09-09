# UnrealUIMechanics — high level

## Purpose

Intended to hold default UI functionality and reusable widgets for games (per its
`.uplugin` description: "Default functionality and widgets for games").

## Status

**Stub — nothing implemented.** The only source files are the default
`FUnrealUIMechanicsModule` (`StartupModule` / `ShutdownModule` are empty) and an
unmodified Epic-boilerplate `.Build.cs` (deps: `Core`, `CoreUObject`, `Engine`, `Slate`,
`SlateCore`). No widget classes, headers, or gameplay code. Not enabled in
`ConnectIt.uproject`. No `old/Plugins/UnrealUIMechanics/Docs/` folder.

## Key modules

- `UnrealUIMechanics` (Runtime) — generated module skeleton only. No plugin-level
  dependencies. Has `Resources/Icon128.png`.

## Public API surface

None (module interface stub only).

## Consumers

None.

## In-repo reference

- `../../Plugins/UnrealUIMechanics/` — source only; no `old/Plugins/UnrealUIMechanics/Docs/` yet.

## Vault code docs

Not ingested — the plugin has no public types (empty module skeleton). `_section.md`
stays `schemas: [updates]`; add `code` / `systems` / `recipes` once it has real code.

## Next steps before this is real

- Decide the first widgets/systems it should own (candidates: HUD base, menu framework,
  the debug-widget story currently split between `UnrealGameMechanics` `UDWidgetBase` and
  the per-plugin `UDWidget_*` classes).
- Add a `old/Plugins/UnrealUIMechanics/Docs/README.md` + `old/Plugins/UnrealUIMechanics/Docs/Systems.md` to match the other plugins.
- Enable in `ConnectIt.uproject` once it has content.
