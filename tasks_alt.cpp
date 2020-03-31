#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <iostream>
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


int expSeqNum;
int seqCurr = 0;
int resendMsg = 0;
int FSM = 0;
struct msg lastMsg;
queue<msg> buffer;

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
    struct pkt newPacket;
    newPacket.seqnum = seqCurr % 2;
    newPacket.acknum = 0;
    if(FSM == 0){
        if(buffer.size() > 0){
            cout << "Buffer not empty" << endl;
            struct msg messageCurr;
            messageCurr = buffer.front();
            for(int i =0; i < MESSAGE_LEN; i++){
                newPacket.payload[i] = messageCurr.data[i];
                lastMsg.data[i] = messageCurr.data[i];
            }
            buffer.pop();
            cout << "Message put in buffer" << endl;
            printArr(message.data);
            buffer.push(message);
            newPacket.checksum = checksum(newPacket);
            starttimer(0, TIMEOUT, newPacket.payload);
            tolayer3(0, newPacket);
            FSM = 1;
            cout << "sending message:";
            printArr(newPacket.payload);
        }else {
            cout << "Buffer is empty" << endl;
            printArr(message.data);
            for(int i =0; i<sizeof(message.data); i++){
                newPacket.payload[i] = message.data[i];
                lastMsg.data[i] = message.data[i];
            }
            newPacket.checksum = checksum(newPacket);
            cout << "packet.payload contents " << newPacket.seqnum << "-" << newPacket.acknum << endl;
            printArr(newPacket.payload);
            tolayer3(0, newPacket);
            starttimer(0, TIMEOUT, newPacket.payload);
            FSM = 1;
        }
    }else if(FSM ==1){
        buffer.push(message);
        cout << "adding message to buffer";
        printArr(message.data);
    }

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(pkt packet) {
    struct msg newMsg;
    if((packet.acknum == 1 && seqCurr%2 == 1) || (packet.acknum == 0 && seqCurr%2 == 0)){
        cout << " successfully received ackNum: " << packet.acknum << endl;
        seqCurr +=1;
        FSM =0;
        stoptimer(0, packet.payload);
    } else{
        cout << "discarding message" << endl;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void *adata) {
    cout << "Timed out, resending last msg"<< endl;
    printArr(lastMsg.data);
    cout << "seq Number" << seqCurr << endl;
    struct pkt pckt;
    pckt.seqnum = seqCurr %2;
    pckt.acknum = 0;
    for(int i =0; i < MESSAGE_LEN; i++){
        pckt.payload[i] = lastMsg.data[i];
    }
    pckt.checksum = checksum(pckt);
    tolayer3(0, pckt);
    starttimer(0, TIMEOUT, pckt.payload);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
    
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(pkt packet) {
    struct msg newMsg;
    struct pkt ackPkt;
    
    if(checksum(packet) != packet.checksum){
        cout << "checksum for message doesn't match" << endl;
    }else if(expSeqNum % 2 == packet.seqnum){
        for(int i = 0; i < sizeof(packet.payload); i++){
            newMsg.data[i] = packet.payload[i];
        }
        cout << "Sending message:" << endl;
        printArr(newMsg.data);
        tolayer5(1, newMsg.data);
        expSeqNum++;
        cout << "Message sent successfully" << endl;
        cout << "SeqNum: " << packet.seqnum << endl;
        ackPkt.seqnum = packet.seqnum;
        ackPkt.acknum = packet.seqnum;
        cout << "Sending Ack num: " << ackPkt.acknum << endl;
        memset(ackPkt.payload, 0, MESSAGE_LEN);
        ackPkt.checksum = checksum(ackPkt);
        tolayer3(1, ackPkt);
    }else {
        cout << "Duplicate Packet! Ignoring pckt" << endl;
        cout << expSeqNum << endl;
        cout << packet.seqnum << endl;
        ackPkt.seqnum = packet.seqnum;
        ackPkt.acknum = packet.seqnum;
        memset(ackPkt.payload, 0, MESSAGE_LEN);
        ackPkt.checksum = checksum(ackPkt);
        tolayer3(1, ackPkt);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
    
}
