#include "../include/simulator.h"
#include "sr.h"
#include <iostream>
#include <cstring>
#include<algorithm>
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
#define TIMER_ADJUST 1.0


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


void check_and_set_timer(float req_timeout, int seq)
{
  if(!is_timer_running)
  {
    curr_timeout = seq;
    starttimer(A, estimated_RTT);
    is_timer_running =1;
  }
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  printf("SENDER: A_OUTPUT: %f\n", get_sim_time());
  if(A_nextseqnum < (A_base+window_size))
  {
    //struct packet_elem *curr_data = malloc(sizeof(struct packet_elem));
    struct packet_elem curr_data;
    curr_data.packet = make_packet(A_nextseqnum, 0, message.data);
    curr_data.timer_val = get_sim_time() + estimated_RTT;
    curr_data.start_time = get_sim_time();
    curr_data.retrans = 0;
    check_and_set_timer(curr_data.timer_val, A_nextseqnum);
    current_window.push_back(curr_data);
    current_window.sort(sort_timer);
    tolayer3(A, curr_data.packet);
    printf("SENDER: Packet: %d %f\n", A_nextseqnum, get_sim_time());
    A_nextseqnum++;
  }
  else
  {
    printf("SENDER: RDT_SEND:Buffer %d\n", buff_count);
    if(buff_count< MAX_BUFFER)
    {
      struct packet_elem buff_data;
      printf("SENDER: RDT_SEND:alloc\n");
      int buffered_seq = A_nextseqnum + buff_count;
      buff_data.packet = make_packet(buffered_seq, 0, message.data);
      buff_data.retrans = 0;
      printf("SENDER: RDT_SEND:alloc\n");
      buffered_packets.push_back(buff_data);
      printf("SENDER: RDT_SEND:push\n");
      buff_count++;
    }
    printf("SENDER: RDT_SEND: End");
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  if(!isCorrupted(&packet))
  {
    printf("SENDER: ACK!! %d\n", packet.acknum);
    int found=0;
    for(list<struct packet_elem>::iterator ackd = current_window.begin(); ackd!=current_window.end(); ackd++)
    {
      if(packet.acknum == ackd->packet.seqnum)
      {
        float SampleRTT = get_sim_time() - ackd->start_time;
        if(ackd->retrans!=1 && SampleRTT >=10.0)
          estimated_RTT = 0.875*estimated_RTT + 0.125*(SampleRTT);
        printf("%f %d\n", ackd->timer_val, curr_timeout);
        if(ackd->packet.seqnum== curr_timeout)
        {
          stoptimer(A);
          is_timer_running = 0;
          //`struct packet_elem *temp= list_entry(list_next(e), struct packet_elem, elem);
          ackd = current_window.erase(ackd);
          if(current_window.size()>0)
          {
            printf("SENDER: Not empty\n");
            struct packet_elem temp = current_window.front();
            float timeout = temp.timer_val - get_sim_time();
            curr_timeout = temp.packet.seqnum;
            starttimer(A, timeout);
            is_timer_running = 1;
          }
        }
        else
          ackd = current_window.erase(ackd);
        break;
      }
    }
    if(current_window.size()!=0)
    {
      list<struct packet_elem>::iterator move_win = min_element(current_window.begin(),
          current_window.end(), sort_sequence);
      int move_window = move_win->packet.seqnum;
      printf("SENDER: window %d Base: %d", move_window, A_base);
      if(A_base < move_window-1)
      {
        A_base = move_window-1;
        send_buffered();
      }
    }
    else
    {
      A_base = packet.acknum;
      send_buffered();
    }
  }
}
void send_buffered()
{
  while(A_nextseqnum < (A_base+window_size) && buff_count)
  {
    //struct packet_elem *buff= list_entry(list_pop_front(&buffered_packets)
    //                                      ,struct packet_elem, elem);
    struct packet_elem buff = buffered_packets.front();
    buff.timer_val = get_sim_time() + estimated_RTT;
    buff.start_time = get_sim_time();
    buff.retrans = 0;
    buffered_packets.pop_front();
    current_window.push_back(buff);
    check_and_set_timer(buff.timer_val, buff.packet.seqnum);
    tolayer3(A, buff.packet);
    A_nextseqnum++;
    buff_count--;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("\n\nSENDER: meh\n");
  adjust_timer();
  if(current_window.size()!=0)
  {
    struct packet_elem sndpkt = current_window.front();
    printf("SENDER: TIMERINT: %f %d\n", get_sim_time(), sndpkt.packet.seqnum);
    tolayer3(A, sndpkt.packet);

    sndpkt.timer_val = get_sim_time() + estimated_RTT;
    sndpkt.retrans = 1;
    current_window.pop_front();
    current_window.push_back(sndpkt);
    struct packet_elem temp = current_window.front();
    float timeout = temp.timer_val - get_sim_time();
    curr_timeout = temp.packet.seqnum;
    starttimer(A, timeout);
  }
  else 
    is_timer_running=0;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_base = 1;
  A_nextseqnum = 1;
  window_size = getwinsize();
  is_timer_running = 0;
  curr_timeout = 1;
  estimated_RTT = 20.0;

  in_flight = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  if(isCorrupted(&packet))
    return;
  printf("RECEIVER: B: sequence %d %d\n", packet.seqnum, B_base);
  if(packet.seqnum>=B_base && packet.seqnum <=(B_base + receiver_window_size))
  {
    //Send Ack
    tolayer3(B, make_packet(0, packet.seqnum, NULL));
    if(packet.seqnum != B_base)   //Buffer data
    {
      //struct packet_elem *buff_pkt = malloc(sizeof(struct packet_elem));
      struct packet_elem buff_pkt;
      buff_pkt.packet = packet;
      //memcpy(&buff_pkt->packet, &packet, sizeof(struct pkt));
      printf("RECEIVER: Buffer %d %d\n", buff_pkt.packet.seqnum, packet.seqnum);
      //list_insert_ordered(&receiver_window, &buff_pkt->elem,
      //    (list_less_func*)&sort_sequence, NULL);
      receiver_window.push_back(buff_pkt);
      receiver_window.sort(sort_sequence);
      buff_pkt = receiver_window.front();
      printf("RECEIVER: HEAD: %d\n", buff_pkt.packet.seqnum);
    }
    else
    {
      printf("RECEIVER: B: success %d\n", packet.seqnum);
      tolayer5(B, packet.payload);
      B_base++;
      //for(struct list_elem *e = list_begin(&receiver_window); 
      //    e!=list_end(&receiver_window);)
      for(list<struct packet_elem>::iterator packet = receiver_window.begin(); 
          packet!=receiver_window.end();)
      {
        printf("RECEIVER: B: buffered %d %d\n", packet->packet.seqnum, B_base);
        if(packet->packet.seqnum == B_base)
        {
          tolayer5(B, packet->packet.payload);
          packet = receiver_window.erase(packet);
          B_base++;
        }
        else if(packet->packet.seqnum <B_base)
        {
          packet=receiver_window.erase(packet);
        }
        else
        {
          printf("RECIEVER: B: break\n");
          break;
        }
      }
    }
  }
  else
  {
    tolayer3(B, make_packet(0, packet.seqnum, NULL));
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_base = 1;
  receiver_window_size = getwinsize();
}
