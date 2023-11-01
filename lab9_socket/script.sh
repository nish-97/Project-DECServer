#!/bin/bash

if [ $# -ne 4 ]; then
    echo "Usage: $0 <numClients> <loopNum> <sleepTimeSeconds> <timeOutSeconds"
    exit 1
fi


#for ((k=1; k<=10; k++));
#do
numClients=$1
loopNum=$2
sleepTime=$3
timeout=$4
loopNumless=`expr $loopNum - 1`

# Create an array to store individual throughputs
throughputs=()

# Start multiple clients in the background
for ((i = 1; i <= $numClients; i++)); do
    file=source.cpp
    ./client 127.0.0.1:5555 $file $loopNum $sleepTime $timeout > client_$i.txt &
done
#wait

s_pid=$(pgrep server)
starttime=$(date +%s)
echo "Average number of threads : " > nlwp.txt
echo "Average CPU Utilisation : " > cput.txt
nlwp=$(ps -T -p $s_pid | wc -l)
nlwp=`expr $nlwp - 2`
echo $nlwp >> nlwp.txt
# vmstat | tail -1 | sed -E 's/[ ]+/./g' | awk -F. '{print $16}' >> cput.txt
vmstat 2 >> cput.txt &
v_pid=$(pgrep vmstat)

echo $nlwp
less=$(ps aux | grep -i "./client 127.0" | wc -l)

while [[ $less -gt 1 ]];
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
        # vmstat | tail -1 | sed -E 's/[ ]+/./g' | awk -F. '{print $16}' >> cput.txt
    fi
    less=$(ps aux | grep -i "./client 127.0" | wc -l)
done

kill -9 $v_pid
echo "$nlwp"

totalRequests=0
totalTime=0
totalTimeoutRate=0.0
totalErrorRate=0.0

# overallErrorRate=0
# overallTimeoutRate=0
# Calculate total requests and total time
for ((i = 1; i <= $numClients; i++)); do
    # Parse client log file to extract total requests and total time
    requests_i=$(grep "Number of Successful Responses" client_$i.txt | awk '{print $5}')
    time_i=$(grep "Average Response Time" client_$i.txt | awk '{print $4}')
    # timeout_i=$(grep "Number of Timeouts" client_$i.txt | awk '{print $4}')
    # error_i=$(grep "Number of Errors:" client_$i.txt | awk '{print $4}')
    timeoutrate_i=$(grep "Average Timeout Rate" client_$i.txt | awk '{print $4}')
    errorrate_i=$(grep "Average Error Rate:" client_$i.txt | awk '{print $4}')
    
    # Accumulate values for all clients
    totalRequests=`expr $totalRequests + $requests_i`
    totalTimeoutRate=$(echo "scale=3; $totalTimeoutRate + $timeoutrate_i" | bc)
    totalErrorRate=$(echo "scale=3; $totalErrorRate + $errorrate_i" | bc)
    totalTime=$(echo "scale=3; $totalTime + $time_i" | bc)
    
    # Calculate throughput for client i
    throughput_i=$(echo "scale=3; $requests_i / (($requests_i * $time_i) + ($sleepTime * $loopNumless))" | bc)
    throughputs+=($throughput_i)
done

# overallTimeoutRate=$(echo "scale=3; ($totalTimeouts * 100) / ($numClients * $loopNum)" | bc)
# overallErrorRate=$(echo "scale=3; ($totalErrors * 100)/ ($numClients * $loopNum)" | bc)
# echo "$totalTimeoutRate  $totalErrorRate" | bc
echo "Overall Timeout Rate_$numClients: $totalTimeoutRate timeouts/second" >> output.txt
echo "Overall Error Rate_$numClients: $totalErrorRate errors/second" >> output.txt


#Calculate overall throughput as the sum of individual throughputs
overallThroughput=0

for throughput in "${throughputs[@]}"; do
    overallThroughput=$(echo "$overallThroughput + $throughput" | bc)
done
echo "Overall Throughput_$numClients: $overallThroughput requests/second" >> output.txt
requestSentRate=$(echo "scale=3; $totalTimeoutRate + $totalErrorRate + $overallThroughput" | bc)
echo "Request Sent Rate_$numClients: $requestSentRate requests/second" >> output.txt



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

echo "Average Response Time_$numClients: $averageResponseTime seconds" >> output.txt

cput=$(cat cput.txt | sed -E 's/[ ]+/./g' | awk -F. '{print $(NF-2)}' | grep -E "[0-9]+")
lwp=$(cat nlwp.txt | grep -E "[0-9]+")
avg_cpu_ut=0
avg_nlwp=0
it=0
for i in $cput
    do
    avg_cpu=`expr 100 - $i`
    avg_cpu_ut=`expr $avg_cpu_ut + $avg_cpu`
    it=`expr $it + 1`
done
avg_cpu_ut=$(echo "scale=3; $avg_cpu_ut / $it" | bc)

it=0
for i in $lwp
    do
    avg_nlwp=`expr $i + $avg_nlwp`
    it=`expr $it + 1`
done
avg_nlwp=$(echo "scale=3; $avg_nlwp / $it" | bc)

echo "Average CPU Utilisation_$numClients : $avg_cpu_ut %" >> output.txt
echo "Average no of Threads_$numClients : $avg_nlwp Threads" >> output.txt

echo "" >> output.txt
rm program_*
rm received_*
rm compile_*
rm executable*
rm exp_output*
# rm client_*
# rm diff_*

#done