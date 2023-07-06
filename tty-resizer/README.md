TTY Resizer for SimpleCom
===

TTY Resizer would resize specified console when SimpleCom sends resize request.

# How to build

## Requirements

Linux kernel v6.0 or later

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
* /etc/default/tty-resizer

# How it works

```bash
sudo tty-resizer [-v] /dev/<TTY device>
```

Add `-v` if you want to see the log from libbpf.

## Install systemd unit file

Specify TTY device file to /etc/default/tty-resizer (/dev/ttyS0 is set by default), then run `systemd enable` in below:

```bash
sudo systemd enable tty-resizer
```

## Send command

Send following format command to TTY console:

```
[0xF4][Row];[Col]t
```

Both `Row` and `Col` are unsigned short.

When `0xF4` is received in tty-resizer, subsequent chars are captured in tty-resizer, and they will not propagate to real TTY. `t` is terminator, then capture mode in tty-resizer is finished, and subsequent chars are propagated to real TTY, and `TIOCSWINSZ` ioctl would be issued to the specified TTY. `c` means "cancel" for tty-resizer, then capture mode will be finished, and happens nothing.

0-9 and `;`, `t`, `c` is valid chars on capture mode. Capture mode will be aborted when other char is received - it would be treated as `c`.
