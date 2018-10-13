// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmAnimInstance.h"
#include "VrmMetaObject.h"
#include "Animation/AnimNodeBase.h"


void FVrmAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance) {
}
bool FVrmAnimInstanceProxy::Evaluate(FPoseContext& Output) {
	
	Output.ResetToRefPose();

	UVrmAnimInstance *animInstance = Cast<UVrmAnimInstance>(GetAnimInstanceObject());
	if (animInstance == nullptr) {
		return false;
	}

	UVrmMetaObject *meta = animInstance->MetaObject;
	if (meta == nullptr) {
		return false;
	}

	USkeletalMeshComponent *srcMesh = animInstance->BaseSkeletalMeshComponent;
	if (srcMesh == nullptr) {
		return false;
	}

	auto &pose = Output.Pose;

	for (auto &boneName : meta->humanoidBoneTable) {
		//auto i = srcMesh->GetBoneIndex();
		if (srcMesh->GetBoneIndex(*(boneName.Key)) < 0) {
			continue;
		}

		auto t = srcMesh->GetSocketTransform(*(boneName.Key), RTS_ParentBoneSpace);
		
		auto i = GetSkelMeshComponent()->GetBoneIndex(*(boneName.Value));
		if (i < 0) {
			continue;
		}
		FCompactPose::BoneIndexType bi(i);
		pose[bi] = t;
	}

	return true;
}
void FVrmAnimInstanceProxy::UpdateAnimationNode(float DeltaSeconds) {
}

/////

UVrmAnimInstance::UVrmAnimInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FAnimInstanceProxy* UVrmAnimInstance::CreateAnimInstanceProxy() {
	return new FVrmAnimInstanceProxy(this);
}


void UVrmAnimInstance::NativeInitializeAnimation() {
}
void UVrmAnimInstance::NativeUpdateAnimation(float DeltaSeconds) {

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
void UVrmAnimInstance::NativePostEvaluateAnimation() {
	auto targetComponent = Cast<USkeletalMeshComponent>(this);
	if (targetComponent) {
		for (auto &a : targetComponent->BoneSpaceTransforms) {
			//a.SetIdentity();
		}
	}
}
void UVrmAnimInstance::NativeUninitializeAnimation() {
}
void UVrmAnimInstance::NativeBeginPlay() {
}
