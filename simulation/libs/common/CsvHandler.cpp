/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CsvHandler.hpp"
#include "Logger.hpp"
#include "FileInfo.hpp"
#include <fstream>

#define DELIMETER ','

namespace telux {

namespace common {

inline bool fileExists(const std::string &csvFile) {
  std::ifstream f(csvFile.c_str());
  return f.good();
}

CsvHandler::CsvHandler(std::string filename) {
    std::string csvFilePath = std::string(DEFAULT_SIM_CSV_FILE_PATH) + filename;
    if (!fileExists(csvFilePath)) {
        csvFilePath = std::string(DEFAULT_SIM_FILE_PREFIX)
        + std::string(DEFAULT_SIM_CSV_FILE_PATH) + filename;
        if (!fileExists(csvFilePath)) {
            return;
        }
    }
    filename_ = csvFilePath;
}

CsvHandler::~CsvHandler() {
    filename_ = "";
}

Status CsvHandler::readCsv(csvData &data) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lk(CsvHandler::fileMutex_);
    std::string line, colname, val  = "";
    std::vector<std::string> headers;

    std::ifstream ifs(filename_);
    if(!ifs.is_open()) {
        LOG(ERROR, __FUNCTION__, "Could not open the file: ", filename_);
        return Status::FAILED;
    }

    if(ifs.good()) {
        LOG(DEBUG, "Starting to read csv");
        while(std::getline(ifs, line)) {
            //skipping empty line & lines that contains license text (starts with ##)
            if (line.size() != 0 && line.find("##") != 0) {
                break;
            }
        }

        /* Extracting the data header part */
        std::stringstream colStream(line);

        LOG(DEBUG, "Extracting Headers");
        // Extract each column name
        while(std::getline(colStream, colname, DELIMETER)) {
            headers.emplace_back(colname);
        }

        LOG(DEBUG, "Extracting data");
        // Extracting the row data
        while(std::getline(ifs, line))
        {
            // Create a stringstream of the current line
            std::stringstream rowStream(line);
            int row = 0;

            while (std::getline(rowStream, val, DELIMETER)) {
                /**additional handling for data within double-quotes.
                 For example: nmea_sentence could hold values like below
                 "$GLGSV,1,1,03,82,11,169,35,69,26,340,35,68,14,032,36,1*4C"

                 if case will be executed for the data that doesn't exist
                 within double-quotes.
                 **/
                if(val.front() != '"') {
                    data[headers[row]].emplace_back(val);
                } else {
                    //skipping ',' as delimeter if value inside double-quotes
                    string str = val;
                    std::getline(rowStream, val, '"');
                    str += ", " + val;
                    str.erase(0,1);
                    data[headers[row]].emplace_back(str);
                    std::getline(rowStream, val, ',');
                }
                row++;
            }
        }
    }
    LOG(DEBUG, "File read complete");
    // Close file
    ifs.close();
    return Status::SUCCESS;
}

Status CsvHandler::writeCsv(const std::vector<std::string>& headers, csvData &data,
    const LicenseHeader &license) {

    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(CsvHandler::fileMutex_);
    // Create an output filestream object
    std::ofstream ofs(filename_);
    /* Print the license header to the file.*/
    std::stringstream writeStream;

    if (license.isAvailable) {
        LOG(DEBUG, "Writing license content");
        for (auto val : license.license)
        {
            writeStream << val << "\n";
        }
        ofs << writeStream.str();
    }

    auto itr = data.begin();
    int columnSize = headers.size();
    long int rowSize = data[itr->first].size();
    int columnIdx = 0;
    writeStream.str(std::string());

    LOG(DEBUG, "Starting to write headers");
    for (auto headerVal: headers)
    {
        writeStream << headerVal;
        if(columnIdx != columnSize - 1) {
            writeStream << ","; // No comma at end of line
        }
        columnIdx++;
    }

    writeStream << "\n";
    ofs << writeStream.str();
    writeStream.str(std::string());

    LOG(DEBUG, "Starting to write data");
    for (long int idx=0;idx<rowSize;idx++)
    { //Writing data row-by-row
        columnIdx = 0;
        for (auto headerVal: headers)
        {
            if(data.find(headerVal) != data.end()) {
                writeStream << data[headerVal][idx];
            } else {
                writeStream << ""; //if header not matches
            }

            if(columnIdx != columnSize - 1) {
                writeStream << ","; // No comma at end of line
            }
            columnIdx++;
        }
        writeStream << "\n";
        ofs << writeStream.str();
        writeStream.str(std::string());
    }

    LOG(DEBUG, "Writing csv completed");
    ofs.close();
    return Status::SUCCESS;
}

} // end of namespace common

} // end of namespace telux
