SimpleCom
![CodeQL](../../workflows/CodeQL/badge.svg)

Console app for serial connection.
===================
# How to use

1. Conenct serial adaptor to your PC
2. Start `SimpleCom.exe`
3. Setup serial console
    * If you start SimpleCom without any command line options, use dialog to configure
        * `> SimpleCom.exe`
    * You can configure serial port with command line, then dialog would not be shown
        * `> SimpleCom.exe <options> COM[N]`
        * `COM[N]` is mandatory to specify serial port
4. Operate target device via the console
5. Press F1 to leave its serial session and to finish SimpleCom

## Command line options

* `--baud-rate [num]`
    * baud rate
    * default: `115200`
* `--byte-size [num]`
    * byte size
    * default: `8`
* `--parity [val]`
    * parity
    * set one of following values
        * `none`
        * `odd`
        * `even`
        * `mark`
        * `space`
    * default: `none`
* `--stop-bits [val]`
    * stop bits
    * set one of following values
        * `1`
        * `1.5`
        * `2`
    * default: `1`
* `--flow-control`
    * flow control
    * set one of following values
        * `none`
        * `hardware`
        * `software`
    * default: `none`

# How to build

Use [SimpleCom.sln](https://github.com/YaSuenag/SimpleCom/blob/master/SimpleCom.sln) on your Visual Studio.  
I confirmed x64 build on VS 2019.

## Distribution package

You can get distribution package when you do release build.

* [dist](dist): ZIP archive
* [Installer](Installer): MSI installer

MSI installer installes SimpleCom.exe and README.md (and also custom action assembly). It will deploy JSON fragment for SimpleCom into `%ProgramData%` or `%LocalAppData%` (it depends on install user choice). So the use can use SimpleCom without any configuration (as following) on Windows Terminal.

# How to use on [Windows Terminal](https://github.com/microsoft/terminal)

You can use [fragments.json](fragments.json) to add SimpleCom to your Windows Terminal.  
For example, run following commands to add SimpleCom for all users.

```
PS > mkdir "C:\ProgramData\Microsoft\Windows Terminal\Fragments\SimpleCom"
PS > cp C:\Path\To\fragment.json "C:\ProgramData\Microsoft\Windows Terminal\Fragments\SimpleCom"
```

Please see [Applications installed from the web](https://docs.microsoft.com/ja-jp/windows/terminal/json-fragment-extensions#applications-installed-from-the-web) in Microsoft Docs.

# Notes

* SimpleCom sends / receives VT100 escape sequences. So the serial device to connect via SimpleCom needs to support VT100 or compatible shell.
* SimpleCom supports ANSI chars only, so it would not work if multibyte chars (e.g. CJK chars) are given.

# License

GNU General Public License v2.0
