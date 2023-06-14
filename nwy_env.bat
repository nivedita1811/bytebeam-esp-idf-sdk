@echo off

@REM launch the nwy core
call tools\core_launch.bat

@REM add the nwy.py.exe to the path
set PATH=%PATH%;%cd%\script\out