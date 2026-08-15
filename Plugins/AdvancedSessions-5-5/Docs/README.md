# AdvancedSessions (Third-Party, Vendored)

## What It Is / Source

This is the well-known community "Advanced Sessions" plugin by Joshua Statzer, vendored wholesale under `Plugins/AdvancedSessions-5-5/AdvancedSessions/{AdvancedSessions, AdvancedSteamSessions}`. It uses the old-style UE4 "Classes"/"Private" source folder layout (not the newer Public/Private split), standard Epic-style boilerplate, and carries `CreatedBy: "Joshua Statzer"` in both `.uplugin` files — there is no first-party attribution anywhere in this plugin, confirming it is not authored by the team maintaining this plugin suite.

## Why It's Vendored Here

It provides Blueprint-accessible async ("callback proxy") wrappers around Unreal's OnlineSubsystem/OnlineSubsystemSteam APIs for session create/find/join/start/end, friends, identity/login, and Steam-specific features — functionality the consuming project will need for online matchmaking/session management, but that isn't worth reimplementing from scratch.

## Module Structure

Two modules:

- **`AdvancedSessions`** (Runtime, LoadingPhase `PreDefault`) — the base session/friends/identity module. Mostly `U*CallbackProxy` classes: `CreateSessionCallbackProxyAdvanced`, `FindSessionsCallbackProxyAdvanced`, `StartSessionCallbackProxyAdvanced`, `UpdateSessionCallbackProxyAdvanced`, `EndSessionCallbackProxy`, `LoginUserCallbackProxy`, `LogoutUserCallbackProxy`, `AutoLoginUserCallbackProxy`, `GetUserPrivilegeCallbackProxy`, `GetFriendsCallbackProxy`, `GetRecentPlayersCallbackProxy`, `SendFriendInviteCallbackProxy`, `FindFriendSessionCallbackProxy`, `CancelFindSessionsCallbackProxy` — plus `AdvancedSessionsLibrary`, `AdvancedFriendsLibrary`/`AdvancedFriendsInterface`/`AdvancedFriendsGameInstance`, `AdvancedIdentityLibrary`, `AdvancedExternalUILibrary`, `AdvancedVoiceLibrary`, and `AdvancedGameSession`. Declares plugin dependencies on `OnlineSubsystem` and `OnlineSubsystemUtils`.
- **`AdvancedSteamSessions`** (Runtime, LoadingPhase `PostDefault`) — Steam-specific extensions (workshop, groups, notifications): `AdvancedSteamFriendsLibrary`, `AdvancedSteamWorkshopLibrary`, `SteamNotificationsSubsystem`, `SteamRequestGroupOfficersCallbackProxy`, `SteamWSRequestUGCDetailsCallbackProxy`. Declares a plugin dependency on `AdvancedSessions` itself plus `OnlineSubsystem`, `OnlineSubsystemSteam`, `OnlineSubsystemUtils`, and `SteamShared`.

Both modules follow the standard Blueprint async-node ("latent proxy") pattern: a static factory function creates a proxy object, the proxy performs the async OnlineSubsystem call, and Blueprint-exposed delegates fire on success/failure.

## Current Integration Status

**Not yet integrated.** A repo-wide search of the consuming game module's source for `OnlineSubsystem`, `AdvancedSession`, and `SessionInterface` returns no matches, and that module's build file has its `OnlineSubsystem` dependency commented out:

```csharp
// Uncomment if you are using online features
// PrivateDependencyModuleNames.Add("OnlineSubsystem");
```

The consuming module's build file does not declare a dependency on this plugin at all. It has been vendored into the repo in preparation for future online session/matchmaking work, but that integration has not started yet.

## Do Not Modify (Vendored Code)

This is unmodified third-party code. If a bug or missing feature is found, patch it via a fork/subclass in the consuming project's own code rather than editing files under `Plugins/AdvancedSessions-5-5/` directly, so future upstream updates aren't blocked by local changes.

Both `.uplugin` files (`AdvancedSessions.uplugin` and `AdvancedSteamSessions.uplugin`) have empty `DocsURL` and `MarketplaceURL` fields (`AdvancedSteamSessions.uplugin` doesn't even declare those fields) — this repo's copy carries no filled-in documentation link, so there is nothing further to link to here. It should be treated as a marketplace/community plugin whose canonical documentation lives outside this repository.
