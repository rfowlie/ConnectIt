# ConnectIt — decisions

One dated note per settled, non-obvious call, so it doesn't get silently re-argued.
Governed by [[_core/_schema/_decisions|_core/_schema/_decisions.md]]. Append-only; a
superseded note is marked, not deleted.

Newest first:

| Date | Decision | Status |
|---|---|---|
| [[ConnectIt/decisions/2026-09-09-faction-visuals-subsystem-built-then-removed\|2026-09-09-faction-visuals-subsystem-built-then-removed]] | Build the faction-visuals subsystem, then remove it — stay white-box | Active |
| [[ConnectIt/decisions/2026-09-09-board-registries-to-world-subsystem\|2026-09-09-board-registries-to-world-subsystem]] | Tile/piece registries live on a `UWorldSubsystem`, duplicated per world from level-config templates | Active |
| [[ConnectIt/decisions/2026-09-08-board-architecture-overhaul\|2026-09-08-board-architecture-overhaul]] | Retire `AConnectIt_BoardManager`; split its jobs across GameMode / GameState / config | Active |
| [[ConnectIt/decisions/2026-09-08-retire-legacy-mvvm-pipeline\|2026-09-08-retire-legacy-mvvm-pipeline]] | Keep the non-networked state-machine / facade / view-model pipeline as dead code, don't build on it | Active |
| [[ConnectIt/decisions/2026-09-06-suite-conventions\|2026-09-06-suite-conventions]] | Cross-cutting conventions for the plugin suite (tags, delegates, RPCs, extension hook) | Active |

## Related

- [[ConnectIt/code/index|code/index.md]] — inventory, suite map, known issues
- [[ConnectIt/logs/README|logs/]] — ConnectIt-scoped maintenance passes
