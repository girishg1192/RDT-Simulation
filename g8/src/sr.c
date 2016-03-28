#include "sr.h"

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

#define MAX_BUFFER 500


void check_and_set_timer(float req_timeout)
{
  float curr_timer = get_sim_time();
  if(curr_timeout<=curr_timer && !is_timer_running)
  {
    curr_timeout = curr_timer + TIMER_EXPIRE;
    starttimer(A, TIMER_EXPIRE);
    is_timer_running =1;
  }
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  printf("SENDER: A_OUTPUT: %f\n", get_sim_time());
  if(A_nextseqnum < (A_base+window_size))
  {
    struct packet_elem *curr_data = malloc(sizeof(struct packet_elem));
    curr_data->packet = make_packet(A_nextseqnum, 0, message.data);
    curr_data->timer_val = get_sim_time() + TIMER_EXPIRE;
    check_and_set_timer(curr_data->timer_val);
    list_insert_ordered(&current_window, &curr_data->elem,
        (list_less_func *)&sort_timer, NULL);
    tolayer3(A, curr_data->packet);
    printf("SENDER: Packet: %d %f\n", A_nextseqnum, get_sim_time());
    A_nextseqnum++;
  }
  else
  {
    printf("SENDER: RDT_SEND:Buffer %d\n", buff_count);
    if(buff_count< MAX_BUFFER)
    {
      struct packet_elem *buff_data = malloc(sizeof(struct packet_elem));
      printf("SENDER: RDT_SEND:alloc\n");
      //int buffered_sequence = A_nextseqnum + buff_count;
      A_nextseqnum++;
      buff_data->packet = make_packet(A_nextseqnum, 0, message.data);
      printf("SENDER: RDT_SEND:alloc\n");
      list_push_back(&buffered_packets, &buff_data->elem);
      printf("SENDER: RDT_SEND:push\n");
      buff_count++;
    }
    printf("SENDER: RDT_SEND: End");
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  if(!isCorrupted(&packet))
  {
    printf("SENDER: ACK!! %d\n", packet.acknum);
    struct list_elem *e = list_begin(&current_window);
    int found=0;
    while(e!=list_end(&current_window))
    {
      struct packet_elem *ackd = list_entry(e, struct packet_elem, elem);
      if(packet.acknum == ackd->packet.seqnum)
      {
        printf("%f %f\n", ackd->timer_val, curr_timeout);
        if(ackd->timer_val == curr_timeout)
        {
          stoptimer(A);
          is_timer_running = 0;
          float timeout = list_entry(list_next(e), struct packet_elem, 
              elem)->timer_val - get_sim_time();
          curr_timeout = timeout + get_sim_time();
          settimer(A, timeout);
          is_timer_running = 1;
        }
        e = list_remove(e);
        free(ackd);
        break;
      }
      else if(packet.acknum >= ackd->packet.seqnum)
        break;
      e = list_next(e);
    };
    e = list_begin(&current_window);
    int move_window = list_entry(e, struct packet_elem, elem)->packet.seqnum;
    if(A_base < move_window-1)
    {
      A_base = move_window-1;
      send_buffered();
    }
  }
}
void send_buffered()
{
  while(A_nextseqnum < (A_base+window_size) && buff_count)
  {
    struct packet_elem *buff= list_entry(list_pop_front(&buffered_packets)
                                          ,struct packet_elem, elem);
    buff->timer_val = get_sim_time() + TIMER_EXPIRE;
    list_insert_ordered(&current_window, &buff->elem,
        (list_less_func *)&sort_timer, NULL);
    check_and_set_timer(buff->timer_val);
    tolayer3(A, buff->packet);
    A_nextseqnum++;
    buff_count--;
    free(buff);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("\n\nSENDER: meh\n");
  struct list_elem *e = list_pop_front(&current_window);
  struct packet_elem *sndpkt= list_entry(e, struct packet_elem, elem);
  printf("SENDER: TIMERINT: %f %d\n", get_sim_time(), sndpkt->packet.seqnum);
  tolayer3(A, sndpkt->packet);

  sndpkt->timer_val = get_sim_time() + TIMER_EXPIRE;
  list_insert_ordered(&current_window, &sndpkt->elem,
      (list_less_func*)&sort_timer, NULL);
  float timeout = list_entry(list_front(&current_window), struct packet_elem, 
      elem)->timer_val - get_sim_time();
  curr_timeout = timeout + get_sim_time();
  printf("SENDER: TIMERINT: %f %f\n", timeout, get_sim_time());
  starttimer(A, timeout);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_base = 1;
  A_nextseqnum = 1;
  window_size = getwinsize();
  is_timer_running = 0;
  curr_timeout = 0.0;

  in_flight = 0;

  list_init(&current_window);
  list_init(&buffered_packets);
  
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
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
      struct packet_elem *buff_pkt = malloc(sizeof(struct packet_elem));
      memcpy(&buff_pkt->packet, &packet, sizeof(struct pkt));
      printf("RECEIVER: Buffer %d %d\n", buff_pkt->packet.seqnum, packet.seqnum);
      buff_pkt->seq = packet.seqnum;
      list_insert_ordered(&receiver_window, &buff_pkt->elem,
          (list_less_func*)&sort_sequence, NULL);
      buff_pkt= list_entry(list_begin(&receiver_window), struct packet_elem
          ,elem);
      printf("RECEIVER: HEAD: %d\n", buff_pkt->packet.seqnum);
    }
    else
    {
      printf("RECEIVER: B: success %d\n", packet.seqnum);
      tolayer5(B, packet.payload);
      B_base++;
      for(struct list_elem *e = list_begin(&receiver_window); 
          e!=list_end(&receiver_window);)
      {
        struct packet_elem *pkt = list_entry(e, struct packet_elem, elem);
        printf("RECEIVER: B: buffered %d %d\n", pkt->packet.seqnum, B_base);
        if(pkt->packet.seqnum == B_base)
        {
          tolayer5(B, pkt->packet.payload);
          free(pkt);
          e = list_remove(e);
          B_base++;
        }
        else if(pkt->packet.seqnum <B_base)
        {
          e = list_remove(e);
          free(pkt);
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
  list_init(&receiver_window);
  receiver_window_size = getwinsize();
}
