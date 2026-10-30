# Asset Naming Keeper

**Status:** early tool. Reviewed from the binary assets only, not from opening them in the editor, so the logic is inferred from strings and may be wrong in places.

## Summary

A small Unreal Editor Utility Widget that applies naming conventions to assets selected in the Content Browser. It does two jobs:

- **Apply Prefix** adds a recommended prefix based on the asset's class (for example `ABP_`, `AIC_`, `BBD_`, `CAS_`, `CDT_`, `CRA_`, `CVC_`, `DLA_`, `FFE_`, `HLODL_`, `MPC_`, and so on, read from the widget's class-to-prefix list).
- **Apply Suffix** adds a suffix for a chosen asset type. The strings show level types (`_Persistent`, `_Gameplay`, `_Geo`, `_Lighting`, `_Audio`), textures (single-letter suffixes such as `_D`, `_N`, `_R`, `_M`, `_S`, `_A`), curves (`_Float`, `_Color`, `_Vector`, `_Table`), sound (`_Cue`, `_SC`), `_Decal`, `_Context`, and `_Component`. The exact texture letters are worth confirming in the editor.

It also has a **Rename** button and string helpers (split into path, filename and extension parts; starts-with, ends-with and replace), which suggests it can rebuild names rather than only append.

**Files:**
- `Content/AssetNamingKeeper/EditorUtilityWidgets/EUW_AssetNamingKeeper.uasset`: the widget. Its tooltip reads "Automatically gives recommended asset prefixes to selected assets in the Content Browser."
- `Content/AssetNamingKeeper/Enums/E_Suffixes.uasset`: a user-defined enum of suffix choices with a display-name map.

## What looks good

- **One place for the convention.** The suffix list lives in an enum, so the choices are browsable in the editor and a new suffix means adding one entry.
- **Covers the common Unreal types.** Textures, curves, levels, sounds and blueprints are the classes that most often get inconsistent names, so this targets the right problems.
- **Works on the selection.** Using `GetSelectedAssets` means it fits the normal Content Browser workflow with no extra setup.
- **Plain name handling.** Splitting the name into parts and checking start and end before changing it is the right approach for avoiding double prefixes and suffixes, if it does that.

## What looks risky

- **Enum entries are placeholders.** The enumerators are named `NewEnumerator0` through `NewEnumerator23`, and the real labels exist only in the display-name map. Reordering or deleting entries will silently change what the buttons mean, and any tooling that reads the enum by value will break.
- **Possible label mismatch.** The enum display strings look like `E_Decal`, `E_CurveColor` and `E_SoundConcurrency`, while the suffixes the buttons apply are `_Decal`, `_Color` and `_SC`. This could be intentional, or it could be a leftover from an earlier scheme. The `_SC` suffix is described as "named after a SoundClass", but its button is labelled Sound Concurrency. Check these in the editor before trusting either.
- **Duplicate buttons.** There are about 24 near-identical "Apply Suffix" buttons, each with its own event binding. A single button with a dropdown would be easier to maintain and would stop the enum and the buttons drifting apart.
- **Single-letter suffixes.** `_D`, `_N`, `_R`, `_M`, `_S`, `_O`, `_P` and `_E` can be mistaken for part of a name, which makes the "is this already suffixed?" check harder to get right.
- **Texture naming is inconsistent.** The button is "TextureBaseColor" but the suffix is `_D`, and the enum uses "Diffuse". Pick one term and use it throughout.
- **Rename needs care.** Renaming in Unreal leaves redirectors and can break references if other assets point at the old path. Check that the tool calls the Content Browser's rename path, not a raw file move.
- **Lives in the game's Content folder.** The whole `Content/` folder is untracked in git right now. Decide whether this tool should be committed to the project or kept as a separate editor plugin.

## Suggestions

1. **Verify the logic** by running the widget on a small test folder: apply a prefix twice, apply a suffix to an already-suffixed asset, and rename an asset with references. Record the results here.
2. **Make the enum self-describing.** Give each enumerator a real name (for example `Texture_Diffuse`) and keep the display text as metadata, so the values stay stable if the list changes.
3. **Collapse the suffix buttons** into one button that reads the enum, and generate the prefix and suffix tables from one source of truth.
4. **Settle the terminology** (Diffuse vs Base Color, `_SC` vs Sound Concurrency) and write the final list into a convention note in this vault, so the tool and the documentation agree.
5. **Add a dry-run mode** that lists the names it would produce without renaming anything. That catches mistakes before they reach references.
6. **Commit the source of truth** (or a short README) alongside the assets so a future change can be reviewed, since binary `.uasset` diffs are unreadable.

## Open questions

- Is the prefix table meant to be the team's standard, or a starting point?
- Should suffixes be appended to the end of the name, or should the tool also enforce the order `Prefix_Name_Suffix`?
