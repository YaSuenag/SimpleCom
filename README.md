SimpleCom
![CodeQL](../../workflows/CodeQL/badge.svg)
![Unit test](../../workflows/Unit%20test/badge.svg)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/YaSuenag/SimpleCom/badge)](https://securityscorecards.dev/viewer/?uri=github.com/YaSuenag/SimpleCom)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8152/badge)](https://www.bestpractices.dev/projects/8152)
===================

Console app for serial connection.

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

| Option | Default | Description |
| :----- | :------ | :---------- |
| `--show-dialog` | false | Show setup dialog even if command line arguments are passed. |
| `--wait-serial-device [seconds]` | 0 (disable) | Wait specified seconds for serial devices are available. |
| `--utf8` | false | Use UTF-8 code page on SimpleCom console. |
| `--tty-resizer` | false | Use TTY resizer. See [README.md in TTY resizer](tty-resizer/README.md). |
| `--baud-rate [num]` | 115200 | Baud rate |
| `--byte-size [num]` | 8 | Byte size |
| `--parity [val]` | `none` | Set one of following values as a parity: <ul><li>none</li><li>odd</li><li>even</li><li>mark</li><li>space</li></ul> |
| `--stop-bits [val]` | `1` | Set one of following values as a stop bits: <ul><li>1</li><li>1.5</li><li>2</li></ul> |
| `--flow-control [val]` | `none` | Set one of following values as a flow control: <ul><li>none</li><li>hardware</li><li>software</li></ul> |
| `--auto-reconnect` | false | Reconnect to peripheral automatically when serial session is disconnected. |
| `--auto-reconnect-pause [num]` | 3 | Pause time in seconds before reconnecting to peripheral. |
| `--auto-reconnect-timeout [num]` | 120 | Reconnect timeout |
| `--log-file [logfile]` | &lt;none&gt; | Log serial communication to file |
| `--stdin-logging` | false | Enable stdin logging<br><br>⚠️Possible to be logged each chars duplicately due to echo back from the console when this option is set, and also secrets (e.g. passphrase) typed into the console will be logged even if it is not shown on the console. |
| `--help` | - | Show help message |

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
* F1 key is hooked by SimpleCom, so escase sequence of F1 (`ESC O P`) would not be propagated.
* SimpleCom supports ANSI chars only, so it would not work if multibyte chars (e.g. CJK chars) are given.
* Run [resize](https://linux.die.net/man/1/resize) provided by xterm if you want to align VT size of Linux box with your console window.

# License

GNU General Public License v2.0
