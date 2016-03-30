#ifndef __SIMULATOR__
#include "../include/simulator.h"
#define __SIMULATOR__
#endif
#include <stdlib.h>
#include <stdio.h>
#include <list>
using namespace std;


int A_base;
int A_nextseqnum;
int window_size;

//---------TIMER---------
int in_flight;
int curr_timeout;

int is_timer_running;
int buff_count;

int B_base;
struct pkt B_last_ack;
list<struct packet_elem> receiver_window;
int receiver_window_size;

struct list<struct packet_elem> current_window;
struct list<struct packet_elem> buffered_packets;

struct packet_elem
{
  struct pkt packet;
  float timer_val;
  float start_time;
  int retrans;
};

#define TIMER_EXPIRE 30
void send_buffered();

bool sort_sequence(struct packet_elem &a, struct packet_elem &b)
{
  return a.packet.seqnum < b.packet.seqnum;
}
bool sort_timer(struct packet_elem &a, struct packet_elem &b)
{
  return a.timer_val < b.timer_val;
}
