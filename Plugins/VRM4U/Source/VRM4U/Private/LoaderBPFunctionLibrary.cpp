// Fill out your copyright notice in the Description page of Project Settings.
#include "LoaderBPFunctionLibrary.h"
#include "VRM4U.h"
#include "VrmSkeleton.h"
#include "VrmSkeletalMesh.h"
#include "VrmModelActor.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmLicenseObject.h"

#include "VrmConvert.h"

#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"

//#include ".h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

#include "RenderingThread.h"
#include "SkeletalMeshModel.h"
#include "SkeletalMeshLODModel.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "AssetRegistryModule.h"
#include "UObject/Package.h"

// tem
UPackage *package = nullptr;
FString baseFileName;
FReturnedData result;


static bool saveObject(UObject *u) {
#if WITH_EDITOR
	if (u == nullptr) return false;
	package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(u);
	//bool bSaved = UPackage::SavePackage(package, u, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(package->GetName()), GError, nullptr, true, true, SAVE_NoError);

	u->PostEditChange();
#endif
	return true;
}

////

static void ReTransformHumanoidBone(USkeleton *targetHumanoidSkeleton, const UVrmMetaObject *meta, const USkeleton *displaySkeleton) {

	FReferenceSkeleton &ReferenceSkeleton = const_cast<FReferenceSkeleton&>(targetHumanoidSkeleton->GetReferenceSkeleton());
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetHumanoidSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	//auto &humanoidTrans = humanoidSkeleton->GetReferenceSkeleton().GetRawRefBonePose();

	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, targetHumanoidSkeleton);

	for (int ind_target = 0; ind_target < targetHumanoidSkeleton->GetBoneTree().Num(); ++ind_target) {
		FTransform t;
		t.SetIdentity();
		RefSkelModifier.UpdateRefPoseTransform(ind_target, t);

		auto boneName = ReferenceSkeleton.GetBoneName(ind_target);
		//auto &info = targetHumanoidSkeleton->GetReferenceSkeleton().GetRefBoneInfo();
		//auto &a = info[ind_target];

		int32 ind_disp = 0;

		if (meta) {
			auto p = meta->humanoidBoneTable.Find(boneName.ToString());
			if (p == nullptr) {
				continue;
			}
			ind_disp = displaySkeleton->GetReferenceSkeleton().FindBoneIndex(**p);
		}else {
			ind_disp = displaySkeleton->GetReferenceSkeleton().FindBoneIndex(boneName);
		}

		if (ind_disp == INDEX_NONE) {
			continue;
		}
		t = displaySkeleton->GetReferenceSkeleton().GetRefBonePose()[ind_disp];

		auto parent = displaySkeleton->GetReferenceSkeleton().GetParentIndex(ind_disp);
		while (parent != INDEX_NONE) {

			auto s = displaySkeleton->GetReferenceSkeleton().GetBoneName(parent);

			if (meta) {
				if (meta->humanoidBoneTable.FindKey(s.ToString()) != nullptr) {
					// parent == humanoidBone
					break;
				}
			}
			//t.SetLocation(t.GetLocation() + displaySkeleton->GetReferenceSkeleton().GetRefBonePose()[parent].GetLocation());;
			parent = displaySkeleton->GetReferenceSkeleton().GetParentIndex(parent);
		}
		RefSkelModifier.UpdateRefPoseTransform(ind_target, t);
	}

	ReferenceSkeleton.RebuildRefSkeleton(targetHumanoidSkeleton, true);

}

bool ULoaderBPFunctionLibrary::VRMReTransformHumanoidBone(USkeletalMeshComponent *targetHumanoidSkeleton, const UVrmMetaObject *meta, const USkeletalMeshComponent *displaySkeleton) {

	if (targetHumanoidSkeleton == nullptr) return false;
	if (targetHumanoidSkeleton->SkeletalMesh == nullptr) return false;

	if (displaySkeleton == nullptr) return false;
	if (displaySkeleton->SkeletalMesh == nullptr) return false;

	// no meta. use default name.
	//if (meta == nullptr) return false;

	ReTransformHumanoidBone(targetHumanoidSkeleton->SkeletalMesh->Skeleton, meta, displaySkeleton->SkeletalMesh->Skeleton);
	auto &sk = targetHumanoidSkeleton->SkeletalMesh;
	auto &k = sk->Skeleton;

	sk->RefSkeleton = k->GetReferenceSkeleton();
	//sk->RefSkeleton.RebuildNameToIndexMap();

	//sk->RefSkeleton.RebuildRefSkeleton(sk->Skeleton, true);
	//sk->Proc();

	//src->RefSkeleton = sk->RefSkeleton;
	
	sk->Skeleton = k;
	sk->RefSkeleton = k->GetReferenceSkeleton();

	sk->CalculateInvRefMatrices();
	sk->CalculateExtendedBounds();

#if WITH_EDITORONLY_DATA
	sk->ConvertLegacyLODScreenSize();
	sk->UpdateGenerateUpToData();
#endif

	k->SetPreviewMesh(sk);
	k->RecreateBoneTree(sk);

	return true;
}



static bool bImportMode = false;
void ULoaderBPFunctionLibrary::SetImportMode(bool bIm, class UPackage *p) {
	bImportMode = bIm;

	package = p;
}

bool ULoaderBPFunctionLibrary::LoadVRMFile(UVrmAssetListObject *src, FString filepath) {

	Assimp::Importer mImporter;
	const aiScene* mScenePtr = nullptr;

	if (src == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("VRM4U: no UVrmAssetListObject.\n"));
		return false;
	}

	if (filepath.IsEmpty())
	{
	}
	std::string file;
	//switch (type)
	//{
	//case EPathType::Absolute:
	file = TCHAR_TO_UTF8(*filepath);
	//	break;
	//case EPathType::Relative:
	//	file = TCHAR_TO_UTF8(*FPaths::Combine(FPaths::ProjectContentDir(), filepath));
	//	break;
	//}

	mScenePtr = mImporter.ReadFile(file, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes);

	if (mScenePtr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRM4U: read failure.\n"));
		return false;
	}

	{
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		//FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		//path += FPaths::GameDevelopersDir() + TEXT("VRM/");

		baseFileName = FPaths::GetBaseFilename(filepath);
		FString name = basepath + baseFileName + TEXT("/") + FPaths::GetBaseFilename(filepath);

		if (bImportMode == false) {
			package = CreatePackage(nullptr, *name);
		}

	}

	src->BaseFileName = baseFileName;
	src->Package = package;

	VRM::ConvertTextureAndMaterial(src, mScenePtr);
	VRM::ConvertVrmMeta(src, mScenePtr);	// use texture.
	VRM::ConvertModel(src, mScenePtr);
#if WITH_EDITOR
	VRM::ConvertMorphTarget(src, mScenePtr);
	VRM::ConvertHumanoid(src, mScenePtr);
#endif
	src->VrmMetaObject->SkeletalMesh = src->SkeletalMesh;

	if (src->bAssetSave) {
		for (auto &t : src->Textures) {
			saveObject(t);
		}
		for (auto &t : src->Materials) {
			saveObject(t);
		}
		saveObject(src->SkeletalMesh);
		saveObject(src->SkeletalMesh->PhysicsAsset);
		saveObject(src->VrmMetaObject);
		saveObject(src->VrmLicenseObject);
		saveObject(src->HumanoidSkeletalMesh);
	}

	if (bImportMode == false){
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		ULevel::LevelDirtiedEvent.Broadcast();
	}

	return nullptr;
}


bool ULoaderBPFunctionLibrary::VRMTransMatrix(TArray<float> &matrix1, TArray<float> &matrix2, const FTransform trans) {

	FMatrix m1 = trans.ToMatrixWithScale().Inverse();
	matrix1.SetNum(16);
	for (int i = 0; i < 16; ++i) {
		matrix1[i] = m1.M[i/4][i%4];
	}

	return true;
}


