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

		// Editor-only modules: linking them unconditionally breaks packaged
		// game builds. Code that uses them stays inside #if WITH_EDITOR.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"UMGEditor",
				"Blutility",
				"EngineSettings",
			});
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
