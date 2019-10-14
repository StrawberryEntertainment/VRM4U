// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Factories/ImportSettings.h"
#include "VrmConvert.h"
#include "VrmImportUI.generated.h"

DECLARE_DELEGATE(FOnResolveFbxReImport);

UCLASS(config=EditorPerProjectUserSettings, AutoExpandCategories=(FTransform), HideCategories=Object, MinimalAPI)
class UVrmImportUI : public UObject, public IImportSettingsParser
{
	GENERATED_UCLASS_BODY()

public:
	/** Whether or not the imported file is in OBJ format */
	//UPROPERTY(BlueprintReadWrite, Category = Miscellaneous)
	//bool bIsObjImport;

	/** The original detected type of this import */
	//UPROPERTY(BlueprintReadWrite, Category = Miscellaneous)
	//TEnumAsByte<enum EFBXImportType22> OriginalImportType;

	/** Type of asset to import from the FBX file */
	//UPROPERTY(BlueprintReadWrite, Category = Miscellaneous)
	//TEnumAsByte<enum EFBXImportType22> MeshTypeToImport;

	/** Use the string in "Name" field as full name of mesh. The option only works when the scene contains one mesh. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, config, Category=Miscellaneous, meta=(OBJRestrict="true"))
	//uint32 bOverrideFullName:1;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Thumbnail"))
	//UVrmLicenseObject *License;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "VRM Title / Author"))
	FString TitleAuthor;

	/** Remove bone has no mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Remove bone used DCC tool"))
	bool bSimpleRoot = true;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Delete bone without mesh"))
	bool bSkipNoMeshBone = false;

	/** Duplicate mesh and renamed humanoid bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Create renamed humanoid mesh"))
	bool bCreateHumanoidRenamedMesh = false;

	/** Physics asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="No physics asset"))
	bool bSkipPhysics = false;

	/** MorphTarget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="No MorphTarget"))
	bool bSkipMorphTarget = false;

	/** MorphTarget Normal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Eable MorphTarget Normal(TangentZDelta)"))
	bool bEnableMorphTargetNormal = false;

	/** Material merge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Merge material"))
	bool bMergeMaterial = true;

	/** Material optimize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Optimize material"))
	bool bOptimizeMaterial = true;

	/** for Mobile. Import root bone only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Reduce bonemap<=75 for mobile"))
	bool bMobileBone = false;

	/** for DEBUG. Import root bone only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="[Debug]One bone only"))
	bool bDebugOneBone = false;

	/** Skeleton to use for imported asset. When importing a mesh, leaving this as "None" will create a new skeleton. When importing an animation this MUST be specified to import the asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	class USkeleton* Skeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Model scale"))
	float ModelScale = 1.0f;

	/** Materal Type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
	TEnumAsByte<enum EVRMImportMaterialType> MaterialType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = Thumbnail, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Thumbnail"))
	UTexture2D *Thumbnail;


	/** If checked, create new PhysicsAsset if it doesn't have it */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, config, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	//uint32 bCreatePhysicsAsset:1;

	/** If this is set, use this PhysicsAsset. It is possible bCreatePhysicsAsset == false, and PhysicsAsset == NULL. It is possible they do not like to create anything. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Mesh, meta=(ImportType="SkeletalMesh", editcondition="!bCreatePhysicsAsset"))
	//class UPhysicsAsset* PhysicsAsset;

	UFUNCTION(BlueprintCallable, Category = Miscellaneous)
	void ResetToDefault();

	/** UObject Interface */
	virtual bool CanEditChange( const UProperty* InProperty ) const override;

	/** IImportSettings Interface */
	virtual void ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson) override;

	/** sets MeshTypeToImport */
	void SetMeshTypeToImport()
	{
		//MeshTypeToImport = bImportAsSkeletal ? FBXIT_SkeletalMesh : FBXIT_StaticMesh;
	}

	/* Whether this UI is construct for a reimport */
	bool bIsReimport;
};


