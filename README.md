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
``
- Loss probability
    - this is for number of packets in transmission
``
- Average time between sending packets
``
- Trace level
``

### High Speed TCP

``

### Standard TCP

``

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