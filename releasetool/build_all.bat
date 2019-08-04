::4_23
call build_ver.bat 4.23 Win64 MyProjectBuildScriptEditor VRM4U_4_23.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.23 Win64 MyProjectBuildScript VRM4U_4_23_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 4.23 Android MyProjectBuildScript VRM4U_4_23_android.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_22

call build_ver.bat 4.22 Win64 MyProjectBuildScriptEditor VRM4U_4_22.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.22 Win64 MyProjectBuildScript VRM4U_4_22_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 4.22 Android MyProjectBuildScript VRM4U_4_22_android.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_21


call build_ver.bat 4.21 Win64 MyProjectBuildScriptEditor VRM4U_4_21.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


call build_ver.bat 4.20 Win64 MyProjectBuildScriptEditor VRM4U_4_20.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.19 Win64 MyProjectBuildScriptEditor VRM4U_4_19.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

:finish
exit /b 0

:err
exit /b 1


