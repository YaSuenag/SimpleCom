param(
    [string]$version,
    [string]$solutiondir
)

$distname = "SimpleCom-$version"
$destdir = "$solutiondir\dist\$distname"

Remove-Item -Recurse -Force $destdir*
New-Item $destdir -itemType Directory
Copy-Item $solutiondir\x64\Release\SimpleCom.exe $destdir
Copy-Item $solutiondir\README.md $destdir
Copy-Item $solutiondir\fragments.json $destdir

[Reflection.Assembly]::LoadWithPartialName("System.IO.Compression.FileSystem")
echo "$solutiondir\dist\$distname.zip"
[System.IO.Compression.ZipFile]::CreateFromDirectory($destdir, "$solutiondir\dist\$distname.zip", [System.IO.Compression.CompressionLevel]::Optimal, $true)