@echo off

cd out\appimage_release
if "%1"=="" (cmake ..\.. -G Ninja -U BUILD_CUST_PROJ) else (cmake ..\.. -G Ninja -D BUILD_CUST_PROJ=%1)
ninja > ../../log.txt
cd hex
copy /Y /B  ..\..\..\tools\win32\target-upgrade.xml target-upgrade.xml
nwy_mkpackage.exe
cd ..\..\..\