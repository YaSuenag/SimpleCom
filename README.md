# SimpleCom

Console app for serial connection.

# How to use

1. Conenct serial adaptor to your PC
2. Start `SimpleCom.exe`
3. Setup serial console via setup dialog
4. Operate target device via the console
5. Press F1 to leave its serial session and to finish SimpleCom

# How to build

Use [SimpleCom.sln](https://github.com/YaSuenag/SimpleCom/blob/master/SimpleCom.sln) on your Visual Studio.  
I confirmed x64 build on VS 2019.

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
