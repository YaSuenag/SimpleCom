.PHONY: clean

TARGET = tty-resizer
CC := clang
BPFTOOL := bpftool

CFLAGS = -g -O2 -Wall
LDFLAGS = -lbpf

all: tty-resizer.skel.h
	$(CC) $(CFLAGS) -c tty-resizer.c -o tty-resizer.o
	$(CC) $(CFLAGS) tty-resizer.o -o $(TARGET) $(LDFLAGS)

tty-resizer.skel.h: tty-resizer.bpf.o
	$(BPFTOOL) gen skeleton tty-resizer.bpf.o > $@

tty-resizer.bpf.o: vmlinux.h
	$(CC) $(CFLAGS) -target bpf -D__TARGET_ARCH_x86 -c tty-resizer.bpf.c -o $@

vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

clean:
	$(RM) $(TARGET) *.o *.skel.h vmlinux.h

install: $(TARGET)
	cp -f $(TARGET) /usr/local/sbin/
	cp -f tty-resizer.service /usr/lib/systemd/system/
	echo 'DEVFILE=/dev/ttyS0' > /etc/default/tty-resizer

uninstall:
	systemctl disable tty-resizer
	$(RM) /usr/lib/systemd/system/tty-resizer.service /etc/default/tty-resizer /usr/local/sbin/$(TARGET)
