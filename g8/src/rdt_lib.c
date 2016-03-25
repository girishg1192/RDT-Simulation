#include "rdt_lib.h"

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
