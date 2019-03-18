// Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmAssetListObject.h"
//#include "LoaderBPFunctionLibrary.h"



UVrmAssetListObject::UVrmAssetListObject(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	Package = GetTransientPackage();

	//Result = new FReturnedData();
}


void UVrmAssetListObject::CopyMember(UVrmAssetListObject *dst) const {
	dst->bAssetSave = bAssetSave;
	dst->bSkipMorphTarget = bSkipMorphTarget;

	dst->BaseMToonOpaqueMaterial = BaseMToonOpaqueMaterial;

	dst->BaseMToonTransparentMaterial = BaseMToonTransparentMaterial;
	dst->BaseMToonUnlitOpaqueMaterial = BaseMToonUnlitOpaqueMaterial;
	dst->BaseUnlitOpaqueMaterial = BaseUnlitOpaqueMaterial;
	dst->BaseUnlitTransparentMaterial = BaseUnlitTransparentMaterial;
	dst->BasePBROpaqueMaterial = BasePBROpaqueMaterial;

	dst->OptimizedMToonOpaqueMaterial = OptimizedMToonOpaqueMaterial;
	dst->OptimizedMToonTransparentMaterial = OptimizedMToonTransparentMaterial;
	dst->OptimizedMToonOUtlineMaterial = OptimizedMToonOUtlineMaterial;

	dst->BaseSkeletalMesh = BaseSkeletalMesh;
	dst->VrmMetaObject = VrmMetaObject;
	dst->VrmLicenseObject = VrmLicenseObject;
	dst->SkeletalMesh = SkeletalMesh;
	dst->Textures = Textures;
	dst->Materials = Materials;
	dst->HumanoidRig = HumanoidRig;
	dst->Package = Package;
	dst->OrigFileName = OrigFileName;
	dst->BaseFileName = BaseFileName;
	dst->HumanoidSkeletalMesh = HumanoidSkeletalMesh;
}
