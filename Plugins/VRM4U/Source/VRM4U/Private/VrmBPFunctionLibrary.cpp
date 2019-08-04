// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmBPFunctionLibrary.h"
#include "Materials/MaterialInterface.h"

#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logging/MessageLog.h"
#include "Engine/Canvas.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/EngineVersionComparison.h"

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
#if	UE_VERSION_NEWER_THAN(4,23,0)
	ShadingModel = Material->GetShadingModels().GetFirstShadingModel();
#else
	ShadingModel	= Material->GetShadingModel();
#endif
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



void UVrmBPFunctionLibrary::VRMDrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material)
{
#if	UE_VERSION_NEWER_THAN(4,20,0)
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidWorldContextObject", "DrawMaterialToRenderTarget: WorldContextObject is not valid."));
	} else if (!Material)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidMaterial", "DrawMaterialToRenderTarget: Material must be non-null."));
	} else if (!TextureRenderTarget)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidTextureRenderTarget", "DrawMaterialToRenderTarget: TextureRenderTarget must be non-null."));
	} else if (!TextureRenderTarget->Resource)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_ReleasedTextureRenderTarget", "DrawMaterialToRenderTarget: render target has been released."));
	} else
	{
		UCanvas* Canvas = World->GetCanvasForDrawMaterialToRenderTarget();

		FCanvas RenderCanvas(
			TextureRenderTarget->GameThread_GetRenderTargetResource(),
			nullptr,
			World,
			World->FeatureLevel);

		Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr, &RenderCanvas);
		Canvas->Update();



		TDrawEvent<FRHICommandList>* DrawMaterialToTargetEvent = new TDrawEvent<FRHICommandList>();

		FName RTName = TextureRenderTarget->GetFName();
		ENQUEUE_RENDER_COMMAND(BeginDrawEventCommand)(
			[RTName, DrawMaterialToTargetEvent](FRHICommandListImmediate& RHICmdList)
		{
			// Update resources that were marked for deferred update. This is important
			// in cases where the blueprint is invoked in the same frame that the render
			// target is created. Among other things, this will perform deferred render
			// target clears.
			FDeferredUpdateResource::UpdateResources(RHICmdList);

			BEGIN_DRAW_EVENTF(
				RHICmdList,
				DrawCanvasToTarget,
				(*DrawMaterialToTargetEvent),
				*RTName.ToString());
		});

		Canvas->K2_DrawMaterial(Material, FVector2D(0, 0), FVector2D(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY), FVector2D(0, 0));

		RenderCanvas.Flush_GameThread();
		Canvas->Canvas = NULL;

		FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget->GameThread_GetRenderTargetResource();
		ENQUEUE_RENDER_COMMAND(CanvasRenderTargetResolveCommand)(
			[RenderTargetResource, DrawMaterialToTargetEvent](FRHICommandList& RHICmdList)
		{
			RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, FResolveParams());
			STOP_DRAW_EVENT((*DrawMaterialToTargetEvent));
			delete DrawMaterialToTargetEvent;
		}
		);
	}
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialParent(UMaterialInstanceConstant *dst, UMaterialInterface* NewParent) {
	if (dst == nullptr) {
		return;
	}

	if (dst->Parent == NewParent) {
		return;
	}
	dst->MarkPackageDirty();

#if WITH_EDITOR
	dst->PreEditChange(NULL);
	dst->SetParentEditorOnly(NewParent);
	dst->PostEditChange();
#else
	dst->Parent = NewParent;
	dst->PostLoad();
#endif


}
