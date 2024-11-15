#!/bin/bash

# run testscript.sh to generate log files
./testscript.sh

input_file="benchmark.log"
output_file="${input_file%.log}.csv"

# Add header row to CSV file
echo "Benchmark,Start Time,End Time,Kernel,Execution Time (us),Status" > "$output_file"

# Initialize variables to hold start time, end time, and benchmark name
benchmark=""
start_time=""
end_time=""

# Process each line in the log file
while IFS= read -r line; do
  if [[ $line == Benchmark:* ]]; then
    # Extract benchmark name
    benchmark=$(echo "$line" | cut -d':' -f2 | xargs)
  elif [[ $line == Start\ time:* ]]; then
    # Extract start time
    start_time=$(echo "$line" | cut -d':' -f2- | xargs)
  elif [[ $line == End\ time:* ]]; then
    # Extract end time
    end_time=$(echo "$line" | cut -d':' -f2- | xargs)
  elif [[ $line == Average\ execution\ time* ]]; then
    # Extract kernel name and execution time
    kernel=$(echo "$line" | awk '{print $5, $6}')
    exec_time=$(echo "$line" | awk '{print $7}')
  elif [[ $line == PASS ]]; then
    # Write the information to CSV
    echo "\"$benchmark\",\"$start_time\",\"$end_time\",\"$kernel\",\"$exec_time\",\"PASS\"" >> "$output_file"
  fi
done < "$input_file"

echo "Conversion complete. Output saved to $output_file"