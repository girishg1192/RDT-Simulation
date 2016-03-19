#include "../include/simulator.h"
#include <stdlib.h>
#include <string.h>

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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

#define A 0
#define B 1

#define TRUE 1
#define FALSE 0

#define TIMER_EXPIRE 10

// A/B_sequence is the current sequence of the sender
int A_sequence, B_sequence;

//First received packet by B

// A/B_ack is the last acknowledged packet
// A sends 0 and A_sequence incremented to 1, 
// and Await_for_ack is 1, A_ack should be 0 to indicate
// successful delivery, i.e if A_sequence == Await_for_ack
// last packet was not acknowledged
int B_ack;
enum ACK_STATES {NOT_WAITING_ACK = -1, 
                 BIT_0 = 0, 
                 BIT_1};
enum ACK_STATES Await_for_ack;

//-----------Move to Library------------
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
int isValidAck(int ackNum)
{
  if(ackNum == Await_for_ack)
    return TRUE;
  return FALSE;
}
void flipSequence(int AorB)
{
  if(AorB == A)
    A_sequence = (A_sequence+1)%2;
  if(AorB == B)
    B_sequence = (B_sequence+1)%2;
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
//-----------Move to Library------------

struct pkt last_sent_packet;
int is_timer_running;
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  if(Await_for_ack==NOT_WAITING_ACK)
  {
    last_sent_packet = make_packet(A_sequence, 0, message.data);
    tolayer3(A, last_sent_packet);
    Await_for_ack = A_sequence;
    flipSequence(A);
    //Start Timer if not started
    if(!is_timer_running)
    {
      starttimer(A, TIMER_EXPIRE);
      is_timer_running = 1;
    }
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  if(Await_for_ack==NOT_WAITING_ACK)
    return;
  if(!isCorrupted(&packet) && isValidAck(packet.acknum))
  {
    stoptimer(A);
    is_timer_running = 0;
    Await_for_ack = NOT_WAITING_ACK;
    //tolayer5(A, packet.payload);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  tolayer3(A, last_sent_packet);
  starttimer(A, TIMER_EXPIRE);
  is_timer_running = 1;
}  

/* The following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_sequence = 0;
  Await_for_ack = NOT_WAITING_ACK;
  is_timer_running = 0;
}
/*---------------------B implementation --------------*/
/* Note that with simplex transfer from a-to-B, there is no B_output() */
int B_last_ack;
int B_active = 1;
/* called from layer 3, when a packet arrives for layer 4 at B*/
int isValidSeq(int seqNum)
{
  if(seqNum == B_sequence)
    return TRUE;
  return FALSE;
}
void B_input(packet)
  struct pkt packet;
{
  int is_corrupt = isCorrupted(&packet);
  int is_valid = isValidSeq(packet.seqnum);
  printf("B: received %d waiting for %d\n", packet.seqnum, B_sequence);
  if(!is_corrupt && is_valid)
  {
    B_active = TRUE;
    tolayer5(B, packet.payload);
    B_last_ack = B_sequence;
    tolayer3(B, make_packet(0, B_last_ack, NULL));
    flipSequence(B);
  }
  else if(is_corrupt || !is_valid)
  {
    //Send previous ACK back.
    //ACK lives matter, data can be null, 
    //A isn't checking it anyway
    if(B_active)
      tolayer3(B, make_packet(0, B_last_ack, NULL));
  }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_sequence = 0;
  B_active = FALSE;
  B_last_ack = -1;
}
