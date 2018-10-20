// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmConvertHumanoid.h"
#include "VrmConvert.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"

#include "Engine/SkeletalMesh.h"
#include "RenderingThread.h"
#include "SkeletalMeshModel.h"
#include "SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"

static void renameToHumanoidBone(USkeleton *targetSkeleton, const UVrmMetaObject *meta) {

	//k->RemoveBonesFromSkeleton()
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	for (auto &a : allbone) {
		auto p = meta->humanoidBoneTable.FindKey(a.Name.ToString());
		if (p == nullptr) {
			continue;
		}
		for (auto &b : allbone) {
			if (a == b) continue;
			if (b.Name == **p) {
				b.Name = *(b.Name.ToString() + TEXT("_renamed_vrm4u"));
			}
		}
		a.Name = **p;
	}

	const_cast<FReferenceSkeleton&>(targetSkeleton->GetReferenceSkeleton()).RebuildRefSkeleton(targetSkeleton, true);
	//targetSkeleton->HandleSkeletonHierarchyChange();
}



namespace VRM {
	bool ConvertHumanoid(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
		const USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
		const USkeleton *k = sk->Skeleton;

		USkeleton *base = DuplicateObject<USkeleton>(k, vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_humanoid_Skeleton")));
		USkeletalMesh *ss = DuplicateObject<USkeletalMesh>(sk, vrmAssetList->Package, *(FString(TEXT("SK_")) + vrmAssetList->BaseFileName + TEXT("_humanoid")));

		renameToHumanoidBone(base, vrmAssetList->VrmMetaObject);

		ss->Skeleton = base;
		ss->RefSkeleton = base->GetReferenceSkeleton();

		ss->CalculateInvRefMatrices();
		ss->CalculateExtendedBounds();
#if WITH_EDITORONLY_DATA
		ss->ConvertLegacyLODScreenSize();
		ss->UpdateGenerateUpToData();
#endif
		base->SetPreviewMesh(ss);
		base->RecreateBoneTree(ss);

		vrmAssetList->HumanoidSkeletalMesh = ss;

		return true;
	}
}

VrmConvertHumanoid::VrmConvertHumanoid()
{
}

VrmConvertHumanoid::~VrmConvertHumanoid()
{
}
