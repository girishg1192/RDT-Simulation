#include "gbn.h"
#include "../include/simulator.h"
#include <stdio.h>
#include<iostream>
#include <string.h>
using namespace std;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define A 0
#define B 1

#define TRUE 1
#define FALSE 0


int compute_checksum(struct pkt *packet)
{
  int checksum = packet->seqnum + packet->acknum;
  for(int i=0; i<20; i++)
  {
    checksum += packet->payload[i];
  }
  return checksum;
}
int isCorrupted(struct pkt *packet)
{
  if(compute_checksum(packet) != packet->checksum)
    return TRUE;
  return FALSE;
}
struct pkt make_packet(int sequence, int acknum, char* message)
{
  struct pkt packet;
  packet.seqnum = sequence;
  packet.acknum = acknum;
  memset(packet.payload, 0, sizeof(packet.payload));
  if(message!=NULL)
    memcpy(packet.payload, &message, sizeof(message));
  packet.checksum = compute_checksum(&packet);
  return packet;
}
void adjust_timer()
{
  estimated_RTT +=TIMER_ADJUST;
  if(estimated_RTT>=20)
  {
    estimated_RTT = 20;
  }
}

#define MAX_BUFFER 500

void check_and_start_timer()
{
  if(A_base == A_nextseqnum && !is_timer_running)
  {
    starttimer(A, estimated_RTT);
    is_timer_running = 1;
  }
}
void restart_timer()
{
  if(is_timer_running)
    stoptimer(A);
  starttimer(A, estimated_RTT);
  is_timer_running = 1;
}
void check_and_stop_timer()
{
  if(is_timer_running)
  {
    stoptimer(A);
    is_timer_running = 0;
  }
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if(A_nextseqnum < (A_base+window_size))
  {
    struct packet_elem pkt;
    pkt.packet = make_packet(A_nextseqnum, 0, message.data);
    pkt.start_time = get_sim_time();
    //list_push_back(&current_window, &curr_data->elem);
    current_window.push_back(pkt);
    tolayer3(A, pkt.packet);
    check_and_start_timer();
    A_nextseqnum++;
  }
  else
  {
    if(buff_count< MAX_BUFFER)
    {
      //struct packet_elem *buff_data = malloc(sizeof(struct packet_elem));
      int buffered_sequence = A_nextseqnum + buff_count;
      struct pkt packet = make_packet(buffered_sequence, 0, message.data);
      //list_push_back(&buffered_packets, &buff_data->elem);
      buffered_packets.push_back(packet);
      buff_count++;
    }
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

  if(!isCorrupted(&packet))
  {
    A_base = packet.acknum + 1;
    printf("Packet: %d %f\n", packet.acknum, get_sim_time());
    //for(struct list_elem *e = list_begin(&current_window); 
    //    e!=list_end(&current_window);)
    for(list<struct packet_elem>::iterator packet = current_window.begin(); packet!=current_window.end(); )
    {
      if(packet->packet.seqnum < A_base)
      {
        printf("PACKET: acked %f %f\n", get_sim_time(), packet->start_time);
        float SampleRTT = get_sim_time() - packet->start_time;
        if(packet->retrans!=1 && SampleRTT >=10.0)
          estimated_RTT = 0.875*estimated_RTT + 0.125*(SampleRTT);
        packet = current_window.erase(packet);
      }
      else
        break;
    }
    send_buffered();
    if(A_base == A_nextseqnum)
      check_and_stop_timer();
    else
      restart_timer();
  }

}
void send_buffered()
{
  while(A_nextseqnum< (A_base+window_size) && buff_count)
  {
    //struct packet_elem *buff= list_entry(list_pop_front(&buffered_packets)
    //                                      ,struct packet_elem, elem);
    struct packet_elem buff;
    buff.packet = buffered_packets.front();
    buff.start_time = get_sim_time();
    buff.retrans = 0;
    tolayer3(A, buff.packet);
    buffered_packets.pop_front();
    current_window.push_back(buff);
    A_nextseqnum++;
    buff_count--;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("TIMERINT start %f\n", estimated_RTT);
  adjust_timer();
  starttimer(A,estimated_RTT);
  is_timer_running = 1;
  for(list<struct packet_elem>::iterator packet = current_window.begin(); packet!=current_window.end(); packet++)
  {
    packet->retrans = 1;
    tolayer3(A, packet->packet);
  }
  printf("TIMERINT End\n Send %d frames\n", A_nextseqnum - A_base);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_base = 1;
  A_nextseqnum = 1;
  buff_count = 0;
  is_timer_running = 0;
  estimated_RTT = 20;

  window_size = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  if(!isCorrupted(&packet) && packet.seqnum == B_expectedseq)
  {
    tolayer5(B, packet.payload);
    B_last_ack = make_packet(0, B_expectedseq, NULL);
    tolayer3(B, B_last_ack);
    B_expectedseq++;
  }
  else
    tolayer3(B, B_last_ack);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_expectedseq = 1;
  B_last_ack = make_packet(0, 0, NULL);
}
