#include "VRM4UImporterFactory.h"
#include "VRM4UImporterLog.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"
//#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmAssetListObject.h"

#define LOCTEXT_NAMESPACE "VRMImporter"


UVRM4UImporterFactory::UVRM4UImporterFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add( TEXT( "vrm;Model" ) );

	bCreateNew = false;
	bEditorImport = true;

}


bool UVRM4UImporterFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if( Extension == TEXT("vrm") || Extension == TEXT("gltf") || Extension == TEXT("glb"))
	{
		fullFileName = Filename;
		return true;
	}
	return false;
}

UClass* UVRM4UImporterFactory::ResolveSupportedClass()
{
	UClass* ImportClass = USkeletalMesh::StaticClass();

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

UObject* UVRM4UImporterFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	//static ConstructorHelpers::FObjectFinder<UVrmAssetListObject> MatClass(TEXT("Class'/VRM4U/VrmObjectListBP.VrmObjectListBP'"));
	//static ConstructorHelpers::FObjectFinder<UVrmAssetListObject> MatClass(TEXT("Blueprint'/VRM4U/VrmObjectListBP'"));

	//UObject* objFinder = StaticLoadObject(UVrmAssetListObject::StaticClass(), nullptr, TEXT("Blueprint'/VRM4U/VrmObjectListBP'"));
	UObject* objFinder = NewObject<UVrmAssetListObject>(InParent, NAME_None, RF_Transactional);

	//if (MatClass.Object != NULL)
	{
		//auto a = NewObject<UVrmAssetListObject>(MatClass.Object, NAME_None, RF_Transactional);
		//MatClass.Object; 
		//ULoaderBPFunctionLibrary::LoadVRMFile(nullptr, fullFileName);
		ULoaderBPFunctionLibrary::LoadVRMFile(Cast<UVrmAssetListObject>(objFinder), fullFileName);

		//ULoaderBPFunctionLibrary::LoadVRMFile(Cast<UVrmAssetListObject>(objFinder), fullFileName);

		//return a;
	}

	return objFinder;
}
UObject* UVRM4UImporterFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
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