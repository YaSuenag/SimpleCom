# SimpleCom

Console app for serial connection.

# How to use

1. Conenct serial adaptor to your PC
2. Start `SimpleCom.exe`
3. Setup serial console via setup dialog
4. Operate target device via the console
5. Press F1 to leave its serial session and to finish SimpleCom

# How to use on [Windows Terminal](https://github.com/microsoft/terminal)

You need to add a profile for SimpleCom. The example is available [here](WindowsTerminal-setting-example/profile.json).

# Notes

SimpleCom sends arrow keys as VT100 escape sequences. So the serial device to connect via SimpleCom need to support VT100 or compatible shell if you want to use arrow keys.

# License

GNU General Public License v2.0
