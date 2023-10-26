/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#include <android-base/logging.h>
#include <android/hardware/nfc/1.1/INfcClientCallback.h>
#include <android/hardware/nfc/1.2/INfc.h>
#include <android/hardware/nfc/1.2/types.h>
#include <gtest/gtest.h>
#include <hardware/nfc.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <string>
#include <iostream>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::nfc::V1_0::NfcData;
using ::android::hardware::nfc::V1_0::NfcStatus;
using ::android::hardware::nfc::V1_1::INfcClientCallback;
using ::android::hardware::nfc::V1_1::NfcEvent;
using ::android::hardware::nfc::V1_2::INfc;

constexpr char kCallbackNameSendEvent[] = "sendEvent";
constexpr char kCallbackNameSendData[]  = "sendData";

/* NCI Commands */
#define CORE_RESET_CMD_CONFIG_RESET  { 0x20, 0x00, 0x01, 0x01 }
#define CORE_INIT_CMD_NCI20          { 0x20, 0x01, 0x02, 0x00, 0x00 }
#define NCI_SWITCH_RF_FIELD_CMD_1    { 0x2F, 0x0E, 0x02, 0x00, 0x80}
#define NCI_SWITCH_RF_FIELD_CMD_2    { 0x2F, 0x3F, 0x03, 0x32, 0x01, 0x00}
#define NCI_SWITCH_RF_FIELD_CMD_3    { 0x2F, 0x3F, 0x02, 0x32, 0x00 }
#define NCI_SWITCH_RF_FIELD_CMD_4    { 0x2F, 0x3F, 0x03, 0x32, 0x01, 0x01}
#define NCI_SWITCH_RF_FIELD_CMD_5    { 0x2F, 0x3F, 0x03, 0x32, 0x02, 0x01}
#define NCI_SWITCH_RF_FIELD_CMD_6    { 0x2F, 0x3F, 0x03, 0x32, 0x02, 0x00}
#define NCI_CTS_TEST_CMD_2           { 0x2F, 0x22, 0x02, 0x81, 0x01}
#define NCI_CTS_TEST_CMD_3           { 0x2F, 0x22, 0x02, 0x81, 0x00}
#define NCI_DISCOVERY_CMD            { 0x21, 0x03, 0x07, 0x03, 0x00, 0x01, \
                                       0x01, 0x03, 0x02, 0x05}
#define NCI_CTS_TEST_CMD_1           { 0x2F, 0x22, 0x2F, 0x80, 0x00, 0x00, \
                                       0x07, 0x00, 0x00, 0x02, 0x9B, 0x00, \
                                       0x00, 0x00, 0x9B, 0x00, 0x00, 0x00, \
                                       0x00, 0x10, 0x00, 0x00, 0x00, 0x10, \
                                       0x00, 0x00, 0x00, 0x77, 0x77, 0x07, \
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                       0x00, 0x00, 0x00, 0x77, 0x77, 0x07, \
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                       0x00, 0x00 }
#define NCI_LOAD_PROTOCOL_CMD_1      { 0x2F, 0x0E, 0x02, 0x04, 0x80 }
#define NCI_LOAD_PROTOCOL_CMD_2      { 0x2F, 0x0E, 0x02, 0x04, 0x84 }
#define NCI_LOAD_PROTOCOL_CMD_3      { 0x2F, 0x0E, 0x02, 0x08, 0x88 }
#define NCI_LOAD_PROTOCOL_CMD_4      { 0x2F, 0x0E, 0x02, 0xFE, 0xFE }
#define NCI_LOAD_PROTOCOL_SUB_CMD_1  { 0x2F, 0x3F, 0x03, 0x32, 0x01, 0x00 }
#define NCI_LOAD_PROTOCOL_SUB_CMD_2  { 0x2F, 0x3F, 0x02, 0x32, 0x00 }
#define NCI_PRBS_CMD_1               { 0x2F, 0x0E, 0x02, 0x00, 0x80 }
#define NCI_PRBS_CMD_2               { 0x2F, 0x3F, 0x03, 0x32, 0x01, 0x00 }
#define NCI_PRBS_CMD_3               { 0x2F, 0x30, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define NCI_PRBS_CMD_4               { 0x2F, 0x3F, 0x02, 0x32, 0x00 }
#define NCI_PRBS_CMD_5               { 0x2F, 0x3F, 0x03, 0x32, 0x01, 0x00 }
#define NCI_PRBS_CMD_6               { 0x2F, 0x30, 0x06, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00 }
#define NCI_PRBS_CMD_7               { 0x2F, 0x3F, 0x02, 0x32, 0x00 }
#define NCI_PRBS_CMD_8               { 0x2F, 0x3F, 0x03, 0x32, 0x01, 0x00 }
#define NCI_PRBS_CMD_9               { 0x2F, 0x30, 0x06, 0x01, 0x00, 0x02, 0x01, 0x00, 0x00 }
#define NCI_PRBS_CMD_10              { 0x2F, 0x3F, 0x02, 0x32, 0x00 }
#define NCI_GET_EPROM_91_DATA_CMD    { 0x20, 0x03, 0x04, 0x01, 0xA0, 0x0D, 0x91}
#define NCI_GET_EPROM_92_DATA_CMD    { 0x20, 0x03, 0x04, 0x01, 0xA0, 0x0D, 0x92}

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
    Return<void> sendEvent(__attribute__((unused))::android::hardware::nfc::V1_0::NfcEvent event,
                           __attribute__((unused)) NfcStatus event_status) override {
        return Void();
    }

    /* sendData callback function. Records the data and notifies the TEST*/
    Return<void> sendData(const NfcData& data) override {
        NfcClientCallbackArgs args;
        args.last_data_ = data;
        NotifyFromCallback(kCallbackNameSendData, args);
        return Void();
    };
};

// The main test class for NFC SelfTest  HAL.
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
