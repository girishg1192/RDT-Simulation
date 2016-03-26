#ifndef __SIMULATOR__
#include "../include/simulator.h"
#define __SIMULATOR__
#include <stdlib.h>
#endif

#include "rdt_lib.h"
#include "list_lib.h"

int A_base;
int A_nextseqnum;
int window_size;
int is_timer_running;
int buff_count;

int B_expectedseq;
struct pkt B_last_ack;

struct list current_window;
struct list buffered_packets;

struct packet_elem
{
  struct pkt packet;
  struct list_elem elem;
};

void send_buffered();

#define TIMER_EXPIRE 20
