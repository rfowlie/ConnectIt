# _core/ — plugins used in this vault

Which plugins the **vault** (the notes, not the game) relies on. Obsidian plugins only: the game's own Unreal plugins are
described in their domains (`UnrealGameMechanics/`, `UnrealTurnBasedMechanics/`, and so on) and in the root
[`CLAUDE.md`](../CLAUDE.md). Claude Code skills in `.claude/skills/` are not plugins and are tracked under `_skills/`.

Last checked 2026-10-29 against `.obsidian/community-plugins.json` and `.obsidian/core-plugins.json`.

## Community plugins (enabled)

| Plugin | Version | What the vault uses it for | Where |
|---|---|---|---|
| **Dataview** | 0.5.68 | Every index that lists notes is a Dataview query over the notes' properties (plans, decisions, logs, recipes, systems, and the design-note to decision links) | `ConnectIt/design/__INDEX.md`, `ConnectIt/_decisions/__INDEX.md`, `ConnectIt/_logs/__INDEX.md`, `ConnectIt/code/recipes/__INDEX.md`, `ConnectIt/code/systems/__INDEX.md` |

Dataview needs the note properties it reads (`Date`, `status`, `summary`, `run`, `type`, `current_stage`, `started`,
`approved_on`, `completed_on`, `decision`, `task`, `reconciled`, `schema`). Property names are underscored where they are
multi-word, so queries can read them directly. Templates for the notes live in `_templates/`.

## Core plugins (enabled)

File explorer, global search, quick switcher, graph view, backlinks, outgoing links, tag pane, properties, page preview,
daily notes, templates, note composer, command palette, editor status, bookmarks, outline, word count, file recovery, sync,
bases.

## Core plugins (disabled, on purpose or unused)

Footnotes, slash command, markdown importer, Zettelkasten prefixer, random note, slides, audio recorder, workspaces, publish,
webviewer. Enable one here before relying on it, and record it in this note.

## Rules

- Add or remove a plugin in the same session as the change, and update this note with it.
- A note that needs a plugin says so at the top of the note (the indexes say "Needs the Dataview plugin").
- Do not depend on a plugin that is not listed here as enabled.
