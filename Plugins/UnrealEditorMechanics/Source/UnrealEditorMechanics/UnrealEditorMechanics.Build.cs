// Editor-only tools. This module is Editor type in the .uplugin, so it is built and loaded only in the editor and never ships.
// Nothing at runtime may depend on it.

using UnrealBuildTool;

public class UnrealEditorMechanics : ModuleRules
{
	public UnrealEditorMechanics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UnrealEd",
			}
			);
	}
}
