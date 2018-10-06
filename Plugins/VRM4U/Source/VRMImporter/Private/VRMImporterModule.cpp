
#include "CoreMinimal.h"
#include "VRMImporterLog.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "VRMImporter"

DEFINE_LOG_CATEGORY(LogVRMImporter);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRMImporterModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRMImporterModule, VRMImporter);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
