// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmSkeleton.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "Engine/SkeletalMesh.h"
//#include "UnStringConv.h

#include <assimp/scene.h>       // Output data structure
#include <assimp/mesh.h>       // Output data structure


static void aaaa(USkeleton *targetSkeleton, const UVrmMetaObject *meta) {

	//k->RemoveBonesFromSkeleton()
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	for (auto &a : allbone) {
		auto p = meta->humanoidBoneTable.FindKey(a.Name.ToString());
		if (p == nullptr) {
			continue;
		}
		a.Name = **p;
	}
}

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

int countParent(aiNode *node, TArray<aiNode*> &t, int c) {
	for (auto &a : t) {
		for (uint32_t i = 0; i < a->mNumChildren; ++i) {
			if (node == a->mChildren[i]) {
				return countParent(a, t, c + 1);
			}
		}
	}
	return c;
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

void UVrmSkeleton::applyBoneFrom(const USkeleton *src, const UVrmMetaObject *meta) {
	//UVrmAssetListObject *alo;

	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

	FTransform t;
	t.SetIdentity();

	for (int i = 0; i < GetBoneTree().Num(); ++i) {
		RefSkelModifier.UpdateRefPoseTransform(i, t);
	}

	auto allbone = ReferenceSkeleton.GetRawRefBoneInfo();
	for (auto &a : allbone) {
		auto targetBoneName = meta->humanoidBoneTable.Find(a.Name.ToString());
		if (targetBoneName == nullptr) {
			continue;
		}

		auto index_src = src->GetReferenceSkeleton().FindRawBoneIndex(**targetBoneName);
		auto &t = src->GetReferenceSkeleton().GetRawRefBonePose()[index_src];

		auto index_my = GetReferenceSkeleton().FindRawBoneIndex(a.Name);

		RefSkelModifier.UpdateRefPoseTransform(index_my, t);
	}

	ReferenceSkeleton.RebuildRefSkeleton(this, true);
	HandleSkeletonHierarchyChange();
}

void UVrmSkeleton::readVrmBone(aiScene* s, int &boneOffset) {

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

			TArray<FString> rec_low;
			TArray<FString> rec_orig;
			for (int targetBondeID = 0; targetBondeID < bone.Num(); ++targetBondeID) {
				auto &a = bone[targetBondeID];

				FString str = (a->mName.C_Str());
				auto f = rec_low.Find(str.ToLower());
				if (f >= 0) {
					// same name check
					//aiString origName = a->mName;
					//str += TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), targetBondeID);
					//a->mName.Set(TCHAR_TO_ANSI(*str));

					if (rec_orig[f] == a->mName.C_Str()) {
						// same name node!
						aiNode *n[] = {
							s->mRootNode->FindNode(a->mName),
							s->mRootNode->FindNode(TCHAR_TO_ANSI(*rec_orig[f])),
						};
						int t[] = {
							countParent(n[0], bone, 0),
							countParent(n[1], bone, 0),
						};

						char tmp[512];
						snprintf(tmp, 512, "%s_DUP", n[0]->mName.C_Str());
						if (t[0] < t[1]) {
							//n[0]->mName = tmp;
						}else {
							//n[1]->mName = tmp;
						}

						//countParent(

						//continue;
					}

					TMap<FString, FString> renameTable;
					{
						FString s;
						s = rec_orig[f];
						s += TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), f);
						renameTable.FindOrAdd(rec_orig[f]) = s;

						s = a->mName.C_Str();
						s += TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), targetBondeID);
						renameTable.FindOrAdd(a->mName.C_Str()) = s;
						str = s;
					}


					//add
					for (uint32_t meshID = 0; meshID < s->mNumMeshes; ++meshID) {
						auto &aiM = *(s->mMeshes[meshID]);
						
						for (uint32_t allBoneID = 0; allBoneID < aiM.mNumBones; ++allBoneID) {
							auto &aiB = *(aiM.mBones[allBoneID]);
							auto res = renameTable.Find(aiB.mName.C_Str());
							if (res) {
							//if (strcmp(aiB.mName.C_Str(), origName.C_Str()) == 0) {

								char tmp[512];
								//if (bone.Num() == aiM.mNumBones) {
								//	snprintf(tmp, 512, "%s_renamed_vrm4u_%02d", origName.C_Str(), allBoneID);
								//} else {
								//snprintf(tmp, 512, "%s", a->mName.C_Str());
								snprintf(tmp, 512, "%s", TCHAR_TO_ANSI(**res));
								//}
								//FString tmp = origName.C_Str();
								//tmp += TEXT("_renamed_vrm4u") + FString::Printf(TEXT("%02d"), allBoneID);

								aiB.mName = tmp;
							}
						}
					}
				}
				if (0) {
					// ascii code
					aiString origName = a->mName;
					str = TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), targetBondeID);
					a->mName.Set(TCHAR_TO_ANSI(*str));

					//add
					for (uint32_t meshID = 0; meshID < s->mNumMeshes; ++meshID) {
						auto &aiM = *(s->mMeshes[meshID]);

						for (uint32_t allBoneID = 0; allBoneID < aiM.mNumBones; ++allBoneID) {
							auto &aiB = *(aiM.mBones[allBoneID]);
							if (strcmp(aiB.mName.C_Str(), origName.C_Str()) == 0) {

								char tmp[512];
								if (bone.Num() == aiM.mNumBones) {
									snprintf(tmp, 512, "_renamed_vrm4u_%02d", allBoneID);
								}else {
									snprintf(tmp, 512, "_renamed_vrm4u_%02d", allBoneID + bone.Num()*targetBondeID);
								}
								//FString tmp = origName.C_Str();
								//tmp += TEXT("_renamed_vrm4u") + FString::Printf(TEXT("%02d"), allBoneID);

								aiB.mName = tmp;
							}
						}
					}

				}

				rec_low.Add(str.ToLower());
				rec_orig.Add(str);
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
				// bad bone. root?
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