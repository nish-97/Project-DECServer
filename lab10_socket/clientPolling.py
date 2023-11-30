import subprocess
import sys
import time

if len(sys.argv) != 4:
    print(f"Usage: {sys.argv[0]} <serverIP:port> <sourceCodeFile> <polling_interval>")
    sys.exit(1)

server_ip_port = sys.argv[1]
source_code_file = sys.argv[2]
polling_interval = int(sys.argv[3])
max_attempts = 20

# Function to get the current timestamp
def get_timestamp():
    return int(time.time() * 1e9)

# Function to calculate response time
def calculate_response_time(start_time, end_time):
    elapsed_time = end_time - start_time
    # print(elapsed_time)
    print(f"Response time: {round((elapsed_time / 1000000000.0),2)} seconds")

# Submit a new request and capture the request ID
print("Submitting new request...")
start_time = get_timestamp()
response = subprocess.check_output(["./client", "new", server_ip_port, source_code_file])
request_id = response.split()[-1].decode('utf-8')
print(f"Received request ID: {request_id}")

# Poll the server for status until "done" response or reaching max attempts
attempts = 0
while attempts < max_attempts:
    status_response = subprocess.check_output(["./client", "status", server_ip_port, request_id], text=True)
    print(status_response)
    # Check if the response contains "done"
    if "done" in status_response:
        print("Received 'done' response.")
        end_time = get_timestamp()
        calculate_response_time(start_time, end_time)
        break

    print(f"Attempt {attempts + 1}: Status not done yet. Polling again after {polling_interval} seconds...")
    time.sleep(polling_interval)
    attempts += 1
else:
    print("Max attempts reached. Server response not received.")
