---
date: 2026-09-11
source: "Actions setup 1.m4a (pasted transcript)"
topics:
  - swap-action-ui
  - actions-mediator
  - hud-playercontroller-interface-pattern
  - swap-action-design-variants
  - actions-as-items-vision
tags:
  - discussion
---

# 2026-09-11 — SWAP Action & Actions UI Design

## Summary

Solo design pass on the SWAP action: an Actions UI (HUD-owned widget) that shows a
player's available actions and highlights the active one, fed by a GameMode-side
"actions mediator" that pushes server-authoritative availability down to PlayerState, with
an interface+delegate pattern keeping the PlayerController decoupled from the widget
itself. Also sketches two competing designs for how SWAP interacts with the mandatory
PlacePiece action, and a longer-term vision of actions as configurable, Smash-Bros-style
"items" per level/game mode.

## Topics

- **Swap action UI** — first time this came up
- **Actions mediator (GameMode-driven availability)** — first time this came up
- **HUD/PlayerController interface pattern** — first time this came up
- **Swap action design variants (bonus vs. alternate-mandatory)** — first time this came up
- **Actions-as-items vision (Smash Bros-inspired ruleset)** — first time this came up

## Tasks

- [ ] Build an Actions UI widget (image + embedded button per action) that lists the
      player's available actions and highlights the currently active one
- [ ] Have the HUD instantiate and own the Actions UI widget, exposing it via a const
      getter
- [ ] Add a read-only available-actions field to PlayerState (tags, or a
      `TSubclassOf<TurnBasedAction>` — undecided, see Questions)
- [x] Define an interface (e.g. `ITurnBasedActionNotifier`) for the HUD to implement,
      taking a delegate the PlayerController binds to
- [ ] Wire that delegate through to the Actions Component to set/stack the new root
      action
- [ ] Add GameMode-side logic (extending or renaming the Board Request Mediator) that
      recomputes action availability whenever a board-state request changes it, and
      pushes the result to PlayerState
- [ ] Prototype both SWAP variants (bonus action stacked on top of PlacePiece; alternate
      mandatory action) to compare balance

## Deliverables

- Actions UI widget (HUD-owned)
- PlayerState available-actions field
- HUD interface + delegate contract for action changes
- GameMode-side actions mediator
- Two SWAP-action game-mode prototypes, for balance comparison

## Questions

- Represent available actions as gameplay tags, or `TSubclassOf<TurnBasedAction>`? ("Tags
  might work. I don't know.")
- How does the Actions UI reliably resolve "this player controller's own PlayerState"
  specifically? Assumed trivial but not worked out.
- Is SWAP a bonus action stacked on top of the mandatory PlacePiece action, or an
  alternate mandatory action (only one of the two required to end a turn)? Explicitly
  undecided — wants to prototype both.
- What's the actual name for the GameMode-side mediator — Board Request Mediator, Game
  State Mediator, or a new, separate Actions Mediator? Flip-flopped during the discussion,
  not settled.
- Should the UI show *all* possible actions for the level, not just the currently
  available ones? Deferred as possibly overcomplicated for now.
- Long-term: how are actions earned — always available with a limited number of uses, or
  earned by connecting a special, randomly-spawned "power tile" (Scrabble-tile-style)? Not
  decided.

## Workflows

- **Proposed data flow / ownership chain:** a GameMode-side actions mediator computes
  action availability from board-state changes (server-authoritative) → pushes it to
  PlayerState (read-only on clients) → the HUD reads PlayerState to build/update the
  Actions UI → the PlayerController never touches the UI directly, only listens to the HUD
  through a delegate-based interface → forwards the selected action to the Actions
  Component to set or stack it.
- **Long-term vision:** treat actions like configurable "items," Super Smash Bros-style —
  a level or game mode's ruleset decides which actions are enabled at all, and how they're
  acquired (always-on with limited uses vs. earned by connecting a special, randomly
  spawned tile). Meant to support both puzzle/adventure-style single-player levels and a
  customizable-ruleset online multiplayer mode.

## Transcript

Discussion for SWAP action: connect it. Um, allright, so just from the top: we'll need a new UI that shows all of the available actions to the player, highlighting the current available action. When we click on one of the action buttons, it'll be like an image and then a button embedded on it. That should swap the action that's currently active for that player. So we'll need to have the player controller listening to the owner of this widget, I think the HUD. I mean, because it's— I mean, because it's really relevant to itself, it would make sense to keep it as a player controller. To have the player controller own the, like, available actions widget, just so it can, like, cleanly read from it. At the same time, it's probably a good idea to have the— to have, I mean, to make it clean, you have the HUD own the widget. And then I guess the player controller can grab the widget and listen. Yeah, and listen for relevance changes. That's probably ideal, actually. HUD instantiates the widget, makes it available to get as a, like, a const getter. Ugh. Realistically. Or I don't know. Hmm. We'll have to navigate to see what the best source for that is. But, okay, so then the widget— what— how does the widget know what actions the player has and at the same time what actions the, the, the are available? Oh, I missed my turn. Shit. Um,right, like, so I'm thinking player state will hold a read-only array of either tags, or we could keep it within the turn action-based realm and so it reads a T subclass of turn action-based. And so when the widget is turned on, when the actions display widget is turned on, it reads the player state, grabs the available actions, makes sure that they're set up appropriately, that each one has a thing, whether it's available to pick or not, yada yada. So then what controls the player state's read-only thing? So the player state's going to have to listen to something. Not board state, but, uh, basically when a request goes through and the board state gets changed. In the calculations, we will have to calculate if an action gets updated, or if the player is granted a new type of action, or if they lose access to a certain type of action. Um, I guess in general we'll probably want to— I feel like you'll want to display all the possible actions for that level in some way. Maybe that's overly complicated for now. Let's perhaps just keep it to the calculations. Oh God. Uh, okay, I can't go that way. Oh, I can. Fuck. The calculations just read from— walk the street, the street, I can't think of it now. The calculation after a request goes through and the board state changes, and hypothetically if a player is granted or they lose access to an action, some sort of calculation goes on. And then basically player states— hmm, hmm— need to listen. Like, where is this going to get stored? Like, if I want to set it on the player state, I guess I can make a function that's server-only on player state that allows the server to set the player state, but then not the other way around. Or I could have the player state listening to, I think, maybe the board state mediator. Ah, the game state? No, we don't want to start on the game state. We don't want to— no, no, no. Yeah, like the board state mediator, but it's really like the board request mediator. Yeah, it's listening to that for an update to a player's action set. Um, yeah, yeah, yeah. Okay, so it listens to that. Then the UI reads from the player state to know what player— what actions are available, sets them up. When the button is— when a UI element is pressed, the player controller is listening to that UI. It could also just listen to the HUD and not know anything about the UI. That would probably be better. And so what we can do is create an interface for the HUD to implement. Um, like an I action. Like a I turn— turn-based action notifier. Something like that. And then the player— so then generically the player controller can just re-check if it has that interface. Like, the interface should take in a delegate. That's what it should do. Just that kind of classic pattern that we rediscovered recently. So it should take in the— it should take in a delegate. And that delegate will be a player controller. It won't even be the player controller, actually. It'll probably be the actions. Something in the actions component. Yeah, like the actions component, like set new action. Oh, set new action base, I guess. Like set new root action. Uh, root action. It's really going to depend on how we want to do this. Just as a side design note, we should experiment with two types of game modes just with this new swap action. We should have the swap action two versions. One will be place piece is the root action that the player needs to complete. Well, no, no, no. I guess it won't— it wouldn't matter. Either. Uh, yeah, because the first way is that the swap action can act as another mandatory type of action. So the way that I was thinking of it originally is that the place piece action is the mandatory action that needs to happen in order for the turn to be over. The swap action would be sort of like a bonus action on your turn, where you can swap— you could use it to swap two pieces, and then the player still needs to place a piece in order to finish their turn. The alternate version, which could be kind of interesting to balance because that might be a little too powerful the first way— oh my God, what's going on? Look at these fucking big heads. Uh, the first possib— the second possibility is that swap action doubles as another type of mandatory action. So you only need to complete either place piece or swap piece. Like, you basically only get one main action to use. And so it could be to place a piece, or it could be to swap pieces. Um, yeah, just as a design note. So just a quick summary: there will be some sort of actions UI that displays the player's available actions. It reads the available actions from the player state. Now, I'm not sure if we're going to need to identify or— what, what the fuck is going onright now? Identify any sort of, uh— what the fuck? Oh, I lost my train of thought because this motherfucker's driving like an insane person. What the— people are fucked. Um, okay, sorry. From the top. There is an actions UI that displays the player's available actions, highlighting the current selected action, and kind of maybe blurring out actions that are unavailable at this time. It reads the player's actions, available actions, from the player state. We might need to, uh, figure out some way to, like, determine, you know, what player state to read. It should ideally just be that player controller's player state. So we'll have to figure out how that happens. But, um, it has the things. It reads it from the player state. When the player— when some— when the button is— the player controller will then listen to this UI to determine when to set an action. So in the original setup, the root action would be place piece. If the player were to select swap, that would stack on top. So they would complete the swap action, it would become unavailable to use again that turn, and then they would— the player would still need to finish the place piece action to then finish their turn. Alternatively, what we could do is have both of these actions act as, um, mand— they wouldn't both be mandatory. They would— well, they would act as a, I guess, mandatory action, but you would only need to complete one of them. So I'm not sure how that would change any logic. It would change logic. I'm just not sure at this time how that would happen. Um, allright, so ideally the player won't know about this UI. It'll simply know about the HUD. And it'll, uh, call an interface that the HUD implements that takes in a delegate. And, um, executes the delegate basically when there's been an update to— oh my God, these assholes. Update the delegate when, uh, when there's been a change,right? So it basically will just, like, the interface takes in a delegate, and the delegate takes in a T subclass of turn-based action. Um, yeah, so essentially— yeah, and then we'll have to figure out a mapping. Like, it'll have to be maybe— like, we could try to use tags maybe. Eh, I don't know. Either way, then that way the player knows to tell the actions component, "Hey, set this action," and then again determining if— ah, geez. Okay. We'll have to do some— basically some logic. Tags might work. I don't know. We do subclass, but then there's going to need to be some custom logic, basically. It says, "If the subclass is this class," you know, stack the action. If it's this class, set it as the new root action or whatever,right? Um, cool. The UI, yeah, reads from the player state what actions are available. The player state gets— the player state knows what actions are available for the player. Oop, going the wrong way here. Did I just go way too north? I think I'm at a— yeah, I went way too north. I'm on 12. Fuck. Um, the action knows— oh, I could have just went down kingsway. Fuck me. What am I doing? The action knows— yeah, we have to— the player state will probably be told— we'll have to listen to, like, the, the, the mediator. I don't know what we want to call that. The game state mediator. Right now it's the board state mediator, but really it's the game state mediator. Or it can just listen to the game mode so that it knows what, uh— yeah, so that it just knows what's what. Anyways, okay, that's good for now.

Yeah, so again, we're going to— we want to set up the— so the board state mediator is on the game mode. So we're going to want to have some functionality on the game mode that dictates to player states what actions are available, what actions are not available. Right? This way, when the player state is listening to the game mode,right, and the game mode can't be tampered with,right, so it's—right? Same thing with the board state: it's listening to the game mode. Well, specifically the mediator, which is on the game mode, which only exists on the server, so again, it can't be tampered with. And then everything on the state is just read-only, so nothing can get set, nothing can get tampered with by the client. Right, so this will be ideal because we'll probably want some way to control what actions are even available in the current level,right? If we're doing, like, adventure mode, there might be scenarios where we only want the players to have access to certain actions. Like, you know, if they're learning about a new action. Or maybe we want to have it set up so that, you know, they have a, like, certain number of uses of an action to beat the level,right? Because we want to kind of create puzzle-designy type scenarios. At the same time, we also— big picture— we want to support a multiplayer, online multiplayer environment that is inspired by, like, Super Smash Bros, where you can go in and you can, you know, set, "This is the level," and, you know, "These are all the different kind of rules." It's not just Super Smash Bros, but I'm going to use it as the model. Especially when it comes to, like, the items,right? I want to think of the actions as, like, the items that are available in this combat when you're playing Super Smash. You can either turn them all off so that it's just straight-up, like, classic connect it, where it's just very logical, very, like, strategic. Or you can only include certain other actions,right? Like swap or board shift. And then you can control if these actions, you know, how they're earned. Whether it's you always have them, or whether— again, we have to flesh this out a little bit— whether it's, like, you get them by creating a connection that involves a certain tile that's, like, randomly spawned. Like, you know what I mean? Kind of like a Scrabble type thing, where there'll be certain tiles will be like, "Oh, this is a swap it tile," and if you create a connection on it, you, like, gain the swap action, but then the tile goes back to normal type situation. And/or, like, yeah, it can be something where it's like, you always have it, but it has a certain number of uses,right? So that's kind of the big picture, and I think we should make sure we adhere the whole system to sort of fit that narrative. So that's going to include some sort of actions— let's call it an actions mediator— on the game mode. Player states listen to the action mediator, which tells them— did I go too far again? No, once? Okay. That's so weird. It tells them, you know, what's available. It doesn't need to explain why. It just— and then,right, because it's on the server, it's telling the player state, which will then be on the client and read-only, so that the clients can update accordingly. The HUD can then make sure that the UI is displaying theright things. And then the player controller is listening to the HUD to know when, "Hey, a new action has been selected. Change the current action in the actions component." Whether that's a stack, whether that's a, you know, a pop and then a stack, or a set, you know, new route type situation. Kind of just basic state machine stuff,right? Cool.
