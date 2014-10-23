/*
 * Written by :
 * Benoît Legat <benoit.legat@student.uclouvain.be>
 * Mélanie Sedda <melanie.sedda@student.uclouvain.be>
 * October 2014
 */

#include "common.h"

/* Node representing a slot in the buffer */
struct slot {
  bool received;
  char data[PACKET_SIZE];
};