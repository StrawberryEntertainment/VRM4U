// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertRig.h"
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
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"
#include "Animation/NodeMappingContainer.h"
#include "Animation/Rig.h"
#include "Animation/Skeleton.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"


//#include "Engine/.h"

namespace {
#if WITH_EDITOR
	struct Table {
		FString BoneUE4;
		FString BoneVRM;
	};

	static const Table table[] = {
	{"Root",""},
	{"Pelvis","hips"},
	{"spine_01","spine"},
	{"spine_02","chest"},
	{"spine_03","upperChest"},
	{"clavicle_l","leftShoulder"},
	{"UpperArm_L","leftUpperArm"},
	{"lowerarm_l","leftLowerArm"},
	{"Hand_L","leftHand"},
	{"index_01_l","leftIndexProximal"},
	{"index_02_l","leftIndexIntermediate"},
	{"index_03_l","leftIndexDistal"},
	{"middle_01_l","leftMiddleProximal"},
	{"middle_02_l","leftMiddleIntermediate"},
	{"middle_03_l","leftMiddleDistal"},
	{"pinky_01_l","leftLittleProximal"},
	{"pinky_02_l","leftLittleIntermediate"},
	{"pinky_03_l","leftLittleDistal"},
	{"ring_01_l","leftRingProximal"},
	{"ring_02_l","leftRingIntermediate"},
	{"ring_03_l","leftRingDistal"},
	{"thumb_01_l","leftThumbProximal"},
	{"thumb_02_l","leftThumbIntermediate"},
	{"thumb_03_l","leftThumbDistal"},
	{"lowerarm_twist_01_l",""},
	{"upperarm_twist_01_l",""},
	{"clavicle_r","rightShoulder"},
	{"UpperArm_R","rightUpperArm"},
	{"lowerarm_r","rightLowerArm"},
	{"Hand_R","rightHand"},
	{"index_01_r","rightIndexProximal"},
	{"index_02_r","rightIndexIntermediate"},
	{"index_03_r","rightIndexDistal"},
	{"middle_01_r","rightMiddleProximal"},
	{"middle_02_r","rightMiddleIntermediate"},
	{"middle_03_r","rightMiddleDistal"},
	{"pinky_01_r","rightLittleProximal"},
	{"pinky_02_r","rightLittleIntermediate"},
	{"pinky_03_r","rightLittleDistal"},
	{"ring_01_r","rightRingProximal"},
	{"ring_02_r","rightRingIntermediate"},
	{"ring_03_r","rightRingDistal"},
	{"thumb_01_r","rightThumbProximal"},
	{"thumb_02_r","rightThumbIntermediate"},
	{"thumb_03_r","rightThumbDistal"},
	{"lowerarm_twist_01_r",""},
	{"upperarm_twist_01_r",""},
	{"neck_01","neck"},
	{"head","head"},
	{"Thigh_L","leftUpperLeg"},
	{"calf_l","leftLowerLeg"},
	{"calf_twist_01_l",""},
	{"Foot_L","leftFoot"},
	{"ball_l","leftToes"},
	{"thigh_twist_01_l",""},
	{"Thigh_R","rightUpperLeg"},
	{"calf_r","rightLowerLeg"},
	{"calf_twist_01_r",""},
	{"Foot_R","rightFoot"},
	{"ball_r","rightToes"},
	{"thigh_twist_01_r",""},
	{"ik_foot_root",""},
	{"ik_foot_l",""},
	{"ik_foot_r",""},
	{"ik_hand_root",""},
	{"ik_hand_gun",""},
	{"ik_hand_l",""},
	{"ik_hand_r",""},
	{"Custom_1",""},
	{"Custom_2",""},
	{"Custom_3",""},
	{"Custom_4",""},
	{"Custom_5",""},
	};
#endif
}


bool VRMConverter::ConvertRig(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
#if	UE_VERSION_NEWER_THAN(4,20,0)
#if WITH_EDITOR
	FString name = FString(TEXT("RIG_")) + vrmAssetList->BaseFileName;
	UNodeMappingContainer* mc = NewObject<UNodeMappingContainer>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

	auto *k = vrmAssetList->SkeletalMesh->Skeleton;
	vrmAssetList->SkeletalMesh->NodeMappingData.Add(mc);

	//USkeletalMeshComponent* PreviewMeshComp = PreviewScenePtr.Pin()->GetPreviewMeshComponent();
	//USkeletalMesh* PreviewMesh = PreviewMeshComp->SkeletalMesh;

	{
		URig *EngineHumanoidRig = LoadObject<URig>(nullptr, TEXT("/Engine/EngineMeshes/Humanoid.Humanoid"), nullptr, LOAD_None, nullptr);
		//FSoftObjectPath r(TEXT("/Engine/EngineMeshes/Humanoid.Humanoid"));
		//UObject *u = r.TryLoad();
		mc->SetSourceAsset(EngineHumanoidRig);

		vrmAssetList->SkeletalMesh->Skeleton->SetRigConfig(EngineHumanoidRig);
	}


	mc->SetTargetAsset(vrmAssetList->SkeletalMesh);
	mc->AddDefaultMapping();

	FString PelvisBoneName;
	{
		VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		if (meta) {
			for (auto &t : table) {
				FString target = t.BoneVRM;
				const FString &ue4 = t.BoneUE4;

				if (ue4.Compare(TEXT("Root"), ESearchCase::IgnoreCase) == 0) {
					auto &a = vrmAssetList->SkeletalMesh->Skeleton->GetReferenceSkeleton().GetRefBoneInfo();
					target = a[0].Name.ToString();
				}

				if (target.Len() == 0) {
					continue;
				}


				for (auto b : meta->humanoidBone) {

					if (target.Compare(b.humanBoneName.C_Str()) != 0) {
						continue;
					}
					target = b.nodeName.C_Str();
					break;
				}

				if (PelvisBoneName.Len() == 0) {
					if (ue4.Compare(TEXT("Pelvis"), ESearchCase::IgnoreCase) == 0) {
						PelvisBoneName = target;
					}
				}
				mc->AddMapping(*ue4, *target);
				vrmAssetList->SkeletalMesh->Skeleton->SetRigBoneMapping(*ue4, *target);
			}
		}
	}

	{
		int bone = -1;
		for (int i = 0; i < k->GetReferenceSkeleton().GetRawBoneNum(); ++i) {
			//const int32 BoneIndex = k->GetReferenceSkeleton().FindBoneIndex(InBoneName);
			k->SetBoneTranslationRetargetingMode(i, EBoneTranslationRetargetingMode::Skeleton);
			//FAssetNotifications::SkeletonNeedsToBeSaved(k);
			if (k->GetReferenceSkeleton().GetBoneName(i).Compare(*PelvisBoneName) == 0) {
				bone = i;
			}
		}

		bool first = true;
		while(bone >= 0){
			if (first) {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::AnimationScaled);
			} else {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::Animation);
			}
			first = false;

			bone = k->GetReferenceSkeleton().GetParentIndex(bone);
		}

		if (VRMConverter::Options::Get().IsVRMModel() == false) {
			k->SetBoneTranslationRetargetingMode(0, EBoneTranslationRetargetingMode::Animation, true);
		}

	}
	//mc->AddMapping
	mc->PostEditChange();
	vrmAssetList->HumanoidRig = mc;

#else
#endif
#endif //420
	return true;

}


