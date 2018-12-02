// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmSkeletalMeshComponent.h"
#include "VrmAnimInstance.h"




UVrmSkeletalMeshComponent::UVrmSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

	AnimClass = UVrmAnimInstance::StaticClass();
}

void UVrmSkeletalMeshComponent::RefreshBoneTransforms(FActorComponentTickFunction* TickFunction) 
{
	Super::RefreshBoneTransforms(TickFunction);
}