#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include "prog2.h"
#include <iostream>
#include <math.h>

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
//states defined
#define SLOWSTART 0
#define CONGAVOID 1
#define FASTRECOV 2

unsigned long cwnd;
int counterSS;
int rcvBase;
int sendBase;
int ssthresh;
int dupAckCount;
int state;
int nxtSeqNum;
int expRcvPkt;

struct TCPRcvPkt {
    pkt packet;
    int ack;
};
std::vector<pkt> sendBuffer;
std::vector<TCPRcvPkt> rcvBuffer;

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

void sendFromCwnd(){
    while(sendBase + cwnd > nxtSeqNum && nxtSeqNum < sendBuffer.size()) {
        //send packets in cwnd
        sendBuffer[nxtSeqNum].seqnum = nxtSeqNum;
        sendBuffer[nxtSeqNum].acknum = 0;
        sendBuffer[nxtSeqNum].checksum = checksum(sendBuffer[nxtSeqNum]);
        cout << "A sending packet: ";
        printArr(sendBuffer[nxtSeqNum].payload);
        tolayer3(0,sendBuffer[nxtSeqNum]);
        //start timer for each packet
        starttimer(0, TIMEOUT, (void*)nxtSeqNum);
        nxtSeqNum++;
    }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(msg message) {
    cout << "A_out sending data from layer5" << endl;
    struct pkt newPacket;
    for(int i =0; i<20; i++){
        newPacket.payload[i] = message.data[i];
    }
    sendBuffer.push_back(newPacket);
    cout << "AOutput Next SeqNum: " << nxtSeqNum << endl;
    cout << "AOutput base: " << sendBase << endl;
    if (sendBase + cwnd <= nxtSeqNum) {
        cout << "queuing message: " ;
        printArr(newPacket.payload);
    }
    sendFromCwnd();

    cout << "cwnd, nxtSeqNum, sendBase, State: " << cwnd << ", " << nxtSeqNum << ", " << sendBase << ", " << state << endl;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(pkt packet) {
    if(checksum(packet) == packet.checksum){
        if(packet.acknum - 1 < sendBase) {//dup ack 
            dupAckCount++;
            cout << "Duplicate ack: " << packet.acknum << " dupAckCount: " << dupAckCount << endl;
            if(dupAckCount == 3){
                if(state == SLOWSTART){
                    counterSS = 0;
                }
                cout << "3 Duplicate Ack occurred: " << packet.acknum << endl;
                state = FASTRECOV;
                ssthresh = cwnd/2;
                cwnd = ssthresh + 3;
                //send unack pkt
                int seqNum = packet.acknum;
                sendBuffer[seqNum].seqnum = seqNum;
                sendBuffer[seqNum].acknum = 0;
                sendBuffer[seqNum].checksum = checksum(sendBuffer[seqNum]);
                stoptimer(0, (void *) seqNum);
                cout << "A resending dup sck packet: " << seqNum << " data: ";
                printArr(sendBuffer[seqNum].payload);
                tolayer3(0,sendBuffer[seqNum]);
                starttimer(0, TIMEOUT, (void*)seqNum);
            }else if (state == FASTRECOV){
                cwnd = cwnd + 1;
                sendFromCwnd();
            }
        } else { // new ack
            cout << "New Ack'd seq: " << packet.acknum << endl;
            for(int i = sendBase; i < packet.acknum; i++){
                cout << "stopping timer for: " << i << endl;
                stoptimer(0, (void*)  i);
            }
            sendBase = packet.acknum;
            dupAckCount = 0;
            //change cwnd
            switch (state){
                case SLOWSTART:
                    cwnd = pow(2, counterSS++);
                    if(cwnd >= ssthresh){
                        state = CONGAVOID;
                        counterSS = 0;
                    }
                    break;  
                case CONGAVOID:
                    cwnd = cwnd + 1;
                    break;
                case FASTRECOV:
                    cwnd = ssthresh;
                    break;                
            }
            //if not fast recovery transmit else state = congavd
            if(state == FASTRECOV){
                state = CONGAVOID;
            }else {
                sendFromCwnd();
            }
        }
    } else {
        cout << "Corrupted Ack Packet, waiting" << endl;
    }
    cout << "cwnd, nxtSeqNum, sendBase, State: " << cwnd << ", " << nxtSeqNum << ", " << sendBase << ", " << state << endl;
}

/* called when A's timer goes off */
void A_timerinterrupt(void *adata) {
    int seqNum = *((int *) &adata);
    cout << "A timeout occurred on: " << seqNum << endl;
    //if fast recovery or CONAVD do SLOWSTART for all really
    state = SLOWSTART;
    ssthresh = cwnd/2;
    cwnd = 1;
    dupAckCount = 0;
    //trnsmit timeout package ack
    sendBuffer[seqNum].seqnum = seqNum;
    sendBuffer[seqNum].acknum = 0;
    sendBuffer[seqNum].checksum = checksum(sendBuffer[seqNum]);
    cout << "A sending timeout packet: ";
    printArr(sendBuffer[seqNum].payload);
    tolayer3(0,sendBuffer[seqNum]);
    starttimer(0, TIMEOUT, (void*)seqNum);
    cout << "cwnd, nxtSeqNum, sendBase, State: " << cwnd << ", " << nxtSeqNum << ", " << sendBase << ", " << state << endl;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(int ssthresh, int cwnd) {
    counterSS = 0;
    cwnd = pow(2, counterSS++);
    ssthresh = 8; //8 byte segments
    dupAckCount = 0;
    state = SLOWSTART;
    sendBase = 0;
    nxtSeqNum = 0;
}

void insert(TCPRcvPkt pkt){
    if(0 == rcvBuffer.size()){
        rcvBuffer.push_back(pkt);
        return;
    } 
    for(int i = rcvBuffer.size()-1; i >= 0; i--){
        if(rcvBuffer[i].packet.seqnum < pkt.packet.seqnum){
            rcvBuffer.insert(rcvBuffer.begin()+i+1, pkt);
            return;
        }
    }
}

int checkDuplicateRcv(int seqNum){
    for(int i = 0; i < rcvBuffer.size(); i++){
        if(rcvBuffer[i].packet.seqnum == seqNum){
            return true;
        }
    }
    return false;
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(pkt packet) {
    //buffer out of order and send most inorder pkt ack
    cout << "B received packet: ";
    printArr(packet.payload);
    struct pkt newAckPkt;
    if(checksum(packet) == packet.checksum){
        if(!checkDuplicateRcv(packet.seqnum)){// check if duplicate
            //deliver all in sequence
            struct TCPRcvPkt rcvPkt;
            rcvPkt.packet = packet;
            rcvPkt.ack = 1;
            insert(rcvPkt);//buffer pkt
            expRcvPkt = rcvBase;
            // int assign = (rcvBuffer[rcvBase].packet.seqnum == lastRcvPkt)?true:false;
            int assign = false;
            while(rcvBuffer[rcvBase].ack == 1 && packet.seqnum == expRcvPkt && rcvBase == rcvBuffer[rcvBase].packet.seqnum){// if in sequence deliver, might not need 2nd condition
                cout << "Delivering message: ";
                printArr(rcvBuffer[rcvBase].packet.payload);
                tolayer5(1, rcvBuffer[rcvBase].packet.payload);
                rcvBase++;
                assign = true;
            }
            //send ack of rcvBase
            expRcvPkt = (assign)?rcvBase:expRcvPkt;
            cout << "Sending ack message seq: " << expRcvPkt << endl;
            newAckPkt.acknum = expRcvPkt;
            newAckPkt.seqnum = 0;
            memset(newAckPkt.payload, 0, MESSAGE_LEN);
            newAckPkt.checksum = checksum(newAckPkt);
            tolayer3(1,newAckPkt);
        } else {
            cout << "B got Duplicate message: " << packet.seqnum << endl;
            cout << "Sending ack message seq: " << expRcvPkt << endl;
            newAckPkt.acknum = expRcvPkt;
            newAckPkt.seqnum = 0;
            memset(newAckPkt.payload, 0, MESSAGE_LEN);
            newAckPkt.checksum = checksum(newAckPkt);
            tolayer3(1,newAckPkt);
        }
    } else{
        cout << "corrupted message" << endl;
    }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
    rcvBase = expRcvPkt = 0;
}
