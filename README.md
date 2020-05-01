# CS 422 High Speed TCP implementation and Analysis

## Compile

`make`

## Files

All files run with 10 seconds between each send and trace of 2

tcp500Out: output trace 2 of standrad tcp with 500 packets

cwndFinal500.csv: csv of \<cwnd>,\<state> for every transmission round 
with 500 packets

tcp1000Out: output trace 2 of standrad tcp with 1000 packets

cwndFinal1000.csv: csv of \<cwnd>,\<state> for every transmission round 
with 1000 packets

tcpHS500Out: output trace 2 of standrad tcp with 500 packets

cwndHSFinal500.csv: csv of \<cwnd>,\<state> for every transmission round with 500 packets

tcpHS1000Out: output trace 2 of standrad tcp with 1000 packets

cwndHSFinal1000.csv: csv of \<cwnd>,\<state> for every transmission round with 1000 packets

## Run

### Input

- Number of packets
    - number of packets to send in simulation
- Loss probability
    - a probability function variable that changes probability based on number of packets in transmission at a given time.
    - equation = -(1+lossprob)^(-1* \<packets in flight>) + 1
- Average time between sending packets
    - average time the sender waits before sending/queueing a new packet
- Trace level
    - an int to print trace at different levels

### High Speed TCP

`tasks_tcp_HIGHSPEED highwin <High_Window> lowwin <Low_Window>`

### Standard TCP

`tasks_tcp cwnd <initial cwnd> ssthresh <ssthresh>`

## Output
The output generates a comma separated file for each transmission round, with the cwnd and the State. 

### Standard TCP states
- 0: Slow Start
- 1: Congestion Avoidance
- 2: Fast Recovery

### High Speed TCP states
- 0: Window below Low Window
- 1: Window between Low Window and High Window
- 2: Window above High Window

Output is sent to the corresponding CSV files, for standard TCP, cwnd.csv, for High speed TCP, cwndHS.csv.