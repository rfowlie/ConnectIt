# CLAUDE.md — UnrealUIMechanics

Intended to hold default UI functionality and reusable widgets for games (per its
`.uplugin`: "Default functionality and widgets for games").

## Domain

- **Source:** `../Plugins/UnrealUIMechanics/Source/` — a generated module skeleton only:
  `FUnrealUIMechanicsModule` with empty `StartupModule`/`ShutdownModule`, an unmodified
  Epic-boilerplate `.Build.cs` (`Core`, `CoreUObject`, `Engine`, `Slate`, `SlateCore`),
  and `Resources/Icon128.png`. No classes, headers, or gameplay code.
- **Status:** **stub — nothing implemented.** Not enabled in `ConnectIt.uproject`.
- **Type:** game-agnostic plugin scaffold.
- **Consumed by:** nothing.

## Sections

None yet — there is no source to document. Create `code/` (per
[[_core/_schema/_code|_core/_schema/_code.md]]) once the plugin has real types.

## Next steps before this is real

- Decide the first widgets/systems it owns (candidates: HUD base, menu framework,
  consolidating the debug-widget story now split between
  [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics]]'s `UDWidgetBase` and the per-plugin
  `UDWidget_*` subclasses).
- Enable in `ConnectIt.uproject` once it has content.
