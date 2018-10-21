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
#include "VrmLicenseObject.h"

#include "AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"


namespace VRM{
	bool ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {

		VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		UVrmMetaObject *m;
		m = NewObject<UVrmMetaObject>(vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_Meta")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		vrmAssetList->VrmMetaObject = m;

		UVrmLicenseObject *lic;
		lic = NewObject<UVrmLicenseObject>(vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_License")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		vrmAssetList->VrmLicenseObject = lic;

		if (meta == nullptr) {
			return false;
		}

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

		// license
		{
			struct TT {
				FString key;
				FString &dst;
			};
			const TT table[] = {
				TEXT("version"),		lic->version,
				TEXT("author"),			lic->author,
				TEXT("contactInformation"),	lic->contactInformation,
				TEXT("reference"),		lic->reference,
				// texture skip
				TEXT("title"),			lic->title,
				TEXT("allowedUserName"),	lic->allowedUserName,
				TEXT("violentUssageName"),	lic->violentUssageName,
				TEXT("sexualUssageName"),	lic->sexualUssageName,
				TEXT("commercialUssageName"),	lic->commercialUssageName,
				TEXT("otherPermissionUrl"),		lic->otherPermissionUrl,
				TEXT("licenseName"),			lic->licenseName,
				TEXT("otherLicenseUrl"),		lic->otherLicenseUrl,
			};
			for (int i = 0; i < meta->license.licensePairNum; ++i) {

				auto &p = meta->license.licensePair[i];

				for (auto &t : table) {
					if (t.key == p.Key.C_Str()) {
						t.dst = UTF8_TO_TCHAR(p.Value.C_Str());
					}
				}
				if (FString(TEXT("texture")) == p.Key.C_Str()) {
					
					int t = FCString::Atoi(*FString(p.Value.C_Str()));
					if (t >= 0 && t < vrmAssetList->Textures.Num()) {
						lic->thumbnail = vrmAssetList->Textures[t];
					}
				}

			}
		}



		return true;
	}
}




VrmConvertMetadata::VrmConvertMetadata()
{
}

VrmConvertMetadata::~VrmConvertMetadata()
{
}
