#!/bin/bash

if [ $# -ne 4 ]; then
    echo "Usage: $0 <numClients> <loopNum> <sleepTimeSeconds> <timeOutSeconds>"
    exit 1
fi

IP="10.130.171.134"
PORT="5555"
SOURCE="source.cpp"

#for ((k=1; k<=10; k++));
#do
numClients=$1
loopNum=$2
sleepTime=$3
timeout=$4
# loopNumless=`expr $loopNum - 1`


# Start multiple clients in the background
for ((i = 1; i <= $numClients; i++)); 
do
    ./client $IP:$PORT $SOURCE $loopNum $sleepTime $timeout > client_$i.txt &
done
#wait

s_pid=$(pgrep server)  #PID of server
starttime=$(date +%s)  #start time of client execution
echo "Average number of threads : " > nlwp.txt
echo "Average CPU Utilisation : " > cput.txt

nlwp=$(ps -T -p $s_pid | wc -l)  #number of threads of server 
nlwp=`expr $nlwp - 2`  #deducting extra lines
echo $nlwp >> nlwp.txt

vmstat 2 >> cput.txt & #running vmstat in background
v_pid=$(pgrep vmstat)  #PID of vmstat

echo $nlwp
less=$(ps aux | grep -i "./client $IP" | wc -l)  #count of clients running

while [[ $less -gt 1 ]];  #execute until there is atleast 2 threads
    do 
    endtime=$(date +%s)  #monitor time after some interval
    diff=`expr $endtime - $starttime` #calculate time elapsed

    if [[ $diff -gt 2 ]]; then  #atleast one client exists
        starttime=$(date +%s)  # update startime
        echo "Average number of threads : " >> nlwp.txt
        echo "Average CPU Utilisation : " >> cput.txt
        nlwp=$(ps -T -p $s_pid | wc -l)  #number of threads of server
        nlwp=`expr $nlwp - 2`
        echo $nlwp >> nlwp.txt
    fi
    less=$(ps aux | grep -i "./client $IP" | wc -l)  #count of clients running
done

kill -9 $v_pid  #kill the vmstat process
echo "$nlwp"

totalRequests=0
totalTime=0
totalTimeoutRate=0.0
totalErrorRate=0.0
goodPut=0.0

for ((i = 1; i <= $numClients; i++)); do
    # Parse client txt file to extract individual successful request count, repsonse time, timeout time
    requests_i=$(grep "Number of Successful Responses" client_$i.txt | awk '{print $5}')
    time_i=$(grep "Average Response Time" client_$i.txt | awk '{print $4}')
    timeoutrate_i=$(grep "Average Timeout Rate" client_$i.txt | awk '{print $4}')
    errorrate_i=$(grep "Average Error Rate:" client_$i.txt | awk '{print $4}')
    goodPut_i=$(grep "Goodput:" client_$i.txt | awk '{print $2}')
    
    # Accumulate values for all clients
    totalRequests=`expr $totalRequests + $requests_i`
    totalTimeoutRate=$(echo "scale=3; $totalTimeoutRate + $timeoutrate_i" | bc)
    totalErrorRate=$(echo "scale=3; $totalErrorRate + $errorrate_i" | bc)
    totalTime=$(echo "scale=3; $totalTime + $time_i" | bc)
    goodPut=$(echo "scale=3; $goodPut + $goodPut_i" | bc)
done

echo "Overall Timeout Rate_$numClients: $totalTimeoutRate timeouts/second" >> output.txt
echo "Overall Error Rate_$numClients: $totalErrorRate errors/second" >> output.txt


echo "Overall Goodput_$numClients: $goodPut requests/second" >> output.txt
requestSentRate=$(echo "scale=3; $totalTimeoutRate + $totalErrorRate + $goodPut" | bc)
echo "Request Sent Rate(Th)_$numClients: $requestSentRate requests/second" >> output.txt



#Calculate average response time
totalResponseTime=0
totalResponses=0

for ((i = 1; i <= $numClients; i++)); do
    # Parse client txt file to extract individual response time
    responseTime_i=$(grep "Average Response Time" client_$i.txt | awk '{print $4}')
    responses_i=$(grep "Number of Successful Responses" client_$i.txt | awk '{print $5}')

    totalResponseTime=$(echo "$totalResponseTime + ($responseTime_i * $responses_i)" | bc)
    totalResponses=$(($totalResponses + $responses_i))
done
    
averageResponseTime=$(echo "scale=3; $totalResponseTime / $totalResponses" | bc)

echo "Average Response Time_$numClients: $averageResponseTime seconds" >> output.txt

#parse client txt file to get no of thread and cpu utilisation 
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