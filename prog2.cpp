#include "prog2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

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

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the transmission and delivery (possibly with bit-level corruption
	and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
	interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NO REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you definitely should not have to modify
******************************************************************/

typedef struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	void *adata;			/* additional data for the event */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
} event;
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2


int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int ntolayer3;           /* number sent into layer 3 */
int nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/
int nInFlight = 0;           /* number in network flowing currently */

void init();
void generate_next_arrival();
void insertevent(event *p);

int main(int argc, char *argv[]) {
	event *eventptr;
	msg  msg2give;
	pkt  pkt2give;

	int i, j;

	int param1 = 0;
	int param2 = 24;
	if (argc > 2){
		for(int i = 1; i < argc; i++){
			if(strcmp(argv[i], "cwnd") == 0 || strcmp(argv[i], "highwin") == 0){
				param1 = atoi(argv[i+1]);
			}
			if(strcmp(argv[i], "ssthresh") == 0 || strcmp(argv[i], "lowwin") == 0){
				param2 = atoi(argv[i+1]);
			}
		}
	}

	init();
	A_init(param1, param2, lossprob);
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr == NULL)
			break;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist != NULL)
			evlist->prev = NULL;
		if (TRACE >= 2) {
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype == 0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype == 1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime;        /* update time to next event time */
		if (nsim == nsimmax && eventptr == NULL)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5) {
			if(nsim < nsimmax){
				generate_next_arrival();   /* set up future arrival */
				/* fill in msg to give with string of same letter */
				j = nsim % 26;
				for (i = 0; i < MESSAGE_LEN; i++)
					msg2give.data[i] = 97 + j;
				if (TRACE > 2) {
					printf("          MAINLOOP: data given to student: ");
					for (i = 0; i < MESSAGE_LEN; i++)
						printf("%c", msg2give.data[i]);
					printf("\n");
				}
				nsim++;
				if (eventptr->eventity == A)
					A_output(msg2give);
			}
		}
		else if (eventptr->evtype == FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i = 0; i < MESSAGE_LEN; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			nInFlight--;
			if (eventptr->eventity == A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype == TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt(eventptr->adata);
			//else
			//	B_timerinterrupt();
		}
		else {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

	fprintf(stderr, " Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
	//system("pause");
	return 0;
}


/* initialize the simulator */
void init() {                       
	int i;
	float sum, avg;
	float jimsrand();


	fprintf(stderr, "-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	fprintf(stderr, "Enter the number of messages to simulate: ");
	scanf("%d", &nsimmax);
	fprintf(stderr, "Enter  initial packet loss probability (increases linearly as more packets in flight) [enter 0.0 for no loss]:");
	scanf("%f", &lossprob);
	// fprintf(stderr, "Enter packet corruption probability [0.0 for no corruption]:");
	// scanf("%f", &corruptprob);
	corruptprob = 0.0;
	fprintf(stderr, "Enter average time between messages from sender's layer5 [ > 0.0]:");
	scanf("%f", &lambda);
	fprintf(stderr, "Enter TRACE:");
	scanf("%d", &TRACE);

	srand(9999);              /* init random number generator */
	sum = 0.0f;                /* test random number generator for students */
	for (i = 0; i < 1000; i++)
		sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum / 1000.0f;
	if (avg < 0.25 || avg > 0.75) {
		fprintf(stderr, "It is likely that random number generation on your machine\n");
		fprintf(stderr, "is different from what this emulator expects.  Please take\n");
		fprintf(stderr, "a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time = 0.0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in the range [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	float mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = rand() / mmm;            /* x should be uniform in [0,1] */
	return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
void generate_next_arrival()
{
	float x;
	struct event *evptr;

	if (TRACE > 2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda*jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
							  /* having mean of lambda        */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = time + x;
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand() > 0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}


void insertevent(event *p) {
	event *q, *qold;

	if (TRACE > 2) {
		printf("            INSERTEVENT: time is %lf\n", time);
		printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q == NULL) {   /* list is empty */
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else {
		for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
		if (q == NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q == evlist) { /* front of list */
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}

void printevlist() {
	event *q;
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q != NULL; q = q->next) {
		printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
/* AorB represents A or B is trying to stop timer */
void stoptimer(int AorB, void *adata) {
	event *q;

	if (TRACE > 2)
		printf("          STOP TIMER: stopping timer at %f\n", time);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB && q->adata == adata)) {
			/* remove this event */
			if (q->next == NULL && q->prev == NULL)
				evlist = NULL;         /* remove first and only event on list */
			else if (q->next == NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q == evlist) { /* front of list - there must be event after */
				q->next->prev = NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB, float increment, void *adata) {
	event *q;
	event *evptr;

	if (TRACE > 2)
		printf("          START TIMER: starting timer at %f\n", time);
	/* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT  && q->eventity == AorB && q->adata == adata)) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(event));
	evptr->evtime = time + increment;
	evptr->evtype = TIMER_INTERRUPT;
	evptr->eventity = AorB;
	evptr->adata = adata;
	insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB, pkt packet) {
	pkt *mypktptr;
	event *evptr, *q;
	float lastime, x, jimsrand();
	int i;

	ntolayer3++;
	nInFlight++;

	float probability = (-1*pow(1+lossprob,-1*nInFlight)) +1; // function to make congestion losses more realistic. as nInflight increase probability gets closer to 1
	/* simulate losses: */
	if (jimsrand() < probability) {//TODO make permenant
		nlost++;
		nInFlight--;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being lost.\n");
		if (TRACE > 2) {
			printf("                    seq: %d, ack %d, check: %d ", packet.seqnum,
				packet.acknum, packet.checksum);
			for (i = 0; i < MESSAGE_LEN; i++)
				printf("%c", packet.payload[i]);
			printf("\n");
		}
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (pkt *)malloc(sizeof(pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i = 0; i < MESSAGE_LEN; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE > 2) {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
			mypktptr->acknum, mypktptr->checksum);
		for (i = 0; i < MESSAGE_LEN; i++)
			printf("%c", mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(event));
	evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
	 medium can not reorder, so make sure packet arrives between 1 and 10
	 time units after the latest arrival time of packets
	 currently in the medium on their way to the destination */
	lastime = time;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q = evlist; q != NULL; q = q->next) //TODO for high speed
		if ((q->evtype == FROM_LAYER3  && q->eventity == evptr->eventity))
			lastime = q->evtime;
	float rand = jimsrand();
	evptr->evtime = lastime + 1 + 9 * rand;



	/* simulate corruption: */
	if (jimsrand() < corruptprob) {
		ncorrupt++;
		if ((x = jimsrand()) < .75)
			mypktptr->payload[0] = 'Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE > 2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

void tolayer5(int AorB, char *datasent) {
	int i;
	if (TRACE > 2) {
		printf("          TOLAYER5: data received: ");
		for (i = 0; i < MESSAGE_LEN; i++)
			printf("%c", datasent[i]);
		printf("\n");
	}
}