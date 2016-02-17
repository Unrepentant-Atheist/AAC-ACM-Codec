@ECHO OFF

IF "%PROCESSOR_ARCHITECTURE%"=="" GOTO 32_BIT_OS
IF %PROCESSOR_ARCHITECTURE%==x86 GOTO 32_BIT_OS

SET FILENAME=AACACM64.inf
TITLE AAC ACM Codec 64-bit
ECHO Installing...

REM Workaround for the lost current dir while using "Run as Administrator"
SETLOCAL ENABLEEXTENSIONS
IF EXIST "%~dp0" CD /D "%~dp0"

IF NOT EXIST "%FILENAME%" GOTO FILE_MISSING
rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 .\%FILENAME%

CLS
TYPE ..\GPL.txt | more
GOTO End

:FILE_MISSING
ECHO The file %FILENAME% is missing!
pause > nul
GOTO End

:32_BIT_OS
ECHO You cannot install this on a 32-bit OS.
pause > nul

:End
