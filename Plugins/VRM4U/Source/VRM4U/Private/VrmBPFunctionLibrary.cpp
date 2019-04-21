// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmBPFunctionLibrary.h"
#include "Materials/MaterialInterface.h"
//#include "VRM4U.h"

void UVrmBPFunctionLibrary::VRMTransMatrix(const FTransform &transform, TArray<FLinearColor> &matrix, TArray<FLinearColor> &matrix_inv){

	FMatrix m = transform.ToMatrixWithScale();
	FMatrix mi = transform.ToMatrixWithScale().Inverse();

	matrix.SetNum(4);
	matrix_inv.SetNum(4);

	for (int i = 0; i < 4; ++i) {
		matrix[i] = FLinearColor(m.M[i][0], m.M[i][1], m.M[i][2], m.M[i][3]);
		matrix_inv[i] = FLinearColor(mi.M[i][0], mi.M[i][1], mi.M[i][2], mi.M[i][3]);
	}

	return;
}

void UVrmBPFunctionLibrary::VRMGetMaterialPropertyOverrides(const UMaterialInterface *Material, TEnumAsByte<EBlendMode> &BlendMode, TEnumAsByte<EMaterialShadingModel> &ShadingModel, bool &IsTwoSided, bool &IsMasked){
	if (Material == nullptr) {
		return;
	}
	BlendMode		= Material->GetBlendMode();
	ShadingModel	= Material->GetShadingModel();
	IsTwoSided		= Material->IsTwoSided();
	IsMasked		= Material->IsMasked();
}

void UVrmBPFunctionLibrary::VRMGetMobileMode(bool &IsMobile, bool &IsAndroid, bool &IsIOS) {
	IsMobile = false;
	IsAndroid = false;
	IsIOS = false;

#if PLATFORM_ANDROID
	IsMobile = true;
	IsAndroid = true;
#endif

#if PLATFORM_IOS
	IsMobile = true;
	IsIOS = true;
#endif

}


