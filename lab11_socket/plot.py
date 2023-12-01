import subprocess
import csv
import matplotlib.pyplot as plt
from os import sys

if len(sys.argv) != 4:
    print(f"Usage: {sys.argv[0]} <serverIP:port> <sourceCodeFile> <polling_interval>")
    sys.exit(1)

server_ip_port = sys.argv[1]
source_code_file = sys.argv[2]
polling_interval = int(sys.argv[3])



# Initialize lists to store results
num_clients_list = [5,10,15,20,25,30,35,40,45,50]
avg_response_time_list = []
throughput_list = []
cpu_utilization_list = []

# Create a CSV file to store the data
csv_file_path = "performance_metrics_test.csv"
with open(csv_file_path, mode='w', newline='') as csv_file:
    fieldnames = ['Number of Clients', 'Response Time (seconds)', 'Throughput', 'CPU Utilization']
    writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
    
    # Write the CSV header
    writer.writeheader()

    # Loop through different numbers of clients
    for num_clients in num_clients_list:
        # Run the bash script with the specified number of clients
        command = f"./loadtest.sh {server_ip_port} {source_code_file} {num_clients} {polling_interval}"
        result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, text=True)

        # Parse the output to extract metrics
        output_lines = result.stdout.strip().split('\n')
        avg_response_time = float(output_lines[0].split(':')[1].split()[0])
        throughput = float(output_lines[1].split(':')[1])
        cpu_utilization = float(output_lines[2].split(': ')[1].rstrip('%'))


        # Append the metrics to the lists
        avg_response_time_list.append(avg_response_time)
        throughput_list.append(throughput)
        cpu_utilization_list.append(cpu_utilization)

        # Write the data to the CSV file
        writer.writerow({
            'Number of Clients': num_clients,
            'Response Time (seconds)': avg_response_time,
            'Throughput': throughput,
            'CPU Utilization': cpu_utilization,
        })

# Create subplots
fig, axes = plt.subplots(1, 3, figsize=(15, 5))
fig.suptitle('Performance Metrics vs. Number of Clients')

# Plot metrics
metrics = {
    'Average Response Time (seconds)': avg_response_time_list,
    'Throughput': throughput_list,
    'CPU Utilization': cpu_utilization_list,
}

for i, (metric_name, metric_data) in enumerate(metrics.items()):
    ax = axes[i]
    ax.plot(num_clients_list, metric_data, marker='o')
    ax.set_xlabel('Number of Clients')
    ax.set_ylabel(metric_name)
    ax.set_title(f'{metric_name} vs. Number of Clients')
    ax.grid(True)

# Save the plot to an image file
plt.savefig("performance_metrics_plot_test.png")

# Display the plots
# plt.show()