// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LEETDEMO2 : ModuleRules
{
	public LEETDEMO2(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D", "HTTP", "Json", "JsonUtilities"});

        //PublicAdditionalLibraries.Add("cryptlib.lib");
    }
}
