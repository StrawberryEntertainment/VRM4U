#include "VRMImporterFactory.h"
#include "VRMImporterLog.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "PaperJSONHelpers.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"

#define LOCTEXT_NAMESPACE "VRMImporter"


UVRMImporterFactory::UVRMImporterFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}


bool UVRMImporterFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if( Extension == TEXT("vrm") || Extension == TEXT("gltf") || Extension == TEXT("glb"))
	{
		return true;
	}
	return false;
}

UClass* UVRMImporterFactory::ResolveSupportedClass()
{
	UClass* ImportClass = NULL;
	/*
	if (ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh)
	{
		ImportClass = USkeletalMesh::StaticClass();
	}
	else if (ImportUI->MeshTypeToImport == FBXIT_Animation)
	{
		ImportClass = UAnimSequence::StaticClass();
	}
	else
	{
		ImportClass = UStaticMesh::StaticClass();
	}
*/
	return ImportClass;
}

/*
UObject* UVRMImporterFactory::FactoryCreateFile
(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	const FString& InFilename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled
)
{
	return nullptr;
}
*/

UObject* UVRMImporterFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	return nullptr;
}

/*
UObject* UVRMImporterFactory::CreateNewAsset(UClass* AssetClass, const FString& TargetPath, const FString& DesiredName, EObjectFlags Flags)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	// Create a unique package name and asset name for the frame
	const FString TentativePackagePath = PackageTools::SanitizePackageName(TargetPath + TEXT("/") + DesiredName);
	FString DefaultSuffix;
	FString AssetName;
	FString PackageName;
	AssetToolsModule.Get().CreateUniqueAssetName(TentativePackagePath, DefaultSuffix,  PackageName,  AssetName);

	// Create a package for the asset
	UObject* OuterForAsset = CreatePackage(nullptr, *PackageName);

	// Create a frame in the package
	UObject* NewAsset = NewObject<UObject>(OuterForAsset, AssetClass, *AssetName, Flags);
	FAssetRegistryModule::AssetCreated(NewAsset);

	NewAsset->Modify();
	return NewAsset;
}
*/

/*
UObject* USpriterImporterFactory::ImportAsset(const FString& SourceFilename, const FString& TargetSubPath)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<FString> FileNames;
	FileNames.Add(SourceFilename);

	TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(FileNames, TargetSubPath);
	return (ImportedAssets.Num() > 0) ? ImportedAssets[0] : nullptr;
}
*/

//////////////////////////////////////////////////////////////////////////

//#undef SPRITER_IMPORT_ERROR
#undef LOCTEXT_NAMESPACE