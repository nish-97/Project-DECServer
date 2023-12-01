#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <serverIP:port> <sourceCodeFile>  <num_clients> <polling_interval> "
    exit 1
fi

serverIPPort=$1
sourceCodeFile=$2
numClients=$3
pollingInterval=$4
maxAttempts=15

# Function to get the current timestamp
get_timestamp() {
    date +%s%N
}

# Function to calculate response time
calculate_response_time() {
    local start_time=$1
    local end_time=$2
    local elapsed_time=$((end_time - start_time))
    echo $((elapsed_time / 1000000))
}


# Array to store process IDs of background clients
client_pids=()

# Array to store response times and throughput
response_times=()
throughputs=()

# Create a directory to store client output files
outputDir="client_output"
mkdir -p "$outputDir"


vmstat 1  > "$outputDir/vmstat.log" &
vmstat_pid=$!

# Launch multiple clients
for ((i = 1; i <= $numClients; i++)); do
    output_file="$outputDir/client_$client_num_$i.txt"
     python3 clientPolling.py $serverIPPort $sourceCodeFile $pollingInterval  > $output_file &
    pids[${i}]=$! # Store the PID of each background job
done

for ((i = 1; i <= $numClients; i++)); do
    wait ${pids[i]}
done

# Wait for vmstat to finish and collect data
# wait $vmstat_pid

# Parse vmstat output and calculate CPU utilization
vmstat_file="$outputDir/vmstat.log"
lines=0
while read -r line; do
    if [[ "$line" =~ ^[0-9]+ ]]; then
    	let lines=lines+1
    	if [[ $lines -ne 1 ]]; then
		idle=$(echo "$line" | awk '{print $15}')
		totalIdle=$((totalIdle + idle))
	fi
    fi
done < "$vmstat_file"
let lines=lines-1
if [[ $lines -eq 0 ]]; then
    averageIdle=$(($totalIdle ))
else
    averageIdle=$(($totalIdle / lines))
fi
averageCPU=$(echo "100 - $averageIdle" | bc)

# kill -9 $vmstat_pid > /dev/null

# Calculate average response time
for ((i = 1; i <= $numClients; i++)); do
    output_file="$outputDir/client_$i.txt"
    response_time=$(cat $output_file | grep "Response time:" | awk '{print $3}')
    if grep -q "Received 'done' response" $output_file; then
        # throughputs+=$(echo "scale=2; 1 / $response_time" | bc)
        throughputs+=(1/$response_time)
    else
        throughputs+=(0)
    fi
    response_times+=($response_time)
done


# Calculate average response time
average_time=0
for time in "${response_times[@]}"; do
    average_time=$(awk "BEGIN {print $average_time + $time}")
done
average_time=$(awk "BEGIN {print $average_time / $numClients}")

total_throughput=0
for throughput in "${throughputs[@]}"; do
    total_throughput=$(awk "BEGIN {print $total_throughput + $throughput}")
done

echo "Average Response Time Across $numClients Clients: $average_time"
echo "Overall Throughput Across $numClients Clients: $total_throughput"
echo "Overall CPU Utilization: $averageCPU%"
