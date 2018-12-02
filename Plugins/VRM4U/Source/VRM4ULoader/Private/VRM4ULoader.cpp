
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

#define LOCTEXT_NAMESPACE "FVRM4ULoaderModule"

class FVRM4ULoaderModule : public IModuleInterface {
	void StartupModule()
	{
		// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	}

	void ShutdownModule()
	{
		// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
		// we call this function before unloading the module.
	}
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVRM4ULoaderModule, VRM4ULoader)