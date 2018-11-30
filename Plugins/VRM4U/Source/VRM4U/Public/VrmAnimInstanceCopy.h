// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"

#include "VrmAnimInstanceCopy.generated.h"

class UVrmMetaObject;
class USkeleton;


USTRUCT()
struct VRM4U_API FVrmAnimInstanceCopyProxy : public FAnimInstanceProxy {

public:
	GENERATED_BODY()

	FVrmAnimInstanceCopyProxy()
	{
	}

	FVrmAnimInstanceCopyProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(float DeltaSeconds) override;
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAnimInstanceCopy : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	UVrmMetaObject *MetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	USkeleton *SrcSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	USkeletalMeshComponent *SrcSkeletalMeshComponent;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
		USceneComponent *ComponentHandJointTargetLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
		USceneComponent *ComponentHandJointTargetRight;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
public:
	virtual void NativeInitializeAnimation()override;
	// Native update override point. It is usually a good idea to simply gather data in this step and 
	// for the bulk of the work to be done in NativeUpdateAnimation.
	virtual void NativeUpdateAnimation(float DeltaSeconds)override;
	// Native Post Evaluate override point
	virtual void NativePostEvaluateAnimation()override;
	// Native Uninitialize override point
	virtual void NativeUninitializeAnimation()override;

	// Executed when begin play is called on the owning component
	virtual void NativeBeginPlay()override;

	//virtual USkeleton* GetTargetSkeleton() const override;

	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetMorphTargetVRM(EVrmMorphGroupType type, float Value);

	
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetVrmData(USkeletalMeshComponent *baseSkeletalMesh, UVrmMetaObject *meta);

};
