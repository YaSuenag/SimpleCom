How to increase version number
===
[versionup.ps1](versionup.ps1) helps to bump the new version number. Following files would be updated automatically.

* [commons/generated/version.h](commons/generated/version.h)
    * This header is used in resource file - it affects property of SimpleCom.exe
    * [zip-packaging.ps1](zip-packaging.ps1) would refer this header file when generates ZIP archive
* [SimpleCom/app.manifest](SimpleCom/app.manifest)
    * Assembly version
* [Installer/Installer.vdproj](Installer/Installer.vdproj)
    * GUID for product code, package code
    * Product version
    * Output filename
    * I cannot ensure the result of this script because the spec of .vdproj file is closed.

# Usage

```
versionup.ps1 [version number]
```

* Version number should be in 3 or 4 digits (x.y.z or w.x.y.z)
* Version number is converted into [SemVer](https://semver.org/)
    * x.y.z is given, it is used straightly as SemVer, and adds `0` as a revision number in Windows
    * w.x.y.z is given, it is converted into w.x.t+z
