#include "../include/simulator.h"
#include "rdt_lib.h"

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

#define TIMER_EXPIRE 10
int A_base;
int A_nextseqnum;
int window_size;
int is_timer_running;
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
}
void check_and_stop_timer()
{
  if(is_timer_running)
    stoptimer(A);
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  if(A_nextseqnum < (A_base+window_size))
  {
    struct pkt data = make_packet(A_nextseqnum, 0, message.data);
    //Add data to list
    tolayer3(A, data);
    check_and_start_timer();
    A_nextseqnum++;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  if(!isCorrupted(&packet))
  {
    A_base = packet.acknum + 1;
    if(A_base == A_nextseqnum)
      check_and_stop_timer();
    else
      restart_timer();
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //Send all data in list
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_base = 1;
  A_nextseqnum = 1;
  window_size = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
//===================B code ========================//
int B_expectedseq;
struct pkt B_last_ack;
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  if(!isCorrupted(packet) && packet.seqnum == B_expectedseq)
  {
    tolayer5(B, packet.payload);
    B_last_ack = make_packet(0, B_expectedseq, NULL);
    tolayer3(B, B_last_ack);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_expectedseq = 1;
  B_last_ack = make_packet(0, 0, NULL);
}
