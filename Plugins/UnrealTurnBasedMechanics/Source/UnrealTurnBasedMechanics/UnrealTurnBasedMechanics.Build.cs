// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealTurnBasedMechanics : ModuleRules
{
	public UnrealTurnBasedMechanics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"EnhancedInput",
				"GameplayTags",
				// SlateCore -- public because Action/TurnBasedAction.h (a
				// public header) now carries an FSlateBrush property
				// (display icon, see UIValueCatalogue.md Gap 7).
				"SlateCore",

				// ... add other public dependencies that you statically link with here ...
				"UnrealGameMechanics",
				"UnrealGridMechanics"

			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"UMG",
				"EnhancedInput",
				"AIModule",

				// ... add private dependencies that you statically link with here ...
				"UnrealGameMechanics",
				"UnrealGridMechanics"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
