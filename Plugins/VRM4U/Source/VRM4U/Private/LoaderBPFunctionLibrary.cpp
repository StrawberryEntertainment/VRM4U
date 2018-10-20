// Fill out your copyright notice in the Description page of Project Settings.
#include "LoaderBPFunctionLibrary.h"
#include "VRM4U.h"
#include "VrmSkeleton.h"
#include "VrmSkeletalMesh.h"
#include "VrmModelActor.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"

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
	package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(u);
	//bool bSaved = UPackage::SavePackage(package, u, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(package->GetName()), GError, nullptr, true, true, SAVE_NoError);

	u->PostEditChange();
#endif
	return true;
}

void FindMeshInfo2(const aiScene* scene, aiNode* node, FReturnedData& result)
{

	for (uint32 i = 0; i < node->mNumMeshes; i++)
	{
		std::string TestString = node->mName.C_Str();
		FString Fs = FString(TestString.c_str());
		UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %s\n"), *Fs);
		int meshidx = node->mMeshes[i];
		aiMesh *mesh = scene->mMeshes[meshidx];
		FMeshInfo &mi = result.meshInfo[meshidx];

		result.meshToIndex.FindOrAdd(mesh) = mi.Vertices.Num();
		
		//if (mesh->mNumBones == 0) {
		//	continue;
		//}
		

		//transform.
		aiMatrix4x4 tempTrans = node->mTransformation;
		FMatrix tempMatrix;
		tempMatrix.M[0][0] = tempTrans.a1; tempMatrix.M[0][1] = tempTrans.b1; tempMatrix.M[0][2] = tempTrans.c1; tempMatrix.M[0][3] = tempTrans.d1;
		tempMatrix.M[1][0] = tempTrans.a2; tempMatrix.M[1][1] = tempTrans.b2; tempMatrix.M[1][2] = tempTrans.c2; tempMatrix.M[1][3] = tempTrans.d2;
		tempMatrix.M[2][0] = tempTrans.a3; tempMatrix.M[2][1] = tempTrans.b3; tempMatrix.M[2][2] = tempTrans.c3; tempMatrix.M[2][3] = tempTrans.d3;
		tempMatrix.M[3][0] = tempTrans.a4; tempMatrix.M[3][1] = tempTrans.b4; tempMatrix.M[3][2] = tempTrans.c4; tempMatrix.M[3][3] = tempTrans.d4;
		mi.RelativeTransform = FTransform(tempMatrix);

		//vet
		for (uint32 j = 0; j < mesh->mNumVertices; ++j)
		{
			FVector vertex = FVector(
				mesh->mVertices[j].x,
				mesh->mVertices[j].y,
				mesh->mVertices[j].z);

			vertex = mi.RelativeTransform.TransformFVector4(vertex);
			mi.Vertices.Push(vertex);

			//Normal
			if (mesh->HasNormals())
			{
				FVector normal = FVector(
					mesh->mNormals[j].x,
					mesh->mNormals[j].y,
					mesh->mNormals[j].z);

				//normal = mi.RelativeTransform.TransformFVector4(normal);
				mi.Normals.Push(normal);
			}
			else
			{
				mi.Normals.Push(FVector::ZeroVector);
			}

			//UV Coordinates - inconsistent coordinates
			if (mesh->HasTextureCoords(0))
			{
				FVector2D uv = FVector2D(mesh->mTextureCoords[0][j].x, -mesh->mTextureCoords[0][j].y);
				mi.UV0.Add(uv);
			}

			//Tangent
			if (mesh->HasTangentsAndBitangents())
			{
				FVector v(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
				FProcMeshTangent meshTangent = FProcMeshTangent(v.X, v.Y, v.Z);
				mi.Tangents.Push(v);
				mi.MeshTangents.Push(meshTangent);
			}

			//Vertex color
			if (mesh->HasVertexColors(0))
			{
				FLinearColor color = FLinearColor(
					mesh->mColors[0][j].r,
					mesh->mColors[0][j].g,
					mesh->mColors[0][j].b,
					mesh->mColors[0][j].a
				);
				mi.VertexColors.Push(color);
			}

		}
	}
}


void FindMesh2(const aiScene* scene, aiNode* node, FReturnedData& retdata)
{
	FindMeshInfo2(scene, node, retdata);

	for (uint32 m = 0; m < node->mNumChildren; ++m)
	{
		FindMesh2(scene, node->mChildren[m], retdata);
	}
}

////

static void createConstraint(USkeletalMesh *sk, UPhysicsAsset *pa, FName con1, FName con2){
	UPhysicsConstraintTemplate *ct = NewObject<UPhysicsConstraintTemplate>(pa, NAME_None, RF_Transactional);
	//"skirt_01_01"
	ct->Modify(false);
	ct->DefaultInstance.JointName = TCHAR_TO_ANSI(*(con1.ToString() + TEXT("_") + con2.ToString()));
	ct->DefaultInstance.ConstraintBone1 = con1;
	ct->DefaultInstance.ConstraintBone2 = con2;


	ct->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 10);
	ct->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 10);
	ct->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 10);

	ct->DefaultInstance.ProfileInstance.ConeLimit.Stiffness = 100.f;
	ct->DefaultInstance.ProfileInstance.TwistLimit.Stiffness = 100.f;

	const int32 BoneIndex1 = sk->RefSkeleton.FindBoneIndex(ct->DefaultInstance.ConstraintBone1);
	const int32 BoneIndex2 = sk->RefSkeleton.FindBoneIndex(ct->DefaultInstance.ConstraintBone2);

	if (BoneIndex1 == INDEX_NONE || BoneIndex2 == INDEX_NONE) {
		return;
	}

	check(BoneIndex1 != INDEX_NONE);
	check(BoneIndex2 != INDEX_NONE);

	const TArray<FTransform> &t = sk->RefSkeleton.GetRawRefBonePose();

	const FTransform BoneTransform1 = t[BoneIndex1];//sk->GetBoneTransform(BoneIndex1);
	FTransform BoneTransform2 = t[BoneIndex2];//EditorSkelComp->GetBoneTransform(BoneIndex2);

	int32 tb = BoneIndex2;
	while (sk->RefSkeleton.GetRawParentIndex(tb) != BoneIndex1) {
		int32 p = sk->RefSkeleton.GetRawParentIndex(tb);
		if (p < 0) break;

		FTransform f = t[p];
		BoneTransform2 = f.GetRelativeTransform(BoneTransform2);

		tb = p;
	}

	auto b = BoneTransform1;
	ct->DefaultInstance.Pos1 = FVector::ZeroVector;
	ct->DefaultInstance.PriAxis1 = FVector(1, 0, 0);
	ct->DefaultInstance.SecAxis1 = FVector(0, 1, 0);


	auto r = BoneTransform2;// .GetRelativeTransform(BoneTransform2);
//	auto r = BoneTransform1.GetRelativeTransform(BoneTransform2);
	auto twis = r.GetLocation().GetSafeNormal();
	auto p1 = twis;
	p1.X = p1.Z = 0.f;
	auto p2 = FVector::CrossProduct(twis, p1).GetSafeNormal();
	p1 = FVector::CrossProduct(p2, twis).GetSafeNormal();
	
	ct->DefaultInstance.Pos2 = -r.GetLocation();
	//ct->DefaultInstance.PriAxis2 = p1;
	//ct->DefaultInstance.SecAxis2 = p2;
	ct->DefaultInstance.PriAxis2 = r.GetUnitAxis(EAxis::X);
	ct->DefaultInstance.SecAxis2 = r.GetUnitAxis(EAxis::Y);

	// child 
	//ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
	// parent
	//ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, BoneTransform1.GetRelativeTransform(BoneTransform2));

#if WITH_EDITOR
	ct->SetDefaultProfile(ct->DefaultInstance);
#endif
	//ct->DefaultInstance.InitConstraint();


	pa->ConstraintSetup.Add(ct);
	pa->DisableCollision(BoneIndex1, BoneIndex2);

}

static bool RetargetTest(){
//	USkeleton *sk;

	//FReferenceSkeletonModifier RefSkelModifier(sk->ReferenceSkeleton, sk);


}

static bool readVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {

	VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

	UVrmMetaObject *m;

	m = NewObject<UVrmMetaObject>(package, TEXT("VrmMeta"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);


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
			bind.nodeName= aiGroup.bind[b].nodeName.C_Str();
			bind.weight = aiGroup.bind[b].weight;
			bind.meshID = aiGroup.bind[b].meshID;
			bind.shapeIndex = aiGroup.bind[b].shapeIndex;
		}
	}

	vrmAssetList->VrmMetaObject = m;
	return true;
}

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

			if (meta->humanoidBoneTable.FindKey(s.ToString()) != nullptr) {
				// parent == humanoidBone
				break;
			}
			//t.SetLocation(t.GetLocation() + displaySkeleton->GetReferenceSkeleton().GetRefBonePose()[parent].GetLocation());;
			parent = displaySkeleton->GetReferenceSkeleton().GetParentIndex(parent);
		}


		RefSkelModifier.UpdateRefPoseTransform(ind_target, t);
	}

	ReferenceSkeleton.RebuildRefSkeleton(targetHumanoidSkeleton, true);

}

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


static bool createHumanoidSkeletalMesh(UVrmAssetListObject *vrmAssetList) {
	if (1){
		USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
		USkeleton *k = sk->Skeleton;

		USkeleton *base = DuplicateObject<USkeleton>(k, package, *(baseFileName + TEXT("_humanoid_Skeleton")));
		USkeletalMesh *ss = DuplicateObject<USkeletalMesh>(sk, package, *(FString(TEXT("SK_")) + baseFileName + TEXT("_humanoid")));

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

		saveObject(base);
		//

		UAnimInstance *anim = nullptr;
		//anim = NewObject<UAnimInstance>(package, *(baseFileName + TEXT("_anim")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

		//anim->ske

		//saveObject(anim);
	}

	return true;
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




////////////////
//static bool LoadVRMPtr(UVrmAssetListObject *src, ) {
//}

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

	readVrmMeta(src, mScenePtr);
	VRM::ConvertTextureAndMaterial(src, mScenePtr);
	VRM::ConvertModel(src, mScenePtr);
	VRM::ConvertMorphTarget(src, mScenePtr);
	createHumanoidSkeletalMesh(src);

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
	}

	if (bImportMode == false){
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		ULevel::LevelDirtiedEvent.Broadcast();
	}

	return nullptr;
}
