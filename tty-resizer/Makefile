.PHONY: clean

TARGET = tty-resizer
KERNEL_DEVEL_ROOT ?= /usr/src/kernels/$(shell uname -r)
CC := clang
BPFTOOL ?= bpftool

CFLAGS = -g -O2 -Wall
CPPFLAGS = -I$(KERNEL_DEVEL_ROOT) -I../common
LDFLAGS = -lbpf

all: tty-resizer.skel.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c tty-resizer.c -o tty-resizer.o
	$(CC) $(CFLAGS) tty-resizer.o -o $(TARGET) $(LDFLAGS)

tty-resizer.skel.h: tty-resizer.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

tty-resizer.bpf.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -target bpf -c tty-resizer.bpf.c -o $@

clean:
	$(RM) $(TARGET) *.o *.skel.h

install: $(TARGET)
	cp -f $(TARGET) /usr/local/sbin/
	cp -f tty-resizer.service /usr/lib/systemd/system/

uninstall:
	systemctl disable tty-resizer
	$(RM) /usr/lib/systemd/system/tty-resizer.service /usr/local/sbin/$(TARGET)
