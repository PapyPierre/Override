@echo off
setlocal enabledelayedexpansion
title Tchoupi Engine Auto Build - Override

rem ==========================================================
rem  Tchoupi Engine - Auto Build Launcher
rem  Projet : Override
rem  Auteur : Pierre Ferrari
rem  Description :
rem    Script automatisé pour les builds Client / Serveur
rem    utilisable en CI (GitHub Actions, Jenkins, etc.)
rem ==========================================================

rem ---------- CONFIGURATION ----------
set PROJECT_NAME=Override
set PROJECT_PATH=D:\Work\Repo\Override\Override
set ENGINE_PATH=D:\Work\Repo\UnrealEngine
set PLATFORM=Win64
set CONFIG=Development
set ARCHIVE_BASE=D:\Work\Build\Override
set LOG_DIR=%PROJECT_PATH%\Saved\Logs

rem ---------- PARAMETRES ----------
rem Usage :
rem   Build_Override_Auto.bat [client|server] [iterate|full]
rem Exemples :
rem   Build_Override_Auto.bat client iterate
rem   Build_Override_Auto.bat server full

set TARGET_INPUT=%1
set COOK_INPUT=%2

if "%TARGET_INPUT%"=="" (
    echo [ERREUR] Aucun type de build spécifié. Utilisez : client ou server
    exit /b 1
)
if "%COOK_INPUT%"=="" (
    echo [ERREUR] Aucun mode de cook spécifié. Utilisez : iterate ou full
    exit /b 1
)

rem ---------- INTERPRETATION DES PARAMETRES ----------
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

rem ---------- CONFIGURATION DYNAMIQUE ----------
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

set ARCHIVE_DIR=%ARCHIVE_BASE%\%PLATFORM%_%CONFIG%_%TARGET%_%DATETIME%
set LOG_FILE=%LOG_DIR%\Build_%TARGET%_%DATETIME%.log

echo ==========================================================
echo  TCHOUPI ENGINE - BUILD AUTO
echo ==========================================================
echo  Projet : %PROJECT_NAME%
echo  Type : %TARGET%
echo  Mode de cook : %COOK_DESC%
echo  Plateforme : %PLATFORM%
echo  Configuration : %CONFIG%
echo  Archive : %ARCHIVE_DIR%
echo  Log : %LOG_FILE%
echo ==========================================================

rem ---------- EXECUTION ----------
pushd "%ENGINE_PATH%\Engine\Build\BatchFiles"

RunUAT.bat BuildCookRun ^
 -project="%PROJECT_PATH%\%PROJECT_NAME%.uproject" ^
 -noP4 ^
 -targetplatform=%PLATFORM% ^
 -%TARGET%config=%CONFIG% ^
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