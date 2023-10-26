/******************************************************************************
 *
 *  Copyright 2023 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#define LOG_TAG "CockpitApp"

#include "pugixml.hpp"
#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/logging.h>
#include <android/hardware/nfc/1.1/INfcClientCallback.h>
#include <android/hardware/nfc/1.2/INfc.h>
#include <android/hardware/nfc/1.2/types.h>
#include <gtest/gtest.h>
#include <hardware/nfc.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <iostream>
#include <log/log.h>
#include <string>
#include <vector>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::nfc::V1_0::NfcData;
using ::android::hardware::nfc::V1_0::NfcStatus;
using ::android::hardware::nfc::V1_1::INfcClientCallback;
using ::android::hardware::nfc::V1_1::NfcEvent;
using ::android::hardware::nfc::V1_2::INfc;

constexpr char kCallbackNameSendEvent[] = "sendEvent";
constexpr char kCallbackNameSendData[] = "sendData";
#define DUMMP_XML_FILE_PATH "/vendor/etc/PN72xx_DumpData.xml"

class NfcClientCallbackArgs {
public:
  NfcEvent last_event_;
  NfcStatus last_status_;
  NfcData last_data_;
};

/* Callback class for data & Event. */
class NfcClientCallback
    : public ::testing::VtsHalHidlTargetCallbackBase<NfcClientCallbackArgs>,
      public INfcClientCallback {
public:
  virtual ~NfcClientCallback() = default;

  /* sendEvent callback function - Records the Event & Status
   * and notifies the TEST
   **/
  Return<void> sendEvent_1_1(NfcEvent event, NfcStatus event_status) override {
    NfcClientCallbackArgs args;
    args.last_event_ = event;
    args.last_status_ = event_status;
    NotifyFromCallback(kCallbackNameSendEvent, args);
    return Void();
  };

  /** NFC 1.1 HAL shouldn't send 1.0 callbacks */
  Return<void> sendEvent(
      __attribute__((unused)) ::android::hardware::nfc::V1_0::NfcEvent event,
      __attribute__((unused)) NfcStatus event_status) override {
    return Void();
  }

  /* sendData callback function. Records the data and notifies the TEST*/
  Return<void> sendData(const NfcData &data) override {
    NfcClientCallbackArgs args;
    args.last_data_ = data;
    NotifyFromCallback(kCallbackNameSendData, args);
    return Void();
  };
};

class NfcSelfTestTest : public ::testing::TestWithParam<std::string> {
public:
  void core_reset_command(void);
  void core_init_command(void);
  void nci_prop_command(void);

  virtual void SetUp() override {
    nfc_ = INfc::getService(GetParam());
    ASSERT_NE(nfc_, nullptr);

    nfc_cb_ = new NfcClientCallback();
    ASSERT_NE(nfc_cb_, nullptr);
    EXPECT_EQ(NfcStatus::OK, nfc_->open_1_1(nfc_cb_));
    // Wait for OPEN_CPLT event
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
    EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
  }

  virtual void TearDown() override {
    EXPECT_EQ(NfcStatus::OK, nfc_->close());
    // Wait for CLOSE_CPLT event
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
    EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
  }

  uint8_t nci_version;
  sp<INfc> nfc_;
  sp<NfcClientCallback> nfc_cb_;
};
