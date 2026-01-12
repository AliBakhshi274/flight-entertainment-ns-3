#!/bin/bash

# Build an script to run ns-3 simulations for Blatt9.cc with varying parameters 
./ns3 build scratch/Blatt9

# The Results CSV file
FILE="results.csv"

# Header for the CSV file
echo "MeanIPD,OfferedLoad,QueueCap,AvgQueue,LossRate,Delay,Throughput" > $FILE

echo "Starting simulation..."

# Test 1: Varying Load with fixed Queue Size
echo "Load Sweep???????????????????????????????"
for ipd in $(seq 0.001 0.0002 0.005); do
    ./ns3 run "scratch/Blatt9 --meanIpd=$ipd --queueSize=50" --no-build >> $FILE # use queue size of 50 packets and vary load 
done

# Test 2: Varying Queue Size with fixed Load
echo "Queue Size Sweep....................."
for q in 5 10 20 50 100 200; do
    ./ns3 run "scratch/Blatt9 --meanIpd=0.00133 --queueSize=$q" --no-build >> $FILE # use mean IPD of 1.33ms (750 packets/sec) and vary queue size
done

echo "Done! Check $FILE !!!!!!!!!!!!"