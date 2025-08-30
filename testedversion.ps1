$solutiondir = $PSScriptRoot
$os_version = (Get-ComputerInfo).OsVersion

Write-Output "OS Version: $os_version"

$manifest_file = "$solutiondir\SimpleCom\app.manifest"
$xml = [xml]::new()
$xml.Load($manifest_file)
$xml.assembly.compatibility.application.maxversiontested.Id = $os_version
$xml.Save($manifest_file)