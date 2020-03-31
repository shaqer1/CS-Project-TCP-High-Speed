#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include "prog2.h"
#include <iostream>

using namespace std;

/* ******************************************************************
ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR:

VERSION 1.1 by J.F.Kurose

This code should be used for PA2, unidirectional or bidirectional
data transfer protocols (from A to B. Bidirectional transfer of data
is not required for this assignment).  Network properties:
- one way network delay averages five time units (longer if there
are other messages in the channel for GBN), but can be larger
- packets can be corrupted (either the header or the data portion)
or lost, according to user-defined probabilities
- packets will be delivered in the order in which they were sent
(although some can be lost).

VERSION 2.0 by Siyuan
- enable multiple timer interrupts for the same side, for
implementing selective repeat
**********************************************************************/


/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

int nxtSeqNum = 0;
int base = 0;

int expSeqNum;

std::vector<pkt> buffer;

int checksum(struct pkt packet){
    int sum = 0;
    sum += packet.seqnum;
    sum += packet.acknum;
    for(int i =0; i <sizeof(packet.payload); i++){
        sum += packet.payload[i];
    }
    return sum;
}

void printArr(char * array){
    for (int i = 0; i < sizeof(array) ; i++){
        cout << array[i];
    }
    cout << endl;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(msg message) {
    cout << "A_out sending data from layer5" << endl;
    struct pkt newPacket;
    for(int i =0; i<20; i++){
        newPacket.payload[i] = message.data[i];
    }
    buffer.push_back(newPacket);
    cout << "AOutput Next SeqNum" << nxtSeqNum << endl;
    cout << "AOutput base" << base << endl;
    cout << "AOutput win size" << WINDOWSIZE << endl;
    if(nxtSeqNum < base + WINDOWSIZE){
        buffer[nxtSeqNum].seqnum = nxtSeqNum;
        buffer[nxtSeqNum].acknum = 0;
        buffer[nxtSeqNum].checksum = checksum(buffer[nxtSeqNum]);
        cout << "A sending packet: ";
        printArr(buffer[nxtSeqNum].payload);
        tolayer3(0,buffer[nxtSeqNum]);
        if(base == nxtSeqNum){
            starttimer(0, TIMEOUT, buffer[nxtSeqNum].payload);
        }
        nxtSeqNum++;
    }else {
        cout << "AOutput: data refused" << endl;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(pkt packet) {
    if(checksum(packet) == packet.checksum){
        base = packet.acknum + 1;
        if (base == nxtSeqNum){
            stoptimer(0, packet.payload);
        }else {
            stoptimer(0, packet.payload);
            starttimer(0, TIMEOUT, packet.payload);
        }
    } else {
        cout << "Corrupted Packet, waiting" << endl;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void *adata) {
    starttimer(0, TIMEOUT, adata);
    for(int i =base; i < nxtSeqNum; i++){
        tolayer3(0, buffer[i]);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(pkt packet) {
    cout << "B received packet: ";
    printArr(packet.payload);
    struct pkt newAckPkt;
    if(checksum(packet) == packet.checksum && packet.seqnum == expSeqNum){
        char message[MESSAGE_LEN];
        for(int i =0; i < MESSAGE_LEN; i++){
            message[i] = packet.payload[i];
        }
        tolayer5(1, message);
        newAckPkt.acknum = expSeqNum;
        newAckPkt.seqnum = expSeqNum;
        memset(newAckPkt.payload, 0, MESSAGE_LEN);
        newAckPkt.checksum = checksum(newAckPkt);
        tolayer3(1, newAckPkt);
        expSeqNum++;
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
    expSeqNum = 0;
}
