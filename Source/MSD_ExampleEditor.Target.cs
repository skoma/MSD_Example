// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MSD_ExampleEditorTarget : TargetRules
{
	public MSD_ExampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		ExtraModuleNames.Add("MSD_Example");
	}
}
