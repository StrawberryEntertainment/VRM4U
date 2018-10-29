// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "Misc/EngineVersionComparison.h"
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
#if	UE_VERSION_NEWER_THAN(4,20,0)
	virtual USkeletalMesh* GetPreviewMesh(bool bFindIfNotSet = false) override;
	virtual USkeletalMesh* GetPreviewMesh() const override;
	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true);
#else
#endif

	virtual bool IsPostLoadThreadSafe() const override;

	TSoftObjectPtr<class USkeletalMesh> PreviewSkeletalMesh;

	///
public:
	void applyBoneFrom(const class USkeleton *src, const class UVrmMetaObject *meta);
	void readVrmBone(struct aiScene* s, int &offset);
	
	//FReferenceSkeleton& getRefSkeleton();
};
