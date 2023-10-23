#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: $0 <numClients> <loopNum> <sleepTimeSeconds>"
    exit 1
fi


#for ((k=1; k<=10; k++));
#do
numClients=$1
loopNum=$2
sleepTime=$3
loopNumless=`expr $loopNum - 1`

# Create an array to store individual throughputs
throughputs=()

# Start multiple clients in the background
for ((i = 1; i <= $numClients; i++)); do
    file=source.cpp
    echo $file
    ./client 127.0.0.1:5555 $file $loopNum $sleepTime > client_$i.txt &
done
#wait
s_pid=$(pgrep server)
starttime=$(date +%s)
echo "Average number of threads : " > nlwp.txt
echo "Average CPU Utilisation : " > cput.txt
nlwp=$(ps -T -p $s_pid | wc -l)
nlwp=`expr $nlwp - 2`
echo $nlwp >> nlwp.txt
vmstat | tail -1 | sed -E 's/[ ]+/./g' | awk -F. '{print $14}' >> cput.txt

echo $nlwp
while [[ $nlwp -gt 0 ]];
do 
endtime=$(date +%s)
diff=`expr $endtime - $starttime`
if [[ $diff -gt 2 ]]; then
starttime=$(date +%s)
echo "Average number of threads : " >> nlwp.txt
echo "Average CPU Utilisation : " >> cput.txt
nlwp=$(ps -T -p $s_pid | wc -l)
nlwp=`expr $nlwp - 2`
echo $nlwp >> nlwp.txt
vmstat | tail -1 | sed -E 's/[ ]+/./g' | awk -F. '{print $14}' >> cput.txt
fi
done


totalRequests=0
totalTime=0

# Calculate total requests and total time
for ((i = 1; i <= $numClients; i++)); do
    # Parse client log file to extract total requests and total time
    requests_i=$(grep "Number of Successful Responses" client_$i.txt | awk '{print $5}')
    time_i=$(grep "Average Response Time" client_$i.txt | awk '{print $4}')
    # Accumulate values for all clients
    totalRequests=`expr $totalRequests + $requests_i`
    totalTime=$(echo "scale=3; $totalTime + $time_i" | bc)
    
    # Calculate throughput for client i
    throughput_i=$(echo "scale=3; $requests_i / (($requests_i * $time_i) + ($sleepTime * $loopNumless))" | bc)
    throughputs+=($throughput_i)
done

#Calculate overall throughput as the sum of individual throughputs
overallThroughput=0
for throughput in "${throughputs[@]}"; do
    overallThroughput=$(echo "$overallThroughput + $throughput" | bc)
done

echo "Overall Throughput: $overallThroughput requests/second" >> output.txt

#Calculate average response time
totalResponseTime=0
totalResponses=0

for ((i = 1; i <= $numClients; i++)); do
    responseTime_i=$(grep "Average Response Time" client_$i.txt | awk '{print $4}')
    responses_i=$(grep "Number of Successful Responses" client_$i.txt | awk '{print $5}')

    totalResponseTime=$(echo "$totalResponseTime + ($responseTime_i * $responses_i)" | bc)
    totalResponses=$(($totalResponses + $responses_i))
done
    
averageResponseTime=$(echo "scale=3; $totalResponseTime / $totalResponses" | bc)

echo "Average Response Time: $averageResponseTime seconds" >> output.txt
#done