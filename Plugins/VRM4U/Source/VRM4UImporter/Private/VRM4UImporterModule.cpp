
#include "CoreMinimal.h"
#include "VRM4UImporterLog.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"
#include "VrmRuntimeSettings.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "VRM4UImporter"

DEFINE_LOG_CATEGORY(LogVRM4UImporter);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRM4UImporterModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->RegisterSettings("Project", "Plugins", "VRM4U",
				LOCTEXT("RuntimeSettingsName", "VRM4U"),
				LOCTEXT("RuntimeSettingsDescription", "Configure the VRM4U plugin"),
				GetMutableDefault<UVrmRuntimeSettings>());
		}
	}

	virtual void ShutdownModule() override
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "VRM4U");
		}
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRM4UImporterModule, VRM4UImporter);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
