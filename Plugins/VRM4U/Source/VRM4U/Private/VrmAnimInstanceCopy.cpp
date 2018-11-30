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
	const FName table[] = {
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
}


void FVrmAnimInstanceCopyProxy::Initialize(UAnimInstance* InAnimInstance) {
}
bool FVrmAnimInstanceCopyProxy::Evaluate(FPoseContext& Output) {
	Output.ResetToRefPose();

	UVrmAnimInstanceCopy *animInstance = Cast<UVrmAnimInstanceCopy>(GetAnimInstanceObject());
	if (animInstance == nullptr) {
		return false;
	}

	const UVrmMetaObject *meta = animInstance->MetaObject;

	//auto srcSkeleton = animInstance->SrcSkeleton;
	auto srcSkeletalMeshComp = animInstance->SrcSkeletalMeshComponent;

	if (srcSkeletalMeshComp == nullptr) {
		return false;
	}

	const auto &dstRefSkeletonTransform = GetSkelMeshComponent()->SkeletalMesh->RefSkeleton.GetRefBonePose();
	const auto &srcRefSkeletonTransform = srcSkeletalMeshComp->SkeletalMesh->RefSkeleton.GetRefBonePose();

	auto &pose = Output.Pose;

	for (auto t : table) {
		auto srcName = srcSkeletalMeshComp->SkeletalMesh->Skeleton->GetRigBoneMapping(t);
		if (srcName == NAME_None) {
			continue;
		}

		auto dstName = GetSkelMeshComponent()->SkeletalMesh->Skeleton->GetRigBoneMapping(t);
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

		FQuat diff = srcCurrentTrans.GetRotation();// *srcRefTrans.GetRotation().Inverse();

		FTransform dstTrans = dstRefTrans;
		dstTrans.SetRotation( dstTrans.GetRotation()*diff );
		//dstTrans.SetLocation(dstRefTrans.GetLocation());// +srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation());

		FVector newLoc = dstTrans.GetLocation();
		if (newLoc.Normalize()) {
			//auto dstRefLocation = dstRefTrans.GetLocation();
			//newLoc *= dstRefLocation.Size();
			//dstTrans.SetLocation(newLoc);
			
		}

		FCompactPose::BoneIndexType bi(dstIndex);
		pose[bi] = dstTrans;
	}
	
/*

	for (auto &boneName : meta->humanoidBoneTable) {
	
		if (boneName.Key == TEXT("leftEye") || boneName.Key == TEXT("rightEye")) {
			continue;
		}
		srcSkeleton

		//auto i = srcMesh->GetBoneIndex();
		if (srcMesh->GetBoneIndex(*(boneName.Key)) < 0) {
			continue;
		}

		auto t = srcMesh->GetSocketTransform(*(boneName.Key), RTS_ParentBoneSpace);
		
		auto i = GetSkelMeshComponent()->GetBoneIndex(*(boneName.Value));
		if (i < 0) {
			continue;
		}
		auto refLocation = RefSkeletonTransform[i].GetLocation();
		
		FVector newLoc = t.GetLocation();
		if (newLoc.Normalize()) {
			newLoc *= refLocation.Size();
			t.SetLocation(newLoc);
		}

		FCompactPose::BoneIndexType bi(i);
		pose[bi] = t;
	}
*/
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

	{
		if (SrcSkeletalMeshComponent == nullptr) {
			return;
		}
		auto a = SrcSkeletalMeshComponent->GetAnimInstance();
		if (a == nullptr) {
			return;
		}

		if (SrcSkeletalMeshComponent->SkeletalMesh == nullptr) {
			return;
		}

		a->CurrentSkeleton = SrcSkeletalMeshComponent->SkeletalMesh->Skeleton;
	}

//	USkeletalMesh *m;

	auto targetComponent = Cast<USkeletalMeshComponent>(this);

	//BaseSkeletalMeshComponent->GetSocketTransform();
	//const auto boneTree = BaseSkeletalMeshComponent->SkeletalMesh->Skeleton->GetBoneTree();

	if (targetComponent) {
		for (auto &a : targetComponent->BoneSpaceTransforms) {
		//	a.SetIdentity();
		}
	}

	//for (auto &b : boneTree) {
	//}
}
void UVrmAnimInstanceCopy::NativePostEvaluateAnimation() {
	auto targetComponent = Cast<USkeletalMeshComponent>(this);
	if (targetComponent) {
		for (auto &a : targetComponent->BoneSpaceTransforms) {
			//a.SetIdentity();
		}
	}

	if (SrcSkeletalMeshComponent) {
		if (SrcSkeletalMeshComponent->AnimScriptInstance) {
			IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(this->GetClass());
			//const USkeleton* AnimSkeleton = (AnimClassInterface) ? AnimClassInterface->GetTargetSkeleton() : nullptr;
			if (AnimClassInterface) {
				//AnimClassInterface->
			}
			//BaseSkeletalMeshComponent->AnimScriptInstance->target
			//BaseSkeletalMeshComponent->AnimScriptInstance->CurrentSkeleton = 
				//BaseSkeletalMeshComponent->SkeletalMesh->Skeleton;

		}
	}
}
void UVrmAnimInstanceCopy::NativeUninitializeAnimation() {
}
void UVrmAnimInstanceCopy::NativeBeginPlay() {
}

static TArray<FString> shapeBlend = {
	"Neutral","A","I","U","E","O","Blink","Joy","Angry","Sorrow","Fun","LookUp","LookDown","LookLeft","LookRight","Blink_L","Blink_R",
};


void UVrmAnimInstanceCopy::SetMorphTargetVRM(EVrmMorphGroupType type, float Value) {

	if (MetaObject == nullptr){
		return;
	}

	USkeletalMeshComponent *skc = GetOwningComponent();
	//auto &morphMap = skc->GetMorphTargetCurves();
	for (auto &a : MetaObject->BlendShapeGroup) {
		if (a.name != shapeBlend[(int)type]) {
			continue;
		}

		for (auto &b : a.BlendShape) {
			//b.meshName

			SetMorphTarget(*(b.morphTargetName), Value);

			/*
			for (auto &m : skc->SkeletalMesh->MorphTargets) {
				auto s = m->GetName();
				//if (s.Find(b.meshName) < 0) {
				//	continue;
				//}
				auto s1 = FString::Printf(TEXT("%02d_"), b.meshID);
				if (s.Find(s1) < 0) {
					continue;
				}
				auto s2 = FString::Printf(TEXT("_%02d"), b.shapeIndex);
				if (s.Find(s2) < 0) {
					continue;
				}
				SetMorphTarget(*s, Value);
			}
			*/
		}
	}


}

void UVrmAnimInstanceCopy::SetVrmData(USkeletalMeshComponent *baseSkeletalMesh, UVrmMetaObject *meta) {
	IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(this->GetClass());
	//const USkeleton* AnimSkeleton = (AnimClassInterface) ? AnimClassInterface->GetTargetSkeleton() : nullptr;

	if (meta) {
		USkeletalMeshComponent *skc = GetOwningComponent();
		skc->SetSkeletalMesh(meta->SkeletalMesh);
	}

	SrcSkeletalMeshComponent = baseSkeletalMesh;
	MetaObject = meta;

	//if (AnimClassInterface) {
	//	AnimClassInterface->
	//}
	//BaseSkeletalMeshComponent->AnimScriptInstance->target
	//BaseSkeletalMeshComponent->AnimScriptInstance->CurrentSkeleton = 
	//BaseSkeletalMeshComponent->SkeletalMesh->Skeleton;
}
