// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmConvertMorphTarget.h"
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


static bool readMorph2(TArray<FMorphTargetDelta> &MorphDeltas, aiString targetName,const aiScene *mScenePtr) {

	//return readMorph33(MorphDeltas, targetName, mScenePtr);

	MorphDeltas.Reset(0);
	uint32_t currentVertex = 0;

	for (uint32_t m = 0; m < mScenePtr->mNumMeshes; ++m) {
		const aiMesh &aiM = *(mScenePtr->mMeshes[m]);
		for (uint32_t a = 0; a < aiM.mNumAnimMeshes; ++a) {
			const aiAnimMesh &aiA = *(aiM.mAnimMeshes[a]);
			if (targetName != aiA.mName) {
				continue;
			}

			if (aiM.mNumVertices != aiA.mNumVertices) {
				UE_LOG(LogTemp, Warning, TEXT("test18.\n"));
			}
			for (uint32_t i = 0; i < aiA.mNumVertices; ++i) {
				FMorphTargetDelta v;
				v.SourceIdx = i + currentVertex;
				v.PositionDelta.Set(
					-aiA.mVertices[i][0] * 100.f,
					-aiA.mVertices[i][2] * 100.f,  // ? minus? 
					aiA.mVertices[i][1] * 100.f
				);
				v.TangentZDelta.Set(0, 0, 0);
				MorphDeltas.Add(v);
			}
			//break;
		}
		currentVertex += aiM.mNumVertices;
	}
	return MorphDeltas.Num() != 0;
}

namespace VRM{
	bool ConvertMorphTarget(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
#if WITH_EDITOR


		USkeletalMesh *sk = vrmAssetList->SkeletalMesh;

		{
			///sk->MarkPackageDirty();
			// need to refresh the map
			//sk->InitMorphTargets();
			// invalidate render data
			//sk->InvalidateRenderData();
			//return true;
		}

		TArray<FSoftSkinVertex> sVertex;
		sk->GetImportedModel()->LODModels[0].GetVertices(sVertex);
		//mScenePtr->mMeshes[0]->mAnimMeshes[0]->mWeight

		int currentVertex = 0;

		TArray<FString> MorphNameList;

		for (uint32_t m = 0; m < mScenePtr->mNumMeshes; ++m) {
			const aiMesh &aiM = *(mScenePtr->mMeshes[m]);
			for (uint32_t a = 0; a < aiM.mNumAnimMeshes; ++a) {
				const aiAnimMesh &aiA = *(aiM.mAnimMeshes[a]);
				//aiA.
				TArray<FMorphTargetDelta> MorphDeltas;


				if (MorphNameList.Find(FString(aiA.mName.C_Str())) != INDEX_NONE) {
					continue;
				}
				MorphNameList.Add(FString(aiA.mName.C_Str()));
				if (readMorph2(MorphDeltas, aiA.mName, mScenePtr) == false) {
					continue;
				}

				//FString sss = FString::Printf(TEXT("%02d_%02d_"), m, a) + FString(aiA.mName.C_Str());
				FString sss = aiA.mName.C_Str();// FString::Printf(TEXT("%02d_%02d_"), m, a) + FString();
				UMorphTarget *mt = NewObject<UMorphTarget>(sk, *sss);

				/*

				//FString sss = FString::FromInt(a) + FString(aiA.mName.C_Str());
				FString sss = FString::Printf(TEXT("%02d_%02d_"), m, a) + FString(aiA.mName.C_Str());
				//sss = FString::Printf(TEXT("%03d_%s"), a, aiA.mName.C_Str());
				//std::string aaa;
				//aaa << a;
				UMorphTarget *mt = NewObject<UMorphTarget>(sk, *sss);
				//mt->name
				if (m == 18) {
				UE_LOG(LogTemp, Warning, TEXT("test18.\n"));
				}
				//TArray<FMorphTargetDelta> MorphDeltas;
				MorphDeltas.SetNum(aiA.mNumVertices);
				//mt->PopulateDeltas

				//FMorphTargetLODModel *mtm = new(mt->MorphLODModels) FMorphTargetLODModel();
				//new(p->LODRenderData) FSkeletalMeshLODRenderData()
				//FMorphTargetLODModel *mtm = NewObject<FMorphTargetLODModel>();
				//mtm->Vertices.SetNum(aiA.mNumVertices);
				//mtm->SectionIndices.SetNum(aiA.mNumVertices);
				//mtm->SectionIndices.Add( sk->GetImportedModel()->LODModels[0].Sections[m].MaterialIndex );
				for (uint32_t i = 0; i < aiA.mNumVertices; ++i) {
				//auto &v = mtm->Vertices[i];
				auto &v = MorphDeltas[i];
				v.SourceIdx = i +currentVertex;
				v.PositionDelta.Set(
				-aiA.mVertices[i][0] * 100.f,
				aiA.mVertices[i][2] * 100.f,
				aiA.mVertices[i][1] * 100.f
				//aiA.mVertices[i][0],
				//aiA.mVertices[i][1],
				//aiA.mVertices[i][2]
				);
				//v.PositionDelta -= sVertex[i + currentVertex].Position;
				//v.TangentZDelta -= sVertex[i + currentVertex].nor

				//MorphVertex.TangentZDelta = BaseSample->Normals[UsedNormalIndex] - AverageSample->Normals[UsedNormalIndex];

				v.TangentZDelta.Set(0, 0, 0);
				//mtm->SectionIndices[i] = m;// i;// sk->GetImportedModel()->LODModels[0].Sections[m].MaterialIndex;
				}
				//mtm->NumBaseMeshVerts = aiA.mNumVertices;
				*/

				//mt->MorphLODModels.Add(*mtm);
				mt->PopulateDeltas(MorphDeltas, 0, sk->GetImportedModel()->LODModels[0].Sections);
				sk->RegisterMorphTarget(mt);

			}

			currentVertex += aiM.mNumVertices;
		}

#endif
		return true;
	}
}


VrmConvertMorphTarget::VrmConvertMorphTarget()
{
}

VrmConvertMorphTarget::~VrmConvertMorphTarget()
{
}
