// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmConvertMetadata.h"
#include "VrmConvert.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"

#include "AssetRegistryModule.h"
#include "UObject/Package.h"


namespace VRM{
	bool ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {

		VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		UVrmMetaObject *m;

		m = NewObject<UVrmMetaObject>(vrmAssetList->Package, TEXT("VrmMeta"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);


		// bone
		for (auto &a : meta->humanoidBone) {
			m->humanoidBoneTable.Add(a.humanBoneName.C_Str()) = a.nodeName.C_Str();
		}

		//shape
		m->BlendShapeGroup.SetNum(meta->blensShapeGroupNum);
		for (int i = 0; i < meta->blensShapeGroupNum; ++i) {
			auto &aiGroup = meta->blensShapeGourp[i];

			m->BlendShapeGroup[i].name = aiGroup.groupName.C_Str();

			m->BlendShapeGroup[i].BlendShape.SetNum(aiGroup.bindNum);
			for (int b = 0; b < aiGroup.bindNum; ++b) {
				auto &bind = m->BlendShapeGroup[i].BlendShape[b];
				bind.morphTargetName = aiGroup.bind[b].blendShapeName.C_Str();
				bind.meshName = aiGroup.bind[b].meshName.C_Str();
				bind.nodeName = aiGroup.bind[b].nodeName.C_Str();
				bind.weight = aiGroup.bind[b].weight;
				bind.meshID = aiGroup.bind[b].meshID;
				bind.shapeIndex = aiGroup.bind[b].shapeIndex;
			}
		}

		vrmAssetList->VrmMetaObject = m;
		return true;
	}
}




VrmConvertMetadata::VrmConvertMetadata()
{
}

VrmConvertMetadata::~VrmConvertMetadata()
{
}
