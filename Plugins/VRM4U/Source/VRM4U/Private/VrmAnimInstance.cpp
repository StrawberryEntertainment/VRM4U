// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmAnimInstance.h"
#include "VrmMetaObject.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/Morphtarget.h"


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

	{
		if (BaseSkeletalMeshComponent == nullptr) {
			return;
		}
		auto a = BaseSkeletalMeshComponent->GetAnimInstance();
		if (a == nullptr) {
			return;
		}

		if (BaseSkeletalMeshComponent->SkeletalMesh == nullptr) {
			return;
		}

		a->CurrentSkeleton = BaseSkeletalMeshComponent->SkeletalMesh->Skeleton;
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
void UVrmAnimInstance::NativePostEvaluateAnimation() {
	auto targetComponent = Cast<USkeletalMeshComponent>(this);
	if (targetComponent) {
		for (auto &a : targetComponent->BoneSpaceTransforms) {
			//a.SetIdentity();
		}
	}

	if (BaseSkeletalMeshComponent) {
		if (BaseSkeletalMeshComponent->AnimScriptInstance) {
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
void UVrmAnimInstance::NativeUninitializeAnimation() {
}
void UVrmAnimInstance::NativeBeginPlay() {
}

static TArray<FString> shapeBlend = {
	"Neutral","A","I","U","E","O","Blink","Joy","Angry","Sorrow","Fun","LookUp","LookDown","LookLeft","LookRight","Blink_L","Blink_R",
};


void UVrmAnimInstance::SetMorphTargetVRM(EVrmMorphGroupType type, float Value) {

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
		}
	}


}

void UVrmAnimInstance::SetVrmData(USkeletalMeshComponent *baseSkeletalMesh, UVrmMetaObject *meta) {
	IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(this->GetClass());
	//const USkeleton* AnimSkeleton = (AnimClassInterface) ? AnimClassInterface->GetTargetSkeleton() : nullptr;

	USkeletalMeshComponent *skc = GetOwningComponent();

	skc->SetSkeletalMesh(meta->SkeletalMesh);
	BaseSkeletalMeshComponent = baseSkeletalMesh;
	MetaObject = meta;

	//if (AnimClassInterface) {
	//	AnimClassInterface->
	//}
	//BaseSkeletalMeshComponent->AnimScriptInstance->target
	//BaseSkeletalMeshComponent->AnimScriptInstance->CurrentSkeleton = 
	//BaseSkeletalMeshComponent->SkeletalMesh->Skeleton;
}
