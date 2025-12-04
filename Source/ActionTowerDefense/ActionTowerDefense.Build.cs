// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActionTowerDefense : ModuleRules
{
    public ActionTowerDefense(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Niagara",
                "EnhancedInput",
                "UMG"          // 👈 for UUserWidget / UMG
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",       // 👈 for UI
                "SlateCore"    // 👈 for UI
            }
        );

        // If you later use online stuff, you can add OnlineSubsystem etc. here
    }
}


/* old
using UnrealBuildTool;

public class ActionTowerDefense : ModuleRules
{
	public ActionTowerDefense(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
*/
