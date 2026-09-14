---
Date: 2026-09-11
---
### Crash log with this error

Fatal error: [File:D:\build\++UE5\Sync\Engine\Source\Runtime\Core\Private\GenericPlatform\GenericPlatformMemory.cpp] [Line: 263]

Ran out of memory allocating 13788941 (13.2 MiB) bytes with alignment 1. Last error msg: The paging file is too small for this operation to complete..

### Unreal Editor Standalone cmd showing these warnings and errors

LogSteamShared: Display: Loading Steam SDK 1.57
LogSteamShared: Steam SDK Loaded!
LogOnline: Error: STEAM: Failed to create file: C:/Program Files/Epic Games/UE_5.5/Engine/Binaries/Win64/steam_appid.txt
LogOnline: Warning: STEAM: Could not create/update the steam_appid.txt file! Make sure the directory is writable and there isn't another instance using this file
LogOnline: Warning: STEAM: Could not set up the steam environment! Falling back to another OSS.
LogOnline: Warning: STEAM: Steam API failed to initialize!

### Answer 

That one's straightforward — it's a Windows file-permissions issue, and it's unrelated to the out-of-memory crash from before.

**What's happening:** When the Steam SDK initializes, Unreal tries to write a small `steam_appid.txt` file into `Engine/Binaries/Win64/` so the Steam API knows which app ID to associate with your local test session. Your engine install is sitting inside `C:/Program Files/Epic Games/...`, and Windows locks down write access to `Program Files` for normal (non-elevated) processes via UAC — so the write fails.

**The consequence:** Because Steam can't initialize properly, Unreal's Online Subsystem falls back to a different backend (usually the null OSS). For most local standalone testing, this is harmless — you won't see Steam overlay, friends, achievements, or lobby features, but the game itself will still run fine.

**When it actually matters:** if your project relies on Steamworks features for what you're testing — multiplayer sessions via Steam, achievements, rich presence, etc. — those won't work until this is fixed.

A few ways to fix it, any one of which works:

- **Run the editor/standalone as Administrator** — gives it write access to the Program Files folder for that session.
- **Move your Unreal Engine install out of Program Files** (e.g. to `D:\UE_5.5\` or `C:\UnrealEngine\`) — the cleanest long-term fix, avoids UAC entirely.
- **Manually create the file once with elevated permissions**: create `steam_appid.txt` in that folder yourself (as admin) containing your Steam App ID — or `480` for the public Spacewar test ID — and then grant your user account write permission on that specific file so future updates succeed.
- **Grant your user account write permission** on the `Win64` folder itself via its Properties > Security tab, so you don't need to run elevated every time.

If you're not testing any Steam-specific functionality right now, you can also just ignore it — it won't block your standalone session from running.

[2026.09.11-07.59.11:996][  0]LogTemp: Error: ConnectIt_GameUtilityLibrary: No ConnectIt_LevelConfigDataAsset registered for level 'CIL_OnlineMultiplayer' in ConnectIt_LevelConfigSettings
[2026.09.11-07.59.11:996][  0]LogTemp: Error: ConnectIt_BoardRegistrySubsystem: OnWorldBeginPlay — no ConnectIt_LevelConfigDataAsset found for the current level

### Thoughts

- Points out an obvious bug where the world subsystem and game utility library are looking for the level config for levels it should not. How can we prevent this from happening? We could make an interface IBoardRegistryUser and slap it on the GameMode. Then the subsystem UConnectIt_BoardRegistrySubsystem calls this on the game mode to determine whether it loads or not.
	- Same things should apply to any other subsystem. We can make Interfaces that the GameMode will have etc.