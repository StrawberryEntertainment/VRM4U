// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmSkeleton.h"
#include "Engine/SkeletalMesh.h"

#include <assimp/scene.h>       // Output data structure
#include <assimp/mesh.h>       // Output data structure


USkeletalMesh* UVrmSkeleton::GetPreviewMesh(bool bFindIfNotSet)
{
	//return Super::GetPreviewMesh(bFindIfNotSet);
#if 1 //WITH_EDITORONLY_DATA
	USkeletalMesh* PreviewMesh = PreviewSkeletalMesh.LoadSynchronous();

	if(PreviewMesh && PreviewMesh->Skeleton != this) // fix mismatched skeleton
	{
		PreviewSkeletalMesh.Reset();
		PreviewMesh = nullptr;
	}

	// if not existing, and if bFindIfNotExisting is true, then try find one
	if(!PreviewMesh && bFindIfNotSet)
	{
		USkeletalMesh* CompatibleSkeletalMesh = FindCompatibleMesh();
		if(CompatibleSkeletalMesh)
		{
			SetPreviewMesh(CompatibleSkeletalMesh, false);
			// update PreviewMesh
			PreviewMesh = PreviewSkeletalMesh.Get();
		}
	}

	return PreviewMesh;
#else
	return nullptr;
#endif
}



USkeletalMesh* UVrmSkeleton::GetPreviewMesh() const
{
	//return Super::GetPreviewMesh();
#if 1 //WITH_EDITORONLY_DATA
	return PreviewSkeletalMesh.Get();
#else
	return nullptr;
#endif
}

void UVrmSkeleton::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty)
{
	//Super::SetPreviewMesh(PreviewMesh, bMarkAsDirty);
#if 1 //WITH_EDITORONLY_DATA
	if (bMarkAsDirty)
	{
		Modify();
	}

	PreviewSkeletalMesh = PreviewMesh;
#endif
}


bool UVrmSkeleton::IsPostLoadThreadSafe() const
{
	return true;
}

void rr(aiNode *node, TArray<aiNode*> &t) {
	if (node == nullptr) {
		return;
	}
	t.Push(node);
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		rr(node->mChildren[i], t);
	}
}

void UVrmSkeleton::Proc(aiScene* s, int &boneOffset) {

	boneOffset = 0;
	//FBoneNode n;

	//n.Name_DEPRECATED = TEXT("aaaaa");
	//n.ParentIndex_DEPRECATED = 1;
	//n.TranslationRetargetingMode

	//this->BoneTree.Push(n);


	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

	FTransform t;
	t.SetIdentity();

	for (int i = 0; i < GetBoneTree().Num(); ++i) {
		RefSkelModifier.UpdateRefPoseTransform(i, t);
	}


	{
		//const int offset = GetBoneTree().Num();
		//if (offset > 30) {
		//	return;
		//}
		//boneOffset = offset-1;

		TArray<aiNode*> bone;
		rr(s->mRootNode, bone);

		{

			TArray<FString> rec;
			for (auto &a : bone) {
				FString str = a->mName.C_Str();
				if (rec.Find(str) >= 0) {
					aiString origName = a->mName;
					str += TEXT("_renamed_vrm4u");
					a->mName.Set(TCHAR_TO_ANSI(*str));

					//add
					for (uint32_t meshID = 0; meshID < s->mNumMeshes; ++meshID) {
						auto &aiM = *(s->mMeshes[meshID]);
						
						for (uint32_t boneID = 0; boneID < aiM.mNumBones; ++boneID) {
							auto &aiB = *(aiM.mBones[boneID]);
							if (strcmp(aiB.mName.C_Str(), origName.C_Str()) == 0) {
								aiB.mName = a->mName;
							}
						}
					}
				}
				rec.Add(str);
			}
		}


		int totalBoneCount = 0;
		for (auto &a: bone) {
			FMeshBoneInfo info;
			info.Name = a->mName.C_Str();
			info.ExportName = a->mName.C_Str();

			FTransform pose;
			int32 index = 0;
			FMatrix m;
			if (bone.Find(a->mParent, index) == false) {
				//continue;
				// root
				info.ParentIndex = INDEX_NONE;
				m.SetIdentity();
				//m = m.ApplyScale(100);
			}
			else {
				if (index != 0) {
					//index += offset - 1;
				}
				info.ParentIndex = index;
				auto &t = a->mTransformation;
				m.M[0][0] = t.a1; m.M[1][0] = t.a2; m.M[2][0] = t.a3; m.M[3][0] = -t.a4*100.f;
				m.M[0][1] = t.b1; m.M[1][1] = t.b2; m.M[2][1] = t.b3; m.M[3][1] = t.c4*100.f;//t.b4*100.f;
				m.M[0][2] = t.c1; m.M[1][2] = t.c2; m.M[2][2] = t.c3; m.M[3][2] = t.b4*100.f;//t.c4*100.f;
				m.M[0][3] = t.d1; m.M[1][3] = t.d2; m.M[2][3] = t.d3; m.M[3][3] = t.d4;

				if (a == s->mRootNode) {
					//pose.SetScale3D(FVector(100));
				}
				
			}
			pose.SetFromMatrix(m);

			if (ReferenceSkeleton.FindRawBoneIndex(info.Name) != INDEX_NONE) {
				info.Name = *(info.Name.ToString() + FString(TEXT("_DUP")));
				//continue;
			}
			if (totalBoneCount > 0 && info.ParentIndex == INDEX_NONE) {
				continue;
			}
			RefSkelModifier.Add(info, pose);
			totalBoneCount++;
		}
	}



	HandleSkeletonHierarchyChange();
//	bShouldHandleHierarchyChange = true;
	//ReferenceSkeleton.
}