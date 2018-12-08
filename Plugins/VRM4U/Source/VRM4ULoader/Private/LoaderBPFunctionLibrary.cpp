// Fill out your copyright notice in the Description page of Project Settings.
#include "LoaderBPFunctionLibrary.h"
//#include "VRM4U.h"
#include "VrmSkeleton.h"
#include "VrmSkeletalMesh.h"
#include "VrmModelActor.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmLicenseObject.h"

#include "VrmConvert.h"

#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/Skeleton.h"
#include "Animation/MorphTarget.h"
#include "Animation/NodeMappingContainer.h"

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
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Misc/FeedbackContext.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "AssetRegistryModule.h"
#include "UObject/Package.h"

//#include "Windows/WindowsSystemIncludes.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "HideWindowsPlatformTypes.h"
#endif

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

static void UpdateProgress(int prog) {
#if WITH_EDITOR
	GWarn->UpdateProgress( prog, 100 );
#endif
}

////

static void ReTransformHumanoidBone(USkeleton *targetHumanoidSkeleton, const UVrmMetaObject *meta, const USkeleton *displaySkeleton) {

	FReferenceSkeleton &ReferenceSkeleton = const_cast<FReferenceSkeleton&>(targetHumanoidSkeleton->GetReferenceSkeleton());
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetHumanoidSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	//auto &humanoidTrans = humanoidSkeleton->GetReferenceSkeleton().GetRawRefBonePose();

	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, targetHumanoidSkeleton);

	for (int ind_target = 0; ind_target < targetHumanoidSkeleton->GetReferenceSkeleton().GetRawBoneNum(); ++ind_target) {
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
#if	UE_VERSION_NEWER_THAN(4,20,0)
	sk->UpdateGenerateUpToData();
#endif
#endif

#if WITH_EDITORONLY_DATA
	k->SetPreviewMesh(sk);
#endif
	k->RecreateBoneTree(sk);

	return true;
}



void ULoaderBPFunctionLibrary::SetImportMode(bool bIm, class UPackage *p) {
	VRMConverter::SetImportMode(bIm);
	package = p;
}

namespace {
#if PLATFORM_WINDOWS
	std::string utf_16_to_shift_jis(const std::wstring& str) {
		static_assert(sizeof(wchar_t) == 2, "this function is windows only");
		const int len = ::WideCharToMultiByte(932/*CP_ACP*/, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string re(len * 2, '\0');
		if (!::WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, &re[0], len, nullptr, nullptr)) {
			const auto ec = ::GetLastError();
			switch (ec)
			{
			case ERROR_INSUFFICIENT_BUFFER:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INSUFFICIENT_BUFFER"); break;
			case ERROR_INVALID_FLAGS:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INVALID_FLAGS"); break;
			case ERROR_INVALID_PARAMETER:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INVALID_PARAMETER"); break;
			default:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: unknown(" + std::to_string(ec) + ')'); break;
				break;
			}
		}
		const std::size_t real_len = std::strlen(re.c_str());
		re.resize(real_len);
		re.shrink_to_fit();
		return re;
	}
#endif
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
	//filepath.esc
	//setlocale(LC_ALL, "");
	//_tsetlocale(LC_ALL, _T(""));
	//_wsetlocale(LC_ALL, _T(""));
	std::string file;
	//TChar a;
	//switch (type)
	//{
	//case EPathType::Absolute:
	//file = TCHAR_TO_UTF8(filepath.GetCharArray().GetData());
	//file = TCHAR_TO_ANSI( *filepath );
	//file = TCHAR_TO_UTF8(*filepath.ReplaceCharWithEscapedChar());
	//file = TCHAR_TO_UTF8(*filepath);
#if PLATFORM_WINDOWS
	file = utf_16_to_shift_jis(*filepath);
#else
	file = TCHAR_TO_UTF8(*filepath);
#endif

	//	break;
	//case EPathType::Relative:
	//	file = TCHAR_TO_UTF8(*FPaths::Combine(FPaths::ProjectContentDir(), filepath));
	//	break;
	//}

	mScenePtr = mImporter.ReadFile(file, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes);

	UpdateProgress(20);
	if (mScenePtr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRM4U: read failure.\n"));
		return false;
	}

	{
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		//FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		baseFileName = FPaths::GetBaseFilename(filepath);

		if (VRMConverter::IsImportMode() == false) {
			FString name = basepath + baseFileName + TEXT("/") + VRMConverter::NormalizeFileName(FPaths::GetBaseFilename(filepath));
			package = CreatePackage(nullptr, *name);
		}

		if (package == nullptr) {
			package = GetTransientPackage();
		}
	}

	src->OrigFileName = baseFileName;
	src->BaseFileName = VRMConverter::NormalizeFileName(baseFileName);
	src->Package = package;

	{
		bool ret = true;
		ret &= VRMConverter::NormalizeBoneName(mScenePtr);
		ret &= VRMConverter::ConvertTextureAndMaterial(src, mScenePtr);
		UpdateProgress(40);
		ret &= VRMConverter::ConvertVrmMeta(src, mScenePtr);	// use texture.
		UpdateProgress(60);
		ret &= VRMConverter::ConvertModel(src, mScenePtr);
		ret &= VRMConverter::ConvertRig(src, mScenePtr);
#if WITH_EDITOR
		ret &= VRMConverter::ConvertMorphTarget(src, mScenePtr);
		ret &= VRMConverter::ConvertHumanoid(src, mScenePtr);
#endif
		UpdateProgress(80);
		if (ret == false) {
			return false;
		}
	}
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
		saveObject(src->HumanoidRig);

	}

	if (VRMConverter::IsImportMode() == false){
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		ULevel::LevelDirtiedEvent.Broadcast();
	}

	// force delete vrmdata
	if (mScenePtr->mVRMMeta) {
		auto *a = const_cast<aiScene*>(mScenePtr);
		if (a) {
			VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(a->mVRMMeta);
			//VRM::ReleaseVRMMeta(meta);
			//delete meta;
			//a->mVRMMeta = nullptr;
		}

	}
	UpdateProgress(100);
	return true;
}



void ULoaderBPFunctionLibrary::VRMTransMatrix(const FTransform &transform, TArray<FLinearColor> &matrix, TArray<FLinearColor> &matrix_inv){

	FMatrix m = transform.ToMatrixWithScale();
	FMatrix mi = transform.ToMatrixWithScale().Inverse();

	matrix.SetNum(4);
	matrix_inv.SetNum(4);

	for (int i = 0; i < 4; ++i) {
		matrix[i] = FLinearColor(m.M[i][0], m.M[i][1], m.M[i][2], m.M[i][3]);
		matrix_inv[i] = FLinearColor(mi.M[i][0], mi.M[i][1], mi.M[i][2], mi.M[i][3]);
	}

	return;
}

void ULoaderBPFunctionLibrary::VRMGetMaterialPropertyOverrides(const UMaterialInterface *Material, TEnumAsByte<EBlendMode> &BlendMode, TEnumAsByte<EMaterialShadingModel> &ShadingModel, bool &IsTwoSided, bool &IsMasked){
	if (Material == nullptr) {
		return;
	}
	BlendMode		= Material->GetBlendMode();
	ShadingModel	= Material->GetShadingModel();
	IsTwoSided		= Material->IsTwoSided();
	IsMasked		= Material->IsMasked();
}
