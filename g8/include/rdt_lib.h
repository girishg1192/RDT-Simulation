#include "../include/simulator.h"
#include <string.h>

#define A 0
#define B 1

#define TRUE 1
#define FALSE 0

int compute_checksum(struct pkt *packet);
int isCorrupted(struct pkt *packet);
struct pkt make_packet(int sequence, int acknum, char* message);
