#ifndef __SIMULATOR__
#include "../include/simulator.h"
#define __SIMULATOR__
#endif
#include <stdlib.h>
#include <iostream>
#include <list>
using namespace std;

int A_base;
int A_nextseqnum;
int window_size;
int is_timer_running;
int buff_count;

int B_expectedseq;
struct pkt B_last_ack;

list<struct packet_elem> current_window;
list<struct pkt> buffered_packets;
float estimated_RTT;
float TIMER_ADJUST = 1;

struct packet_elem
{
  struct pkt packet;
  float start_time;
  int retrans;
};

void send_buffered();

#define TIMER_EXPIRE 20
