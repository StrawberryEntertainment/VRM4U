// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "VrmSkeleton.generated.h"

/**
 * 
 */
UCLASS()
class VRM4U_API UVrmSkeleton : public USkeleton
{
	GENERATED_BODY()
	
public:
	/** IInterface_PreviewMeshProvider interface */
	virtual USkeletalMesh* GetPreviewMesh(bool bFindIfNotSet = false) override;
	virtual USkeletalMesh* GetPreviewMesh() const override;
	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty=true);

	virtual bool IsPostLoadThreadSafe() const override;

	TSoftObjectPtr<class USkeletalMesh> PreviewSkeletalMesh;

	///
public:
	void Proc(const struct aiScene* s, int &offset);
	
	//FReferenceSkeleton& getRefSkeleton();
};
