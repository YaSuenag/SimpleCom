TTY Resizer for SimpleCom
===

TTY Resizer would resize specified console when SimpleCom sends resize request.

# How to build

## Requirements

Linux kernel v6.0 or later

v6.4 or later is required when you run TTY Resizer on AArch64 Linux.

### Fedora

* make
* clang
* libbpf-devel
* bpftool
* kernel-devel

### Ubuntu

* make
* clang
* libbpf-dev
* linux-tools-common

On distros which does not provide vmlinux.h like Ubuntu, you have to generate it via [bpftool btf dump](https://manpages.ubuntu.com/manpages/focal/man8/bpftool-btf.8.html).

```
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

**⚠️NOTE⚠️** [Linux kernel from Raspberry Pi](https://github.com/raspberrypi/linux) disables `CONFIG_DEBUG_INFO_BTF` by default. This configuration is mandatory for tty-resizer. So you have to build and deploy the kernel which enables `CONFIG_DEBUG_INFO_BTF`, and also `bpftool` for it by yourself. See [document by Raspberry Pi](https://www.raspberrypi.com/documentation/computers/linux_kernel.html) for details.

## Build

```bash
make
```

## Install

```bash
sudo make install
```

`make install` will install following files:

* /usr/local/sbin/tty-resizer
* /usr/lib/systemd/system/tty-resizer.service

# How it works

```bash
sudo tty-resizer [-v] /dev/<TTY device>
```

Add `-v` if you want to see the log from libbpf.

## Install systemd unit file

Specify TTY device file to tty-resizer.service (/dev/ttyGS0 is set by default), then run `systemd enable` in below:

```bash
sudo systemctl enable tty-resizer
```

If you want to change the device, run following commands:

```bash
sudo systemctl daemon-reaload
sudo systemctl restart tty-resizer
```

## Send command

Send following format command to TTY console:

```
[0x05][Row];[Col]t
```

Both `Row` and `Col` are unsigned short.

When `0x05` (ENQ) is received in tty-resizer, subsequent chars are captured in tty-resizer, and they will not propagate to real TTY. `t` is terminator, then capture mode in tty-resizer is finished, and subsequent chars are propagated to real TTY, and `TIOCSWINSZ` ioctl would be issued to the specified TTY. `c` means "cancel" for tty-resizer, then capture mode will be finished, and happens nothing.

0-9 and `;`, `t`, `c` is valid chars on capture mode. Capture mode will be aborted when other char is received - it would be treated as `c`.

# Known issue

Some strings for resizing might be shown on your serial console in earlier phase (especially just after the boot) like following:

```
[root@raspberry-pi ~]# 30;122t31;122t31;123t31;124t
```

You should wait few seconds if you start tty-resizer from `tty-resizer.service` when you encounter this problem.

This might be caused that `ioctl` to TTY is failed. So `tty-resizer.service` in this source would restart when the issue happens to avoid it. Then it will work fine.
