# <domain or vault> tasks — active

| Task | Created | Target | Status | Notes |
|---|---|---|---|---|
| <what needs doing> | <YYYY-MM-DD> | <YYYY-MM-DD or —> | active | <context, blockers, [[links]]> |

<!--
Status: active | suspended | complete | dropped
To retire a task: change its Status cell here, then run /reconcile-tasks — the row is
moved to suspended.md / complete.md / dropped.md and stamped with a Moved date.
Archive files use the same table plus a trailing `Moved` column.
-->

## Overview

Determine the best way to create this skill using the projects current setup.

#### Prerequisites

- UI update - we will need to display an action UI which highlights the current action (place piece) and then allows the player to change actions by selecting the relevant UI button.
- Find some UI icons and textures from FAB

#### Steps

- See if we can genericize the friends list widget from Online testing. It won't be perfect as it uses a scroll box but should do the trick for testing.
- If not just create ActionsElements and ActionsList widgets
	- figure out best way to have UI selection swap an action on the PC->ActionsComponent
- Create a new UTurnBasedAction for Swap Pieces.
	- Should we consider creating a ConnectIt default subclass?
	- Create boardstate logic to correctly calculate the swap
- Have the HUD own this widget. We will want to make sure that it is always visible unless some hierarchical widget like Pause UI or Options UI, etc. are shown. In which case we will need to decide how those block (whether they force action UI to collapse, or if they cover will a canvas panel and absorb all mouse input).



