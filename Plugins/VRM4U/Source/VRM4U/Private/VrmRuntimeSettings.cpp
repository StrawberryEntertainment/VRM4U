// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmRuntimeSettings.h"


UVrmRuntimeSettings::UVrmRuntimeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDropVRMFileEnable = false;
	AssetListObject.SetPath(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
}