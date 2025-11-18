:: Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
:: SPDX-License-Identifier: BSD-3-Clause-Clear
:: Usage:
::    a. by default, this script captures detailed reports which has aggregated reports
::       from all engines running on the system.
::    b. Detailed engine reports can be captured by passing following arguments or any combinations
::       to option '-r':
::         FUSED for FUSED reports.
::         SPE means the unmodified SPE position is needed
::         PPE means the unmodified PPE position is needed
::         VPE means the unmodified VPE position is needed
::       for example:
::         location_test_app -r FUSED,SPE



@echo off
IF "%~1"=="" (
    echo Please provide the path to the CSV file.
    exit /b 1
)

set "CsvFilePath=%~1"

for /f "tokens=1-4 delims=/ " %%a in ('date /t') do set year=%%d

(
echo ##
echo ## Copyright ^(c^) %year% Qualcomm Innovation Center, Inc. All rights reserved.
echo ## SPDX-License-Identifier: BSD-3-Clause-Clear
echo ##
) > %CsvFilePath%

adb shell " location_test_app -r | grep '^###' | sed 's/\#\#\#//g' " >> %CsvFilePath%
