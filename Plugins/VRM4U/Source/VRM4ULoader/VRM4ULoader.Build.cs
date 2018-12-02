
using UnrealBuildTool;
using System.IO;


public class VRM4ULoader : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

    public VRM4ULoader(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
                "EditorStyle",
                "ApplicationCore",
                "Engine",
				"Json",
				"UnrealEd",
                "Slate",
                "SlateCore",
                "MainFrame",
                "VRM4U"

            });

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", PlatformString, "assimp-vc140-mt.lib"));

            RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyPath, "assimp/bin", PlatformString, "assimp-vc140-mt.dll")));
        }
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            string PlatformString = "android";
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", PlatformString, "libassimp.so"));

            //RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(ThirdPartyPath, "assimp/bin", PlatformString, "assimp-vc140-mt.dll")));
        }
    }
}
