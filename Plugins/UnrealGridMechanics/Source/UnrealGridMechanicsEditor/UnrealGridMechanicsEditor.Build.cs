using UnrealBuildTool;

public class UnrealGridMechanicsEditor : ModuleRules
{
    public UnrealGridMechanicsEditor(ReadOnlyTargetRules Target) : base(Target)
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
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "DataValidation",
                
                // Runtime module
                "UnrealGridMechanics"
            }
        );
    }
}