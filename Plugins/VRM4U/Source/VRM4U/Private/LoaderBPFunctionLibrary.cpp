// Fill out your copyright notice in the Description page of Project Settings.
#include "LoaderBPFunctionLibrary.h"
#include "VRM4U.h"
#include "VrmSkeleton.h"
#include "VrmSkeletalMesh.h"
#include "VrmModelActor.h"

#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"

//#include ".h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/vrm/vrmmeta.h>

#include "RenderingThread.h"
#include "SkeletalMeshModel.h"
#include "SkeletalMeshLODModel.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "AssetRegistryModule.h"

// tem
UPackage *package = nullptr;
FString baseFileName;

static bool saveObject(UObject *u) {
	package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(u);
	bool bSaved = UPackage::SavePackage(package, u, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(package->GetName()), GError, nullptr, true, true, SAVE_NoError);

	return true;
}

void FindMeshInfo(const aiScene* scene, aiNode* node, FReturnedData& result)
{

	for (uint32 i = 0; i < node->mNumMeshes; i++)
	{
		std::string TestString = node->mName.C_Str();
		FString Fs = FString(TestString.c_str());
		UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %s\n"), *Fs);
		int meshidx = node->mMeshes[i];
		aiMesh *mesh = scene->mMeshes[meshidx];
		FMeshInfo &mi = result.meshInfo[meshidx];
		
		//if (mesh->mNumBones == 0) {
		//	continue;
		//}
		

		//transform.
		aiMatrix4x4 tempTrans = node->mTransformation;
		FMatrix tempMatrix;
		tempMatrix.M[0][0] = tempTrans.a1; tempMatrix.M[0][1] = tempTrans.b1; tempMatrix.M[0][2] = tempTrans.c1; tempMatrix.M[0][3] = tempTrans.d1;
		tempMatrix.M[1][0] = tempTrans.a2; tempMatrix.M[1][1] = tempTrans.b2; tempMatrix.M[1][2] = tempTrans.c2; tempMatrix.M[1][3] = tempTrans.d2;
		tempMatrix.M[2][0] = tempTrans.a3; tempMatrix.M[2][1] = tempTrans.b3; tempMatrix.M[2][2] = tempTrans.c3; tempMatrix.M[2][3] = tempTrans.d3;
		tempMatrix.M[3][0] = tempTrans.a4; tempMatrix.M[3][1] = tempTrans.b4; tempMatrix.M[3][2] = tempTrans.c4; tempMatrix.M[3][3] = tempTrans.d4;
		mi.RelativeTransform = FTransform(tempMatrix);

		//vet
		for (uint32 j = 0; j < mesh->mNumVertices; ++j)
		{
			FVector vertex = FVector(
				mesh->mVertices[j].x,
				mesh->mVertices[j].y,
				mesh->mVertices[j].z);

			vertex = mi.RelativeTransform.TransformFVector4(vertex);
			mi.Vertices.Push(vertex);

			//Normal
			if (mesh->HasNormals())
			{
				FVector normal = FVector(
					mesh->mNormals[j].x,
					mesh->mNormals[j].y,
					mesh->mNormals[j].z);

				//normal = mi.RelativeTransform.TransformFVector4(normal);
				mi.Normals.Push(normal);
			}
			else
			{
				mi.Normals.Push(FVector::ZeroVector);
			}

			//UV Coordinates - inconsistent coordinates
			if (mesh->HasTextureCoords(0))
			{
				FVector2D uv = FVector2D(mesh->mTextureCoords[0][j].x, -mesh->mTextureCoords[0][j].y);
				mi.UV0.Add(uv);
			}

			//Tangent
			if (mesh->HasTangentsAndBitangents())
			{
				FProcMeshTangent meshTangent = FProcMeshTangent(
					mesh->mTangents[j].x,
					mesh->mTangents[j].y,
					mesh->mTangents[j].z
				);
				mi.Tangents.Push(meshTangent);
			}

			//Vertex color
			if (mesh->HasVertexColors(0))
			{
				FLinearColor color = FLinearColor(
					mesh->mColors[0][j].r,
					mesh->mColors[0][j].g,
					mesh->mColors[0][j].b,
					mesh->mColors[0][j].a
				);
				mi.VertexColors.Push(color);
			}

		}
	}
}


void FindMesh(const aiScene* scene, aiNode* node, FReturnedData& retdata)
{
	FindMeshInfo(scene, node, retdata);

	for (uint32 m = 0; m < node->mNumChildren; ++m)
	{
		FindMesh(scene, node->mChildren[m], retdata);
	}
}


static void createConstraint(USkeletalMesh *sk, UPhysicsAsset *pa, FName con1, FName con2){
	UPhysicsConstraintTemplate *ct = NewObject<UPhysicsConstraintTemplate>(package, NAME_None, RF_Transactional);
	pa->ConstraintSetup.Add(ct);
	//"skirt_01_01"
	ct->Modify(false);
	ct->DefaultInstance.JointName = TEXT("tttt");
	ct->DefaultInstance.ConstraintBone1 = con1;
	ct->DefaultInstance.ConstraintBone2 = con2;


	ct->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 0);
	ct->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 0);
	ct->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 0);

	ct->DefaultInstance.ProfileInstance.ConeLimit.Stiffness = 100.f;
	ct->DefaultInstance.ProfileInstance.TwistLimit.Stiffness = 100.f;

	const int32 BoneIndex1 = sk->RefSkeleton.FindBoneIndex(ct->DefaultInstance.ConstraintBone1);
	const int32 BoneIndex2 = sk->RefSkeleton.FindBoneIndex(ct->DefaultInstance.ConstraintBone2);

	check(BoneIndex1 != INDEX_NONE);
	check(BoneIndex2 != INDEX_NONE);

	const TArray<FTransform> &t = sk->RefSkeleton.GetRawRefBonePose();

	const FTransform BoneTransform1 = t[BoneIndex1];//sk->GetBoneTransform(BoneIndex1);
	FTransform BoneTransform2 = t[BoneIndex2];//EditorSkelComp->GetBoneTransform(BoneIndex2);

	int32 tb = BoneIndex2;
	while (sk->RefSkeleton.GetRawParentIndex(tb) != BoneIndex1) {
		int32 p = sk->RefSkeleton.GetRawParentIndex(tb);
		if (p < 0) break;

		FTransform f = t[p];
		BoneTransform2 = f.GetRelativeTransform(BoneTransform2);

		tb = p;
	}

	// child 
	ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
	// parent
	ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, BoneTransform1.GetRelativeTransform(BoneTransform2));

	ct->SetDefaultProfile(ct->DefaultInstance);
	//ct->DefaultInstance.InitConstraint();


	pa->DisableCollision(BoneIndex1, BoneIndex2);

}

static UTexture2D* createTex(int32 InSizeX, int32 InSizeY, FString name) {
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


static bool readTex(AVrmModelActor *modelActor, const aiScene *mScenePtr) {

	TArray<UTexture2D*> texArray;
	if (mScenePtr->HasTextures()) {
		for (uint32_t i = 0; i < mScenePtr->mNumTextures; ++i) {
			auto &t = *mScenePtr->mTextures[i];
			int Width = t.mWidth;
			int Height = t.mHeight;
			const TArray<uint8>* RawData = nullptr;

			IImageWrapperPtr ImageWrapper;
			if (Height == 0) {
				IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
				// Note: PNG format.  Other formats are supported
				ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

				if (ImageWrapper->SetCompressed(t.pcData, t.mWidth)) {

				}
				Width = ImageWrapper->GetWidth();
				Height = ImageWrapper->GetHeight();

				ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);
			}

			UTexture2D* NewTexture2D = createTex(Width, Height, t.mFilename.C_Str());
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
			NewTexture2D->CompressionNone = true;
			NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
			NewTexture2D->CompressionSettings = TC_Default;
			NewTexture2D->AddressX = TA_Wrap;
			NewTexture2D->AddressY = TA_Wrap;

			NewTexture2D->Source.Init(Width, Height, 1, 1, ETextureSourceFormat::TSF_BGRA8, RawData->GetData());

			// Update the remote texture data
			NewTexture2D->UpdateResource();

			texArray.Push(NewTexture2D);
		}
	}
	modelActor->Textures = texArray;

	TArray<UMaterialInterface*> matArray;
	if (mScenePtr->HasMaterials()) {

		modelActor->Materials.SetNum(mScenePtr->mNumMaterials);
		for (uint32_t i = 0; i < mScenePtr->mNumMaterials; ++i) {
			auto &m = *mScenePtr->mMaterials[i];

			UMaterialInterface *baseM = nullptr;;
			if (FString(m.mShaderName.C_Str()).Find(TEXT("UnlitTransparent")) >= 0) {
				baseM = modelActor->BaseTransparentMaterial;
			}
			else {
				baseM = modelActor->BaseOpaqueMaterial;
			}
			//if (FString(m.mShaderName.C_Str()).Find(TEXT("UnlitTexture"))) {

			if (baseM == nullptr) {
				continue;
			}

			aiString texName;
			int index = -1;
			{
				for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
					uint32_t n = m.GetTextureCount((aiTextureType)t);
					for (uint32_t y = 0; y < n; ++y) {
						m.GetTexture((aiTextureType)t, y, &texName);
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
				aiReturn r = m.GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &path);
				if (r == AI_SUCCESS) {
					std::string s = path.C_Str();
					s = s.substr(s.find_last_of('*') + 1);
					index = atoi(s.c_str());
				}
			}

			//UMaterialInstanceDynamic* dm = UMaterialInstanceDynamic::Create(baseM, modelActor, m.GetName().C_Str());
			//UMaterialInstanceDynamic* dm = UMaterialInstance::Create(baseM, modelActor, m.GetName().C_Str());
			//MaterialInstance->TextureParameterValues

			//set paramater with Set***ParamaterValue
			//DynMaterial->SetScalarParameterValue("MyParameter", myFloatValue);
			//MyComponent1->SetMaterial(0, DynMaterial);
			//MyComponent2->SetMaterial(0, DynMaterial);

			if (index >= 0 && index < modelActor->Textures.Num()) {
				UMaterialInstanceDynamic* dm = UMaterialInstanceDynamic::Create(baseM, modelActor, m.GetName().C_Str());
				//UMaterialInstanceConstant* dm = NewObject<UMaterialInstanceConstant>(package, NAME_None, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

				if (dm) {
					//FTextureParameterValue *v = new (dm->TextureParameterValues) FTextureParameterValue();
					//v->ParameterInfo.Index
					//v->ParameterValue = modelActor->Textures[index];
					//dm->TextureParameterValues(
					dm->SetTextureParameterValue(TEXT("vrm_diffuse"), modelActor->Textures[index]);
					matArray.Add(dm);
				}
			}
		}
		modelActor->Materials = matArray;
	}

	for (auto &t : texArray) {
		saveObject(t);
		//package->MarkPackageDirty();
		//FAssetRegistryModule::AssetCreated(t);
		//bool bSaved = UPackage::SavePackage(package, t, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(package->GetName()), GError, nullptr, true, true, SAVE_NoError);
	}
	for (auto &t : matArray) {
		saveObject(t);
		//package->MarkPackageDirty();
		//FAssetRegistryModule::AssetCreated(t);
		//bool bSaved = UPackage::SavePackage(package, t, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(package->GetName()), GError, nullptr, true, true, SAVE_NoError);
	}

	return true;
}

static bool readMorph(AVrmModelActor *modelActor, const aiScene *mScenePtr) {


	USkeletalMesh *sk = modelActor->SkeletalMesh;

	{
		///sk->MarkPackageDirty();
		// need to refresh the map
		//sk->InitMorphTargets();
		// invalidate render data
		//sk->InvalidateRenderData();
		//return true;
	}

	TArray<FSoftSkinVertex> sVertex;
	sk->GetImportedModel()->LODModels[0].GetVertices(sVertex);
	//mScenePtr->mMeshes[0]->mAnimMeshes[0]->mWeight

	int currentVertex = 0;

	for (uint32_t m = 0; m < mScenePtr->mNumMeshes; ++m) {
		const aiMesh &aiM = *(mScenePtr->mMeshes[m]);
		for (uint32_t a = 0; a < aiM.mNumAnimMeshes; ++a) {
			const aiAnimMesh &aiA = *(aiM.mAnimMeshes[a]);
			//aiA.

			//FString sss = FString::FromInt(a) + FString(aiA.mName.C_Str());
			FString sss = FString::Printf(TEXT("%02d_%02d_"), m, a) + FString(aiA.mName.C_Str());
			//sss = FString::Printf(TEXT("%03d_%s"), a, aiA.mName.C_Str());
			//std::string aaa;
			//aaa << a;
			UMorphTarget *mt = NewObject<UMorphTarget>(sk, *sss);
			//mt->name
			if (m == 18) {
				UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
			}
			TArray<FMorphTargetDelta> MorphDeltas;
			MorphDeltas.SetNum(aiA.mNumVertices);
			//mt->PopulateDeltas

			//FMorphTargetLODModel *mtm = new(mt->MorphLODModels) FMorphTargetLODModel();
			//new(p->LODRenderData) FSkeletalMeshLODRenderData()
			//FMorphTargetLODModel *mtm = NewObject<FMorphTargetLODModel>();
			//mtm->Vertices.SetNum(aiA.mNumVertices);
			//mtm->SectionIndices.SetNum(aiA.mNumVertices);
			//mtm->SectionIndices.Add( sk->GetImportedModel()->LODModels[0].Sections[m].MaterialIndex );
			for (uint32_t i = 0; i < aiA.mNumVertices; ++i) {
				//auto &v = mtm->Vertices[i];
				auto &v = MorphDeltas[i];
				v.SourceIdx = i +currentVertex;
				v.PositionDelta.Set(
					-aiA.mVertices[i][0] * 100.f,
					aiA.mVertices[i][2] * 100.f,
					aiA.mVertices[i][1] * 100.f
					//aiA.mVertices[i][0],
					//aiA.mVertices[i][1],
					//aiA.mVertices[i][2]
				);
				//v.PositionDelta -= sVertex[i + currentVertex].Position;
				//v.TangentZDelta -= sVertex[i + currentVertex].nor

				//MorphVertex.TangentZDelta = BaseSample->Normals[UsedNormalIndex] - AverageSample->Normals[UsedNormalIndex];

				v.TangentZDelta.Set(0, 0, 0);
				//mtm->SectionIndices[i] = m;// i;// sk->GetImportedModel()->LODModels[0].Sections[m].MaterialIndex;
			}
			//mtm->NumBaseMeshVerts = aiA.mNumVertices;

			//mt->MorphLODModels.Add(*mtm);
			mt->PopulateDeltas(MorphDeltas, 0, sk->GetImportedModel()->LODModels[0].Sections);
			sk->RegisterMorphTarget(mt);
			
		}

		currentVertex += aiM.mNumVertices;
	}


	return true;
}


static bool readModel(AVrmModelActor *modelActor, const aiScene *mScenePtr) {
	if (modelActor == nullptr) {
		return nullptr;
	}

	FReturnedData result;
	{
		result.bSuccess = false;
		result.meshInfo.Empty();
		result.NumMeshes = 0;

		if (mScenePtr == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
		}

		if (mScenePtr->HasMeshes())
		{
			result.meshInfo.SetNum(mScenePtr->mNumMeshes, false);
			
			FindMesh(mScenePtr, mScenePtr->mRootNode, result);

			int verCount = 0;
			for (uint32 i = 0; i < mScenePtr->mNumMeshes; ++i)
			{
				//Triangle number
				for (uint32 l = 0; l < mScenePtr->mMeshes[i]->mNumFaces; ++l)
				{
					for (uint32 m = 0; m < mScenePtr->mMeshes[i]->mFaces[l].mNumIndices; ++m)
					{
						if (m >= 3){
							UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %d\n"), m);
						}
						result.meshInfo[i].Triangles.Push(mScenePtr->mMeshes[i]->mFaces[l].mIndices[m]);
					}
				}
				verCount += mScenePtr->mMeshes[i]->mNumVertices;
			}
			result.bSuccess = true;
		}

		//return result;
	}

	USkeletalMesh *sk = NewObject<USkeletalMesh>(package, *(baseFileName + TEXT("_skeletalmesh")) , EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	UVrmSkeleton *k = NewObject<UVrmSkeleton>(package, *(baseFileName + TEXT("_skeleton")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

	static int boneOffset = 0;
	{
		sk->Skeleton = k;

		//k->MergeAllBonesToBoneTree(src);
		k->Proc(mScenePtr, boneOffset);

		sk->RefSkeleton = k->GetReferenceSkeleton();
		//sk->RefSkeleton.RebuildNameToIndexMap();

		//sk->RefSkeleton.RebuildRefSkeleton(sk->Skeleton, true);
		//sk->Proc();

		//src->RefSkeleton = sk->RefSkeleton;

		//src->Skeleton->MergeAllBonesToBoneTree(sk);
		sk->CalculateInvRefMatrices();
		sk->CalculateExtendedBounds();
		sk->ConvertLegacyLODScreenSize();
		sk->UpdateGenerateUpToData();

		sk->GetImportedModel()->LODModels.Reset();

		k->SetPreviewMesh(sk);
		k->RecreateBoneTree(sk);


		modelActor->SkeletalMesh = sk;


		{
			FSkeletalMeshLODInfo &info = sk->AddLODInfo();
		}
		sk->AllocateResourceForRendering();
		FSkeletalMeshRenderData *p = sk->GetResourceForRendering();
		FSkeletalMeshLODRenderData *pRd = new(p->LODRenderData) FSkeletalMeshLODRenderData();

		{
			pRd->StaticVertexBuffers.PositionVertexBuffer.Init(10);
			pRd->StaticVertexBuffers.ColorVertexBuffer.Init(10);
			pRd->StaticVertexBuffers.StaticMeshVertexBuffer.Init(10, 10);

			TArray<FSoftSkinVertex> Weight;
			Weight.SetNum(10);

			pRd->SkinWeightVertexBuffer.Init(Weight);

		}

		sk->InitResources();

		sk->Materials.SetNum(modelActor->Materials.Num());
		for (int i = 0; i < sk->Materials.Num(); ++i) {
			sk->Materials[i].MaterialInterface = modelActor->Materials[i];
			sk->Materials[i].UVChannelData = FMeshUVChannelInfo(1);
			sk->Materials[i].MaterialSlotName = mScenePtr->mMaterials[i]->GetName().C_Str();
		}
	}
	//USkeleton* NewAsset = NewObject<USkeleton>();

	//USkeleton* NewAsset = nullptr;// = src->Skeleton;// DuplicateObject<USkeleton>(src, src->GetOuter(), TEXT("/Game/FirstPerson/Character/Mesh/SK_Mannequin_Arms_Skeletonaaaaaaa"));
	{
		{
			{
				//TArray<UMorphTarget*> t;
				//p->InitResources(true, t);
			}
			//p->LODRenderData.
			//p->LODRenderData.Add(&(p->LODRenderData[0]));
			//p->LODRenderData.Add(&d);

			//				FSkeletalMeshLODRenderData* LODData = new(LODRenderData) FSkeletalMeshLODRenderData();


			//FSkeletalMeshLODInfo *i = src->GetLODInfo(0);
			//i->lo
			//pRd->BuildFromLODModel(sk->lod
			//p->LODRenderData.Add(&rd);
			FSkeletalMeshLODRenderData &rd = sk->GetResourceForRendering()->LODRenderData[0];

			for (int i = 0; i < sk->Skeleton->GetBoneTree().Num(); ++i) {
				rd.RequiredBones.Add(i);
				rd.ActiveBoneIndices.Add(i);
			}
			{
				//rd.RenderSections[0].NumTriangles = 0;
				FStaticMeshVertexBuffers	 &v = rd.StaticVertexBuffers;

//				TArray<FModelVertex> m;

				int allIndex = 0;
				int allVertex = 0;
				for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
					allIndex += result.meshInfo[meshID].Triangles.Num();
					allVertex += result.meshInfo[meshID].Vertices.Num();
				}
				v.StaticMeshVertexBuffer.Init(allVertex, 1);
				v.PositionVertexBuffer.Init(allVertex);
				//rd.SkinWeightVertexBuffer.Init(allVertex);

				TArray<uint32> Triangles;
				TArray<FSoftSkinVertex> Weight;
				Weight.SetNum(allVertex);
				int currentIndex = 0;
				int currentVertex = 0;

				new(sk->GetImportedModel()->LODModels) FSkeletalMeshLODModel();
				sk->GetImportedModel()->LODModels[0].Sections.SetNum(result.meshInfo.Num());

				rd.RenderSections.SetNum(result.meshInfo.Num());
				for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
					TArray<FSoftSkinVertex> meshWeight;

					for (int i = 0; i < result.meshInfo[meshID].Vertices.Num(); ++i) {
						FSoftSkinVertex *meshS = new(meshWeight) FSoftSkinVertex();
						auto a = result.meshInfo[meshID].Vertices[i] * 100.f;

						v.PositionVertexBuffer.VertexPosition(currentVertex + i).Set(-a.X, a.Z, a.Y);

						FVector2D uv;
						uv = result.meshInfo[meshID].UV0[i];
						v.StaticMeshVertexBuffer.SetVertexUV(currentVertex + i, 0, uv);
						meshS->UVs[0] = uv;

						auto aiS = rd.SkinWeightVertexBuffer.GetSkinWeightPtr<false>(i);
						//aiS->InfluenceWeights
						// InfluenceBones

						FSoftSkinVertex &s = Weight[currentVertex + i];
						s.Position = v.PositionVertexBuffer.VertexPosition(currentVertex + i);
						meshS->Position = v.PositionVertexBuffer.VertexPosition(currentVertex + i);

						for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
						{
							//s.InfluenceBones[InfluenceIndex] = 0;
							//s.InfluenceWeights[InfluenceIndex] = 0;
						}
						//s.InfluenceBones[0] = 0;// aiS->InfluenceBones[0];// +boneOffset;
						//s.InfluenceWeights[0] = 255;

						//s.InfluenceWeights[0] = aiS->InfluenceWeights[0];
						//s.InfluenceWeights[1] = aiS->InfluenceWeights[1];


						//aiNode *bone = mScenePtr->mRootNode->FindNode( mScenePtr->mMeshes[meshID]->mBones[0]->mName );
					}

					auto &aiM = mScenePtr->mMeshes[meshID];
					if (1) {
						//mScenePtr->mRootNode->mMeshes
						for (uint32 boneIndex = 0; boneIndex < aiM->mNumBones; ++boneIndex) {
							auto &aiB = aiM->mBones[boneIndex];

							int b = sk->RefSkeleton.FindBoneIndex(aiB->mName.C_Str());

							if (b < 0) {
								continue;
							}
							for (uint32 weightIndex = 0; weightIndex < aiB->mNumWeights; ++weightIndex) {
								auto &aiW = aiB->mWeights[weightIndex];

								if (aiW.mWeight == 0.f) {
									continue;
								}
								for (int jj = 0; jj < 8; ++jj) {
									auto &s = Weight[aiW.mVertexId + currentVertex];
									if (s.InfluenceWeights[jj] > 0.f) {
										continue;
									}
									if (jj >= 4) {
										UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
									}
									if (b == 3) {
										UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
									}
									s.InfluenceBones[jj] = b;
									s.InfluenceWeights[jj] = (uint8)(aiW.mWeight * 255.f);

									meshWeight[aiW.mVertexId].InfluenceBones[jj] = s.InfluenceBones[jj];
									meshWeight[aiW.mVertexId].InfluenceWeights[jj] = s.InfluenceWeights[jj];
									break;
								}
							}
						}
					}

					{
						FSkelMeshRenderSection &NewRenderSection = rd.RenderSections[meshID];
						//NewRenderSection = rd.RenderSections[0];
						NewRenderSection.MaterialIndex = aiM->mMaterialIndex;// ModelSection.MaterialIndex;
						NewRenderSection.BaseIndex = currentIndex;
						NewRenderSection.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
						//NewRenderSection.bRecomputeTangent = ModelSection.bRecomputeTangent;
						NewRenderSection.bCastShadow = true;// ModelSection.bCastShadow;
						NewRenderSection.BaseVertexIndex = 0;// currentVertex;// currentVertex;// ModelSection.BaseVertexIndex;
															 //NewRenderSection.ClothMappingData = ModelSection.ClothMappingData;
															 //NewRenderSection.BoneMap.SetNum(1);//ModelSection.BoneMap;
															 //NewRenderSection.BoneMap[0] = 10;
						NewRenderSection.BoneMap.SetNum(sk->Skeleton->GetBoneTree().Num());//ModelSection.BoneMap;
						for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i) {
							NewRenderSection.BoneMap[i] = i;
						}


						NewRenderSection.NumVertices = result.meshInfo[meshID].Vertices.Num();// result.meshInfo[meshID].Triangles.Num();// allVertex;// result.meshInfo[meshID].Vertices.Num();// ModelSection.NumVertices;

						NewRenderSection.MaxBoneInfluences = 4;// ModelSection.MaxBoneInfluences;
															   //NewRenderSection.CorrespondClothAssetIndex = ModelSection.CorrespondClothAssetIndex;
															   //NewRenderSection.ClothingData = ModelSection.ClothingData;
						TMap<int32, TArray<int32>> OverlappingVertices;
						NewRenderSection.DuplicatedVerticesBuffer.Init(NewRenderSection.NumVertices, OverlappingVertices);
						NewRenderSection.bDisabled = false;// ModelSection.bDisabled;
														   //RenderSections.Add(NewRenderSection);

														   //rd.RenderSections[0] = NewRenderSection;

					}

					{
						auto &s = sk->GetImportedModel()->LODModels[0].Sections[meshID];

						TMap<int32, TArray<int32>> OverlappingVertices;
						
						s.MaterialIndex = aiM->mMaterialIndex;
						s.BaseIndex = currentIndex;
						s.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
						s.BaseVertexIndex = currentVertex;
						s.SoftVertices = meshWeight;
						s.BoneMap.SetNum(sk->Skeleton->GetBoneTree().Num());//ModelSection.BoneMap;
						for (int i = 0; i < s.BoneMap.Num(); ++i) {
							s.BoneMap[i] = i;
						}
						s.NumVertices = meshWeight.Num();
						s.MaxBoneInfluences = 4;

						//s.
						//s.NewRenderSection.DuplicatedVerticesBuffer.Init(NewRenderSection.NumVertices, OverlappingVertices);


					}
					//last

					//rd.MultiSizeIndexContainer.CopyIndexBuffer(result.meshInfo[0].Triangles);
					int t1 = Triangles.Num();
					Triangles.Append(result.meshInfo[meshID].Triangles);
					int t2 = Triangles.Num();

					for (int i = t1; i < t2; ++i) {
						Triangles[i] += currentVertex;
					}
					currentIndex += result.meshInfo[meshID].Triangles.Num();
					currentVertex += result.meshInfo[meshID].Vertices.Num();
					//rd.MultiSizeIndexContainer.update
				}


				//rd.MultiSizeIndexContainer.CreateIndexBuffer(sizeof(uint32));
				//rd.MultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
				//rd.MultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
				//rd.MultiSizeIndexContainer.CopyIndexBuffer(Triangles);
				//rd.AdjacencyMultiSizeIndexContainer.CopyIndexBuffer(Triangles);

				//rd.SkinWeightVertexBuffer.Init(Weight);


				// wait check

				{
					for (auto &w : Weight) {
						float f = 0.f;

						for (int i = 0; i < 8; ++i) {
							f += w.InfluenceWeights[i];

							if (w.InfluenceBones[i] == 3) {
								UE_LOG(LogTemp, Warning, TEXT("overr"));
							}
						}
						if (f > 255.f) {
							UE_LOG(LogTemp, Warning, TEXT("overr"));
							w.InfluenceWeights[0] -= (uint8)(f - 255.f);
						}
						if (f <= 250.f) {
							UE_LOG(LogTemp, Warning, TEXT("bad!"));
						}
						if (f <= 254.f) {
							UE_LOG(LogTemp, Warning, TEXT("under"));
							w.InfluenceWeights[0] += (uint8)(255.f - f);
						}

					}

				
				}

				{
					FSkeletalMeshLODModel *p = &(sk->GetImportedModel()->LODModels[0]);
					p->NumVertices = allVertex;
					p->NumTexCoords = 1;// allVertex;
					p->IndexBuffer = Triangles;
					p->ActiveBoneIndices = rd.ActiveBoneIndices;
					p->RequiredBones = rd.RequiredBones;

					//p->RawPointIndices.
				}
				//rd.StaticVertexBuffers.StaticMeshVertexBuffer.TexcoordDataPtr;

				if (0){

					sk->GetImportedModel()->LODModels[0].Sections.SetNum(1);
					auto &s = sk->GetImportedModel()->LODModels[0].Sections[0];
					s.SoftVertices = Weight;

					TMap<int32, TArray<int32>> OverlappingVertices;
					s.NumVertices = Weight.Num();
					//s.
					//s.NewRenderSection.DuplicatedVerticesBuffer.Init(NewRenderSection.NumVertices, OverlappingVertices);

				}


				ENQUEUE_RENDER_COMMAND(UpdateCommand)(
					[sk, Triangles, Weight](FRHICommandListImmediate& RHICmdList)
				{
					FSkeletalMeshLODRenderData &d = sk->GetResourceForRendering()->LODRenderData[0];

					if (d.MultiSizeIndexContainer.IsIndexBufferValid()) {
						d.MultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
					}
					d.MultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
					d.MultiSizeIndexContainer.GetIndexBuffer()->InitResource();

					//d.AdjacencyMultiSizeIndexContainer.CopyIndexBuffer(Triangles);
					if (d.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid()) {
						d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
					}
					d.AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
					d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->InitResource();

					d.SkinWeightVertexBuffer.Init(Weight);
					d.SkinWeightVertexBuffer.InitResource();

					d.StaticVertexBuffers.PositionVertexBuffer.UpdateRHI();
					d.StaticVertexBuffers.StaticMeshVertexBuffer.UpdateRHI();

					d.MultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI();
					d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI();
					d.SkinWeightVertexBuffer.UpdateRHI();

					//src->GetResourceForRendering()->LODRenderData[0].rhi
					//v.PositionVertexBuffer.UpdateRHI();
				});
			}
		}
	}

	{
		//aa
		UPhysicsAsset *pa = NewObject<UPhysicsAsset>(package);
		pa->Modify();
		pa->SetPreviewMesh(sk);
		sk->PhysicsAsset = pa;
		{
			VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);
			
			for (int i = 0; i < meta->sprintNum; ++i) {
				auto &s = meta->springs[i];
				for (int j = 0; j < s.boneNum; ++j) {
					auto &sbone = s.bones_name[j];
					USkeletalBodySetup *bs = NewObject<USkeletalBodySetup>(pa, NAME_None);
					
					//int nodeID = mScenePtr->mRootNode->FindNode(sbone.c_str());
					//sk->RefSkeleton.GetRawRefBoneInfo[0].
					//sk->bonetree
					//bs->constrai
					FKAggregateGeom agg;
					FKSphereElem SphereElem;
					SphereElem.Center = FVector(0);
					SphereElem.Radius = s.hitRadius * 100.f;
					agg.SphereElems.Add(SphereElem);


					bs->Modify();
					bs->BoneName = sbone.C_Str();
					bs->AddCollisionFrom(agg);
					bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
					// newly created bodies default to simulating
					bs->PhysicsType = PhysType_Kinematic;	// fix

					bs->InvalidatePhysicsData();
					bs->CreatePhysicsMeshes();
					pa->SkeletalBodySetups.Add(bs);

					pa->UpdateBodySetupIndexMap();
					pa->UpdateBoundsBodiesArray();
					pa->InvalidateAllPhysicsMeshes();
					pa->UpdateBoundsBodiesArray();

					{
						//Material = (UMaterial*)StaticDuplicateObject(OriginalMaterial, GetTransientPackage(), NAME_None, ~RF_Standalone, UPreviewMaterial::StaticClass()); 
						TArray<int32> child;
						int32 ii = k->GetReferenceSkeleton().FindBoneIndex(sbone.C_Str());
						k->GetChildBones(ii, child);
						for (auto &c : child) {
							k->GetChildBones(ii, child);
							USkeletalBodySetup *bs2 = Cast<USkeletalBodySetup>(StaticDuplicateObject(bs,pa, NAME_None));

							bs2->BoneName = k->GetReferenceSkeleton().GetBoneName(c);
							bs2->PhysicsType = PhysType_Simulated;
							//bs2->profile

							pa->SkeletalBodySetups.Add(bs2);



							createConstraint(sk, pa, sbone.C_Str(), bs2->BoneName);
						}
					}
				}
			}

			for (int i = 0; i < meta->colliderGroupNum; ++i) {
				auto &c = meta->colliderGroups[i];

				//bs->constrai
				FKAggregateGeom agg;

				for (int j = 0; j < c.colliderNum; ++j) {
					FKSphereElem SphereElem;
					VRM::vec3 v = {
						c.colliders[j].offset[0] * 100.f,
						c.colliders[j].offset[1] * 100.f,
						c.colliders[j].offset[2] * 100.f,
					};

					SphereElem.Center = FVector(-v[0], v[2], v[1]);
					SphereElem.Radius = c.colliders[j].radius * 100.f;
					agg.SphereElems.Add(SphereElem);
				}
				USkeletalBodySetup *bs = NewObject<USkeletalBodySetup>(pa, NAME_None);

				bs->Modify();
				bs->BoneName = c.node_name.C_Str();
				bs->AddCollisionFrom(agg);
				bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
				// newly created bodies default to simulating
				bs->PhysicsType = PhysType_Kinematic;	// fix!

				//bs.constra
				//sk->SkeletalBodySetups.Num();
				bs->InvalidatePhysicsData();
				bs->CreatePhysicsMeshes();

				pa->SkeletalBodySetups.Add(bs);

				pa->UpdateBodySetupIndexMap();
				pa->UpdateBoundsBodiesArray();
				pa->InvalidateAllPhysicsMeshes();
			}
		}

		RefreshSkelMeshOnPhysicsAssetChange(sk);
		pa->UpdateBoundsBodiesArray();

		//FSkeletalMeshModel* ImportedResource = sk->GetImportedModel();
		//if (ImportedResource->LODModels.Num() == 0)
	}

	//NewAsset->FindSocket

	saveObject(k);
	saveObject(sk);

	return nullptr;
}// sk, k






////////////////
AVrmModelActor* ULoaderBPFunctionLibrary::customActor(AVrmModelActor *src, FString filepath) {

	Assimp::Importer mImporter;
	const aiScene* mScenePtr = nullptr;

	if (filepath.IsEmpty())
	{
	}
	std::string file;
	//switch (type)
	//{
	//case EPathType::Absolute:
	file = TCHAR_TO_UTF8(*filepath);
	//	break;
	//case EPathType::Relative:
	//	file = TCHAR_TO_UTF8(*FPaths::Combine(FPaths::ProjectContentDir(), filepath));
	//	break;
	//}

	mScenePtr = mImporter.ReadFile(file, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes);

	if (mScenePtr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
	}

	{
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		//path += FPaths::GameDevelopersDir() + TEXT("VRM/");

		baseFileName = FPaths::GetBaseFilename(filepath);
		FString name = basepath + baseFileName + TEXT("/") + FPaths::GetBaseFilename(filepath);

		package = CreatePackage(nullptr, *name);
	}

	readTex(src, mScenePtr);
	readModel(src, mScenePtr);
	readMorph(src, mScenePtr);

	return nullptr;
}
