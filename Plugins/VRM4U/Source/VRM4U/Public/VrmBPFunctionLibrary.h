// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "VrmBPFunctionLibrary.generated.h"

class UTextureRenderTarget2D;
class UMaterialInstanceConstant;
/**
 * 
 */
UCLASS()
class VRM4U_API UVrmBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable,Category="VRM4U")
	static void VRMTransMatrix(const FTransform &transform, TArray<FLinearColor> &matrix, TArray<FLinearColor> &matrix_inv);


	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetMaterialPropertyOverrides(const UMaterialInterface *Material, TEnumAsByte<EBlendMode> &BlendMode, TEnumAsByte<EMaterialShadingModel> &ShadingModel, bool &IsTwoSided, bool &IsMasked);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetMobileMode(bool &IsMobile, bool &IsAndroid, bool &IsIOS);

	UFUNCTION(BlueprintCallable, Category = "Rendering", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void VRMDrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialParent(UMaterialInstanceConstant *dst, UMaterialInterface* NewParent);
};
