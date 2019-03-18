// Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

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


bool VRMConverter::ConvertMorphTarget(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
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

	TArray<FString> MorphNameList;

	for (uint32_t m = 0; m < mScenePtr->mNumMeshes; ++m) {
		const aiMesh &aiM = *(mScenePtr->mMeshes[m]);
		for (uint32_t a = 0; a < aiM.mNumAnimMeshes; ++a) {
			const aiAnimMesh &aiA = *(aiM.mAnimMeshes[a]);
			//aiA.
			TArray<FMorphTargetDelta> MorphDeltas;

			FString morphName = UTF8_TO_TCHAR(aiA.mName.C_Str());
			if (morphName == TEXT("")) {
				//morphName = FString::Printf("%d_%d", m, a);
			}


			if (MorphNameList.Find(morphName) != INDEX_NONE) {
				continue;
			}
			MorphNameList.Add(morphName);
			if (readMorph2(MorphDeltas, aiA.mName, mScenePtr) == false) {
				continue;
			}

			//FString sss = FString::Printf(TEXT("%02d_%02d_"), m, a) + FString(aiA.mName.C_Str());
			FString sss = morphName;// FString::Printf(TEXT("%02d_%02d_"), m, a) + FString();
			UMorphTarget *mt = NewObject<UMorphTarget>(sk, *sss);

			mt->PopulateDeltas(MorphDeltas, 0, sk->GetImportedModel()->LODModels[0].Sections);

			if (mt->HasValidData()) {
				sk->RegisterMorphTarget(mt);
			}
			
		}
	}

#endif
	return true;
}


VrmConvertMorphTarget::VrmConvertMorphTarget()
{
}

VrmConvertMorphTarget::~VrmConvertMorphTarget()
{
}
