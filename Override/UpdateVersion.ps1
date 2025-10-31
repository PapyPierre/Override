# UpdateVersion.ps1
param (
    [string]$VersionFile = "D:\Work\Repo\Override\Override\VersionInfo.txt"
)

# Lecture du fichier existant
$versionData = Get-Content $VersionFile | ForEach-Object {
    $parts = $_ -split "="
    @{ Key = $parts[0].Trim(); Value = [int]$parts[1].Trim() }
} | ForEach-Object { $_ }

$version = @{}
foreach ($v in $versionData) { $version[$v.Key] = $v.Value }

# Stockage des anciens
$oldMajor = $version["Major"]
$oldMinor = $version["Minor"]
$oldRevision = $version["Revision"]
$oldPatch = $version["Patch"]

# Création du numéro de version précédent
$previousBase = "$oldMajor.$oldMinor.$oldRevision"

# Vérifie si un fichier de suivi de dernière version existe
$lastVersionFile = "D:\Work\Repo\Override\Override\LastBaseVersion.txt"
if (Test-Path $lastVersionFile) {
    $lastBase = Get-Content $lastVersionFile -Raw
} else {
    $lastBase = ""
}

# Si la base (0.1.2) a changé, on reset le patch
if ($previousBase -ne $lastBase) {
    $version["Patch"] = 0
} else {
    $version["Patch"]++
}

# Sauvegarde du fichier mis à jour
Set-Content -Path $VersionFile -Value @(
    "Major=$($version["Major"])",
    "Minor=$($version["Minor"])",
    "Revision=$($version["Revision"])",
    "Patch=$($version["Patch"])"
)

# Mise à jour du dernier base
Set-Content -Path $lastVersionFile -Value $previousBase

# Génération de la version finale avec date
$dateTag = (Get-Date).ToString("ddMMyy")
$fullVersion = "$($version["Major"]).$($version["Minor"]).$($version["Revision"]).$($version["Patch"]).$dateTag"

Write-Host "##[group]Generated Version"
Write-Host "Full version: $fullVersion"
Write-Host "##[endgroup]"

# Export GitHub output
"FULL_VERSION=$fullVersion" | Out-File -FilePath $env:GITHUB_ENV -Append
