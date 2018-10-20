// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

struct aiScene;
class UTexture2D;
class UMaterialInterface;
class USkeletalMesh;
class UVrmAssetListObject;

namespace VRM {
	bool ConvertTextureAndMaterial(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	bool ConvertModel(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	bool ConvertMorphTarget(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);
}


class VRM4U_API VrmConvert
{
public:
	VrmConvert();
	~VrmConvert();
};
