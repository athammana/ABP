#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "project2.h"
 
/* ***************************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for Project 2, unidirectional or bidirectional
   data transfer protocols from A to B and B to A.
   Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets may be delivered out of order.

   Compile as gcc -g project2.c student2.c -o p2
**********************************************************************/



/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* 
 * The routines you will write are detailed below. As noted above, 
 * such procedures in real-life would be part of the operating system, 
 * and would be called by other procedures in the operating system.  
 * All these routines are in layer 4.
 */

/* 
 * A_output(message), where message is a structure of type msg, containing 
 * data to be sent to the B-side. This routine will be called whenever the 
 * upper layer at the sending side (A) has a message to send. It is the job 
 * of your protocol to insure that the data in such a message is delivered 
 * in-order, and correctly, to the receiving side upper layer.
 */
typedef unsigned __int32 uint32_t;

#define TIMEOUT 50
const uint32_t MOD_ADLER = 65521;

//global variables
struct pkt* A_prev_packet;
struct pkt* A_curr_packet;
int A_prev_acked; // 1 = waiting for an ack, 0 = ready for another message
// struct pkt* A_prev_packet;
struct pkt* B_curr_ack;
struct pkt* B_prev_ack;
int A_currseqnum;
int B_currseqnum;

int checksum(struct pkt packet){
	uint32_t a = 1, b = 0;
	for(int i = 0; i < MESSAGE_LENGTH; i++){
		a = (a + packet.payload[i]) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}

	a = (a + packet.seqnum) % MOD_ADLER;
	b = (b + a) % MOD_ADLER;
	a = (a + packet.acknum) % MOD_ADLER;
	b = (b + a) % MOD_ADLER;

	return (b << 16) | a;
}

void copyPacket(struct pkt* dest, struct pkt* src){
	dest->seqnum = src->seqnum;
	dest->acknum = src->acknum;
	dest->checksum = src->checksum;
	memcpy(dest->payload, src->payload, MESSAGE_LENGTH);
}

void A_output(struct msg message) {

	struct pkt* packet = (struct pkt*) malloc(sizeof(struct pkt));
	packet->acknum = 0;

	for(int i = 0; i < MESSAGE_LENGTH; i++){
		packet->payload[i] = message.data[i];
	}
	

	A_prev_acked = 0;
	packet->seqnum = (A_curr_packet->seqnum ? 0 : 1);
	packet->checksum = checksum(*packet);
	copyPacket(A_prev_packet, A_curr_packet);
	copyPacket(A_curr_packet, packet);
	tolayer3(AEntity, *packet);
	startTimer(AEntity, TIMEOUT);
}

/*
 * Just like A_output, but residing on the B side.  USED only when the 
 * implementation is bi-directional.
 */
void B_output(struct msg message)  {
}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt packet) {
	if(strcmp(packet.payload, "NULL") == 0){ // Null packet has been sent
		return;
	}

	if(packet.checksum != checksum(packet)){ // if sender got a corrupted ack or nack will resend it
		// if(A_currseqnum == A_prev_packet->seqnum) {
			tolayer3(AEntity, *A_prev_packet);
		// }
		// else {
		// 	tolayer3(AEntity, *A_curr_packet);
		// }
		// startTimer(AEntity, TIMEOUT);
		return;
	}

	if(packet.acknum == A_currseqnum){ // packet recieved, we can move on
		stopTimer(AEntity);
		A_currseqnum = packet.seqnum;
	}
	if((-packet.acknum) == A_curr_packet->seqnum){
		tolayer3(AEntity, *A_prev_packet);
		stopTimer(AEntity);
		startTimer(AEntity, TIMEOUT);
		A_currseqnum = packet.seqnum;		
	} else {
		stopTimer(AEntity);
		A_currseqnum = packet.seqnum;
		A_timerinterrupt();	
	}
}

/*
 * A_timerinterrupt()  This routine will be called when A's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void A_timerinterrupt() {
	if(A_currseqnum == A_prev_packet->seqnum) {
		tolayer3(AEntity, *A_prev_packet);
	}
	else {
		tolayer3(AEntity, *A_curr_packet);
	}
	startTimer(AEntity, TIMEOUT);
}  

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	A_currseqnum = 1;
	A_prev_acked = 0;
	A_prev_packet = (struct pkt*) malloc(sizeof(struct pkt));
	A_curr_packet = (struct pkt*) malloc(sizeof(struct pkt));
}


/* 
 * Note that with simplex transfer from A-to-B, there is no routine  B_output() 
 */


/*
 * B_input(packet),where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the A-side (i.e., as a result 
 * of a tolayer3() being done by a A-side procedure) arrives at the B-side. 
 * packet is the (possibly corrupted) packet sent from the A-side.
 */
void B_input(struct pkt packet) {

	if(packet.checksum != checksum(packet)){ // if sender got a corrupted ack or nack will resend it
		B_curr_ack->seqnum = B_currseqnum;	
		B_curr_ack->checksum = checksum(*B_curr_ack);
		tolayer3(BEntity, *B_curr_ack);
		return;
	}

	if((strncmp(B_curr_ack->payload, packet.payload, MESSAGE_LENGTH) == 0) || (strncmp(B_prev_ack->payload, packet.payload, MESSAGE_LENGTH) == 0) || (strcmp(packet.payload, "NULL") == 0)){ // if it is one the previously acked packets
		tolayer3(BEntity, *B_prev_ack);
		return;
	}
	if(B_currseqnum == packet.seqnum){ // if both the sender and reciever are on the same packet
		struct msg* message = (struct msg*) malloc(sizeof(struct msg));
		memcpy(message->data, packet.payload, 20);
		tolayer5(BEntity, *message);
		packet.acknum = packet.seqnum;
		copyPacket(B_prev_ack, B_curr_ack);
		copyPacket(B_curr_ack, &packet);
		B_currseqnum = ((B_currseqnum == 0) ? 1 : 0); 	
		packet.seqnum = B_currseqnum;
		packet.checksum = checksum(packet);
		tolayer3(BEntity, packet);	
	} else{
		B_curr_ack->acknum = - B_curr_ack->acknum;
		B_curr_ack->seqnum = B_currseqnum;
		B_curr_ack->checksum = checksum(*B_curr_ack);
		tolayer3(BEntity, *B_curr_ack);
	}// if wrong packet with wrong sequence number is sent
}

/*
 * B_timerinterrupt()  This routine will be called when B's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void  B_timerinterrupt() {
}

/* 
 * The following routine will be called once (only) before any other   
 * entity B routines are called. You can use it to do any initialization 
 */
void B_init() {
	B_prev_ack = (struct pkt*) malloc(sizeof(struct pkt));
	B_curr_ack = (struct pkt*) malloc(sizeof(struct pkt));
	B_currseqnum = 0;
	B_curr_ack->seqnum = 0;
	B_curr_ack->acknum = -1;
}

