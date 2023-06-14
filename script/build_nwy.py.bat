@echo off

del out\nwy.py.exe
pyinstaller nwy.py --name nwy.py --onefile
copy dist\nwy.py.exe out
del nwy.py.spec
rmdir /s /q build dist