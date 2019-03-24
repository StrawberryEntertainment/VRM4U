// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_ModifyBone.h"
#include "EngineVersionComparison.h"

#include "AnimNode_VrmSpringBone.generated.h"

class USkeletalMeshComponent;
class UVrmMetaObject;

namespace VRMSpring {
	class VRMSpringManager;
}


/**
*	Simple controller that replaces or adds to the translation/rotation of a single bone.
*/
USTRUCT(BlueprintInternalUseOnly)
struct VRM4U_API FAnimNode_VrmSpringBone : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta=(PinShownByDefault))
	UVrmMetaObject *VrmMetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta=(PinShownByDefault))
	bool bIgnorePhysicsCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta=(PinShownByDefault))
	bool bIgnoreVRMCollision = false;

	TSharedPtr<VRMSpring::VRMSpringManager> SpringManager;

	float CurrentDeltaTime = 0.f;

	FAnimNode_VrmSpringBone();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
//	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

	virtual bool NeedsDynamicReset() const override { return true; }
#if	UE_VERSION_NEWER_THAN(4,20,0)
	virtual void ResetDynamics(ETeleportType InTeleportType) override;
#endif

	virtual void UpdateInternal(const FAnimationUpdateContext& Context)override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};
