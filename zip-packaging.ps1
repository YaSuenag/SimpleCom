$solutiondir = $PSScriptRoot

$version_grep = Select-String -Path $solutiondir\common\generated\version.h -Pattern ' SEMVER \"(.+)\"$'
$version = $version_grep.Matches.Groups[1].Value

if($version -eq $null){
  Write-Output "Unknown version. Please check version.h ."
  return
}

$distname = "SimpleCom-$version"
$destdir = "$solutiondir\dist\$distname"

Remove-Item -Recurse -Force $destdir*
New-Item $destdir -itemType Directory
Copy-Item $solutiondir\x64\Release\SimpleCom.exe $destdir
Copy-Item $solutiondir\README.md $destdir
Copy-Item $solutiondir\SECURITY.md $destdir
Copy-Item $solutiondir\fragments.json $destdir

[Reflection.Assembly]::LoadWithPartialName("System.IO.Compression.FileSystem")
[System.IO.Compression.ZipFile]::CreateFromDirectory($destdir, "$solutiondir\dist\$distname.zip", [System.IO.Compression.CompressionLevel]::Optimal, $true)
