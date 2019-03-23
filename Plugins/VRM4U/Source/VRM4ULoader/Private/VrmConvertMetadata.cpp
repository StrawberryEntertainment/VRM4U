// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

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


bool VRMConverter::ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {

	VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

	UVrmMetaObject *m;
	if (vrmAssetList->Package == GetTransientPackage()) {
		m = NewObject<UVrmMetaObject>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient, NULL);
	}else {
		m = NewObject<UVrmMetaObject>(vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_Meta")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	}
	vrmAssetList->VrmMetaObject = m;

	UVrmLicenseObject *lic;
	if (vrmAssetList->Package == GetTransientPackage()) {
		lic = NewObject<UVrmLicenseObject>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient, NULL);
	}else {
		lic = NewObject<UVrmLicenseObject>(vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_License")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	}
	vrmAssetList->VrmLicenseObject = lic;

	if (meta == nullptr) {
		return false;
	}

	// bone
	for (auto &a : meta->humanoidBone) {
		m->humanoidBoneTable.Add(UTF8_TO_TCHAR(a.humanBoneName.C_Str())) = UTF8_TO_TCHAR(a.nodeName.C_Str());
	}

	//shape
	m->BlendShapeGroup.SetNum(meta->blensShapeGroupNum);
	for (int i = 0; i < meta->blensShapeGroupNum; ++i) {
		auto &aiGroup = meta->blensShapeGourp[i];

		m->BlendShapeGroup[i].name = UTF8_TO_TCHAR(aiGroup.groupName.C_Str());

		m->BlendShapeGroup[i].BlendShape.SetNum(aiGroup.bindNum);
		for (int b = 0; b < aiGroup.bindNum; ++b) {
			auto &bind = m->BlendShapeGroup[i].BlendShape[b];
			bind.morphTargetName = UTF8_TO_TCHAR(aiGroup.bind[b].blendShapeName.C_Str());
			bind.meshName = UTF8_TO_TCHAR(aiGroup.bind[b].meshName.C_Str());
			bind.nodeName = UTF8_TO_TCHAR(aiGroup.bind[b].nodeName.C_Str());
			bind.weight = aiGroup.bind[b].weight;
			bind.meshID = aiGroup.bind[b].meshID;
			bind.shapeIndex = aiGroup.bind[b].shapeIndex;
		}
	}

	// spring
	m->VRMSprintMeta.SetNum(meta->sprintNum);
	for (int i = 0; i < meta->sprintNum; ++i) {
		const auto &vrms = meta->springs[i];

		auto &s = m->VRMSprintMeta[i];
		s.stiffiness = vrms.stiffiness;
		s.gravityPower = vrms.gravityPower;
		s.gravityDir.Set(vrms.gravityDir[0], vrms.gravityDir[1], vrms.gravityDir[2]);
		s.dragForce = vrms.dragForce;
		s.hitRadius = vrms.hitRadius;

		s.bones.SetNum(vrms.boneNum);
		s.boneNames.SetNum(vrms.boneNum);
		for (int b = 0; b < vrms.boneNum; ++b) {
			s.bones[b] = vrms.bones[b];
			s.boneNames[b] = UTF8_TO_TCHAR(vrms.bones_name[b].C_Str());
		}

		
		s.ColliderIndexArray.SetNum(vrms.colliderGourpNum);
		for (int c = 0; c < vrms.colliderGourpNum; ++c) {
			s.ColliderIndexArray[c] = vrms.colliderGroups[c];
		}
	}

	//collider
	m->VRMColliderMeta.SetNum(meta->colliderGroupNum);
	for (int i = 0; i < meta->colliderGroupNum; ++i) {
		const auto &vrmc = meta->colliderGroups[i];

		auto &c = m->VRMColliderMeta[i];
		c.bone = vrmc.node;
		c.boneName = UTF8_TO_TCHAR(vrmc.node_name.C_Str());

		c.collider.SetNum(vrmc.colliderNum);
		for (int b = 0; b < vrmc.colliderNum; ++b) {
			c.collider[b].offset = FVector(vrmc.colliders[b].offset[0], vrmc.colliders[b].offset[1], vrmc.colliders[b].offset[2]);
			c.collider[b].radius = vrmc.colliders[b].radius;
		}
	}

	// license
	{
		struct TT {
			FString key;
			FString &dst;
		};
		const TT table[] = {
			{TEXT("version"),		lic->version},
			{TEXT("author"),			lic->author},
			{TEXT("contactInformation"),	lic->contactInformation},
			{TEXT("reference"),		lic->reference},
				// texture skip
			{TEXT("title"),			lic->title},
			{TEXT("allowedUserName"),	lic->allowedUserName},
			{TEXT("violentUssageName"),	lic->violentUssageName},
			{TEXT("sexualUssageName"),	lic->sexualUssageName},
			{TEXT("commercialUssageName"),	lic->commercialUssageName},
			{TEXT("otherPermissionUrl"),		lic->otherPermissionUrl},
			{TEXT("licenseName"),			lic->licenseName},
			{TEXT("otherLicenseUrl"),		lic->otherLicenseUrl},
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



VrmConvertMetadata::VrmConvertMetadata()
{
}

VrmConvertMetadata::~VrmConvertMetadata()
{
}
