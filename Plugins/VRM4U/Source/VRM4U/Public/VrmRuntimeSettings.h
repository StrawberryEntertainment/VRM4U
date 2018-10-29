// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
//#include "VrmAssetListObject.h"
#include "VrmRuntimeSettings.generated.h"

/**
 * 
 */
UCLASS(config=Engine, defaultconfig)
class VRM4U_API UVrmRuntimeSettings : public UObject
{
	
	GENERATED_UCLASS_BODY()

	// Enables experimental *incomplete and unsupported* texture atlas groups that sprites can be assigned to
	//UPROPERTY(EditAnywhere, config, Category=Settings)
	//bool bEnableSpriteAtlasGroups;

	//UPROPERTY(config, EditAnywhere, Category = Settings, meta = (AllowedClasses = "VrmAssetListObject", ExactClass = false))
	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (AllowedClasses = "Object", ExactClass = false))
	FSoftObjectPath AssetListObject;


};
