:: Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
:: SPDX-License-Identifier: BSD-3-Clause-Clear
:: Usage:
::   By default, this script captures sensor events for UNCALIBRATED_ACCELEROMETER AND
::   UNCALIBRATED_GYROSCOPE with both ROTATED and UNROTATED configurations enabled at
::   104hz sampling rate and 50 batchcount.
:: Note: Once captured, sort CSV using sort_sensor_data.ps1. Provide csv file
:: as input parameter to this script and pass the same csv to sort_sensor_data.ps1 for sorting.

@echo off
REM Check if csv path is provided
IF "%~1"=="" (
    echo Please provide the path to the CSV file.
    exit /b 1
)

set "CsvFilePath=%~1"

adb shell "sensor_test_app -r 2>&1 >/dev/null | grep '^###' | sed 's/\#\#\#//g' " >> %CsvFilePath%
pause
