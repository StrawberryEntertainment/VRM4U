// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "VrmSkeletalMeshComponent.generated.h"

/**
 * 
 */
//UCLASS(meta=(BlueprintSpawnableComponent))
UCLASS()
class VRM4ULOADER_API UVrmSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_UCLASS_BODY()
	
	
public:
	virtual void RefreshBoneTransforms( FActorComponentTickFunction* TickFunction = NULL ) override;


};
