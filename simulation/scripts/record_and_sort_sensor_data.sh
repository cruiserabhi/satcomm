# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
# Usage:
#   By default, this script captures sensor events for UNCALIBRATED_ACCELEROMETER AND
#   UNCALIBRATED_GYROSCOPE with both ROTATED and UNROTATED configurations enabled at
#   104hz sampling rate and 50 batchcount.
# Note: Once captured, it sorts CSV upon pressing ctrl+c. Provide csv file name as input to obtain
# sorted data.

#!/bin/bash

# Check if the file path is provided
if [ -z "$1" ]; then
    echo "Please provide the path to the CSV file."
    exit 1
fi

# Define the ADB command with the -r argument
adb_command="adb shell sensor_test_app -r"

# Define the path to the CSV file
csv_file="$1"

# Function to handle Ctrl+C
function handle_ctrl_c() {
    echo "Ctrl+C pressed. Proceeding with further processing..."
    process_csv
    exit
}

# Function to process the CSV file
function process_csv() {
    # Remove the ### prefix from lines starting with ###
    sed -i 's/^###//' "$csv_file"

    # Remove the last line
    sed -i '$ d' "$csv_file"

    # Sort the data based on the third column and remove headers
    sort -t, -k3,3 "$csv_file" | awk 'NR>1' > temp_sorted_output.csv

    # Clearing the csv before adding copyrights
    > "$csv_file"

    # Adding copyrights to the csv
    echo "##" >> "$csv_file"
    echo "## Copyright (c) $(date +%Y) Qualcomm Innovation Center, Inc. All rights reserved." >> "$csv_file"
    echo "## SPDX-License-Identifier: BSD-3-Clause-Clear" >> "$csv_file"
    echo "##" >> "$csv_file"

    # Remove quotes from the sorted data and save to the original CSV file
    sed 's/"//g' temp_sorted_output.csv >> "$csv_file"

    # Remove the temporary CSV file
    rm temp_sorted_output.csv

    echo "CSV file sorted and saved to $csv_file"
}

# Trap Ctrl+C and call the handle_ctrl_c function
trap handle_ctrl_c SIGINT

# Run the ADB command and capture stderr only
$adb_command 2>&1 >/dev/null | while IFS= read -r line; do
    if [[ $line == \#\#\#* ]]; then
        echo "${line:3}" >> "$csv_file"
    fi
done

# If the script reaches here without Ctrl+C, process the CSV file
process_csv

