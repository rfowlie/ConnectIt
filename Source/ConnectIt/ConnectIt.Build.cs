// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ConnectIt : ModuleRules
{
	public ConnectIt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"UMGEditor",
			"Blutility",
			"UnrealEd",
			"DeveloperSettings",
			"GameplayTags",
			"GameplayTasks",
			"GameplayAbilities",
			"AIModule",
			// add ons
			"UnrealGameMechanics",
			"UnrealGridMechanics",
			"UnrealGameIntelligence",
			"UnrealTurnBasedMechanics"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"AIModule",
			// add ons
			"UnrealCodingUtils",
			"UnrealGameMechanics",
			"UnrealGridMechanics",
			"UnrealGameIntelligence", 
			"UnrealTurnBasedMechanics", 
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
