// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"

/**
 * 
 */

struct aiScene;
class UTexture2D;
class UMaterialInterface;
class USkeletalMesh;
class UVrmAssetListObject;

class VRMConverter {
public:
	static bool IsImportMode();
	static void SetImportMode(bool bImportMode);
	static FString NormalizeFileName(const char *str);
	static FString NormalizeFileName(const FString &str);

	static bool ConvertTextureAndMaterial(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	static bool ConvertModel(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	static bool ConvertMorphTarget(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	static bool ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);
	static bool ConvertHumanoid(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);
};


class VRM4U_API VrmConvert
{
public:
	VrmConvert();
	~VrmConvert();
};
