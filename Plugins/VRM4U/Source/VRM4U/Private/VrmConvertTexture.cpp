// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmConvertTexture.h"
#include "VrmConvert.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "PixelFormat.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "VrmAssetListObject.h"

namespace {
	static UTexture2D* createTex(int32 InSizeX, int32 InSizeY, FString name, UPackage *package) {
		auto format = PF_B8G8R8A8;
		UTexture2D* NewTexture = NULL;
		if (InSizeX > 0 && InSizeY > 0 &&
			(InSizeX % GPixelFormats[format].BlockSizeX) == 0 &&
			(InSizeY % GPixelFormats[format].BlockSizeY) == 0)
		{
			NewTexture = NewObject<UTexture2D>(
				// GetTransientPackage(),
				package,
				*name,
				//RF_Transient
				RF_Public | RF_Standalone
				);

			NewTexture->PlatformData = new FTexturePlatformData();
			NewTexture->PlatformData->SizeX = InSizeX;
			NewTexture->PlatformData->SizeY = InSizeY;
			NewTexture->PlatformData->PixelFormat = format;

			int32 NumBlocksX = InSizeX / GPixelFormats[format].BlockSizeX;
			int32 NumBlocksY = InSizeY / GPixelFormats[format].BlockSizeY;
			FTexture2DMipMap* Mip = new(NewTexture->PlatformData->Mips) FTexture2DMipMap();
			Mip->SizeX = InSizeX;
			Mip->SizeY = InSizeY;
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[format].BlockBytes);
			Mip->BulkData.Unlock();
		}
		else
		{
			UE_LOG(LogTexture, Warning, TEXT("Invalid parameters specified for UTexture2D::Create()"));
		}
		return NewTexture;
	}


	bool createAndAddMaterial(UMaterialInstanceConstant *dm, int matIndex, UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
		auto i = matIndex;
		const VRM::VRMMetadata *meta = static_cast<const VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		if (i >= meta->materialNum) {
			return false;
		}
		auto &vrmMat = meta->material[i];
		{
			struct TT {
				FString key;
				float* value;
			};
			TT table[] = {
				TEXT("_Color"),			vrmMat.vectorProperties._Color,
				TEXT("_ShadeColor"),	vrmMat.vectorProperties._ShadeColor,
				TEXT("_MainTex"),		vrmMat.vectorProperties._MainTex,
				TEXT("_ShadeTexture"),	vrmMat.vectorProperties._ShadeTexture,
				TEXT("_BumpMap"),				vrmMat.vectorProperties._BumpMap,
				TEXT("_ReceiveShadowTexture"),	vrmMat.vectorProperties._ReceiveShadowTexture,
				TEXT("_SphereAdd"),				vrmMat.vectorProperties._SphereAdd,
				TEXT("_EmissionColor"),			vrmMat.vectorProperties._EmissionColor,
				TEXT("_EmissionMap"),			vrmMat.vectorProperties._EmissionMap,
				TEXT("_OutlineWidthTexture"),	vrmMat.vectorProperties._OutlineWidthTexture,
				TEXT("_OutlineColor"),			vrmMat.vectorProperties._OutlineColor,
			};

			for (auto &t : table) {
				FVectorParameterValue *v = new (dm->VectorParameterValues) FVectorParameterValue();
				v->ParameterInfo.Index = INDEX_NONE;
				v->ParameterInfo.Name = *(TEXT("mtoon") + t.key);
				v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
				v->ParameterValue = FLinearColor(t.value[0], t.value[1], t.value[2], t.value[3]);
			}
		}
		{
			struct TT {
				FString key;
				float& value;
			};
			TT table[] = {
				TEXT("_Cutoff"),		vrmMat.floatProperties._Cutoff,
				TEXT("_BumpScale"),	vrmMat.floatProperties._BumpScale,
				TEXT("_ReceiveShadowRate"),	vrmMat.floatProperties._ReceiveShadowRate,
				TEXT("_ShadeShift"),			vrmMat.floatProperties._ShadeShift,
				TEXT("_ShadeToony"),			vrmMat.floatProperties._ShadeToony,
				TEXT("_LightColorAttenuation"),	vrmMat.floatProperties._LightColorAttenuation,
				TEXT("_OutlineWidth"),			vrmMat.floatProperties._OutlineWidth,
				TEXT("_OutlineScaledMaxDistance"),	vrmMat.floatProperties._OutlineScaledMaxDistance,
				TEXT("_OutlineLightingMix"),			vrmMat.floatProperties._OutlineLightingMix,
				TEXT("_DebugMode"),				vrmMat.floatProperties._DebugMode,
				TEXT("_BlendMode"),				vrmMat.floatProperties._BlendMode,
				TEXT("_OutlineWidthMode"),		vrmMat.floatProperties._OutlineWidthMode,
				TEXT("_OutlineColorMode"),	vrmMat.floatProperties._OutlineColorMode,
				TEXT("_CullMode"),			vrmMat.floatProperties._CullMode,
				TEXT("_OutlineCullMode"),		vrmMat.floatProperties._OutlineCullMode,
				TEXT("_SrcBlend"),			vrmMat.floatProperties._SrcBlend,
				TEXT("_DstBlend"),			vrmMat.floatProperties._DstBlend,
				TEXT("_ZWrite"),				vrmMat.floatProperties._ZWrite,
				TEXT("_IsFirstSetup"),		vrmMat.floatProperties._IsFirstSetup,
			};

			for (auto &t : table) {
				FScalarParameterValue *v = new (dm->ScalarParameterValues) FScalarParameterValue();
				v->ParameterInfo.Index = INDEX_NONE;
				v->ParameterInfo.Name = *(TEXT("mtoon") + t.key);
				v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
				v->ParameterValue = t.value;
			}
		}

		return true;
	}

	UMaterial* CreateDefaultMaterial(UVrmAssetListObject *vrmAssetList) {
		//auto MaterialFactory = NewObject<UMaterialFactoryNew>();

#if	UE_VERSION_NEWER_THAN(4,20,0)
		UMaterial* UnrealMaterial = NewObject<UMaterial>(vrmAssetList->Package, TEXT("M_BaseMaterial"), RF_Standalone|RF_Public, NULL, GWarn );
#else
		UMaterial* UnrealMaterial = NewObject<UMaterial>(vrmAssetList->Package, TEXT("M_BaseMaterial"), RF_Standalone | RF_Public);
#endif
//		UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(
//			UMaterial::StaticClass(), vrmAssetList->Package, TEXT("M_BaseMaterial"), RF_Standalone|RF_Public, NULL, GWarn );

		if (UnrealMaterial != NULL)
		{
			//UnrealMaterialFinal = UnrealMaterial;
			// Notify the asset registry
			//FAssetRegistryModule::AssetCreated(UnrealMaterial);

			if(true)
			{
				bool bNeedsRecompile = true;
				UnrealMaterial->GetMaterial()->SetMaterialUsage(bNeedsRecompile, MATUSAGE_SkeletalMesh);
				UnrealMaterial->GetMaterial()->SetMaterialUsage(bNeedsRecompile, MATUSAGE_MorphTargets);
			}

			// Set the dirty flag so this package will get saved later
			//Package->SetDirtyFlag(true);

			// textures and properties

			{
				UMaterialExpressionTextureSampleParameter2D* UnrealTextureExpression = NewObject<UMaterialExpressionTextureSampleParameter2D>(UnrealMaterial);
				UnrealMaterial->Expressions.Add(UnrealTextureExpression);
				UnrealTextureExpression->SamplerType = SAMPLERTYPE_Color;
				UnrealTextureExpression->ParameterName = TEXT("gltf_tex_diffuse");

#if WITH_EDITORONLY_DATA
				UnrealMaterial->BaseColor.Expression = UnrealTextureExpression;
#endif
			}

			UnrealMaterial->BlendMode = BLEND_Masked;
			UnrealMaterial->SetShadingModel(MSM_DefaultLit);
			/*
				UnrealMaterial->BlendMode = BLEND_Translucent;
				CreateAndLinkExpressionForMaterialProperty(FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sTransparencyFactor, UnrealMaterial->OpacityMask, false, UVSets, FVector2D(150, 256));

			}
			FixupMaterial(FbxMaterial, UnrealMaterial); // add random diffuse if none exists
			*/
		}
		return UnrealMaterial;
	}
}

namespace VRM {
	bool ConvertTextureAndMaterial(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {
		if (vrmAssetList == nullptr || mScenePtr==nullptr) {
			return false;
		}

		vrmAssetList->Textures.Reset(0);
		vrmAssetList->Materials.Reset(0);

		TArray<UTexture2D*> texArray;
		if (mScenePtr->HasTextures()) {
			for (uint32_t i = 0; i < mScenePtr->mNumTextures; ++i) {
				auto &t = *mScenePtr->mTextures[i];
				int Width = t.mWidth;
				int Height = t.mHeight;
				const TArray<uint8>* RawData = nullptr;

				TSharedPtr<IImageWrapper> ImageWrapper;
				if (Height == 0) {
					IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
					// Note: PNG format.  Other formats are supported
					ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

					if (ImageWrapper->SetCompressed(t.pcData, t.mWidth)) {

					}
					Width = ImageWrapper->GetWidth();
					Height = ImageWrapper->GetHeight();

					if (Width == 0 || Height == 0) {
						continue;
					}

					ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);
				}
				FString baseName = t.mFilename.C_Str();
				if (baseName.Len() == 0) {
					baseName = FString::FromInt(i);
				}

				UTexture2D* NewTexture2D = createTex(Width, Height, FString(TEXT("T_")) +baseName, vrmAssetList->Package);
				//UTexture2D* NewTexture2D = _CreateTransient(Width, Height, PF_B8G8R8A8, t.mFilename.C_Str());

				// Fill in the base mip for the texture we created
				uint8* MipData = (uint8*)NewTexture2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				if (RawData) {
					FMemory::Memcpy(MipData, RawData->GetData(), RawData->Num());
				}else{
					for (int32 y = 0; y < Height; y++)
					{
						const aiTexel *c = &(t.pcData[y*Width]);
						uint8* DestPtr = &MipData[y * Width * sizeof(FColor)];
						for (int32 x = 0; x < Width; x++)
						{
							*DestPtr++ = c->b;
							*DestPtr++ = c->g;
							*DestPtr++ = c->r;
							*DestPtr++ = c->a;
							c++;
						}
					}
				}
				NewTexture2D->PlatformData->Mips[0].BulkData.Unlock();

				// Set options
				NewTexture2D->SRGB = true;// bUseSRGB;
				NewTexture2D->CompressionSettings = TC_Default;
				NewTexture2D->AddressX = TA_Wrap;
				NewTexture2D->AddressY = TA_Wrap;

#if WITH_EDITORONLY_DATA
				NewTexture2D->CompressionNone = true;
				NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
				NewTexture2D->Source.Init(Width, Height, 1, 1, ETextureSourceFormat::TSF_BGRA8, RawData->GetData());
#endif

				// Update the remote texture data
				NewTexture2D->UpdateResource();

				texArray.Push(NewTexture2D);
			}
			vrmAssetList->Textures = texArray;
		}

		TArray<UMaterialInterface*> matArray;
		if (mScenePtr->HasMaterials()) {

			vrmAssetList->Materials.SetNum(mScenePtr->mNumMaterials);
			for (uint32_t i = 0; i < mScenePtr->mNumMaterials; ++i) {
				auto &aiMat = *mScenePtr->mMaterials[i];

				UMaterialInterface *baseM = nullptr;;
				bool bMToon = false;
				{
					aiString alphaMode;
					aiReturn result = aiMat.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);

					FString ShaderName = aiMat.mShaderName.C_Str();
					if (ShaderName.Find(TEXT("UnlitTexture")) >= 0) {
						baseM = vrmAssetList->BaseUnlitOpaqueMaterial;
					}
					if (ShaderName.Find(TEXT("UnlitTransparent")) >= 0) {
						baseM = vrmAssetList->BaseUnlitTransparentMaterial;
					}
					if (ShaderName.Find(TEXT("MToon")) >= 0) {
						bMToon = true;
						FString alpha = alphaMode.C_Str();
						if (alpha == TEXT("BLEND")) {
							baseM = vrmAssetList->BaseMToonTransparentMaterial;
						}
						else {
							baseM = vrmAssetList->BaseMToonOpaqueMaterial;
						}
					}
					if (ShaderName.Find(TEXT("UnlitTransparent")) >= 0) {
						baseM = vrmAssetList->BaseUnlitTransparentMaterial;
					}


					if (baseM == nullptr){
						baseM = vrmAssetList->BaseMToonOpaqueMaterial;
					}
				}
				//if (FString(m.mShaderName.C_Str()).Find(TEXT("UnlitTexture"))) {

				if (baseM == nullptr) {
					baseM = CreateDefaultMaterial(vrmAssetList);
					vrmAssetList->BaseUnlitOpaqueMaterial = baseM;
				}
				if (baseM == nullptr) {
					continue;
				}

				aiString texName;
				int index = -1;
				{
					for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
						uint32_t n = aiMat.GetTextureCount((aiTextureType)t);
						for (uint32_t y = 0; y < n; ++y) {
							aiMat.GetTexture((aiTextureType)t, y, &texName);
							UE_LOG(LogTemp, Warning, TEXT("R--%s\n"), texName.C_Str());
						}
					}

					for (uint32_t i = 0; i < mScenePtr->mNumTextures; ++i) {
						if (mScenePtr->mTextures[i]->mFilename == texName) {
							index = i;
							break;
						}
					}
				}
				{
					aiString path;
					aiReturn r = aiMat.GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &path);
					if (r == AI_SUCCESS) {
						std::string s = path.C_Str();
						s = s.substr(s.find_last_of('*') + 1);
						index = atoi(s.c_str());
					}
				}

				//UMaterialInstanceDynamic* dm = UMaterialInstanceDynamic::Create(baseM, vrmAssetList, m.GetName().C_Str());
				//UMaterialInstanceDynamic* dm = UMaterialInstance::Create(baseM, vrmAssetList, m.GetName().C_Str());
				//MaterialInstance->TextureParameterValues

				//set paramater with Set***ParamaterValue
				//DynMaterial->SetScalarParameterValue("MyParameter", myFloatValue);
				//MyComponent1->SetMaterial(0, DynMaterial);
				//MyComponent2->SetMaterial(0, DynMaterial);

				if (index >= 0 && index < vrmAssetList->Textures.Num()) {
					UMaterialInstanceConstant* dm = NewObject<UMaterialInstanceConstant>(vrmAssetList->Package, *(FString(TEXT("M_"))+aiMat.GetName().C_Str()), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
					dm->Parent = baseM;

					if (dm) {
						if (bMToon){
							aiColor4D col;
							aiReturn result = aiMat.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, col);
							if (result == 0) {
								FVectorParameterValue *v = new (dm->VectorParameterValues) FVectorParameterValue();
								v->ParameterInfo.Index = INDEX_NONE;
								v->ParameterInfo.Name = TEXT("gltf_basecolor");
								v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;;
								v->ParameterValue = FLinearColor(col.r, col.g, col.b, col.a);
							}
						}

						if (bMToon){
							float f[2] = {1,1};
							aiReturn result0 = aiMat.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, f[0]);
							aiReturn result1 = aiMat.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, f[1]);
							if (result0 == AI_SUCCESS || result1 == AI_SUCCESS) {
								f[0] = (result0==AI_SUCCESS) ? f[0]: 1;
								f[1] = (result1==AI_SUCCESS) ? f[1]: 1;
								if (f[0] == 0 && f[1] == 0) {
									f[0] = f[1] = 1.f;
								}
								FVectorParameterValue *v = new (dm->VectorParameterValues) FVectorParameterValue();
								v->ParameterInfo.Index = INDEX_NONE;
								v->ParameterInfo.Name = TEXT("gltf_RM");
								v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;;
								v->ParameterValue = FLinearColor(f[0], f[1], 0, 0);
							}
						}
						{
							FTextureParameterValue *v = new (dm->TextureParameterValues) FTextureParameterValue();
							v->ParameterInfo.Index = INDEX_NONE;
							v->ParameterInfo.Name = TEXT("gltf_tex_diffuse");
							v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
							v->ParameterValue = vrmAssetList->Textures[index];
						}
						if (bMToon) {
							createAndAddMaterial(dm, i, vrmAssetList, mScenePtr);
						}

						dm->InitStaticPermutation();
						matArray.Add(dm);
					}
				}
			}
			vrmAssetList->Materials = matArray;
		}

		return true;
	}
}


VrmConvertTexture::VrmConvertTexture()
{
}

VrmConvertTexture::~VrmConvertTexture()
{
}
