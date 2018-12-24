// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmAnimInstanceCopy.h"
#include "VrmMetaObject.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/Morphtarget.h"
#include "Animation/Rig.h"
#include "BoneControllers/AnimNode_Fabrik.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "BoneControllers/AnimNode_SplineIK.h"

namespace {
	const FString table[] = {
		"hips",
		"leftUpperLeg",
		"rightUpperLeg",
		"leftLowerLeg",
		"rightLowerLeg",
		"leftFoot",
		"rightFoot",
		"spine",
		"chest",
		"neck",
		"head",
		"leftShoulder",
		"rightShoulder",
		"leftUpperArm",
		"rightUpperArm",
		"leftLowerArm",
		"rightLowerArm",
		"leftHand",
		"rightHand",
		"leftToes",
		"rightToes",
		"leftEye",
		"rightEye",
		"jaw",
		"leftThumbProximal",
		"leftThumbIntermediate",
		"leftThumbDistal",
		"leftIndexProximal",
		"leftIndexIntermediate",
		"leftIndexDistal",
		"leftMiddleProximal",
		"leftMiddleIntermediate",
		"leftMiddleDistal",
		"leftRingProximal",
		"leftRingIntermediate",
		"leftRingDistal",
		"leftLittleProximal",
		"leftLittleIntermediate",
		"leftLittleDistal",
		"rightThumbProximal",
		"rightThumbIntermediate",
		"rightThumbDistal",
		"rightIndexProximal",
		"rightIndexIntermediate",
		"rightIndexDistal",
		"rightMiddleProximal",
		"rightMiddleIntermediate",
		"rightMiddleDistal",
		"rightRingProximal",
		"rightRingIntermediate",
		"rightRingDistal",
		"rightLittleProximal",
		"rightLittleIntermediate",
		"rightLittleDistal",
		"upperChest"
	};
	/*
	const FString table[] = {
		"Root",
		"Pelvis",
		"spine_01",
		"spine_02",
		"spine_03",
		"clavicle_l",
		"UpperArm_L",
		"lowerarm_l",
		"Hand_L",
		"index_01_l",
		"index_02_l",
		"index_03_l",
		"middle_01_l",
		"middle_02_l",
		"middle_03_l",
		"pinky_01_l",
		"pinky_02_l",
		"pinky_03_l",
		"ring_01_l",
		"ring_02_l",
		"ring_03_l",
		"thumb_01_l",
		"thumb_02_l",
		"thumb_03_l",
		"lowerarm_twist_01_l",
		"upperarm_twist_01_l",
		"clavicle_r",
		"UpperArm_R",
		"lowerarm_r",
		"Hand_R",
		"index_01_r",
		"index_02_r",
		"index_03_r",
		"middle_01_r",
		"middle_02_r",
		"middle_03_r",
		"pinky_01_r",
		"pinky_02_r",
		"pinky_03_r",
		"ring_01_r",
		"ring_02_r",
		"ring_03_r",
		"thumb_01_r",
		"thumb_02_r",
		"thumb_03_r",
		"lowerarm_twist_01_r",
		"upperarm_twist_01_r",
		"neck_01",
		"head",
		"Thigh_L",
		"calf_l",
		"calf_twist_01_l",
		"Foot_L",
		"ball_l",
		"thigh_twist_01_l",
		"Thigh_R",
		"calf_r",
		"calf_twist_01_r",
		"Foot_R",
		"ball_r",
		"thigh_twist_01_r",
		"ik_foot_root",
		"ik_foot_l",
		"ik_foot_r",
		"ik_hand_root",
		"ik_hand_gun",
		"ik_hand_l",
		"ik_hand_r",
		"Custom_1",
		"Custom_2",
		"Custom_3",
		"Custom_4",
		"Custom_5",
	};
	*/
}


void FVrmAnimInstanceCopyProxy::Initialize(UAnimInstance* InAnimInstance) {
}
bool FVrmAnimInstanceCopyProxy::Evaluate(FPoseContext& Output) {
	Output.ResetToRefPose();

	UVrmAnimInstanceCopy *animInstance = Cast<UVrmAnimInstanceCopy>(GetAnimInstanceObject());
	if (animInstance == nullptr) {
		return false;
	}

	const UVrmMetaObject *srcMeta = animInstance->SrcVrmMetaObject;
	const UVrmMetaObject *dstMeta = animInstance->DstVrmMetaObject;
	if (srcMeta == nullptr || dstMeta == nullptr){
		return false;
	}


	auto srcSkeletalMeshComp = animInstance->SrcSkeletalMeshComponent;

	if (srcSkeletalMeshComp == nullptr) {
		return false;
	}

	// ref pose
	const auto &dstRefSkeletonTransform = GetSkelMeshComponent()->SkeletalMesh->RefSkeleton.GetRefBonePose();
	const auto &srcRefSkeletonTransform = srcSkeletalMeshComp->SkeletalMesh->RefSkeleton.GetRefBonePose();

	auto &pose = Output.Pose;

	int BoneCount = 0;
	for (const auto &t : table) {
		FName srcName, dstName;
		srcName = dstName = NAME_None;

		++BoneCount;
		{
			auto a = srcMeta->humanoidBoneTable.Find(t);
			if (a) {
				srcName = **a;
			}
		}
		//auto srcName = srcSkeletalMeshComp->SkeletalMesh->Skeleton->GetRigBoneMapping(t);
		if (srcName == NAME_None) {
			continue;
		}

		{
			auto a = dstMeta->humanoidBoneTable.Find(t);
			if (a) {
				dstName = **a;
			}
		}
		//auto dstName = GetSkelMeshComponent()->SkeletalMesh->Skeleton->GetRigBoneMapping(t);
		if (dstName == NAME_None) {
			continue;
		}

		auto dstIndex = GetSkelMeshComponent()->GetBoneIndex(dstName);
		if (dstIndex < 0) {
			continue;
		}
		auto srcIndex = srcSkeletalMeshComp->GetBoneIndex(srcName);
		if (srcIndex < 0) {
			continue;
		}


		const auto srcCurrentTrans = srcSkeletalMeshComp->GetSocketTransform(srcName, RTS_ParentBoneSpace);
		const auto srcRefTrans = srcRefSkeletonTransform[srcIndex];
		const auto dstRefTrans = dstRefSkeletonTransform[dstIndex];


		FTransform dstTrans = dstRefTrans;

		{
			FQuat diff = srcCurrentTrans.GetRotation()*srcRefTrans.GetRotation().Inverse();
			dstTrans.SetRotation(dstTrans.GetRotation()*diff);
		}
		//dstTrans.SetLocation(dstRefTrans.GetLocation());// +srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation());

		//FVector newLoc = dstTrans.GetLocation();
		if (BoneCount==1){
			FVector diff = srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation();
			FVector scale = dstRefTrans.GetLocation() / srcRefTrans.GetLocation();
			dstTrans.SetTranslation(dstRefTrans.GetLocation() + diff * scale);
			//if (newLoc.Normalize()) {
				//auto dstRefLocation = dstRefTrans.GetLocation();
				//newLoc *= dstRefLocation.Size();
				//dstTrans.SetLocation(newLoc);
			//}
		}

		FCompactPose::BoneIndexType bi(dstIndex);
		pose[bi] = dstTrans;
	}

	return true;
}
void FVrmAnimInstanceCopyProxy::UpdateAnimationNode(float DeltaSeconds) {
}

/////

UVrmAnimInstanceCopy::UVrmAnimInstanceCopy(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FAnimInstanceProxy* UVrmAnimInstanceCopy::CreateAnimInstanceProxy() {

	return new FVrmAnimInstanceCopyProxy(this);
}


void UVrmAnimInstanceCopy::NativeInitializeAnimation() {
}
void UVrmAnimInstanceCopy::NativeUpdateAnimation(float DeltaSeconds) {
}
void UVrmAnimInstanceCopy::NativePostEvaluateAnimation() {
}
void UVrmAnimInstanceCopy::NativeUninitializeAnimation() {
}
void UVrmAnimInstanceCopy::NativeBeginPlay() {
}

void UVrmAnimInstanceCopy::SetSkeletalMeshCopyData(UVrmMetaObject *dstMeta, USkeletalMeshComponent *srcSkeletalMesh, UVrmMetaObject *srcMeta) {
	SrcSkeletalMeshComponent = srcSkeletalMesh;
	SrcVrmMetaObject = srcMeta;
	DstVrmMetaObject = dstMeta;
}
