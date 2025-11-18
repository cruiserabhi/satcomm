#
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

# Usage:
#    a. by default, this script captures detailed reports which has aggregated reports
#       from all engines running on the system.
#    b. Detailed engine reports can be captured by passing following arguments or any combinations
#       to option '-r':
#         FUSED for FUSED reports.
#         SPE means the unmodified SPE position is needed
#         PPE means the unmodified PPE position is needed
#         VPE means the unmodified VPE position is needed
#       for example:
#         location_test_app -r FUSED,SPE

# Check if the file path is provided
if [ -z "$1" ]; then
    echo "Please provide the path to the CSV file."
    exit 1
fi

# Define the path to the CSV file
csv_file="$1"

# Clearing the csv before adding copyrights
> "$csv_file"

# Adding copyrights to the csv
echo "##" >> "$csv_file"
echo "## Copyright (c) $(date +%Y) Qualcomm Innovation Center, Inc. All rights reserved." >> "$csv_file"
echo "## SPDX-License-Identifier: BSD-3-Clause-Clear" >> "$csv_file"
echo "##" >> "$csv_file"

adb shell " location_test_app -r | grep '^###' | sed 's/\#\#\#//g' " >> "$csv_file"
