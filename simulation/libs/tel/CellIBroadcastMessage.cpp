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

#include <telux/tel/CellBroadcastManager.hpp>

#define MESSAGE_CODE_MASK 0x3FF
#define UPDATE_NUMBER_MASK 0x000F


namespace telux {
namespace tel {

Polygon::Polygon(std::vector<Point> vertices)
      :vertices_(vertices) {
}

std::vector<Point> Polygon::getVertices() {
   return vertices_;
}

Circle::Circle(Point center, double radius)
      : center_(center)
      , radius_(radius) {
}

Point Circle::getCenter() {
   return center_;
}

double Circle::getRadius() {
   return radius_;
}

Geometry::Geometry(std::shared_ptr<Polygon> polygon)
   : polygon_(polygon) {
      type_ = GeometryType::POLYGON;
}

Geometry::Geometry(std::shared_ptr<Circle> circle)
   : circle_(circle) {
      type_ = GeometryType::CIRCLE;
}

GeometryType Geometry::getType() const {
   return type_;
}

std::shared_ptr<Polygon> Geometry::getPolygon() const {
   return polygon_;
}

std::shared_ptr<Circle> Geometry::getCircle() const {
   return circle_;
}

WarningAreaInfo::WarningAreaInfo(int maxWaitTime,
   std::vector<Geometry> geometries)
      : maxWaitTime_(maxWaitTime)
      , geometries_(geometries) {

}

int WarningAreaInfo::getGeoFenceMaxWaitTime() {
   return maxWaitTime_;
}

std::vector<Geometry> WarningAreaInfo::getGeometries() {
   return geometries_;
}

/**
 * Contains information elements for a GSM/UMTS/E-UTRAN/NG-RAN ETWS warning notification.
 * Supported values for each element are defined in 3GPP TS 23.041.
 */
EtwsInfo::EtwsInfo(GeographicalScope geographicalScope, int msgId, int serialNumber,
      std::string languageCode, std::string body, MessagePriority priority,
      EtwsWarningType warningType, bool emergencyUserAlert, bool activatePopup,
      bool primary, std::vector<uint8_t> warningSecurityInformation)
      : scope_(geographicalScope)
      , messageId_(msgId)
      , serialNum_(serialNumber)
      , languageCode_(languageCode)
      , body_(body)
      , priority_(priority)
      , warningType_(warningType)
      , emergencyUserAlert_(emergencyUserAlert)
      , activatePopup_(activatePopup)
      , isPrimary_(primary)
      , warningInfo_(warningSecurityInformation) {
}

GeographicalScope EtwsInfo::getGeographicalScope() const {
   return scope_;
}

int EtwsInfo::getMessageId() const {
   return messageId_;
}

int EtwsInfo::getSerialNumber() const {
   return serialNum_;
}

std::string EtwsInfo::getLanguageCode() const {
   return languageCode_;
}

std::string EtwsInfo::getMessageBody() const {
   return body_;
}

MessagePriority EtwsInfo::getPriority() const {
   return priority_;
}

int EtwsInfo::getMessageCode() const {
   return ((serialNum_ >> 4) & MESSAGE_CODE_MASK);
}

int EtwsInfo::getUpdateNumber() const {
   return (serialNum_ & UPDATE_NUMBER_MASK);
}

EtwsWarningType EtwsInfo::getEtwsWarningType() {
   return warningType_;
}

bool EtwsInfo::isEmergencyUserAlert() {
   return emergencyUserAlert_;
}

bool EtwsInfo::isPopupAlert() {
   return activatePopup_;
}

bool EtwsInfo::isPrimary() {
   return isPrimary_;
}

std::vector<uint8_t> EtwsInfo::getWarningSecurityInformation() {
   return warningInfo_;
}

/**
 * Contains information elements for a GSM/UMTS/E-UTRAN/NG-RAN CMAS warning notification.
 * Supported values for each element are defined in 3GPP TS 23.041.
 */
CmasInfo::CmasInfo(GeographicalScope geographicalScope, int msgId, int serialNumber,
   std::string languageCode, std::string body, MessagePriority priority,
   CmasMessageClass messageClass, CmasSeverity severity,
   CmasUrgency urgency, CmasCertainty certainty,
   std::shared_ptr<WarningAreaInfo> warningAreaInfo)
   : scope_(geographicalScope)
   , messageId_(msgId)
   , serialNum_(serialNumber)
   , languageCode_(languageCode)
   , body_(body)
   , priority_(priority)
   , messageClass_(messageClass)
   , severity_(severity)
   , urgency_(urgency)
   , certainity_(certainty)
   , warningAreaInfo_(warningAreaInfo) {
}

GeographicalScope CmasInfo::getGeographicalScope() const {
   return scope_;
}

int CmasInfo::getMessageId() const {
   return messageId_;
}

int CmasInfo::getSerialNumber() const {
   return serialNum_;
}

std::string CmasInfo::getLanguageCode() const {
   return languageCode_;
}

std::string CmasInfo::getMessageBody() const {
   return body_;
}

MessagePriority CmasInfo::getPriority() const {
   return priority_;
}

int CmasInfo::getMessageCode() const {
   return ((serialNum_ >> 4) & MESSAGE_CODE_MASK);
}

int CmasInfo::getUpdateNumber() const {
   return (serialNum_ & UPDATE_NUMBER_MASK);
}

CmasMessageClass CmasInfo::getMessageClass() {
   return messageClass_;
}

CmasSeverity CmasInfo::getSeverity() {
   return severity_;
}

CmasUrgency CmasInfo::getUrgency() {
   return urgency_;
}

CmasCertainty CmasInfo::getCertainty() {
   return certainity_;
}

std::shared_ptr<WarningAreaInfo> CmasInfo::getWarningAreaInfo() {
   return warningAreaInfo_;
}

/**
 * CellBroadcastMessage class to expose details to user application.
 */
CellBroadcastMessage::CellBroadcastMessage(std::shared_ptr<EtwsInfo> etwsInfo)
   : etwsInfo_(etwsInfo) {
      messageType_ = MessageType::ETWS;
}

/**
 * CellBroadcastMessage class to expose details to user application.
 */
CellBroadcastMessage::CellBroadcastMessage(std::shared_ptr<CmasInfo> cmasInfo)
   : cmasInfo_(cmasInfo) {
      messageType_ = MessageType::CMAS;
}

MessageType CellBroadcastMessage::getMessageType() const {
   return messageType_;
}

std::shared_ptr<EtwsInfo> CellBroadcastMessage::getEtwsInfo() const {
   return etwsInfo_;
}

std::shared_ptr<CmasInfo> CellBroadcastMessage::getCmasInfo() const {
   return cmasInfo_;
}

}
}