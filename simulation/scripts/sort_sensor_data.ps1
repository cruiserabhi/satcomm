<# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   SPDX-License-Identifier: BSD-3-Clause-Clear
#>
<#
.SYNOPSIS
This script sorts a CSV file based on the values in the third column, removes the last line before sorting.

.DESCRIPTION
The script takes the path of the CSV file as a positional argument. If the CSV file is in the same directory as the script,
you can provide just the file name. The script performs the following steps:
1. Imports the CSV file.
2. Removes the last line from the imported data.
3. Sorts the remaining data based on the values in the third column.
4. Writes the sorted data back to the original CSV file.

.PARAMETER CsvFilePath
The path to the CSV file to be sorted. If the file is in the same directory as the script, just provide the file name.
#>

# This should be the first line, else will throw an error
param (
    [string]$CsvFilePath
)

# Check if the file path is provided
if (-not $CsvFilePath) {
    Write-Error "Please provide the path to the CSV file."
    exit
}

# Get the current date
$currentDate = Get-Date

# Extract the year
$year = $currentDate.Year

$copyRights = @(
    "##",
    "## Copyright (c) $year Qualcomm Innovation Center, Inc. All rights reserved.",
    "## SPDX-License-Identifier: BSD-3-Clause-Clear",
    "##"
)

# Import the CSV file without headers, assuming 9 columns
$data = Import-Csv -Path $CsvFilePath -Header Column1, Column2, Column3, Column4, Column5, Column6, Column7, Column8, Column9

# Check if the data was imported correctly
if ($data -eq $null -or $data.Count -eq 0) {
    Write-Error "Failed to import CSV or CSV is empty."
    exit
}

# Remove the last line
$data = $data[0..($data.Count - 2)]

# Sort the data based on the third column
$sortedData = $data | Sort-Object -Property Column3

# Clear the existing content in the CSV file
Clear-Content -Path $CsvFilePath

# Write the copyright lines to the CSV
$copyRights | Out-File -FilePath $CsvFilePath -Encoding utf8

# Write the sorted data back to the original CSV file without headers and without quotes
$sortedData | ForEach-Object {
    "$($_.Column1),$($_.Column2),$($_.Column3),$($_.Column4),$($_.Column5),$($_.Column6),$($_.Column7),$($_.Column8),$($_.Column9)"
} | Add-Content -Path $CsvFilePath

Write-Host "CSV file sorted and saved to $CsvFilePath"

