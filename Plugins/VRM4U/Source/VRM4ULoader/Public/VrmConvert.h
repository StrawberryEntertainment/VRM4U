// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/EngineVersionComparison.h"

/**
 * 
 */

struct aiScene;
class UTexture2D;
class UMaterialInterface;
class USkeletalMesh;
class UVrmAssetListObject;

UENUM(BlueprintType)
enum EVRMImportMaterialType
{
	VRMIMT_Auto			UMETA(DisplayName="Auto"),
	VRMIMT_MToon		UMETA(DisplayName="MToon Material"),
	VRMIMT_MToonUnlit	UMETA(DisplayName="MToon Unlit"),
	VRMIMT_Unlit		UMETA(DisplayName="Unlit"),
	VRMIMT_glTF			UMETA(DisplayName="PBR(glTF2)"),

	VRMIMT_MAX,
};


class VRM4ULOADER_API VRMConverter {
public:
	static bool IsImportMode();
	static void SetImportMode(bool bImportMode);
	static FString NormalizeFileName(const char *str);
	static FString NormalizeFileName(const FString &str);

	static bool NormalizeBoneName(const aiScene *mScenePtr);

	static bool ConvertTextureAndMaterial(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	static bool ConvertModel(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	static bool ConvertMorphTarget(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	static bool ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);
	static bool ConvertHumanoid(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);
	static bool ConvertRig(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr);

	class VRM4ULOADER_API Options {
	public:
		static Options & Get();

		class UVrmImportUI *Window = nullptr;
		void SetVrmOption(class UVrmImportUI *p) {
			Window = p;
		}

		class USkeleton *GetSkeleton();
		bool IsSimpleRootBone() const;

		bool IsSkipPhysics() const;

		bool IsSkipNoMeshBone() const;

		bool IsSkipMorphTarget() const;

		bool IsCreateHumanoidRenamedMesh() const;

		bool IsDebugOneBone() const;

		bool IsMergeMaterial() const;

		bool IsOptimizeMaterial() const;

		EVRMImportMaterialType GetMaterialType() const;
	};
};


class VRM4ULOADER_API VrmConvert
{
public:
	VrmConvert();
	~VrmConvert();
};
