/*
 * Copyright (C) 2023, Yasumasa Suenaga
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
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
  if(ch == '\x05'){
    send_to_buffer = true;

    ch = '\0';
    bpf_probe_write_user(ubuf, &ch, 1);
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
