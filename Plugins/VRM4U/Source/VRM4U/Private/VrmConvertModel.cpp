// Fill out your copyright notice in the Description page of Project Settings.

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


//#include "Engine/.h"

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


static void FindMesh(const aiScene* scene, aiNode* node, FReturnedData& retdata)
{
	FindMeshInfo(scene, node, retdata);

	for (uint32 m = 0; m < node->mNumChildren; ++m)
	{
		FindMesh(scene, node->mChildren[m], retdata);
	}
}


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



namespace VRM {
	bool ConvertModel(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
		if (vrmAssetList == nullptr) {
			return false;
		}

		FReturnedData &result = *(vrmAssetList->Result);

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
						if (m >= 3){
							UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %d\n"), m);
						}
						result.meshInfo[i].Triangles.Push(mScenePtr->mMeshes[i]->mFaces[l].mIndices[m]);
					}
				}
				verCount += mScenePtr->mMeshes[i]->mNumVertices;
			}
			result.bSuccess = true;
		}

		USkeletalMesh *sk = NewObject<USkeletalMesh>(vrmAssetList->Package, *(FString(TEXT("SK_")) + vrmAssetList->BaseFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		USkeleton *k = NewObject<USkeleton>(vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_Skeleton")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

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

				k->MergeAllBonesToBoneTree(sk_tmp);
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

			k->SetPreviewMesh(sk);
			k->RecreateBoneTree(sk);


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
				sk->Materials[i].MaterialSlotName = mScenePtr->mMaterials[i]->GetName().C_Str();
			}
		}
		//USkeleton* NewAsset = NewObject<USkeleton>();

		//USkeleton* NewAsset = nullptr;// = src->Skeleton;// DuplicateObject<USkeleton>(src, src->GetOuter(), TEXT("/Game/FirstPerson/Character/Mesh/SK_Mannequin_Arms_Skeletonaaaaaaa"));
		{
			FSkeletalMeshLODRenderData &rd = sk->GetResourceForRendering()->LODRenderData[0];

			for (int i = 0; i < sk->Skeleton->GetBoneTree().Num(); ++i) {
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

						if (i < mInfo.VertexColors.Num()){
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
					//mScenePtr->mRootNode->mMeshes
					for (uint32 boneIndex = 0; boneIndex < aiM->mNumBones; ++boneIndex) {
						auto &aiB = aiM->mBones[boneIndex];

						int b = sk->RefSkeleton.FindBoneIndex(aiB->mName.C_Str());

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

								uint8 tabledIndex = 0;
								auto f = bonemap.Find(b);
								if (f != INDEX_NONE) {
									tabledIndex = f;
								}else {
									tabledIndex = bonemap.Add(b);
								}

								s.InfluenceBones[jj] = tabledIndex;
								s.InfluenceWeights[jj] = (uint8)(aiW.mWeight * 255.f);

								meshWeight[aiW.mVertexId].InfluenceBones[jj] = s.InfluenceBones[jj];
								meshWeight[aiW.mVertexId].InfluenceWeights[jj] = s.InfluenceWeights[jj];
								break;
							}
						}
					}

					if (VRM::IsImportMode() == false){
						rd.RenderSections.SetNum(result.meshInfo.Num());

						FSkelMeshRenderSection &NewRenderSection = rd.RenderSections[meshID];
						//NewRenderSection = rd.RenderSections[0];
						NewRenderSection.MaterialIndex = aiM->mMaterialIndex;// ModelSection.MaterialIndex;
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
						NewRenderSection.BoneMap.SetNum(bonemap.Num());//ModelSection.BoneMap;
						for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i) {
							NewRenderSection.BoneMap[i] = bonemap[i];
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

						s.MaterialIndex = aiM->mMaterialIndex;
						s.BaseIndex = currentIndex;
						s.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
						s.BaseVertexIndex = currentVertex;
						s.SoftVertices = meshWeight;
						s.BoneMap.SetNum(bonemap.Num());//ModelSection.BoneMap;
						for (int i = 0; i < s.BoneMap.Num(); ++i) {
							s.BoneMap[i] = bonemap[i];
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


				if (0){
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
							if (w.InfluenceBones[i] == 3) {
								UE_LOG(LogTemp, Warning, TEXT("overr"));
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

				if (VRM::IsImportMode() == false) {
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
		}

		UPhysicsAsset *pa = nullptr;
		if (mScenePtr->mVRMMeta){
			//aa
			pa = NewObject<UPhysicsAsset>(vrmAssetList->Package, *(vrmAssetList->BaseFileName + TEXT("_Physics")) , EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			pa->Modify();
			pa->SetPreviewMesh(sk);
			sk->PhysicsAsset = pa;
			{
				VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

				for (int i = 0; i < meta->sprintNum; ++i) {
					auto &spring = meta->springs[i];
					for (int j = 0; j < spring.boneNum; ++j) {
						auto &sbone = spring.bones_name[j];
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
						bs->BoneName = sbone.C_Str();
						bs->AddCollisionFrom(agg);
						bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
						// newly created bodies default to simulating
						bs->PhysicsType = PhysType_Kinematic;	// fix
																//bs->get

						bs->InvalidatePhysicsData();
						bs->CreatePhysicsMeshes();
						pa->SkeletalBodySetups.Add(bs);

						pa->UpdateBodySetupIndexMap();
						pa->UpdateBoundsBodiesArray();
#if WITH_EDITOR
						pa->InvalidateAllPhysicsMeshes();
#endif
						pa->UpdateBoundsBodiesArray();

						{
							//Material = (UMaterial*)StaticDuplicateObject(OriginalMaterial, GetTransientPackage(), NAME_None, ~RF_Standalone, UPreviewMaterial::StaticClass()); 
							TArray<int32> child;
							int32 ii = k->GetReferenceSkeleton().FindBoneIndex(sbone.C_Str());
							GetChildBoneLocal(k, ii, child);
							for (auto &c : child) {
								USkeletalBodySetup *bs2 = Cast<USkeletalBodySetup>(StaticDuplicateObject(bs,pa, NAME_None));

								bs2->BoneName = k->GetReferenceSkeleton().GetBoneName(c);
								bs2->PhysicsType = PhysType_Simulated;
								//bs2->profile
								bs2->DefaultInstance.InertiaTensorScale.Set(2, 2, 2);

								pa->SkeletalBodySetups.Add(bs2);



								createConstraint(sk, pa, sbone.C_Str(), bs2->BoneName);
							}
						}
					}
				}

				for (int i = 0; i < meta->colliderGroupNum; ++i) {
					auto &c = meta->colliderGroups[i];

					//bs->constrai
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
					USkeletalBodySetup *bs = NewObject<USkeletalBodySetup>(pa, NAME_None);

					bs->Modify();
					bs->BoneName = c.node_name.C_Str();
					bs->AddCollisionFrom(agg);
					bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
					// newly created bodies default to simulating
					bs->PhysicsType = PhysType_Kinematic;	// fix!

															//bs.constra
															//sk->SkeletalBodySetups.Num();
					bs->InvalidatePhysicsData();
					bs->CreatePhysicsMeshes();

					pa->SkeletalBodySetups.Add(bs);

					pa->UpdateBodySetupIndexMap();
					pa->UpdateBoundsBodiesArray();
#if WITH_EDITOR
					pa->InvalidateAllPhysicsMeshes();
#endif
				}
			}

			RefreshSkelMeshOnPhysicsAssetChange(sk);
#if WITH_EDITOR
			pa->RefreshPhysicsAssetChange();
#endif
			pa->UpdateBoundsBodiesArray();

			//FSkeletalMeshModel* ImportedResource = sk->GetImportedModel();
			//if (ImportedResource->LODModels.Num() == 0)
		}

		//NewAsset->FindSocket

		return true;
	}
}

VrmConvertModel::VrmConvertModel()
{
}

VrmConvertModel::~VrmConvertModel()
{
}
