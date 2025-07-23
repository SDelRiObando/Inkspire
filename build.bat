@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set WINSDK_VER=10.0.22621.0
set WINSDK_PATH=C:\Program Files (x86)\Windows Kits\10

cl.exe /EHsc src\main.cpp ^
  /I "%WINSDK_PATH%\Include\%WINSDK_VER%\ucrt" ^
  /I "%WINSDK_PATH%\Include\%WINSDK_VER%\um" ^
  /I "%WINSDK_PATH%\Include\%WINSDK_VER%\shared" ^
  /I "C:\Users\Santiago\vcpkg\installed\x64-windows\include" ^
  /I "C:\Users\Santiago\vcpkg\installed\x64-windows\include\SDL2" ^
  /link ^
  /LIBPATH:"%WINSDK_PATH%\Lib\%WINSDK_VER%\ucrt\x64" ^
  /LIBPATH:"%WINSDK_PATH%\Lib\%WINSDK_VER%\um\x64" ^
  "C:\Users\Santiago\vcpkg\installed\x64-windows\lib\SDL2.lib" ^
  "C:\Users\Santiago\vcpkg\installed\x64-windows\lib\glew32.lib" ^
  opengl32.lib ^
  /OUT:main.exe
