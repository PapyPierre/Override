@echo off
setlocal enabledelayedexpansion
title Tchoupi Builder - Override

rem ==========================================================
rem  Tchoupi Builder
rem  Projet : Override
rem  Auteur : Pierre Ferrari
rem ==========================================================

set PROJECT_NAME=Override
set PROJECT_PATH=D:\Work\Repo\Override\Override
set ENGINE_PATH=D:\Work\Repo\UnrealEngine
set PLATFORM=Win64
set CONFIG=Development
set ARCHIVE_BASE=D:\Work\Build\Override\Build
set LOG_DIR=%PROJECT_PATH%\Saved\Logs

set TARGET_INPUT=%1
set COOK_INPUT=%2

if /I "%TARGET_INPUT%"=="client" (
    set TARGET=Client
) else if /I "%TARGET_INPUT%"=="server" (
    set TARGET=Server
) else (
    echo [ERREUR] Type de build invalide : "%TARGET_INPUT%"
    echo Utilisez "client" ou "server"
    exit /b 1
)

if /I "%COOK_INPUT%"=="iterate" (
    set COOK_MODE=-cook -iterate
    set COOK_DESC=Iteratif
) else if /I "%COOK_INPUT%"=="full" (
    set COOK_MODE=-cook
    set COOK_DESC=Complet
) else (
    echo [ERREUR] Mode de cook invalide : "%COOK_INPUT%"
    echo Utilisez "iterate" ou "full"
    exit /b 1
)

for /f "tokens=1-4 delims=/ " %%a in ("%date%") do (
    set YYYY=%%d
    set MM=%%b
    set DD=%%c
)
for /f "tokens=1-2 delims=: " %%a in ("%time%") do (
    set HH=%%a
    set MN=%%b
)

set DATETIME=%YYYY%-%MM%-%DD%_%HH%h%MN%m

if /I "%TARGET%"=="Client" (
    set TARGET_SUBFOLDER=Client
) else (
    set TARGET_SUBFOLDER=Server
)

set ARCHIVE_DIR=%ARCHIVE_BASE%\%TARGET_SUBFOLDER%\%PLATFORM%_%CONFIG%_%DATETIME%

set LOG_FILE=%LOG_DIR%\Build_%TARGET%_%DATETIME%.log

echo ==========================================================
echo  TCHOUPI BUILDER
echo ==========================================================
echo  Projet : %PROJECT_NAME%
echo  Type : %TARGET%
echo  Mode de cook : %COOK_DESC%
echo  Plateforme : %PLATFORM%
echo  Configuration : %CONFIG%
echo  Archive : %ARCHIVE_DIR%
echo  Log : %LOG_FILE%
echo ==========================================================

pushd "%ENGINE_PATH%\Engine\Build\BatchFiles"

RunUAT.bat BuildCookRun ^
 -project="%PROJECT_PATH%\%PROJECT_NAME%.uproject" ^
 -noP4 ^
 -target="%PROJECT_NAME%%TARGET%" ^
 -targetplatform=%PLATFORM% ^
 -config=%CONFIG% ^
 -build ^
 %COOK_MODE% ^
 -pak ^
 -stage ^
 -archive ^
 -archivedirectory="%ARCHIVE_DIR%" ^
 -utf8output ^
 -CrashReporter ^
 -nocompileeditor ^
 -prereqs ^
 > "%LOG_FILE%" 2>&1

set BUILD_ERROR=%errorlevel%

popd

echo.
echo ==========================================================
if %BUILD_ERROR% neq 0 (
    echo   [ECHEC] Code erreur : %BUILD_ERROR%
    echo   Consultez le log ici :
    echo   %LOG_FILE%
) else (
    echo   [SUCCES] Build %TARGET% (%COOK_DESC%) termine
    echo   Archive : %ARCHIVE_DIR%
)
echo ==========================================================

exit /b %BUILD_ERROR%