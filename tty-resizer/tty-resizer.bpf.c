#include "vmlinux.h"
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>

#define RINGBUF_SZ 10

char LICENSE[] SEC("license") = "GPL";

volatile ino_t tty_ino = -1;
volatile bool send_to_buffer = false;
const int ringbuf_sz = RINGBUF_SZ;

struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, RINGBUF_SZ);
} received_chars SEC(".maps");


SEC("fexit/tty_read")
int BPF_PROG(tty_read, struct kiocb *iocb, struct iov_iter *to, int ret){
  char ch;
  void *ubuf;

  if (BPF_CORE_READ(iocb, ki_filp, f_inode, i_ino) != tty_ino){
    return 0;
  }

  ubuf = BPF_CORE_READ(to, ubuf);
  bpf_probe_read_user(&ch, 1, ubuf);
  if(ch == '\xF4'){
    send_to_buffer = true;
  }
  else if(send_to_buffer){
    u64 flags = BPF_RB_NO_WAKEUP;
    if(ch == 't'){
      flags = BPF_RB_FORCE_WAKEUP;
      send_to_buffer = false;
    }
    else if((ch != ';') && (('0' > ch) || ('9' < ch))){
      /* handles invalid chars */
      ch = 'c';
      flags = BPF_RB_FORCE_WAKEUP;
      send_to_buffer = false;
    }
    bpf_ringbuf_output(&received_chars, &ch, 1, flags);

    ch = '\0';
    bpf_probe_write_user(ubuf, &ch, 1);
  }

  return 0;
}
