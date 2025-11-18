/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <stdlib.h>
#include <vector>

#include "v2x_log.h"

#include <telux/cv2x/Cv2xRxMetaDataHelper.hpp>
#include <telux/cv2x/legacy/v2x_packet_api.h>

using namespace telux::cv2x;

using telux::cv2x::Cv2xRxMetaDataHelper;
using telux::cv2x::RxPacketMetaDataReport;

/*
 * Parse the received packet's meta data from the payload
 */
v2x_status_enum_type v2x_parse_rx_meta_data(const uint8_t *payload, uint32_t length,
    rx_packet_meta_data_t *meta_data, size_t *num, size_t *meta_data_len) {
    if (nullptr == meta_data) {
        LOGE("%s: meta_data is null pointer\n", __FUNCTION__);
        return V2X_STATUS_EBADPARM;
    }

    size_t metalen = 0;
    std::vector<RxPacketMetaDataReport> metaDatasVec;
    auto metaDatas = std::make_shared<std::vector<RxPacketMetaDataReport>>(metaDatasVec);
    if (telux::common::Status::SUCCESS
        != Cv2xRxMetaDataHelper::getRxMetaDataInfo(payload, length, metalen, metaDatas)) {
        LOGE("%s: Error when parse meta data\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    if ((*metaDatas).size() < (*num)) {
        *num = (*metaDatas).size();
    }

    *meta_data_len = metalen;
    for (size_t i = 0; i < (*num); ++i) {
        meta_data[i].validity = static_cast<uint32_t>((*metaDatas)[i].metaDataMask);
        if ((*metaDatas)[i].metaDataMask & RX_SUBFRAME_NUMBER) {
            meta_data[i].sfn = (*metaDatas)[i].sfn;
        }
        if ((*metaDatas)[i].metaDataMask & RX_SUBCHANNEL_INDEX) {
            meta_data[i].sub_channel_index = (*metaDatas)[i].subChannelIndex;
        }
        if ((*metaDatas)[i].metaDataMask & RX_SUBCHANNEL_NUMBER) {
            meta_data[i].sub_channel_num = (*metaDatas)[i].subChannelNum;
        }
        if ((*metaDatas)[i].metaDataMask & RX_DELAY_ESTIMATION) {
            meta_data[i].delay_estimation = (*metaDatas)[i].delayEstimation;
        }
        if ((*metaDatas)[i].metaDataMask & RX_PRX_RSSI) {
            meta_data[i].prx_rssi = (*metaDatas)[i].prxRssi;
        }
        if ((*metaDatas)[i].metaDataMask & RX_DRX_RSSI) {
            meta_data[i].drx_rssi = (*metaDatas)[i].drxRssi;
        }
        if ((*metaDatas)[i].metaDataMask & RX_L2_DEST_ID) {
            meta_data[i].l2_destination_id = (*metaDatas)[i].l2DestinationId;
        }
        if ((*metaDatas)[i].metaDataMask & RX_SCI_FORMAT1) {
            meta_data[i].sci_format1_info = (*metaDatas)[i].sciFormat1Info;
        }
    }
    return V2X_STATUS_SUCCESS;
}
