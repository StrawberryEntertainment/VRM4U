// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmAssetListObject.h"
#include "LoaderBPFunctionLibrary.h"



UVrmAssetListObject::UVrmAssetListObject(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	Package = GetTransientPackage();

	Result = new FReturnedData();
}
