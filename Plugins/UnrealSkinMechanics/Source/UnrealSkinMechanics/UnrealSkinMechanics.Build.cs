// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealSkinMechanics : ModuleRules
{
	public UnrealSkinMechanics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"DeveloperSettings",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UMG",
			}
			);
	}
}
