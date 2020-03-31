#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "prog2.h"

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

struct SRpkt {
    pkt packet;
    int ack;
    float timestamp;
};
vector<SRpkt> bufferA;
SRpkt bufferB[BUFFERSIZE] = {};
int senderBase = 0;
int recBase = 0;
int nxtSeqNum = 0;
int earlistSendUnackedIndex = 0;

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
    cout << "Sending message from Layer5: ";
    printArr(message.data);
    struct SRpkt currPkt;
    struct pkt newPkt;
    for(int i =0; i<MESSAGE_LEN; i++){
        newPkt.payload[i] = message.data[i];
    }
    currPkt.packet = newPkt;
    bufferA.push_back(currPkt);
    if(nxtSeqNum < senderBase + WINDOWSIZE){
        bufferA[nxtSeqNum].packet.seqnum = nxtSeqNum;
        bufferA[nxtSeqNum].packet.acknum = 0;
        bufferA[nxtSeqNum].packet.checksum = checksum(bufferA[nxtSeqNum].packet);
        cout << "AOutput sending data: ";
        printArr(bufferA[nxtSeqNum].packet.payload);
        tolayer3(0, bufferA[nxtSeqNum].packet);
        bufferA[nxtSeqNum].ack = 0;
        bufferA[nxtSeqNum].timestamp = getSimTime();
        if(senderBase == nxtSeqNum){
            starttimer(0, TIMEOUT, bufferA[nxtSeqNum].packet.payload);
        }
        nxtSeqNum++;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(pkt packet) {
    cout << "Acked message received" << endl;
    if(checksum(packet) == packet.checksum){
        bufferA[packet.acknum].ack = 1;
        while(bufferA[senderBase].ack == 1){
            senderBase++;
        }
        cout << "Sender base: " << senderBase << endl;
        cout << "Next seq num: " << nxtSeqNum << endl;
        if(senderBase == nxtSeqNum){
            cout << "stopping timer: "<< endl;
            stoptimer(0, bufferA[packet.acknum].packet.payload);
        } else {
            stoptimer(0, bufferA[packet.acknum].packet.payload);
            starttimer(0, bufferA[senderBase].timestamp + TIMEOUT - getSimTime(), bufferA[senderBase].packet.payload);
        }
    }else {
        cout << "A recieved corrupt ACK" << endl;
    }
}

void earliestPacketUnAck() {
    float leastTime = bufferA[nxtSeqNum-1].timestamp;
    for(int i = 0; i< WINDOWSIZE+senderBase; i++){
        if(bufferA[i].ack ==0 && bufferA[i].timestamp != 0 && bufferA[i].timestamp < leastTime){
            leastTime = bufferA[i].timestamp;
            earlistSendUnackedIndex = i;
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void *adata) {
    cout << "A timeout sending earliest unAck'd pkt" << endl;
    earliestPacketUnAck();
    tolayer3(0, bufferA[earlistSendUnackedIndex].packet);
    cout << "packet num: " << earlistSendUnackedIndex << ", sendtime: " << bufferA[earlistSendUnackedIndex].timestamp << endl;
    starttimer(0, TIMEOUT, bufferA[earlistSendUnackedIndex].packet.payload);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(pkt packet) {
    cout << "B recieved pkt: ";
    printArr(packet.payload);
    cout << " packet seq num: " << packet.seqnum << ", packet acknum: " << packet.acknum << endl;
    struct pkt newPkt;
    if(packet.checksum == checksum(packet)){
        if(bufferB[packet.seqnum].ack != 1){
            cout << "sending ack message" << endl;
            newPkt.acknum = packet.seqnum;
            newPkt.seqnum = 0;
            memset(newPkt.payload, 0, 20);
            newPkt.checksum = checksum(newPkt);
            tolayer3(1,newPkt);
            if(recBase == packet.seqnum){
                bufferB[recBase].packet = packet;
                bufferB[recBase].ack = 1;
                while(bufferB[recBase].ack == 1){
                    cout << "Delivering message: ";
                    printArr(bufferB[recBase].packet.payload);
                    tolayer5(1, bufferB[recBase].packet.payload);
                    recBase++;
                }
            }else {
                bufferB[packet.seqnum].packet = packet;
                bufferB[packet.seqnum].ack = 1;
            }
        } else {
            cout << "Duplicate message" << endl;
            cout << "Sending ack message" << endl;
            newPkt.acknum = packet.seqnum;
            newPkt.seqnum = 0;
            memset(newPkt.payload, 0, 20);
            newPkt.checksum = checksum(newPkt);
            tolayer3(1, newPkt);
        }
    }else {
        cout << "Corrupted message" << endl;
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {}
