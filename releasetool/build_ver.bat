
set UE4VER=%1
set ZIPNAME=%2


cd ../Plugins

git reset HEAD ./
git checkout ./
cd ../releasetool

powershell -ExecutionPolicy RemoteSigned .\version.ps1 \"%UE4VER%\"

set UE4PATH=UE_%UE4VER%
set UNREALVERSIONSELECTOR="D:\Program Files (x86)\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe"
set UPROJECT="D:\Users\user\Documents\Unreal Projects\MyProjectBuildScript\MyProjectBuildScript\MyProjectBuildScript.uproject"

set CLEAN="D:\Program Files\Epic Games\%UE4PATH%\Engine\Build\BatchFiles\Clean.bat"
set BUILD="D:\Program Files\Epic Games\%UE4PATH%\Engine\Build\BatchFiles\Build.bat"
set REBUILD="D:\Program Files\Epic Games\%UE4PATH%\Engine\Build\BatchFiles\Rebuild.bat"
set PROJECTNAME="../MyProjectBuildScript.uproject"
set PROJECTNAMEEDITOR="MyProjectBuildScriptEditor"


call %UNREALVERSIONSELECTOR% /projectfiles %UPROJECT%
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call %CLEAN% %PROJECTNAMEEDITOR% Win64 Development %UPROJECT% -waitmutex
call %BUILD% %PROJECTNAMEEDITOR% Win64 Development %UPROJECT% -waitmutex
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


powershell -ExecutionPolicy RemoteSigned .\compress.ps1 %ZIPNAME%



:finish
exit /b 0

:err
exit  1


