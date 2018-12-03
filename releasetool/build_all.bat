
build_ver.bat 4.19 VRM4U_4_19.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

build_ver.bat 4.20 VRM4U_4_20.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

build_ver.bat 4.21 VRM4U_4_21.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


:finish
exit /b 0

:err
exit /b 1


