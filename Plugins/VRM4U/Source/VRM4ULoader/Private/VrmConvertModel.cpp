// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "VrmConvertModel.h"
#include "VrmConvert.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

#include "VrmAssetListObject.h"
#include "VrmSkeleton.h"
#include "LoaderBPFunctionLibrary.h"

#include "Engine/SkeletalMesh.h"
#include "RenderingThread.h"
#include "SkeletalMeshModel.h"
#include "SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#if WITH_EDITOR
typedef FSoftSkinVertex FSoftSkinVertexLocal;

#else
namespace {
	struct FSoftSkinVertexLocal
	{
		FVector			Position;

		// Tangent, U-direction
		FVector			TangentX;
		// Binormal, V-direction
		FVector			TangentY;
		// Normal
		FVector4		TangentZ;

		// UVs
		FVector2D		UVs[MAX_TEXCOORDS];
		// VertexColor
		FColor			Color;
		uint8			InfluenceBones[MAX_TOTAL_INFLUENCES];
		uint8			InfluenceWeights[MAX_TOTAL_INFLUENCES];

		/** If this vert is rigidly weighted to a bone, return true and the bone index. Otherwise return false. */
		//ENGINE_API bool GetRigidWeightBone(uint8& OutBoneIndex) const;

		/** Returns the maximum weight of any bone that influences this vertex. */
		//ENGINE_API uint8 GetMaximumWeight() const;

		/**
		* Serializer
		*
		* @param Ar - archive to serialize with
		* @param V - vertex to serialize
		* @return archive that was used
		*/
		//friend FArchive& operator<<(FArchive& Ar, FSoftSkinVertex& V);
	};
}
#endif

static const aiNode* GetBoneNodeFromMeshID(const int &meshID, const aiNode *node) {

	if (node == nullptr) {
		return nullptr;
	}
	for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
		if (node->mMeshes[i] == meshID) {
			return node;
		}
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		auto a = GetBoneNodeFromMeshID(meshID, node->mChildren[i]);
		if (a) {
			return a;
		}
	}
	return nullptr;
}

static const  aiNode* GetNodeFromMeshID(int meshID, const aiScene *mScenePtr) {
	return GetBoneNodeFromMeshID(meshID, mScenePtr->mRootNode);
}

static int GetChildBoneLocal(const USkeleton *skeleton, const int32 ParentBoneIndex, TArray<int32> & Children) {
	Children.Reset();
	auto &r = skeleton->GetReferenceSkeleton();

	const int32 NumBones = r.GetRawBoneNum();
	for (int32 ChildIndex = ParentBoneIndex + 1; ChildIndex < NumBones; ChildIndex++)
	{
		if (ParentBoneIndex == r.GetParentIndex(ChildIndex))
		{
			Children.Add(ChildIndex);
		}
	}
	return Children.Num();
}



static void FindMeshInfo(const aiScene* scene, aiNode* node, FReturnedData& result)
{

	for (uint32 i = 0; i < node->mNumMeshes; i++)
	{
		FString Fs = UTF8_TO_TCHAR(node->mName.C_Str());
		UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %s\n"), *Fs);
		int meshidx = node->mMeshes[i];
		aiMesh *mesh = scene->mMeshes[meshidx];
		FMeshInfo &mi = result.meshInfo[meshidx];

		result.meshToIndex.FindOrAdd(mesh) = mi.Vertices.Num();

		bool bSkin = true;
		if (mesh->mNumBones == 0) {
			bSkin = false;
		}


		//transform.
		aiMatrix4x4 tempTrans = node->mTransformation;
		if (bSkin == false) {
			auto *p = node->mParent;
			while (p) {
				aiMatrix4x4 aiMat = p->mTransformation;
				tempTrans *= aiMat;

				p = p->mParent;
			}
		}

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


static void FindMesh(const aiScene* scene, aiNode* node, FReturnedData& retdata)
{
	FindMeshInfo(scene, node, retdata);

	for (uint32 m = 0; m < node->mNumChildren; ++m)
	{
		FindMesh(scene, node->mChildren[m], retdata);
	}
}

static UPhysicsConstraintTemplate *createConstraint(USkeletalMesh *sk, UPhysicsAsset *pa, VRM::VRMSpring &spring, FName con1, FName con2){
	UPhysicsConstraintTemplate *ct = NewObject<UPhysicsConstraintTemplate>(pa, NAME_None, RF_Transactional);
	pa->ConstraintSetup.Add(ct);

	//"skirt_01_01"
	ct->Modify(false);
	{
		FString orgname = con1.ToString() + TEXT("_") + con2.ToString();
		FString cname = orgname;
		int Index = 0;
		while(pa->FindConstraintIndex(*cname) != INDEX_NONE)
		{
			cname = FString::Printf(TEXT("%s_%d"), *orgname, Index++);
		}
		ct->DefaultInstance.JointName = *cname;
	}

	ct->DefaultInstance.ConstraintBone1 = con1;
	ct->DefaultInstance.ConstraintBone2 = con2;

	ct->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 10);
	ct->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 10);
	ct->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 10);

	ct->DefaultInstance.ProfileInstance.ConeLimit.Stiffness = 100.f * spring.stiffiness;
	ct->DefaultInstance.ProfileInstance.TwistLimit.Stiffness = 100.f * spring.stiffiness;

	const int32 BoneIndex1 = sk->RefSkeleton.FindBoneIndex(ct->DefaultInstance.ConstraintBone1);
	const int32 BoneIndex2 = sk->RefSkeleton.FindBoneIndex(ct->DefaultInstance.ConstraintBone2);

	if (BoneIndex1 == INDEX_NONE || BoneIndex2 == INDEX_NONE) {
		return ct;
	}

	check(BoneIndex1 != INDEX_NONE);
	check(BoneIndex2 != INDEX_NONE);

	const TArray<FTransform> &t = sk->RefSkeleton.GetRawRefBonePose();

	const FTransform BoneTransform1 = t[BoneIndex1];//sk->GetBoneTransform(BoneIndex1);
	FTransform BoneTransform2 = t[BoneIndex2];//EditorSkelComp->GetBoneTransform(BoneIndex2);

	{
		int32 tb = BoneIndex2;
		while (1) {
			int32 p = sk->RefSkeleton.GetRawParentIndex(tb);
			if (p < 0) break;

			const FTransform &f = t[p];
			//BoneTransform2 = f.GetRelativeTransform(BoneTransform2);

			if (p == BoneIndex1) {
				break;
			}
			tb = p;
		}
	}

	//auto b = BoneTransform1;
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

	return ct;
}

static void CreateSwingTail(UVrmAssetListObject *vrmAssetList, VRM::VRMSpring &spring, FName &boneName, USkeletalBodySetup *bs, int BodyIndex1,
	TArray<int> &swingBoneIndexArray, int sboneIndex = -1) {

	USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
	USkeleton *k = sk->Skeleton;
	UPhysicsAsset *pa = sk->PhysicsAsset;

	TArray<int32> child;
	int32 ii = k->GetReferenceSkeleton().FindBoneIndex(boneName);
	GetChildBoneLocal(k, ii, child);
	for (auto &c : child) {

		if (sboneIndex >= 0) {
			c = sboneIndex;
		}
		USkeletalBodySetup *bs2 = Cast<USkeletalBodySetup>(StaticDuplicateObject(bs, pa, NAME_None));

		bs2->BoneName = k->GetReferenceSkeleton().GetBoneName(c);
		bs2->PhysicsType = PhysType_Simulated;
		bs2->CollisionReponse = EBodyCollisionResponse::BodyCollision_Enabled;
		//bs2->profile

		int BodyIndex2 = pa->SkeletalBodySetups.Add(bs2);
		auto *ct = createConstraint(sk, pa, spring, boneName, bs2->BoneName);
		pa->DisableCollision(BodyIndex1, BodyIndex2);

		swingBoneIndexArray.AddUnique(BodyIndex2);

		//CreateSwingTail(vrmAssetList, spring, bs2->BoneName, bs2, BodyIndex2, swingBoneIndexArray);
		if (sboneIndex >= 0) {
			break;
		}
	}
}

static void CreateSwingHead(UVrmAssetListObject *vrmAssetList, VRM::VRMSpring &spring, FName &boneName, TArray<int> &swingBoneIndexArray, int sboneIndex) {

	USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
	USkeleton *k = sk->Skeleton;
	UPhysicsAsset *pa = sk->PhysicsAsset;

	{
		int i = sk->RefSkeleton.FindRawBoneIndex(boneName);
		if (i == INDEX_NONE) {
			return;
		}
	}
	//sk->RefSkeleton->GetParentIndex();

	USkeletalBodySetup *bs = NewObject<USkeletalBodySetup>(pa, NAME_None);

	//int nodeID = mScenePtr->mRootNode->FindNode(sbone.c_str());
	//sk->RefSkeleton.GetRawRefBoneInfo[0].
	//sk->bonetree
	//bs->constrai
	FKAggregateGeom agg;
	FKSphereElem SphereElem;
	SphereElem.Center = FVector(0);
	SphereElem.Radius = spring.hitRadius * 100.f;
	agg.SphereElems.Add(SphereElem);


	bs->Modify();
	bs->BoneName = boneName;
	bs->AddCollisionFrom(agg);
	bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	// newly created bodies default to simulating
	bs->PhysicsType = PhysType_Kinematic;	// fix
											//bs->get
	bs->CollisionReponse = EBodyCollisionResponse::BodyCollision_Disabled;
	bs->DefaultInstance.InertiaTensorScale.Set(2, 2, 2);
	bs->DefaultInstance.LinearDamping = 10.0f * spring.dragForce;
	bs->DefaultInstance.AngularDamping = 10.0f * spring.dragForce;

	bs->InvalidatePhysicsData();
	bs->CreatePhysicsMeshes();
	int BodyIndex1 = pa->SkeletalBodySetups.Add(bs);

	pa->UpdateBodySetupIndexMap();
#if WITH_EDITOR
	//pa->InvalidateAllPhysicsMeshes();
#endif

	//aaaaaa2(vrmAssetList, spring, boneName, bs, BodyIndex1, swingBoneIndexArray, sboneIndex);
	CreateSwingTail(vrmAssetList, spring, boneName, bs, BodyIndex1, swingBoneIndexArray);

	/*
	{
		//Material = (UMaterial*)StaticDuplicateObject(OriginalMaterial, GetTransientPackage(), NAME_None, ~RF_Standalone, UPreviewMaterial::StaticClass()); 
		TArray<int32> child;
		int32 ii = k->GetReferenceSkeleton().FindBoneIndex(boneName);
		GetChildBoneLocal(k, ii, child);
		for (auto &c : child) {
			USkeletalBodySetup *bs2 = Cast<USkeletalBodySetup>(StaticDuplicateObject(bs, pa, NAME_None));

			bs2->BoneName = k->GetReferenceSkeleton().GetBoneName(c);
			bs2->PhysicsType = PhysType_Simulated;
			bs2->CollisionReponse = EBodyCollisionResponse::BodyCollision_Enabled;
			//bs2->profile

			int BodyIndex2 = pa->SkeletalBodySetups.Add(bs2);
			createConstraint(sk, pa, spring, boneName, bs2->BoneName);
			pa->DisableCollision(BodyIndex1, BodyIndex2);

			swingBoneIndexArray.AddUnique(BodyIndex2);

			aaaaaa(vrmAssetList, spring, bs2->BoneName, swingBoneIndexArray);
		}
	}
	*/
}

bool VRMConverter::ConvertModel(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
	if (vrmAssetList == nullptr) {
		return false;
	}

	auto a_tmp = new FReturnedData();
	FReturnedData &result = *(a_tmp);
	//FReturnedData &result = *(vrmAssetList->Result);

	result.bSuccess = false;
	result.meshInfo.Empty();
	result.NumMeshes = 0;

	if (mScenePtr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("test null.\n"));
	}

	if (mScenePtr->HasMeshes())
	{
		result.meshInfo.SetNum(mScenePtr->mNumMeshes, false);

		FindMesh(mScenePtr, mScenePtr->mRootNode, result);

		int verCount = 0;
		for (uint32 i = 0; i < mScenePtr->mNumMeshes; ++i)
		{
			//Triangle number
			for (uint32 l = 0; l < mScenePtr->mMeshes[i]->mNumFaces; ++l)
			{
				for (uint32 m = 0; m < mScenePtr->mMeshes[i]->mFaces[l].mNumIndices; ++m)
				{
					if (m >= 3) {
						UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %d\n"), m);
					}
					result.meshInfo[i].Triangles.Push(mScenePtr->mMeshes[i]->mFaces[l].mIndices[m]);
				}
			}
			verCount += mScenePtr->mMeshes[i]->mNumVertices;
		}
		result.bSuccess = true;
	}

	USkeletalMesh *sk = nullptr;
	if (vrmAssetList->Package == GetTransientPackage()) {
		sk = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
	}else {
		sk = NewObject<USkeletalMesh>(vrmAssetList->Package, *(FString(TEXT("SK_")) + vrmAssetList->BaseFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	}

	USkeleton *k = Options::Get().GetSkeleton();
	if (k == nullptr){
		if (vrmAssetList->Package == GetTransientPackage()) {
			k = NewObject<USkeleton>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
		}else {
			k = NewObject<USkeleton>(vrmAssetList->Package, *(TEXT("SKEL_") + vrmAssetList->BaseFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		}
	}

	static int boneOffset = 0;
	{
		// name dup check
		sk->Skeleton = k;
		//k->MergeAllBonesToBoneTree(src);
		{
			USkeletalMesh *sk_tmp = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
			UVrmSkeleton *k_tmp = NewObject<UVrmSkeleton>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);

			k_tmp->readVrmBone(const_cast<aiScene*>(mScenePtr), boneOffset);
			sk_tmp->Skeleton = k_tmp;
			sk_tmp->RefSkeleton = k_tmp->GetReferenceSkeleton();

			bool b = k->MergeAllBonesToBoneTree(sk_tmp);
			if (b == false) {
				// import failure
				return false;
			}
		}

		sk->RefSkeleton = k->GetReferenceSkeleton();
		//sk->RefSkeleton.RebuildNameToIndexMap();

		//sk->RefSkeleton.RebuildRefSkeleton(sk->Skeleton, true);
		//sk->Proc();

		//src->RefSkeleton = sk->RefSkeleton;

		//src->Skeleton->MergeAllBonesToBoneTree(sk);
		sk->CalculateInvRefMatrices();
		sk->CalculateExtendedBounds();
#if WITH_EDITOR
#if	UE_VERSION_NEWER_THAN(4,20,0)
		sk->UpdateGenerateUpToData();
#endif
		sk->ConvertLegacyLODScreenSize();
		sk->GetImportedModel()->LODModels.Reset();
#endif

#if WITH_EDITORONLY_DATA
		k->SetPreviewMesh(sk);
#endif
		k->RecreateBoneTree(sk);

		// changet retarget option
		if (IsImportMode() == false){
			//k->Modify();

			for (int i = 0; i < k->GetReferenceSkeleton().GetRawBoneNum(); ++i) {
				//const int32 BoneIndex = k->GetReferenceSkeleton().FindBoneIndex(InBoneName);
				//k->SetBoneTranslationRetargetingMode(i, EBoneTranslationRetargetingMode::Skeleton);
				//FAssetNotifications::SkeletonNeedsToBeSaved(k);
			}
		}

		// sk end

		vrmAssetList->SkeletalMesh = sk;

#if	UE_VERSION_NEWER_THAN(4,20,0)
		{
			FSkeletalMeshLODInfo &info = sk->AddLODInfo();
		}
#else
		sk->LODInfo.AddZeroed(1);
		//const USkeletalMeshLODSettings* DefaultSetting = sk->GetDefaultLODSetting();
		// if failed to get setting, that means, we don't have proper setting 
		// in that case, use last index setting
		//!DefaultSetting->SetLODSettingsToMesh(sk, 0);
#endif

		sk->AllocateResourceForRendering();
		FSkeletalMeshRenderData *p = sk->GetResourceForRendering();
		FSkeletalMeshLODRenderData *pRd = new(p->LODRenderData) FSkeletalMeshLODRenderData();

		{
			pRd->StaticVertexBuffers.PositionVertexBuffer.Init(10);
			pRd->StaticVertexBuffers.ColorVertexBuffer.Init(10);
			pRd->StaticVertexBuffers.StaticMeshVertexBuffer.Init(10, 10);

#if WITH_EDITOR
			TArray<FSoftSkinVertex> Weight;
			Weight.SetNum(10);
			pRd->SkinWeightVertexBuffer.Init(Weight);
#else
			{
				TArray< TSkinWeightInfo<false> > InWeights;
				InWeights.SetNum(10);
				for (auto &a : InWeights) {
					memset(a.InfluenceBones, 0, sizeof(a.InfluenceBones));
					memset(a.InfluenceWeights, 0, sizeof(a.InfluenceWeights));
				}
				pRd->SkinWeightVertexBuffer = InWeights;
			}
#endif
		}

		sk->InitResources();

		sk->Materials.SetNum(vrmAssetList->Materials.Num());
		for (int i = 0; i < sk->Materials.Num(); ++i) {
			sk->Materials[i].MaterialInterface = vrmAssetList->Materials[i];
			sk->Materials[i].UVChannelData = FMeshUVChannelInfo(1);
			sk->Materials[i].MaterialSlotName = UTF8_TO_TCHAR(mScenePtr->mMaterials[i]->GetName().C_Str());
		}
	}
	//USkeleton* NewAsset = NewObject<USkeleton>();

	//USkeleton* NewAsset = nullptr;// = src->Skeleton;// DuplicateObject<USkeleton>(src, src->GetOuter(), TEXT("/Game/FirstPerson/Character/Mesh/SK_Mannequin_Arms_Skeletonaaaaaaa"));
	{
		FSkeletalMeshLODRenderData &rd = sk->GetResourceForRendering()->LODRenderData[0];

		for (int i = 0; i < sk->Skeleton->GetReferenceSkeleton().GetRawBoneNum(); ++i) {
			rd.RequiredBones.Add(i);
			rd.ActiveBoneIndices.Add(i);
		}
		{
			FStaticMeshVertexBuffers	 &v = rd.StaticVertexBuffers;

			int allIndex = 0;
			int allVertex = 0;
			for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
				allIndex += result.meshInfo[meshID].Triangles.Num();
				allVertex += result.meshInfo[meshID].Vertices.Num();
			}
			v.StaticMeshVertexBuffer.Init(allVertex, 1);
			v.PositionVertexBuffer.Init(allVertex);
			//rd.SkinWeightVertexBuffer.Init(allVertex);

			TArray<uint32> Triangles;
			TArray<FSoftSkinVertexLocal> Weight;
			Weight.SetNum(allVertex);
			for (auto &w : Weight) {
				memset(w.InfluenceWeights, 0, sizeof(w.InfluenceWeights));
			}
			//Weight.AddZeroed(allVertex);
			int currentIndex = 0;
			int currentVertex = 0;

#if WITH_EDITORONLY_DATA
			new(sk->GetImportedModel()->LODModels) FSkeletalMeshLODModel();
			sk->GetImportedModel()->LODModels[0].Sections.SetNum(result.meshInfo.Num());
#endif

			for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
				TArray<FSoftSkinVertexLocal> meshWeight;
				auto &mInfo = result.meshInfo[meshID];

				for (int i = 0; i < mInfo.Vertices.Num(); ++i) {
					FSoftSkinVertexLocal *meshS = new(meshWeight) FSoftSkinVertexLocal();
					auto a = result.meshInfo[meshID].Vertices[i] * 100.f;

					v.PositionVertexBuffer.VertexPosition(currentVertex + i).Set(-a.X, a.Z, a.Y);

					FVector2D uv;
					uv = mInfo.UV0[i];
					v.StaticMeshVertexBuffer.SetVertexUV(currentVertex + i, 0, uv);
					meshS->UVs[0] = uv;

					{
						v.StaticMeshVertexBuffer.SetVertexTangents(currentVertex + i, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
						//v.StaticMeshVertexBuffer.SetVertexTangents(currentVertex + i, result.meshInfo[meshID].Tangents);
						auto &n = mInfo.Normals[i];
						FVector n_tmp(-n.X, n.Z, n.Y);
						FVector t_tmp(-mInfo.Tangents[i].X, mInfo.Tangents[i].Y, mInfo.Tangents[i].Z);

						meshS->TangentX = t_tmp;
						meshS->TangentY = n_tmp ^ t_tmp;
						meshS->TangentZ = n_tmp;
					}

					if (i < mInfo.VertexColors.Num()) {
						auto &c = mInfo.VertexColors[i];
						meshS->Color = FColor(c.R, c.G, c.B, c.A);
					}

					auto aiS = rd.SkinWeightVertexBuffer.GetSkinWeightPtr<false>(i);
					//aiS->InfluenceWeights
					// InfluenceBones

					FSoftSkinVertexLocal &s = Weight[currentVertex + i];
					s.Position = v.PositionVertexBuffer.VertexPosition(currentVertex + i);
					meshS->Position = v.PositionVertexBuffer.VertexPosition(currentVertex + i);

					for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
					{
						//s.InfluenceBones[InfluenceIndex] = 0;
						//s.InfluenceWeights[InfluenceIndex] = 0;
					}
					//s.InfluenceBones[0] = 0;// aiS->InfluenceBones[0];// +boneOffset;
					//meshS->InfluenceBones[0] = 0;
					//s.InfluenceWeights[0] = 255;
					//meshS->InfluenceWeights[0] = 255;

					//s.InfluenceWeights[0] = aiS->InfluenceWeights[0];
					//s.InfluenceWeights[1] = aiS->InfluenceWeights[1];


					//aiNode *bone = mScenePtr->mRootNode->FindNode( mScenePtr->mMeshes[meshID]->mBones[0]->mName );
				} // vertex loop

				auto &aiM = mScenePtr->mMeshes[meshID];
				TArray<int> bonemap;
				bonemap.Add(0);
				//mScenePtr->mRootNode->mMeshes
				for (uint32 boneIndex = 0; boneIndex < aiM->mNumBones; ++boneIndex) {
					auto &aiB = aiM->mBones[boneIndex];

					int b = sk->RefSkeleton.FindBoneIndex(UTF8_TO_TCHAR(aiB->mName.C_Str()));

					if (b < 0) {
						continue;
					}
					for (uint32 weightIndex = 0; weightIndex < aiB->mNumWeights; ++weightIndex) {
						auto &aiW = aiB->mWeights[weightIndex];

						if (aiW.mWeight == 0.f) {
							continue;
						}
						for (int jj = 0; jj < 8; ++jj) {
							auto &s = Weight[aiW.mVertexId + currentVertex];
							if (s.InfluenceWeights[jj] > 0.f) {
								continue;
							}

							int tabledIndex = 0;
							auto f = bonemap.Find(b);
							if (f != INDEX_NONE) {
								tabledIndex = f;
							}
							else {
								tabledIndex = bonemap.Add(b);
							}
							if (tabledIndex > 255) {
								UE_LOG(LogTemp, Warning, TEXT("bonemap over!"));
							}

							if (Options::Get().IsDebugOneBone()) {
								tabledIndex = 0;
							}

							s.InfluenceBones[jj] = tabledIndex;
							s.InfluenceWeights[jj] = (uint8)(aiW.mWeight * 255.f);

							meshWeight[aiW.mVertexId].InfluenceBones[jj] = s.InfluenceBones[jj];
							meshWeight[aiW.mVertexId].InfluenceWeights[jj] = s.InfluenceWeights[jj];
							break;
						}
					}
				}

				if (VRMConverter::IsImportMode() == false) {
					rd.RenderSections.SetNum(result.meshInfo.Num());

					FSkelMeshRenderSection &NewRenderSection = rd.RenderSections[meshID];
					//NewRenderSection = rd.RenderSections[0];
					if (Options::Get().IsMergeMaterial()) {
						NewRenderSection.MaterialIndex = vrmAssetList->MaterialMergeTable[aiM->mMaterialIndex];// ModelSection.MaterialIndex;
					}else {
						NewRenderSection.MaterialIndex = aiM->mMaterialIndex;// ModelSection.MaterialIndex;
					}
					if (NewRenderSection.MaterialIndex >= vrmAssetList->Materials.Num()) NewRenderSection.MaterialIndex = 0;
					NewRenderSection.BaseIndex = currentIndex;
					NewRenderSection.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
					//NewRenderSection.bRecomputeTangent = ModelSection.bRecomputeTangent;
					NewRenderSection.bCastShadow = true;// ModelSection.bCastShadow;
					NewRenderSection.BaseVertexIndex = currentVertex;// currentVertex;// currentVertex;// ModelSection.BaseVertexIndex;
															//NewRenderSection.ClothMappingData = ModelSection.ClothMappingData;
															//NewRenderSection.BoneMap.SetNum(1);//ModelSection.BoneMap;
															//NewRenderSection.BoneMap[0] = 10;
					//NewRenderSection.BoneMap.SetNum(sk->Skeleton->GetBoneTree().Num());//ModelSection.BoneMap;
					//for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i) {
					//	NewRenderSection.BoneMap[i] = i;
					//}
					if (bonemap.Num() > 0) {
						NewRenderSection.BoneMap.SetNum(bonemap.Num());//ModelSection.BoneMap;
						for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i) {
							NewRenderSection.BoneMap[i] = bonemap[i];
						}
					}else {
						NewRenderSection.BoneMap.SetNum(1);
						auto *p = GetNodeFromMeshID(meshID, mScenePtr);
						int32 i = k->GetReferenceSkeleton().FindBoneIndex(UTF8_TO_TCHAR(p->mName.C_Str()));
						if (i <= 0) {
							i = meshID;
						}
						NewRenderSection.BoneMap[0] = i;
					}


					NewRenderSection.NumVertices = result.meshInfo[meshID].Vertices.Num();// result.meshInfo[meshID].Triangles.Num();// allVertex;// result.meshInfo[meshID].Vertices.Num();// ModelSection.NumVertices;

					NewRenderSection.MaxBoneInfluences = 4;// ModelSection.MaxBoneInfluences;
															//NewRenderSection.CorrespondClothAssetIndex = ModelSection.CorrespondClothAssetIndex;
															//NewRenderSection.ClothingData = ModelSection.ClothingData;
					TMap<int32, TArray<int32>> OverlappingVertices;
					NewRenderSection.DuplicatedVerticesBuffer.Init(NewRenderSection.NumVertices, OverlappingVertices);
					NewRenderSection.bDisabled = false;// ModelSection.bDisabled;
														//RenderSections.Add(NewRenderSection);
														//rd.RenderSections[0] = NewRenderSection;
				}

				for (auto &w : meshWeight) {
					int f = 0;
					int maxIndex = 0;
					int maxWeight = 0;
					for (int i = 0; i < 8; ++i) {
						f += w.InfluenceWeights[i];

						if (maxWeight < w.InfluenceWeights[i]) {
							maxWeight = w.InfluenceWeights[i];
							maxIndex = i;
						}
					}
					if (f > 255) {
						UE_LOG(LogTemp, Warning, TEXT("overr"));
						w.InfluenceWeights[0] -= (uint8)(f - 255);
					}
					if (f <= 254) {
						w.InfluenceWeights[maxIndex] += (uint8)(255 - f);
					}

				}

#if WITH_EDITORONLY_DATA
				{
					auto &s = sk->GetImportedModel()->LODModels[0].Sections[meshID];

					TMap<int32, TArray<int32>> OverlappingVertices;

					{
						bool bUseMergeMaterial = Options::Get().IsMergeMaterial();
						if ((int)aiM->mMaterialIndex >= vrmAssetList->MaterialMergeTable.Num()) {
							bUseMergeMaterial = false;
						}
						if (bUseMergeMaterial) {
							s.MaterialIndex = vrmAssetList->MaterialMergeTable[aiM->mMaterialIndex];
						} else {
							s.MaterialIndex = aiM->mMaterialIndex;
						}
					}

					if (s.MaterialIndex >= vrmAssetList->Materials.Num()) s.MaterialIndex = 0;
					s.BaseIndex = currentIndex;
					s.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
					s.BaseVertexIndex = currentVertex;
					s.SoftVertices = meshWeight;
					if (bonemap.Num() > 0) {
						s.BoneMap.SetNum(bonemap.Num());//ModelSection.BoneMap;
						for (int i = 0; i < s.BoneMap.Num(); ++i) {
							s.BoneMap[i] = bonemap[i];
						}
					}else {
						s.BoneMap.SetNum(1);
						auto *p = GetNodeFromMeshID(meshID, mScenePtr);
						int32 i = k->GetReferenceSkeleton().FindBoneIndex(UTF8_TO_TCHAR(p->mName.C_Str()));
						if (i <= 0) {
							i = meshID;
						}
						s.BoneMap[0] = i;
					}
					s.NumVertices = meshWeight.Num();
					s.MaxBoneInfluences = 4;
				}
#endif

				//rd.MultiSizeIndexContainer.CopyIndexBuffer(result.meshInfo[0].Triangles);
				int t1 = Triangles.Num();
				Triangles.Append(result.meshInfo[meshID].Triangles);
				int t2 = Triangles.Num();

				for (int i = t1; i < t2; ++i) {
					Triangles[i] += currentVertex;
				}
				currentIndex += result.meshInfo[meshID].Triangles.Num();
				currentVertex += result.meshInfo[meshID].Vertices.Num();
				//rd.MultiSizeIndexContainer.update
			} // mesh loop


			if (0) {
				for (auto &w : Weight) {
					int f = 0;
					int maxIndex = 0;
					int maxWeight = 0;
					for (int i = 0; i < 8; ++i) {
						f += w.InfluenceWeights[i];

						if (maxWeight < w.InfluenceWeights[i]) {
							maxWeight = w.InfluenceWeights[i];
							maxIndex = i;
						}
					}
					if (f > 255) {
						UE_LOG(LogTemp, Warning, TEXT("overr"));
						w.InfluenceWeights[0] -= (uint8)(f - 255);
					}
					if (f <= 250) {
						UE_LOG(LogTemp, Warning, TEXT("bad!"));
					}
					if (f <= 254) {
						UE_LOG(LogTemp, Warning, TEXT("under"));
						w.InfluenceWeights[maxIndex] += (uint8)(255 - f);
					}

				}


			}

#if WITH_EDITOR
			{
				FSkeletalMeshLODModel *p = &(sk->GetImportedModel()->LODModels[0]);
				p->NumVertices = allVertex;
				p->NumTexCoords = 1;// allVertex;
				p->IndexBuffer = Triangles;
				p->ActiveBoneIndices = rd.ActiveBoneIndices;
				p->RequiredBones = rd.RequiredBones;
			}
#endif
			//rd.StaticVertexBuffers.StaticMeshVertexBuffer.TexcoordDataPtr;

			if (VRMConverter::IsImportMode() == false) {
				ENQUEUE_RENDER_COMMAND(UpdateCommand)(
					[sk, Triangles, Weight](FRHICommandListImmediate& RHICmdList)
				{
					FSkeletalMeshLODRenderData &d = sk->GetResourceForRendering()->LODRenderData[0];

					if (d.MultiSizeIndexContainer.IsIndexBufferValid()) {
						d.MultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
					}
					d.MultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
					d.MultiSizeIndexContainer.GetIndexBuffer()->InitResource();

					//d.AdjacencyMultiSizeIndexContainer.CopyIndexBuffer(Triangles);
					if (d.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid()) {
						d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
					}
					d.AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
					d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->InitResource();

#if WITH_EDITOR
					d.SkinWeightVertexBuffer.Init(Weight);
#else
					{
						TArray< TSkinWeightInfo<false> > InWeights;
						InWeights.Reserve(Weight.Num());

						for (const auto &a : Weight) {
							auto n = new(InWeights) TSkinWeightInfo<false>;

							memcpy(n->InfluenceBones, a.InfluenceBones, sizeof(n->InfluenceBones));
							memcpy(n->InfluenceWeights, a.InfluenceWeights, sizeof(n->InfluenceWeights));
						}
						d.SkinWeightVertexBuffer = InWeights;
					}
#endif
					d.SkinWeightVertexBuffer.InitResource();

					d.StaticVertexBuffers.PositionVertexBuffer.UpdateRHI();
					d.StaticVertexBuffers.StaticMeshVertexBuffer.UpdateRHI();

					d.MultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI();
					d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI();
					d.SkinWeightVertexBuffer.UpdateRHI();

					//src->GetResourceForRendering()->LODRenderData[0].rhi
					//v.PositionVertexBuffer.UpdateRHI();
				});
			}
		}
		{
			FVector MinB(-100, -100, -100);
			FVector MaxB(100, 100, 100);

			FBox BoundingBox(MinB, MaxB);
			sk->SetImportedBounds(FBoxSphereBounds(BoundingBox));
		}
	}

	UPhysicsAsset *pa = nullptr;
	if (mScenePtr->mVRMMeta && Options::Get().IsSkipPhysics()==false) {
		VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);
		if (meta->sprintNum > 0) {
			if (vrmAssetList->Package == GetTransientPackage()) {
				pa = NewObject<UPhysicsAsset>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient, NULL);
			}else {
				pa = NewObject<UPhysicsAsset>(vrmAssetList->Package, *(TEXT("PHYS_") + vrmAssetList->BaseFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			}
			pa->Modify();
#if WITH_EDITORONLY_DATA
			pa->SetPreviewMesh(sk);
#endif
			sk->PhysicsAsset = pa;

			TArray<FString> addedList;
			{
				TArray<int> swingBoneIndexArray;

				for (int i = 0; i < meta->sprintNum; ++i) {
					auto &spring = meta->springs[i];
					for (int j = 0; j < spring.boneNum; ++j) {
						auto &sbone = spring.bones_name[j];

						{
							FString s = UTF8_TO_TCHAR(sbone.C_Str());
							s = s.ToLower();
							if (addedList.Find(s) >= 0) {
								continue;
							}
							addedList.Add(s);
						}

						int sboneIndex = sk->RefSkeleton.FindRawBoneIndex(sbone.C_Str());
						if (sboneIndex == INDEX_NONE) {
							continue;
						}
						FName parentName = sbone.C_Str();

						//int parentIndex = sk->RefSkeleton.GetParentIndex(sboneIndex);
						//FName parentName = sk->RefSkeleton.GetBoneName(parentIndex);

						CreateSwingHead(vrmAssetList, spring, parentName, swingBoneIndexArray, sboneIndex);
						//aaaaaa(vrmAssetList, spring, parentName, swingBoneIndexArray, sboneIndex);

					}
				} // all spring
				{
					swingBoneIndexArray.Sort();

					for (int i = 0; i < swingBoneIndexArray.Num()-1; ++i) {
						for (int j = i+1; j < swingBoneIndexArray.Num(); ++j) {
							pa->DisableCollision(swingBoneIndexArray[i], swingBoneIndexArray[j]);
						}
					}
				}

				// collision
				for (int i = 0; i < meta->colliderGroupNum; ++i) {
					auto &c = meta->colliderGroups[i];

					USkeletalBodySetup *bs = nullptr;
					{
						FString s = UTF8_TO_TCHAR(c.node_name.C_Str());
						s = s.ToLower();
						// addlist changed recur...
						if (1){//addedList.Find(s) >= 0) {
							for (auto &a : pa->SkeletalBodySetups) {
								if (a->BoneName.IsEqual(*s)) {
									bs = a;
									break;
								}
							}

						}else {
							addedList.Add(s);
						}
					}
					if (bs == nullptr) {
						bs = NewObject<USkeletalBodySetup>(pa, NAME_None);
						bs->InvalidatePhysicsData();

						pa->SkeletalBodySetups.Add(bs);
					}

					FKAggregateGeom agg;
					for (int j = 0; j < c.colliderNum; ++j) {
						FKSphereElem SphereElem;
						VRM::vec3 v = {
							c.colliders[j].offset[0] * 100.f,
							c.colliders[j].offset[1] * 100.f,
							c.colliders[j].offset[2] * 100.f,
						};

						SphereElem.Center = FVector(-v[0], v[2], v[1]);
						SphereElem.Radius = c.colliders[j].radius * 100.f;
						agg.SphereElems.Add(SphereElem);
					}

					bs->Modify();
					bs->BoneName = UTF8_TO_TCHAR(c.node_name.C_Str());
					bs->AddCollisionFrom(agg);
					bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
					// newly created bodies default to simulating
					bs->PhysicsType = PhysType_Kinematic;	// fix!

															//bs.constra
															//sk->SkeletalBodySetups.Num();
					bs->CreatePhysicsMeshes();

#if WITH_EDITOR
					//pa->InvalidateAllPhysicsMeshes();
#endif
				}// collision
			}

			pa->UpdateBodySetupIndexMap();

			RefreshSkelMeshOnPhysicsAssetChange(sk);
#if WITH_EDITOR
			pa->RefreshPhysicsAssetChange();
#endif
			pa->UpdateBoundsBodiesArray();

		}
		//FSkeletalMeshModel* ImportedResource = sk->GetImportedModel();
		//if (ImportedResource->LODModels.Num() == 0)
	}

	//NewAsset->FindSocket
	return true;
}


VrmConvertModel::VrmConvertModel()
{
}

VrmConvertModel::~VrmConvertModel()
{
}
