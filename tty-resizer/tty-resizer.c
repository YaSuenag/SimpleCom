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
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#include "tty-resizer.skel.h"
#include "common.h"


static int tty_fd = -1;
static ino_t tty_ino;
static struct tty_resizer_bpf *skel = NULL;
static struct ring_buffer *rb = NULL;
static char *ringbuf_received = NULL;
static int ringbuf_received_idx = 0;


static int libbpf_print(enum libbpf_print_level level, const char *format, va_list args){
  return vfprintf(stderr, format, args);
}

int on_char_received(void *ctx, void *data, size_t size){
  char ch;
  struct winsize ws;

  ch = *(char *)data;
  if(ch == 't'){
    ringbuf_received[ringbuf_received_idx] = '\0';
    if(sscanf(ringbuf_received, "%hu%c%hu", &ws.ws_row, RESIZER_SEPARATOR, &ws.ws_col) == 2){
      if(ioctl(tty_fd, TIOCSWINSZ, &ws) == -1){
        perror("ioctl");
        _exit(-200);
      }
    }
    ringbuf_received_idx = 0;
  }
  else if(ch == 'c'){
    ringbuf_received_idx = 0;
  }
  else{
    ringbuf_received[ringbuf_received_idx++] = ch;
  }

  return 0;
}

static int setup_tty(const char *ttypath){
  struct stat statst;

  tty_fd = open(ttypath, O_RDONLY | O_NOCTTY);
  if(tty_fd == -1){
    perror("open");
    return -1;
  }

  if(fstat(tty_fd, &statst) == -1){
    perror("stat");
    return -2;
  }
  tty_ino = statst.st_ino;

  return 0;
}

static int setup_bpf(){
  int ret;

  skel = tty_resizer_bpf__open_and_load();
  if (!skel) {
    fprintf(stderr, "Could not load BPF program\n");
    return -1;
  }

  skel->data->tty_ino = tty_ino;

  ret = tty_resizer_bpf__attach(skel);
  if(ret != 0){
    fprintf(stderr, "Could not attach BPF skeleton (%d)\n", ret);
    return -2;
  }

  rb = ring_buffer__new(bpf_map__fd(skel->maps.received_chars), on_char_received, NULL, NULL);
  if (!rb) {
    fprintf(stderr, "Could not create ring buffer\n");
    return -3;
  }

  ringbuf_received = (char *)malloc(skel->rodata->ringbuf_sz);
  return 0;
}

int main(int argc, char *argv[]){
  char *devfile;

  if((argc == 3) && (strcmp(argv[1], "-v") == 0)){
    libbpf_set_print(libbpf_print);
    devfile = argv[2];
  }
  else if(argc == 2){
    devfile = argv[1];
  }
  else{
    printf("Usage: %s <-v> [TTY device file]\n", argv[0]);
    return -100;
  }

  if((setup_tty(devfile) == 0) && (setup_bpf() == 0)){
    while(true){
      int ret = ring_buffer__poll(rb, -1);
      if (ret < 0 && errno != EINTR) {
        perror("ring_buffer__poll");
        break;
      }
    }
  }

  if(rb != NULL){
    ring_buffer__free(rb);
  }
  if(ringbuf_received != NULL){
    free(ringbuf_received);
  }
  if(skel != NULL){
    tty_resizer_bpf__destroy(skel);
  }
  if(tty_fd != -1){
    close(tty_fd);
  }

  return -1;
}
