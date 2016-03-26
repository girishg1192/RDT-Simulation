#include "gbn.h"
#include <stdio.h>

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

#define MAX_BUFFER 50

void check_and_start_timer()
{
  if(A_base == A_nextseqnum && !is_timer_running)
  {
    starttimer(A, TIMER_EXPIRE);
    is_timer_running = 1;
  }
}
void restart_timer()
{
  if(is_timer_running)
    stoptimer(A);
  starttimer(A, TIMER_EXPIRE);
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
void A_output(message)
  struct msg message;
{
  if(A_nextseqnum < (A_base+window_size))
  {
    struct packet_elem *curr_data = malloc(sizeof(struct packet_elem));
    curr_data->packet = make_packet(A_nextseqnum, 0, message.data);
    list_push_back(&current_window, &curr_data->elem);
    tolayer3(A, curr_data->packet);
    printf("Packet: %d %f\n", A_nextseqnum, get_sim_time());
    check_and_start_timer();
    A_nextseqnum++;
  }
  else
  {
    printf("RDT_SEND:Buffer %d\n", buff_count);
    if(buff_count< MAX_BUFFER)
    {
      struct packet_elem *buff_data = malloc(sizeof(struct packet_elem));
      printf("RDT_SEND:alloc\n");
      int buffered_sequence = A_nextseqnum + buff_count;
      buff_data->packet = make_packet(buffered_sequence, 0, message.data);
      printf("RDT_SEND:alloc\n");
      list_push_back(&buffered_packets, &buff_data->elem);
      printf("RDT_SEND:push\n");
      buff_count++;
    }
    printf("RDT_SEND: End");
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{

  if(!isCorrupted(&packet))
  {
    A_base = packet.acknum + 1;
    printf("Packet: %d %f\n", packet.acknum, get_sim_time());
    for(struct list_elem *e = list_begin(&current_window); 
        e!=list_end(&current_window) && list_entry(e, struct packet_elem, elem)->packet.seqnum < A_base;
        e = list_remove(e));
    send_buffered();
    if(A_base == A_nextseqnum)
      check_and_stop_timer();
    else
      restart_timer();
  }
  else
    printf("ACK: Corrupt\n");
}
send_buffered()
{
  while(A_nextseqnum< (A_base+window_size) && buff_count)
  {
    struct pkt buff_data = list_entry(list_pop_front(&buffered_packets), 
           struct packet_elem, elem)->packet;
    tolayer3(A, buff_data);
    A_nextseqnum++;
    buff_count--;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("TIMERINT start\n");
  starttimer(A,TIMER_EXPIRE);
  is_timer_running = 1;
  //Send all data in list
  for(struct list_elem *e = list_begin(&current_window); e!=list_end(&current_window);
      e = list_next(e))
  {
    struct packet_elem *sndpkt= list_entry(e, struct packet_elem, elem);
    tolayer3(A, sndpkt->packet);
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

  window_size = getwinsize();
  list_init(&current_window);
  list_init(&buffered_packets);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
//===================B code ========================//
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  if(!isCorrupted(&packet) && packet.seqnum == B_expectedseq)
  {
    tolayer5(B, packet.payload);
    B_last_ack = make_packet(0, B_expectedseq, NULL);
    tolayer3(B, B_last_ack);
    B_expectedseq++;
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_expectedseq = 1;
  B_last_ack = make_packet(0, 0, NULL);
}
