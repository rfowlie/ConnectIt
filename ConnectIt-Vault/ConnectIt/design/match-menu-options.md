# Match menu options

An idea, not built: how the main menu could set up the AI opponent once modes are match presets naming their directors.
Recorded from the 2026-10-16 director discussion ([ai-director-pipeline](ai-director-pipeline.md)); the owner isn't fixated on
the menu yet, so this is a note to come back to.

## What's settled

- **The menu picks a match preset.** A "mode" (Classic, Blitz...) is a preset that names its AI director, so picking a mode is
  picking one thing. Two difficulties of the same mode can simply be two presets.
- **Rule tweaks go to the rules, AI tweaks to the director.** The target score is a rule (everyone's win depends on it, and a
  versus match with no AI still needs it); it stays a menu tweak applied to the rule set (`FConnectItMatchSettings::TargetScore`,
  which changes the last goal's copy). Difficulty, aggression and the like are AI settings and belong to the director.

## The idea

One small **`FConnectItMatchOptions`** struct the menu fills -- difficulty, plus whatever else proves common -- carried with the
match settings and **handed to the director at match start** (e.g. a parameter of `On Match Started`, or a getter). Each
director maps "difficulty" to its own settings: the Classic director might pick depth and a Top N mistake chance from it; an
adventure director might ignore it.

- **Why a struct and not reflection**: the alternative is the menu reading and writing arbitrary director properties by
  reflection -- much more machinery (discovering properties, building UI for them, validating values) for the same result.
- **Why not more presets only**: presets cover a handful of fixed choices well; a slider or several independent options would
  multiply presets.

## Not decided

- Which options beyond difficulty are common enough to be fields.
- Whether presets list which options they honour (so the menu can hide irrelevant ones).
