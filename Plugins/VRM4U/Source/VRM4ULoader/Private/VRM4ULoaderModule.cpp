
#include "Engine.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"


#if PLATFORM_WINDOWS

#include "AllowWindowsPlatformTypes.h"
#include "shellapi.h"
#include "HideWindowsPlatformTypes.h"

#include "Windows/WindowsApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "VrmDropFiles.h"
#include "VrmRuntimeSettings.h"


class FDropMessageHandler : public IWindowsMessageHandler { 
public:

	virtual bool ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam, int32& OutResult) override {

		int32 NumFiles;
		HDROP hDrop;
		TCHAR NextFile[MAX_PATH];

		{
			const UObject *p = UVrmDropFilesComponent::getLatestActiveComponent();
			const UWorld* World = nullptr;
			if (p) {
				World = GEngine->GetWorldFromContextObject(p, EGetWorldErrorMode::ReturnNull);
			}

			{
				bool bCurrentState = false;
				if (World) {

					switch (World->WorldType) {
					case EWorldType::Game:
					case EWorldType::PIE:
						bCurrentState = true;
						break;
					}

				}
				static bool bDropEnable = false;
				if (bDropEnable != bCurrentState) {
					bDropEnable = bCurrentState;
					DragAcceptFiles(hwnd, bDropEnable);
					//DragAcceptFiles(GetForegroundWindow(), bDropEnable);
					DragAcceptFiles(GetActiveWindow(), bDropEnable);
				}
			}
		}

		if (msg == WM_DROPFILES)
		{
			hDrop = (HDROP)wParam;
			// Get the # of files being dropped.
			NumFiles = DragQueryFile(hDrop, -1, NULL, 0);

			for (int32 File = 0; File < NumFiles; File++)
			{
				// Get the next filename from the HDROP info.
				if (DragQueryFile(hDrop, File, NextFile, MAX_PATH) > 0)
				{
					FString Filepath = NextFile;
					//Do whatever you want with the filepath

					UVrmDropFilesComponent::StaticOnDropFilesDelegate.Broadcast(Filepath);

				}
			}
			return false;
		}    
		return false;
	}
};
#endif


#define LOCTEXT_NAMESPACE "FVRM4ULoaderModule"

class FVRM4ULoaderModule : public IModuleInterface {
	void StartupModule()
	{
		// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

#if PLATFORM_WINDOWS
		const UVrmRuntimeSettings* Settings = GetDefault<UVrmRuntimeSettings>();
		if (Settings) {
			if (Settings->bDropVRMFileEnable) {
				static FDropMessageHandler DropMessageHandler;// = FDropMessageHandler();
															  //FWindowsApplication* WindowsApplication = (FWindowsApplication*)GenericApplication.Get();
				TSharedPtr<GenericApplication> App = FSlateApplication::Get().GetPlatformApplication();

				auto WindowsApplication = reinterpret_cast<FWindowsApplication*>(App.Get());

				//WindowsApplication->SetMessageHandler(DropMessageHandler);
				WindowsApplication->AddMessageHandler(DropMessageHandler);
			}
		}

#endif

	}

	void ShutdownModule()
	{
		// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
		// we call this function before unloading the module.
	}
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVRM4ULoaderModule, VRM4ULoader)