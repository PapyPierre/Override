@echo off
REM ==============================
REM Script de lancement du projet UE
REM ==============================

REM --------- CONFIGURATION ---------
REM Chemin vers votre build custom d'UE (modifiez selon votre machine)
set UE_PATH=D:\Work\Repo\UnrealEngine\LocalBuilds\Engine\Windows

REM Chemin vers le projet
set PROJECT_PATH=D:\Work\Repo\Override\Override\Override.uproject

REM Optionnel : Configuration de build (Development, Shipping, etc.)
set CONFIGURATION=Development

REM --------- LANCEMENT ---------
echo Lancement de %PROJECT_PATH% avec le moteur %UE_PATH%
"%UE_PATH%\Engine\Binaries\Win64\UE5Editor.exe" "%PROJECT_PATH%" -game -log

pause