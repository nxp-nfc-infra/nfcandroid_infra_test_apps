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
#define LOG_TAG "nfc_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/nfc/1.1/INfcClientCallback.h>
#include <android/hardware/nfc/1.2/INfc.h>
#include <android/hardware/nfc/1.2/types.h>
#include <gtest/gtest.h>
#include <hardware/nfc.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <VtsHalHidlTargetCallbackBase.h>

#include <string>//memcpy
#include <vendor/nxp/nxpnfc/2.0/INxpNfc.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::vendor::nxp::nxpnfc::V2_0::INxpNfc;
using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::nfc::V1_0::NfcData;
using ::android::hardware::nfc::V1_0::NfcStatus;
using ::android::hardware::nfc::V1_1::INfcClientCallback;
using ::android::hardware::nfc::V1_1::NfcEvent;
using ::android::hardware::nfc::V1_2::INfc;
using ::android::hardware::nfc::V1_2::NfcConfig;

constexpr char kCallbackNameSendEvent[] = "sendEvent";
constexpr char kCallbackNameSendData[] = "sendData";

/* NCI Commands */
#define CORE_RESET_CMD \
  { 0x20, 0x00, 0x01, 0x00 }
#define CORE_RESET_CMD_CONFIG_RESET \
  { 0x20, 0x00, 0x01, 0x01 }
#define CORE_CONN_CREATE_CMD \
  { 0x20, 0x04, 0x02, 0x01, 0x00 }
#define CORE_INIT_CMD \
  { 0x20, 0x01, 0x00 }
#define CORE_INIT_CMD_NCI20 \
  { 0x20, 0x01, 0x02, 0x00, 0x00 }
#define INVALID_COMMAND \
  { 0x20, 0x00, 0x00 }

#define NCI_PROPRIETARY_ACT_CMD \
  { 0x2F, 0x02, 0x00 }
#define NCI_DISABLE_STANDBY_MODE_CMD \
  { 0x2F, 0x00, 0x01, 0x00 }
#define NCI_ENABLE_STANDBY_MODE_CMD \
  { 0x2F, 0x00, 0x01, 0x01 }

#define TEST_SWIO1_CMD \
  { 0x2F, 0x3E, 0x01, 0x00 }
#define TEST_ESE_CMD \
  { 0x2F, 0x3E, 0x01, 0x01 }
#define TEST_SWIO2_CMD \
  { 0x2F, 0x3E, 0x01, 0x02 }

#define TEST_ANNTENA_RF_FIELD_ON_CMD \
  { 0x2F, 0x3D, 0x02, 0x20, 0x01}
#define TEST_ANNTENA_RF_FIELD_OFF_CMD \
  { 0x2F, 0x3D, 0x02, 0x20, 0x00 }

#define TEST_FW_PRBS_A_106_CMD \
  { 0x2F, 0x30, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01 }

#define LOOP_BACK_HEADER_SIZE 3
#define SYNTAX_ERROR 5
#define NUMBER_LOOPS 3922
#define NCI_VERSION_1_1 0x11
#define NCI_VERSION_2 0x20
#define TIMEOUT_PERIOD 5

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

/*// Test environment for Nfc HIDL HAL.
class NfcHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
 public:
  // get the test environment singleton
  static NfcHidlEnvironment* Instance() {
    static NfcHidlEnvironment* instance = new NfcHidlEnvironment;
    return instance;
  }

  virtual void registerTestServices() override {
    registerTestService<INfc>();
    registerTestService<INxpNfc>();
  }
 private:
  NfcHidlEnvironment() {}
};*/

// The main test class for NFC SelfTest  HAL.
class NfcSelfTestTest : public ::testing::TestWithParam<std::string> {
 public:
  void core_reset_command(void);
  void core_init_command(void);
  void nci_prop_command(void);

  virtual void SetUp() override {
    nfc_ = INfc::getService(GetParam());
    ASSERT_NE(nfc_, nullptr);

    nxpnfc_ = INxpNfc::getService(GetParam());
    ASSERT_NE(nxpnfc_, nullptr);

    nfc_cb_ = new NfcClientCallback();
    ASSERT_NE(nfc_cb_, nullptr);

    EXPECT_EQ(NfcStatus::OK, nfc_->open_1_1(nfc_cb_));
    // Wait for OPEN_CPLT event
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
    EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

    /* Get the NCI version that the device supports */
    std::vector<uint8_t> cmd = CORE_RESET_CMD;
    NfcData data = cmd;
    EXPECT_EQ(data.size(), nfc_->write(data));
    // Wait for CORE_RESET_RSP
    res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_GE(6ul, res.args->last_data_.size());
    EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
    if (res.args->last_data_.size() == 6) {
      nci_version = res.args->last_data_[4];
    } else {
      EXPECT_EQ(4ul, res.args->last_data_.size());
      nci_version = NCI_VERSION_2;
      res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
      EXPECT_TRUE(res.no_timeout);
    }

    /*
     * Close the hal and then re-open to make sure we are in a predictable
     * state for all the tests.
     */
    EXPECT_EQ(NfcStatus::OK, nfc_->close());
    // Wait for CLOSE_CPLT event
    res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
    EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

    EXPECT_EQ(NfcStatus::OK, nfc_->open_1_1(nfc_cb_));
    // Wait for OPEN_CPLT event
    res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
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

    /* NCI version the device supports
     * 0x11 for NCI 1.1, 0x20 for NCI 2.0 and so forth */
    uint8_t nci_version;
    sp<INfc> nfc_;
    sp<INxpNfc> nxpnfc_;
    sp<NfcClientCallback> nfc_cb_;
};

/*********************************************************************
**     FW PRBS TESTS:                                               **
**        Bitrate     Tech         Bitrate     Tech                 **
**       212 & 424   A, B, F      106 & 848    A, B                 **
*********************************************************************/
/*
 * Prbs_FW_A_106_SelfTest: Mode-FW,Tech-A,Bitrate-106.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_A_106_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_A_106_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_A_106_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}

/*
 * Prbs_FW_A_212_SelfTest:  Mode-FW,Tech-A,Bitrate-212
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_A_212_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_A_212_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_A_212_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[6]=0x01;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_A_424_SelfTest:  Mode-FW,Tech-A,Bitrate-424
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_A_424_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
 */
TEST_P(NfcSelfTestTest, Prbs_FW_A_424_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_A_424_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[6]=0x02;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}

/*
 * Prbs_FW_A_848_SelfTest:  Mode-FW,Tech-A,Bitrate-848
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_A_848_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
 */
TEST_P(NfcSelfTestTest, Prbs_FW_A_848_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_A_848_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[6]=0x03;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_B_848_SelfTest: Mode-FW,Tech-B,Bitrate-848.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_B_848_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_B_848_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_B_848_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[5]=0x01;
  data4[6]=0x03;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_B_424_SelfTest: Mode-FW,Tech-B,Bitrate-424.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_B_424_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
 */
TEST_P(NfcSelfTestTest, Prbs_FW_B_424_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_B_424_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[5]=0x01;
  data4[6]=0x02;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_B_212_SelfTest: Mode-FW,Tech-B,Bitrate-212.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_B_212_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_B_212_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_B_212_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[5]=0x01;
  data4[6]=0x01;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_B_106_SelfTest: Mode-FW,Tech-B,Bitrate-106.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_A_106_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_B_106_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_B_106_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[5]=0x01;
  data4[6]=0x00;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_F_212_SelfTest: Mode-FW,Tech-B,Bitrate-212.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_F_212_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_F_212_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_F_212_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[5]=0x02;
  data4[6]=0x01;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_FW_F_424_SelfTest: Mode-FW,Tech-B,Bitrate-424.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_FW_PRBS_F_424_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_FW_F_424_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_FW_PRBS_F_424_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[5]=0x02;
  data4[6]=0x02;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*********************************************************************
**     HW PRBS TESTS: (PRBS9 & PRBS15)                              **
**        Bitrate     Tech         Bitrate     Tech                 **
**       212 & 424   A, B, F      106 & 848    A, B                 **
*********************************************************************/
/*
 * Prbs_HW_PRBS9_A_106_SelfTest: Mode-FW, PRBS9, Tech-A,Bitrate-106.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_HW_PRBS9_A_106_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_HW_PRBS9_A_106_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_HW_PRBS9_A_106_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[3]=0x01;
  data4[4]=0x00;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_HW_PRBS9_A_212_SelfTest:  Mode-HW, PRBS9, Tech-A, Bitrate-212
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_HW_PRBS9_A_212_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_HW_PRBS9_A_212_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_HW_PRBS9_A_212_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[3]=0x01;
  data4[4]=0x00;
  data4[6]=0x01;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_HW_PRBS9_A_424_SelfTest: Mode-FW, PRBS9, Tech-A,Bitrate-424.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_HW_PRBS9_A_424_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
 */
TEST_P(NfcSelfTestTest, Prbs_HW_PRBS9_A_424_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_HW_PRBS9_A_424_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[3]=0x01;
  data4[4]=0x00;
  data4[6]=0x02;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_HW_PRBS9_A_848_SelfTest: Mode-FW, PRBS9, Tech-A,Bitrate-848.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_HW_PRBS9_B_848_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_HW_PRBS9_A_848_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_HW_PRBS9_B_848_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[3]=0x01;
  data4[4]=0x00;
  data4[6]=0x03;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_HW_PRBS15_B_848_SelfTest: Mode-FW,Tech-B,Bitrate-848.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_HW_PRBS15_B_848_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_HW_PRBS15_B_848_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_HW_PRBS15_B_848_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[3]=0x01;
  data4[4]=0x01;
  data4[5]=0x01;
  data4[6]=0x03;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * Prbs_HW_PRBS15_B_424_SelfTest: Mode-FW,Tech-B,Bitrate-424.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_DISABLE_STANDBY_MODE_CMD
 *       TEST_HW_PRBS15_B_424_CMD
 *       VEN_RESET
 * Waits for respective Response and Notifications. Validate the same.
*/
TEST_P(NfcSelfTestTest, Prbs_HW_PRBS15_B_424_SelfTest) {
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> prop_act_cmd = NCI_PROPRIETARY_ACT_CMD;
  std::vector<uint8_t> nci_disable_standby_cmd = NCI_DISABLE_STANDBY_MODE_CMD;
  std::vector<uint8_t> test_prbs_fw_a_106_cmd = TEST_FW_PRBS_A_106_CMD;
  std::vector<uint8_t> nci_enable_standby_cmd = NCI_ENABLE_STANDBY_MODE_CMD;
  //CORE_RESET_CMD
  NfcData data = core_reset_cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // Wait for CORE_RESET_NTF
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, res.args->last_data_.size());
  EXPECT_EQ(2ul, res.args->last_data_[3]);
  EXPECT_GE(1ul, res.args->last_data_[4]);
  //CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size()-3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_PROPRIETARY_ACT_CMD
  NfcData data2 = prop_act_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_PROPRIETARY_ACT_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(8ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //NCI_DISABLE_STANDBY_MODE_CMD
  NfcData data3 = nci_disable_standby_cmd;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  //TEST_HW_PRBS15_B_424_CMD
  NfcData data4 = test_prbs_fw_a_106_cmd;
  data4[3]=0x01;
  data4[4]=0x01;
  data4[5]=0x01;
  data4[6]=0x02;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
}
/*
 * HalApi_SelfTest: Test all NXPNFC HAL APIs.
 */

TEST_P(NfcSelfTestTest, INxpNfcApi_selfTest) {
/*********************************SET_TRANSIT_CONFIG************************************/
  //std::string transitconf = "This is dummy data\n";
  std::string transitconf;
  auto res = nxpnfc_->setNxpTransitConfig(transitconf);
  EXPECT_TRUE(res);
/*********************************NCI_TRANSCEIVE***************************************/
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcSelfTestTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, NfcSelfTestTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(INfc::descriptor)),
        android::hardware::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::system("svc nfc disable"); /* Turn off NFC */
    sleep(5);

    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;

    std::system("svc nfc enable"); /* Turn on NFC */
    sleep(5);

    return status;
}
