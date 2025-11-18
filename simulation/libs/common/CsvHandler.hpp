/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file       CsvHandler.hpp
 *
 * @brief      Declares the CsvHandler class that handles the CSV file read/write
 *             operations.
 *
 */

#ifndef CSV_HANDLER_HPP
#define CSV_HANDLER_HPP

#include <telux/common/CommonDefines.hpp>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <mutex>

using namespace std;

namespace telux {

namespace common {

/**
* @brief: Handles the content of the csv file in the following format.
*       For Ex: If csv contains
*       A,B,C,D,E
*       1,2,3,4,5
*       6,7,8,9,10
*       11,12,,14,15
*
*       Data is stored as:
*       Key->Value
*
*       A -> 1,6,11
*       B -> 2,7,12
*       C -> 3,8,0
*       D -> 4,9,14
*       E -> 5,10,15
*
*       To access data:
*       data[A][0]=1  data[B][0]=2
*       data[A][1]=6  data[B][1]=7
*
*/
typedef std::unordered_map<std::string, std::vector<string>> csvData;
typedef std::vector<std::string> License;

struct LicenseHeader {
    bool isAvailable;
    License license;
};

class CsvHandler {
public:
    /**
     * @brief: Open the file
     */
    CsvHandler(std::string filename);
    ~CsvHandler();

    /**
     * @brief:   Reads the complete CSV file
     * @param:   data     -    Reference to the data structure where the parsed content is
     *                         stored.

     */
    Status readCsv(csvData &data);

    /**
     *  @brief: Writes to the CSV file as a fresh file.
     *
     *  @param: headers       -   The data header content to be written to the file.
     *  @param: data          -   The content to be written to the file.
     *  @param: license       -   Lisence related properties are stored.
     */
    Status writeCsv(const std::vector<std::string>& headers, csvData &data,
        const LicenseHeader &license);

private:
    std::mutex fileMutex_;
    std::string filename_;
};

} // end of namespace common

} // end of namespace telux

#endif  // CSV_HANDLER_HPP