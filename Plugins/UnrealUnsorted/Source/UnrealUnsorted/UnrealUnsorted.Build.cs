// Holding area for code migrated from other plugins/projects. Nothing may depend on this module,
// so it is never added to another module's dependency list.

using UnrealBuildTool;

public class UnrealUnsorted : ModuleRules
{
	public UnrealUnsorted(ReadOnlyTargetRules Target) : base(Target)
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
			}
			);
	}
}
