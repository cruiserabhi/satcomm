# SASQUISH

1. Overview:

1.1 What is Sasquish for?
Sasquish is a public test app designed to exercise the Telsdk Congestion Control Library known as Squish.
It also serves as a way to demonstrate how ITS stack developers can integrate Squish into their software.

1.2 What is Squish?
Squish is a Telsdk library that implements the Congestion Control methods detailed in the
SAE j2945/1 and j3161/1 specifications.

2. How to use Sasquish
2.1 Usage
Sasquish usage can be detailed in the help output (by running sasquish-test-app with no args).
Sasquish usage is as follows:
sasquish-test-app
Mandatory Arguments:
        -i input log file path, -o output log file path
Optional Arguments:
        -c Config file path, -n rows to read from input log file (default: all), -t number of threads (default: 1)
         -v Verbose Mode
Examples:
        sasquish –c config_file_path –i input_log_file_path –o output_log_file_path –n 30000 –t 4
        Full congestion control test using input log file where it will output the results in –o argument.
        There will be 4 threads acting as receiving threads of an ITS stack that will be feeding SQUISH.
        Only 30000 rows will be read from –i file. Squish will be configured with ‘config_file_path’ parameters.

2.2 Format of Input and Output CSV Log Files
    The fields per row are as follows. Note: some are unused.
    TimeStamp,TimeStamp_ms,Time_monotonic,LogType,L2 ID,CBR,CPU Util,TXInterval,msgCnt,TempId,GPGSAMode,
    secMark,lat,long,semiMajorDev,speed,heading,longAccel,latAccel,TrackingError,SmoothedVehicleDensity,CQI,
    BSMValid,MaxITT,GPSTime,Events,CongCtrlRandTime,SPSHysterisis,TotalRVs,DistanceFromRV

    TimeStamp -  Date and time string
    TimeStamp_ms - uint64 linux epoch timestamp ms
    Time_monotonic - uint64 time since start ms
    LogType - TX or RX record (we need a mix of both so that Sasquish will emulate HV and feed RX records as RVs to Squish)
    L2 ID - l2 id address of the vehicle (uint64_t)
    CBR - channel busy rate percentage (unused)
    CPU Util - current cpu utilization (unused)
    TXInterval - the measured rate between each TX (float)
    msgCnt - the message count of the packet (int)
    TempId - Temporary id of the vehicle (int)
    GPGSAMode - GPS dilution of provision and Active Satellite (unused)
    secMark - second mark of the BSM
    lat - latitude of vehicle
    long - longitude of vehicle
    semiMajorDev - semi major axis deviation / accuracy (unused)
    speed - speed of the vehicle
    heading - heading of the vehicle
    longAccel - logitudinal acceleration of vehicle
    latAccel - latitude acceleration of vehicle
    TrackingError - current calculated tracking error
    SmoothedVehicleDensity - current smoothed vehicle density
    CQI - channel quality indicator
    BSMValid - if this BSM was correctly decoded and validated or not
    MaxITT - current maximum Inter-Transmit Time
    GPSTime - time at which vehicle receives the corresponding GNSS data
    Events - String of bits representing different critical event flags
        EEBL, FCW, BSW/LCW, IMA, LTA, CLW
    CongCtrlRandTime - Random chance that the max ITT will be updated or not
    SPSHysterisis - Hysteresis used in defining thresholds for making decision on ITT update
    TotalRVs - Total number of RVs this vehicle is currently seeing
    DistanceFromRV - Current distance from the RV whose log this row corresponds to

2.2.1
We have a few sample input CSV files in /sample_input_files/ to demonstrate what the files should look like.
NOTE: If you are using one of the sample csv files. Please remove the first three commented lines.
1. sample_1.csv corresponds to: 1200 RVs with 0% packet loss
2. sample_2.csv corresponds to: 1200 RVs with 10% packet loss
3. sample_3.csv corresponds to: 1200 RVs with 50% packet loss
4. sample_4.csv corresponds to: with HV data that should trigger a Tracking Error-induced Msg from HV

2.3 Config Input File Format
By default, Sasquish can be run without any config file and uses default values according to j2945/1 and j3161/1.
However, for testing purposes, one can run Sasquish with your own custom config file.
Note: All fields below must be in the config file.

# enable logging of SQUISH; Logging Mask
# logging level 0-10
congCtrlLoggingLvl = 0

# packet error rate related
perInterval = 5000
perSubInterval = 1000
perMax = 0.3

# Inter Transmit Time and Tx Decision related
vRescheduleTh      = 25 # 0, 10000; threshold for helping determine transmit rescheduling
vMax_ITT               = 600 # 0, 10000; maximum inter-transmit time allowed
minItt = 100 # 2x 50 5x 20; minimum inter-transmit time allowed
txRand = 0 # +- noise set to scheduling; but set to 0 for CV2X
timeAccuracy = 1000 # precision for time

# channel quality indication related
minChanQualInd = 0.0 # minimum channel quality indicator value
maxChanQualInd = 0.3 # maximum channel quality indicator value

# density related
vDensityWeightFactor = .05 # lambda used for density smoothing
vDensityCoefficient = 25 #16.06 2x; 6.22  5x; 1x = 33.8; default: 25; used for rescheduling based on smoothed density
vDensityMinPerRange = 100 # minimum distance in meters for vehicles to be considered in range

# Tracking Error related
txCtrlInterval = 100 # ms ; interval for periodic Tracking Error and ITT calculations
hvTEMinTimeDiff = 0 # minimum host vehicle tracking error difference
hvTEMaxTimeDiff = 200 # maximum host vehicle tracking error difference
rvTEMinTimeDiff = 0 # minimum host vehicle tracking error difference
rvTEMaxTimeDiff = 3000 # minimum host vehicle tracking error difference
teErrSensitivity = 75 # base error sensitivity value used to calculate probability of successful transmission
teMinThresh = 200 # minimum threshold for tracking error;
teMaxThresh = 500 # maximum threshold for tracking error;

#sps enhancements parameters
enableSpsEnhancements = true # flag for disabling or enabling sps enhancements
spsEnhIntervalRound = 100 # 20, 1000; the resolution at which we round the ITT to the SPS periodicity
spsEnhHysterPerc = 5 #  hysteresis percentage which determines ITT thresholds for choosing when to change ITT and SPS periodicity
spsEnhDelayPerc = 20 # chance for changing the ITT and SPS periodicity

# options to test with a set number of unique RV temp IDs
fakeRVTempIds = false # Fake the temp ids in RXed BSMs
                            # for testing purposes only
totalFakeRVTempIds = 500 # total number of temp ids to simulate RVs
msgCntGap = 1   # gap between each msg cnt from same temp id
