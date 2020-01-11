// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "VrmAssetListThumbnailRenderer.h"
#include "Engine/EngineTypes.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"
#include "CanvasTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Brushes/SlateColorBrush.h"

#include "VrmAssetListObject.h"
#include "VrmLicenseObject.h"
#include "VrmMetaObject.h"

//////////////////////////////////////////////////////////////////////////
// UPaperSpriteThumbnailRenderer

UClass* FAssetTypeActions_VrmAssetList::GetSupportedClass() const {
	return UVrmAssetListObject::StaticClass();
}
FText FAssetTypeActions_VrmAssetList::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_VrmAssetList", "Vrm Asset List");
}
UClass* FAssetTypeActions_VrmLicense::GetSupportedClass() const {
	return UVrmLicenseObject::StaticClass();
}
FText FAssetTypeActions_VrmLicense::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_VrmLicense", "Vrm License");
}
UClass* FAssetTypeActions_VrmMeta::GetSupportedClass() const {
	return UVrmMetaObject::StaticClass();
}
FText FAssetTypeActions_VrmMeta::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_VrmMeta", "Vrm Meta");
}

TSharedPtr<SWidget> FAssetTypeActions_VrmBase::GetThumbnailOverlay(const FAssetData& AssetData) const {

	FString str;

	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmAssetListObject> a = Cast<UVrmAssetListObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" AssetList ");
		}
	}
	if (str.Len() == 0){
		TWeakObjectPtr<UVrmLicenseObject> a = Cast<UVrmLicenseObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" License ");
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmMetaObject> a = Cast<UVrmMetaObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" Meta ");
		}

	}

	FText txt = FText::FromString(str);
	return SNew(SBorder)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		//.Padding(FMargin(4))
		//.Padding(FMargin(4))
		//.BorderImage(new FSlateColorBrush(FColor::White))
		//.AutoWidth()
		[
			SNew(STextBlock)
			.Text(txt)
			.HighlightText(txt)
			.HighlightColor(FColor(64,64,64))
			.ShadowOffset(FVector2D(1.0f, 1.0f))
		];

	//return FAssetTypeActions_Base::GetThumbnailOverlay(AssetData);
}


UVrmAssetListThumbnailRenderer::UVrmAssetListThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UVrmAssetListThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const {
	UVrmAssetListObject* a = Cast<UVrmAssetListObject>(Object);

	if (a) {
		if (a->VrmLicenseObject) {
			if (a->VrmLicenseObject->thumbnail) {
				return Super::GetThumbnailSize(a->VrmLicenseObject->thumbnail, Zoom, OutWidth, OutHeight);
			}
		}
	}
	Super::GetThumbnailSize(Object, Zoom, OutWidth, OutHeight);
}


void UVrmAssetListThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	{
		UVrmAssetListObject* a = Cast<UVrmAssetListObject>(Object);
		if (a) {
			if (a->VrmLicenseObject) {
				if (a->VrmLicenseObject->thumbnail) {
					return Super::Draw(a->VrmLicenseObject->thumbnail, X, Y, Width, Height, RenderTarget, Canvas);
				}
			}
		}
	}
	{
		UVrmLicenseObject* a = Cast<UVrmLicenseObject>(Object);
		if (a) {
			if (a->thumbnail) {
				return Super::Draw(a->thumbnail, X, Y, Width, Height, RenderTarget, Canvas);
			}
		}
	}
	{
		UVrmMetaObject* a = Cast<UVrmMetaObject>(Object);
		if (a) {
			TArray<UObject*> ret;
			UPackage *pk = a->GetOutermost();
			GetObjectsWithOuter(pk, ret);
			for (auto *obj : ret) {
				UVrmLicenseObject* t = Cast<UVrmLicenseObject>(obj);
				if (t == nullptr) {
					continue;
				}
				if (t->thumbnail) {
					return Super::Draw(t->thumbnail, X, Y, Width, Height, RenderTarget, Canvas);
				}
			}
		}
	}

	return Super::Draw(Object, X, Y, Width, Height, RenderTarget, Canvas);
}

/*
void UVrmAssetListThumbnailRenderer::DrawGrid(int32 X, int32 Y, uint32 Width, uint32 Height, FCanvas* Canvas)
{
	static UTexture2D* GridTexture = nullptr;
	if (GridTexture == nullptr)
	{
		GridTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineMaterials/DefaultDiffuse_TC_Masks.DefaultDiffuse_TC_Masks"), nullptr, LOAD_None, nullptr);
	}

	const bool bAlphaBlend = false;

	Canvas->DrawTile(
		(float)X,
		(float)Y,
		(float)Width,
		(float)Height,
		0.0f,
		0.0f,
		4.0f,
		4.0f,
		FLinearColor::White,
		GridTexture->Resource,
		bAlphaBlend);
}

void UVrmAssetListThumbnailRenderer::DrawFrame(class UPaperSprite* Sprite, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, FBoxSphereBounds* OverrideRenderBounds)
{
	const UTexture2D* SourceTexture = nullptr;
	if (Sprite != nullptr)
	{
		SourceTexture = Sprite->GetBakedTexture() ? Sprite->GetBakedTexture() : Sprite->GetSourceTexture();
	}

	if (SourceTexture != nullptr)
	{
		const bool bUseTranslucentBlend = SourceTexture->HasAlphaChannel();

		// Draw the grid behind the sprite
		if (bUseTranslucentBlend)
		{
			DrawGrid(X, Y, Width, Height, Canvas);
		}

		// Draw the sprite itself
		// Use the baked render data, so we don't have to care about rotations and possibly
		// other sprites overlapping in source, UV region, etc.
		const TArray<FVector4>& BakedRenderData = Sprite->BakedRenderData;
		TArray<FVector2D> CanvasPositions;
		TArray<FVector2D> CanvasUVs;

		for (int Vertex = 0; Vertex < BakedRenderData.Num(); ++Vertex)
		{
			new (CanvasPositions) FVector2D(BakedRenderData[Vertex].X, BakedRenderData[Vertex].Y);
			new (CanvasUVs) FVector2D(BakedRenderData[Vertex].Z, BakedRenderData[Vertex].W);
		}

		// Determine the bounds to use
		FBoxSphereBounds* RenderBounds = OverrideRenderBounds;
		FBoxSphereBounds FrameBounds;
		if (RenderBounds == nullptr)
		{
			FrameBounds = Sprite->GetRenderBounds();
			RenderBounds = &FrameBounds;
		}

		const FVector MinPoint3D = RenderBounds->GetBoxExtrema(0);
		const FVector MaxPoint3D = RenderBounds->GetBoxExtrema(1);
		const FVector2D MinPoint(FVector::DotProduct(MinPoint3D, PaperAxisX), FVector::DotProduct(MinPoint3D, PaperAxisY));
		const FVector2D MaxPoint(FVector::DotProduct(MaxPoint3D, PaperAxisX), FVector::DotProduct(MaxPoint3D, PaperAxisY));

		const float UnscaledWidth = MaxPoint.X - MinPoint.X;
		const float UnscaledHeight = MaxPoint.Y - MinPoint.Y;
		const FVector2D Origin(X + Width * 0.5f, Y + Height * 0.5f);
		const bool bIsWider = (UnscaledWidth > 0.0f) && (UnscaledHeight > 0.0f) && (UnscaledWidth > UnscaledHeight);
		const float ScaleFactor = bIsWider ? (Width / UnscaledWidth) : (Height / UnscaledHeight);

		// Scale and recenter
		const FVector2D CanvasPositionCenter = (MaxPoint + MinPoint) * 0.5f;
		for (int Vertex = 0; Vertex < CanvasPositions.Num(); ++Vertex)
		{
			CanvasPositions[Vertex] = (CanvasPositions[Vertex] - CanvasPositionCenter) * ScaleFactor + Origin;
			CanvasPositions[Vertex].Y = Height - CanvasPositions[Vertex].Y;
		}

		// Draw triangles
		if ((CanvasPositions.Num() > 0) && (SourceTexture->Resource != nullptr))
		{
			TArray<FCanvasUVTri> Triangles;
			const FLinearColor SpriteColor(FLinearColor::White);
			for (int Vertex = 0; Vertex < CanvasPositions.Num(); Vertex += 3)
			{
				FCanvasUVTri* Triangle = new (Triangles) FCanvasUVTri();
				Triangle->V0_Pos = CanvasPositions[Vertex + 0]; Triangle->V0_UV = CanvasUVs[Vertex + 0]; Triangle->V0_Color = SpriteColor;
				Triangle->V1_Pos = CanvasPositions[Vertex + 1]; Triangle->V1_UV = CanvasUVs[Vertex + 1]; Triangle->V1_Color = SpriteColor;
				Triangle->V2_Pos = CanvasPositions[Vertex + 2]; Triangle->V2_UV = CanvasUVs[Vertex + 2]; Triangle->V2_Color = SpriteColor;
			}
			FCanvasTriangleItem CanvasTriangle(Triangles, SourceTexture->Resource);
			CanvasTriangle.BlendMode = bUseTranslucentBlend ? ESimpleElementBlendMode::SE_BLEND_Translucent : ESimpleElementBlendMode::SE_BLEND_Opaque;
			Canvas->DrawItem(CanvasTriangle);
		}
	}
	else
	{
		// Fallback for a bogus sprite
		DrawGrid(X, Y, Width, Height, Canvas);
	}
}

*/

