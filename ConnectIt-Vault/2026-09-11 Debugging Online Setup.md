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

[2026.09.11-07.59.11:996][  0]LogTemp: Error: ConnectIt_GameUtilityLibrary: No ConnectIt_LevelConfigDataAsset registered for level 'CIL_OnlineMultiplayer' in ConnectIt_LevelConfigSettings
[2026.09.11-07.59.11:996][  0]LogTemp: Error: ConnectIt_BoardRegistrySubsystem: OnWorldBeginPlay — no ConnectIt_LevelConfigDataAsset found for the current level

### Thoughts

- Points out an obvious bug where the world subsystem and game utility library are looking for the level config for levels it should not. How can we prevent this from happening? We could make an interface IBoardRegistryUser and slap it on the GameMode. Then the subsystem UConnectIt_BoardRegistrySubsystem calls this on the game mode to determine whether it loads or not.
	- Same things should apply to any other subsystem. We can make Interfaces that the GameMode will have etc.