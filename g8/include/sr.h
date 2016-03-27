#ifndef __SIMULATOR__
#include "../include/simulator.h"
#define __SIMULATOR__
#include <stdlib.h>
#include <stdio.h>
#endif

#include "rdt_lib.h"
#include "list_lib.h"

int A_base;
int A_nextseqnum;
int window_size;

//---------TIMER---------
int in_flight;
float curr_timeout;

int is_timer_running;
int buff_count;

int B_base;
struct pkt B_last_ack;
struct list receiver_window;

struct list current_window;
struct list buffered_packets;

struct packet_elem
{
  struct pkt packet;
  float timer_val;
  struct list_elem elem;
};

#define TIMER_EXPIRE 10
void send_buffered();

bool sort_seq(struct list_elem *a_, struct list_elem *b_)
{
  struct packet_elem *a = list_entry(a_, struct packet_elem, elem);
  struct packet_elem *b = list_entry(b_, struct packet_elem, elem);
  return a->packet.seqnum < a->packet.seqnum;
}
bool sort_timer(struct list_elem *a_, struct list_elem *b_)
{
  struct packet_elem *a = list_entry(a_, struct packet_elem, elem);
  struct packet_elem *b = list_entry(b_, struct packet_elem, elem);
  return a->timer_val < b->timer_val;
}
