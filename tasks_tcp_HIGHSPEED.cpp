#include "prog2.h"

#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <iostream>
#include <fstream>

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
#define LOW_WIN 0
#define LOW_HIGH_WIN 1
#define HIGH_WIN 2

float cwnd;
float alphaW = 0.0;
float betaW = 0.0;
int lowWin;
int highWin;
float highDec;
float S;
float highP;
float lowP;

int rcvBase;
int sendBase;
int dupAckCount;
int state;
int nxtSeqNum;
int expRcvPkt;
string CWNDFile("cwndHS.csv");

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

void writeToFile(unsigned long v, int state, string fd){
    ofstream outf;
    outf.open(fd, fstream::in | fstream::out | fstream::app);
    outf << v << "," << state <<"\n";
    outf.close();
}

float probW(float cwnd){
    return pow(((pow(lowP,S)*lowWin)/cwnd) ,1/S);
}

float calculateCWND(int isLoss){
    if (cwnd < lowWin){
        alphaW = cwnd;
        betaW = 0.5;
    }else if (cwnd >= lowWin && cwnd <= highWin){
        betaW = ( (highDec - 0.5)*((log10(cwnd) - log10(lowWin))/(log10(highWin) - log10(lowWin))) ) + 0.5;
        alphaW = (cwnd*cwnd)*probW(cwnd)*2*(betaW/(2-betaW));
    }else if (cwnd > highWin){
        betaW = highDec;
        alphaW = (highWin*highWin)*highP*2*(betaW/(2-betaW));
    }
    
    if (alphaW < 0 || betaW < 0){
        printf("--------------------------------PANIC!!!-------------------------\n");
        printf("----------------------alpha: %.2f, beta: %.2f--------------------\n", alphaW, betaW);
        exit(1);
    }

    if (cwnd < lowWin){//set CWND for next transmission
        state = LOW_WIN;
    }else if (cwnd >= lowWin && cwnd <= highWin){
        state = LOW_HIGH_WIN;
    }else if (cwnd > highWin){
        state = HIGH_WIN;
    }

    if(!isLoss){
        cwnd += alphaW;
    }else {
        cwnd = (1-betaW)*cwnd;
    }

    return (cwnd <1)?1:cwnd;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(pkt packet) {
    int changed = 0;
    if(checksum(packet) == packet.checksum){
        if(packet.acknum - 1 < sendBase && sendBase != nxtSeqNum) {//dup ack 
            dupAckCount++;
            cout << "Duplicate ack: " << packet.acknum << " dupAckCount: " << dupAckCount << endl;
            if(dupAckCount == 3){
                cout << "3 Duplicate Ack occurred: " << packet.acknum << endl;
                changed = 1;
                dupAckCount = 0;
                cwnd = calculateCWND(1);
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
            }else {
                cwnd = calculateCWND(0);
                changed = 1;
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
            changed = 1;
            //change cwnd
            cwnd = calculateCWND(0);
            changed = 1;
            sendFromCwnd();
        }
    } else {
        cout << "Corrupted Ack Packet, waiting" << endl;
    }
    if(changed)
        writeToFile(cwnd, state, CWNDFile);
    cout << "cwnd, nxtSeqNum, sendBase, State: " << cwnd << ", " << nxtSeqNum << ", " << sendBase << ", " << state << endl;
}

/* called when A's timer goes off */
void A_timerinterrupt(void *adata) {
    int seqNum = *((int *) &adata);
    cout << "A timeout occurred on: " << seqNum << endl;
    //if fast recovery or CONAVD do SLOWSTART for all really
    cwnd = calculateCWND(1);
    dupAckCount = 0;
    //trnsmit timeout package ack
    sendBuffer[seqNum].seqnum = seqNum;
    sendBuffer[seqNum].acknum = 0;
    sendBuffer[seqNum].checksum = checksum(sendBuffer[seqNum]);
    cout << "A sending timeout packet: ";
    printArr(sendBuffer[seqNum].payload);
    tolayer3(0,sendBuffer[seqNum]);
    starttimer(0, TIMEOUT, (void*)seqNum);
    writeToFile(cwnd, state, CWNDFile);
    cout << "cwnd, nxtSeqNum, sendBase, State: " << cwnd << ", " << nxtSeqNum << ", " << sendBase << ", " << state << endl;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(int highW, int lowW, float loss) {
    cwnd = 1;
    alphaW = 1;
    betaW = 0.5;
    lowWin = lowW;
    highWin = highW;
    highP = loss*pow(10,-4);
    lowP = loss;
    highDec = 0.2;
    S = log10(highWin/lowWin)/log10(highP/lowP);
    dupAckCount = 0;
    state = LOW_WIN;
    sendBase = 0;
    nxtSeqNum = 0;
    ofstream out;
    out.open(CWNDFile);
    out << "";
    out.close();
    writeToFile(cwnd, state, CWNDFile);
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
