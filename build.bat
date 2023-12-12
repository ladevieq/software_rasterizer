call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64

set C_FLAGS=/nologo /O2 /WX- /W4 /Zi /GS- /GR- /Gs1000000 /std:c11
set L_FLAGS=/nodefaultlib /subsystem:console /stack:100000,100000
set LIBS=kernel32.lib user32.lib gdi32.lib

cl %C_FLAGS% main.c /link %L_FLAGS% %LIBS%
