@echo off
for /d /r "%~dp0" %%i in (*) do (
    mkdir "C:\sync\src_c%%~pnxi" > NUL 2> NUL
)
for /r "%~dp0" %%i in (*.c *.cpp *.h *.hpp makefile) do (
    copy /y "%%i" "C:\sync\src_c%%~pnxi" > NUL 2> NUL
)
REM pause
