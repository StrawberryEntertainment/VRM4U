// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VrmAssetListObject.generated.h"

class UMaterialInterface;
class UTexture2D;
class USkeletalMesh;
class USkeleton;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAssetListObject : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetInput")
	bool bAssetSave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetInput")
	UMaterialInterface* BaseOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetInput")
	UMaterialInterface* BaseTransparentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetInput")
	USkeleton* BaseSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetOutput")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetOutput")
	TArray<UTexture2D*> Textures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetOutput")
	TArray<UMaterialInterface*> Materials;

};
