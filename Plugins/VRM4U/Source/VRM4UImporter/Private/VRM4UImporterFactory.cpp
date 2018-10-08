#include "VRM4UImporterFactory.h"
#include "VRM4UImporterLog.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"
#include "Misc/Paths.h"
#include "Engine/SkeletalMesh.h"
//#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmAssetListObject.h"
#include "Engine/Blueprint.h"

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
template<class T>
T* GetObjectFromStringAsset(FStringAssetReference const& AssetRef)
{
	UObject* AlreadyLoadedObj = AssetRef.ResolveObject();
	if (AlreadyLoadedObj)
	{
		return Cast<T>(AlreadyLoadedObj);
	}

	UObject* NewlyLoadedObj = AssetRef.TryLoad();
	if (NewlyLoadedObj)
	{
		return Cast<T>(NewlyLoadedObj);
	}

	return nullptr;
}


UObject* UVRM4UImporterFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	
	//static ConstructorHelpers::FObjectFinder<UObject> MatClass(TEXT("/Game/test/NewMaterial.NewMaterial"));
	//static ConstructorHelpers::FObjectFinder<UClass> MatClass(TEXT("Blueprint'/VRM4U/VrmObjectListBP.VrmObjectListBP_C'"));
	//static ConstructorHelpers::FObjectFinder<UVrmAssetListObject> MatClass(TEXT("Blueprint'/VRM4U/VrmObjectListBP'"));
	//static ConstructorHelpers::FObjectFinder<UObject> MatClass(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
	//UVrmAssetListObject *m = Cast<UVrmAssetListObject>(MatClass.Object);

	//UObject* objFinder = StaticLoadObject(UVrmAssetListObject::StaticClass(), nullptr, TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP_C"));
	//UObject* objFinder = NewObject<UVrmAssetListObject>(InParent, NAME_None, RF_Transactional);

	FSoftObjectPath r(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
	UObject *u = r.TryLoad();
	UClass *c = (UClass*)(Cast<UBlueprint>(u)->GeneratedClass);

	//UVrmAssetListObject *m = Cast<UVrmAssetListObject>(u);
	//FSoftClassPath r(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
	//UObject *u = r.TryLoad();
	//auto aaa = NewObject<UObject>(c);
	UVrmAssetListObject *m = NewObject<UVrmAssetListObject>((UObject*)GetTransientPackage(), c);

	if (m){
		//auto a = NewObject<UVrmAssetListObject>(MatClass.Object, NAME_None, RF_Transactional);
		//MatClass.Object; 
		//ULoaderBPFunctionLibrary::LoadVRMFile(nullptr, fullFileName);
		ULoaderBPFunctionLibrary::SetImportMode(true, Cast<UPackage>(InParent));
		ULoaderBPFunctionLibrary::LoadVRMFile(m, fullFileName);
		ULoaderBPFunctionLibrary::SetImportMode(false, nullptr);
	}

	return InParent;
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