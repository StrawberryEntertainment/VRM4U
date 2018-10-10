// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VrmAssetListObject.generated.h"

class UMaterialInterface;
class UTexture2D;
class USkeletalMesh;
class USkeleton;
class UVrmMetaObject;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAssetListObject : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InSave")
	bool bAssetSave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BaseMToonOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BaseMToonTransparentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BaseUnlitOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BaseUnlitTransparentMaterial;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	//USkeleton* BaseSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	USkeletalMesh* BaseSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UVrmMetaObject* VrmMetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<UTexture2D*> Textures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<UMaterialInterface*> Materials;

};
