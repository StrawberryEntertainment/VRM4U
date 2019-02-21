// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmConvert.h"


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

#if WITH_EDITOR

#include "VrmOptionWindow.h"

#endif


static bool bImportMode = false;
bool VRMConverter::IsImportMode() {
	return bImportMode;
}
void VRMConverter::SetImportMode(bool b) {
	bImportMode = b;
}


FString VRMConverter::NormalizeFileName(const char *str) {
	FString ret = UTF8_TO_TCHAR(str);

	return NormalizeFileName(ret);
}

FString VRMConverter::NormalizeFileName(const FString &str) {
	FString ret = str;
	FText error;

	if (!FName::IsValidXName(*ret, INVALID_OBJECTNAME_CHARACTERS INVALID_LONGPACKAGE_CHARACTERS, &error)) {
		FString s = INVALID_OBJECTNAME_CHARACTERS;
		s += INVALID_LONGPACKAGE_CHARACTERS;

		auto a = s.GetCharArray();
		for (int i = 0; i < a.Num(); ++i) {
			FString tmp;
			tmp.AppendChar(a[i]);
			ret = ret.Replace(tmp.GetCharArray().GetData(), TEXT("_"));
			//int ind;
			//if (ret.FindChar(s[i], ind)) {
			//}
		}
	}
	return ret;
}

static bool hasInvalidBoneName(const aiString &s) {
	for (int i = 0; i < s.length; ++i) {
		if (s.data[i] >= 0 && s.data[i] <= 0x7e) {
			continue;
		}
		return true;
	}
	return false;
}

static int totalCount = 0;

static bool AddReplaceList(const aiNode *node, TMap<FString, FString> &map) {

	if (node == nullptr) {
		return true;
	}

	if (hasInvalidBoneName(node->mName)) {
		FString s = TEXT("replace_") + FString::FromInt(totalCount);
		if (map.Find(UTF8_TO_TCHAR(node->mName.C_Str())) == nullptr) {
			map.Add(UTF8_TO_TCHAR(node->mName.C_Str()), s);
			totalCount++;
		}
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		AddReplaceList(node->mChildren[i], map);
	}
	return true;
}

static bool ReplaceNodeName(aiNode *node, const TMap<FString, FString> &map) {

	if (node == nullptr) {
		return true;
	}

	auto a = map.Find(UTF8_TO_TCHAR(node->mName.C_Str()));
	if (a) {
		node->mName.Set(TCHAR_TO_ANSI(**a));
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		ReplaceNodeName(node->mChildren[i], map);
	}
	return true;
}

bool VRMConverter::NormalizeBoneName(const aiScene *mScenePtr) {

	return true;
	//auto p = const_cast<aiScene*>(mScenePtr);
	//if (p == nullptr) return false;

	TMap<FString, FString> replaceTable;

	totalCount = 0;
	//mScenePtr->mMeshes[0]->bon

	AddReplaceList(mScenePtr->mRootNode, replaceTable);

	for (uint32_t m = 0; m < mScenePtr->mNumMeshes; ++m) {
		if (hasInvalidBoneName(mScenePtr->mMeshes[m]->mName)) {
			FString s = TEXT("replace_") + FString::FromInt(totalCount);
			if (replaceTable.Find(UTF8_TO_TCHAR(mScenePtr->mMeshes[m]->mName.C_Str())) == nullptr) {
				replaceTable.Add(UTF8_TO_TCHAR(mScenePtr->mMeshes[m]->mName.C_Str()), s);
				totalCount++;
			}
		}
	}

	// replace!

	ReplaceNodeName(mScenePtr->mRootNode, replaceTable);

	for (uint32_t m = 0; m < mScenePtr->mNumMeshes; ++m) {
		if (auto a = replaceTable.Find(UTF8_TO_TCHAR(mScenePtr->mMeshes[m]->mName.C_Str()))){
			mScenePtr->mMeshes[m]->mName.Set(TCHAR_TO_ANSI(**a));
		}

		auto &aiM = mScenePtr->mMeshes[m];
		for (uint32_t b = 0; b < aiM->mNumBones; ++b) {
			if (auto a = replaceTable.Find(UTF8_TO_TCHAR(aiM->mBones[b]->mName.C_Str()))) {
				aiM->mBones[b]->mName.Set(TCHAR_TO_ANSI(**a));
			}
		}
	}

	{
		VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		for (int s = 0; s < meta->sprintNum; ++s) {
			auto &a = meta->springs[s];
			if (a.bones_name == nullptr) {
				continue;
			}
			for (int b = 0; b < a.boneNum; ++b) {
				if (auto str = replaceTable.Find(UTF8_TO_TCHAR(a.bones_name[b].C_Str()))) {
					a.bones_name[b].Set(TCHAR_TO_ANSI(**str));
				}
			}
		}

		for (int c = 0; c < meta->colliderGroupNum; ++c) {
			auto &a = meta->colliderGroups[c];
			if (auto str = replaceTable.Find(UTF8_TO_TCHAR(a.node_name.C_Str()))) {
				a.node_name.Set(TCHAR_TO_ANSI(**str));
			}
		}

		for (auto &a : meta->humanoidBone) {
			if (auto str = replaceTable.Find(UTF8_TO_TCHAR(a.nodeName.C_Str()))) {
				a.nodeName.Set(TCHAR_TO_ANSI(**str));
			}
		}
	}


	return true;
}


////

VRMConverter::Options& VRMConverter::Options::Get(){
	static VRMConverter::Options o;
	return o;
}

USkeleton *VRMConverter::Options::GetSkeleton() {
#if WITH_EDITOR
	if (Window == nullptr) return nullptr;

	return Window->Skeleton;
#else
	return nullptr;
#endif
}

bool VRMConverter::Options::IsSkipNoMeshBone() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bSkipNoMeshBone;
#else
	return false;
#endif
}

bool VRMConverter::Options::IsSkipMorphTarget() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bSkipMorphTarget;
#else
	return false;
#endif
}

bool VRMConverter::Options::IsCreateHumanoidRenamedMesh() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bCreateHumanoidRenamedMesh;
#else
	return false;
#endif
}

bool VRMConverter::Options::IsDebugOneBone() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bDebugOneBone;
#else
	return false;
#endif
}

bool VRMConverter::Options::IsSimpleRootBone() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bSimpleRoot;
#else
	return true;
#endif
}

bool VRMConverter::Options::IsSkipPhysics() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bSkipPhysics;
#else
	return true;
#endif
}


bool VRMConverter::Options::IsMergeMaterial() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bMergeMaterial;
#else
	return false;
#endif
}

bool VRMConverter::Options::IsOptimizeMaterial() const {
#if WITH_EDITOR
	if (Window == nullptr) return false;

	return Window->bOptimizeMaterial;
#else
	return false;
#endif
}


EVRMImportMaterialType VRMConverter::Options::GetMaterialType() const {
#if WITH_EDITOR
	if (Window == nullptr) return EVRMImportMaterialType::VRMIMT_Auto;

	return Window->MaterialType;
#else
	return EVRMImportMaterialType::VRMIMT_Auto;
#endif
}

////








VrmConvert::VrmConvert()
{
}

VrmConvert::~VrmConvert()
{
}

