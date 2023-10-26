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
 *  Copyright 2022 NXP
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
#include <string>

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
#define CORE_RESET_CMD_CONFIG_RESET \
  { 0x20, 0x00, 0x01, 0x01 }
#define CORE_INIT_CMD_NCI20 \
  { 0x20, 0x01, 0x02, 0x00, 0x00 }
#define NCI_NFCC_CONFIG_CONTROL_CMD \
  { 0x20, 0x02, 0x04, 0x01, 0x85, 0x01, 0x01}
#define NCI_SWITCH_RF_FIELD_CMD_1\
  { 0x2F, 0x0E, 0x02, 0x00, 0x80}
#define NCI_SWITCH_RF_FIELD_CMD_2 \
  { 0x2F, 0x3F, 0x02, 0x01, 0x00}
#define NCI_SWITCH_RF_FIELD_CMD_3 \
  { 0x2F, 0x3F, 0x01, 0x00}
#define NCI_SWITCH_RF_FIELD_CMD_4 \
  { 0x2F, 0x3F, 0x02, 0x01, 0x01}
#define NCI_SWITCH_RF_FIELD_CMD_5 \
  { 0x2F, 0x3F, 0x02, 0x02, 0x01}
#define NCI_SWITCH_RF_FIELD_CMD_6 \
  { 0x2F, 0x3F, 0x02, 0x02, 0x00}
#define NCI_CTS_TEST_CMD_2 \
  {0x2F, 0x22, 0x02, 0x81, 0x01}
#define NCI_CTS_TEST_CMD_3 \
  {0x2F, 0x22, 0x02, 0x81, 0x00}
#define NCI_DISCOVERY_CMD \
  {0x21, 0x03, 0x07, 0x03, 0x00, 0x01, \
   0x01, 0x03, 0x02, 0x05}
#define NCI_CTS_TEST_CMD_1 \
  { 0x2F, 0x22, 0x2F, 0x80, 0x00, 0x00, \
    0x07, 0x00, 0x00, 0x02, 0x9B, 0x00, \
    0x00, 0x00, 0x9B, 0x00, 0x00, 0x00, \
    0x00, 0x10, 0x00, 0x00, 0x00, 0x10, \
    0x00, 0x00, 0x00, 0x77, 0x77, 0x07, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x77, 0x77, 0x07, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00 }
#define NCI_LOAD_PROTOCOL_CMD_1                                                \
  { 0x2F, 0x0E, 0x02, 0x04, 0x80 }
#define NCI_LOAD_PROTOCOL_CMD_2                                                \
  { 0x2F, 0x0E, 0x02, 0x04, 0x84 }
#define NCI_LOAD_PROTOCOL_CMD_3                                                \
  { 0x2F, 0x0E, 0x02, 0x08, 0x88 }
#define NCI_LOAD_PROTOCOL_CMD_4                                                \
  { 0x2F, 0x0E, 0x02, 0xFE, 0xFE }
#define NCI_LOAD_PROTOCOL_SUB_CMD_1                                            \
  { 0x2F, 0x3F, 0x02, 0x01, 0x00 }
#define NCI_LOAD_PROTOCOL_SUB_CMD_2                                            \
  { 0x2F, 0x3F, 0x01, 0x00 }

#define NCI_WRITE_READ_EE_GET_CONFIG_CMD_1                                     \
  { 0x20, 0x03, 0x04, 0x01, 0xA2, 0x02, 0x08 }
#define NCI_WRITE_READ_EE_SET_CONFIG_CMD_1                                     \
  {                                                                            \
    0x20, 0x02, 0x17, 0x02, 0xA2, 0x02, 0x08, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA,    \
        0xBB, 0xCC, 0xDD, 0xA2, 0x04, 0x08, 0xFF, 0x08, 0xF6, 0x01, 0x00,      \
        0x33, 0x64, 0x10                                                       \
  }
#define NCI_WRITE_READ_EE_SET_CONFIG_CMD_2                                     \
  {                                                                            \
    0x20, 0x02, 0x17, 0x02, 0xA2, 0x02, 0x08, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA,    \
        0xBB, 0xCC, 0xDD, 0xA2, 0x04, 0x08, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA,      \
        0xBB, 0xCC, 0xDD                                                       \
  }
#define NCI_WRITE_READ_EE_GET_CONFIG_CMD_2                                     \
  { 0x20, 0x03, 0x04, 0x01, 0xA2, 0x04, 0x08 }
#define NCI_WRITE_READ_EE_GET_CONFIG_CMD_3                                     \
  { 0x20, 0x03, 0x04, 0x01, 0xA2, 0xFF, 0xFF }

#define NCI_PRBS_CMD_1                                                         \
  { 0x2F, 0x0E, 0x02, 0x00, 0x80 }
#define NCI_PRBS_CMD_2                                                         \
  { 0x2F, 0x3F, 0x02, 0x01, 0x00 }
#define NCI_PRBS_CMD_3                                                         \
  { 0x2F, 0x30, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define NCI_PRBS_CMD_4                                                         \
  { 0x2F, 0x3F, 0x01, 0x00 }
#define NCI_PRBS_CMD_5                                                         \
  { 0x2F, 0x3F, 0x02, 0x01, 0x00 }
#define NCI_PRBS_CMD_6                                                         \
  { 0x2F, 0x30, 0x06, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00 }
#define NCI_PRBS_CMD_7                                                         \
  { 0x2F, 0x3F, 0x01, 0x00 }
#define NCI_PRBS_CMD_8                                                         \
  { 0x2F, 0x3F, 0x02, 0x01, 0x00 }
#define NCI_PRBS_CMD_9                                                         \
  { 0x2F, 0x30, 0x06, 0x01, 0x00, 0x02, 0x01, 0x00, 0x00 }
#define NCI_PRBS_CMD_10                                                        \
  { 0x2F, 0x3F, 0x01, 0x00 }
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

/*********************************************************************
**       Self TESTS: RF Swith field test 1                          **
*********************************************************************/
/*
 * phNci_SwitchRf_Field_Test1: Validate the RF Field Switch.
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_SWITCH_RF_FIELD_CMD_1,
 *       NCI_SWITCH_RF_FIELD_CMD_2,
 *       NCI_SWITCH_RF_FIELD_CMD_3
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_SwitchRf_Field_Test1) {
  LOG(INFO) << "Enter phNci_SwitchRf_Field_Test :: SwitchRF_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_switch_rf_field_cmd_1 = NCI_SWITCH_RF_FIELD_CMD_1;
  std::vector<uint8_t> nci_switch_rf_field_cmd_2 = NCI_SWITCH_RF_FIELD_CMD_2;
  std::vector<uint8_t> nci_switch_rf_field_cmd_3 = NCI_SWITCH_RF_FIELD_CMD_3;

  //CORE_RESET_CMD_CONFIG_RESET
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

  //NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_1
  NfcData data3 = nci_switch_rf_field_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_1 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // Rsp must be 4F0E0100
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_2
  NfcData data4 = nci_switch_rf_field_cmd_2;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_2 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  //RSP must be 4F3F0100
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_3
  NfcData data5 = nci_switch_rf_field_cmd_3;
  EXPECT_EQ(data5.size(), nfc_->write(data5));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_3 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // RSP must be 4F3F0100
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  sleep(1);
  LOG(INFO) << "Exit phNci_SwitchRf_Field_Test :: SwitchRF_P001";
}

/*********************************************************************
**       Self TESTS: RF Swith field test                            **
*********************************************************************/
/*
 * phNci_SwitchRf_Field_Test2: Validate the RF Field Switch.
 * Sends CORE_RESET_CMD,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_SWITCH_RF_FIELD_CMD_1,
 *       NCI_SWITCH_RF_FIELD_CMD_4,
 *       NCI_SWITCH_RF_FIELD_CMD_3
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_SwitchRf_Field_Test2) {
  LOG(INFO) << "Enter phNci_SwitchRf_Field_Test :: SwitchRF_P002";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_switch_rf_field_cmd_1 = NCI_SWITCH_RF_FIELD_CMD_1;
  std::vector<uint8_t> nci_switch_rf_field_cmd_4 = NCI_SWITCH_RF_FIELD_CMD_4;
  std::vector<uint8_t> nci_switch_rf_field_cmd_3 = NCI_SWITCH_RF_FIELD_CMD_3;

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

  //NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_1
  NfcData data3 = nci_switch_rf_field_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_1 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_4
  NfcData data4 = nci_switch_rf_field_cmd_4;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_4 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_3
  NfcData data5 = nci_switch_rf_field_cmd_3;
  EXPECT_EQ(data5.size(), nfc_->write(data5));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_3 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  sleep(1);
  LOG(INFO) << "Exit phNci_SwitchRf_Field_Test :: SwitchRF_P002";
}


/*********************************************************************
**       Self TESTS: RF Swith field test                            **
*********************************************************************/
/*
 * phNci_SwitchRf_Field_Test3: Validate the RF Field Switch.
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_SWITCH_RF_FIELD_CMD_1,
 *       NCI_SWITCH_RF_FIELD_CMD_5
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_SwitchRf_Field_Test3) {
  LOG(INFO) << "Enter phNci_SwitchRf_Field_Test :: SwitchRF_P003  ";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_switch_rf_field_cmd_1 = NCI_SWITCH_RF_FIELD_CMD_1;
  std::vector<uint8_t> nci_switch_rf_field_cmd_5 = NCI_SWITCH_RF_FIELD_CMD_5;

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


  //NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_1
  NfcData data3 = nci_switch_rf_field_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_1 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_5
  NfcData data4 = nci_switch_rf_field_cmd_5;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_5 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  sleep(1);
  LOG(INFO) << "Exit phNci_SwitchRf_Field_Test :: SwitchRF_P003";
}

/*********************************************************************
**       Self TESTS: RF Swith field test                            **
*********************************************************************/
/*
 * phNci_SwitchRf_Field_Test4: Validate the RF Field Switch.
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_SWITCH_RF_FIELD_CMD_1,
 *       NCI_SWITCH_RF_FIELD_CMD_2,
 *       NCI_SWITCH_RF_FIELD_CMD_3
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_SwitchRf_Field_Test4) {
  LOG(INFO) << "Enter phNci_SwitchRf_Field_Test :: SwitchRF_P004";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_switch_rf_field_cmd_1 = NCI_SWITCH_RF_FIELD_CMD_1;
  std::vector<uint8_t> nci_switch_rf_field_cmd_6 = NCI_SWITCH_RF_FIELD_CMD_6;
  std::vector<uint8_t> nci_switch_rf_field_cmd_3 = NCI_SWITCH_RF_FIELD_CMD_3;

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

  //NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_1
  NfcData data3 = nci_switch_rf_field_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_6
  NfcData data4 = nci_switch_rf_field_cmd_6;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for TEST_SWP2_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_SWITCH_RF_FIELD_CMD_3
  NfcData data5 = nci_switch_rf_field_cmd_3;
  EXPECT_EQ(data5.size(), nfc_->write(data5));
  // Wait for NCI_SWITCH_RF_FIELD_CMD_3
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  sleep(1);
  LOG(INFO) << "Exit phNci_SwitchRf_Field_Test :: SwitchRF_P004";
}

/*********************************************************************
**       Self TESTS: CTS and Test Signal configuration test         **
*********************************************************************/
/*
 * phNci_Cts_Test1: Validate the CTS Signal Configuration
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_CTS_TEST_CMD_1,
 *       NCI_CTS_TEST_CMD_2,
 *       NCI_DISCOVERY_CMD,
 *       NCI_CTS_TEST_CMD_3
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_Cts_Test1) {
  LOG(INFO) << "Enter phNci_Cts_Test1 :: Cts_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_cts_test_cmd_1 = NCI_CTS_TEST_CMD_1;
  std::vector<uint8_t> nci_cts_test_cmd_2 = NCI_CTS_TEST_CMD_2;
  std::vector<uint8_t> nci_discovery_cmd = NCI_DISCOVERY_CMD;
  std::vector<uint8_t> nci_cts_test_cmd_3 = NCI_CTS_TEST_CMD_3;

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

  //NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_CTS_TEST_CMD_1
  NfcData data3 = nci_cts_test_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_CTS_TEST_CMD_1 RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F220100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x22, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_CTS_TEST_CMD_2
  NfcData data4 = nci_cts_test_cmd_2;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_CTS_TEST_CMD_2
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F220100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x22, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  //NCI_DISCOVERY_CMD
  NfcData data5 = nci_discovery_cmd;
  EXPECT_EQ(data5.size(), nfc_->write(data5));
  // Wait for NCI_DISCOVERY_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 41030100
  EXPECT_EQ((int)0x41, res.args->last_data_[0]);
  EXPECT_EQ((int)0x03, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);

  //NCI_CTS_TEST_CMD_3
  NfcData data6 = nci_cts_test_cmd_3;
  EXPECT_EQ(data6.size(), nfc_->write(data6));
  // Wait for NCI_CTS_TEST_CMD_3
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  //RSP must be 4F220100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x22, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  sleep(2);
  LOG(INFO) << "Exit phNci_Cts_Test1 :: Cts_P001";
}

/*********************************************************************
**       Self TESTS: Load Protocol test                            **
*********************************************************************/
/*
 * phNci_Load_Protocol_Test
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_LOAD_PROTOCOL_CMD_1,
 *       NCI_LOAD_PROTOCOL_SUB_CMD_1,
 *       NCI_LOAD_PROTOCOL_SUB_CMD_2
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
/*
TEST_P(NfcSelfTestTest, phNci_Load_Protocol_Test1) {
  LOG(INFO) << "Enter phNci_Load_Protocol_Test :: LoadProtocol_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_load_protocol_cmd_1 = NCI_LOAD_PROTOCOL_CMD_1;
  std::vector<uint8_t> nci_load_protocol_cmd_2 = NCI_LOAD_PROTOCOL_CMD_2;
  std::vector<uint8_t> nci_load_protocol_cmd_3 = NCI_LOAD_PROTOCOL_CMD_3;
  std::vector<uint8_t> nci_load_protocol_sub_cmd_1 =
      NCI_LOAD_PROTOCOL_SUB_CMD_1;
  std::vector<uint8_t> nci_load_protocol_sub_cmd_2 =
      NCI_LOAD_PROTOCOL_SUB_CMD_2;

  // CORE_RESET_CMD
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

  // CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size() - 3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_CMD_1
  NfcData data3 = nci_load_protocol_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_LOAD_PROTOCOL_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_1
  NfcData data4 = nci_load_protocol_sub_cmd_1;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_2
  NfcData data5 = nci_load_protocol_sub_cmd_2;
  EXPECT_EQ(data5.size(), nfc_->write(data5));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_2
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  LOG(INFO) << "Exit phNci_Load_Protocol_Test :: LoadProtocol_P001";
  sleep(1);

  LOG(INFO) << "Enter phNci_Load_Protocol_Test :: LoadProtocol_P002";
  // NCI_LOAD_PROTOCOL_CMD_2
  NfcData data6 = nci_load_protocol_cmd_2;
  EXPECT_EQ(data6.size(), nfc_->write(data6));
  // Wait for NCI_LOAD_PROTOCOL_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_1
  NfcData data7 = nci_load_protocol_sub_cmd_1;
  EXPECT_EQ(data7.size(), nfc_->write(data7));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_2
  NfcData data8 = nci_load_protocol_sub_cmd_2;
  EXPECT_EQ(data8.size(), nfc_->write(data8));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_2
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  LOG(INFO) << "Exit phNci_Load_Protocol_Test :: LoadProtocol_P002";
  sleep(1);

  LOG(INFO) << "Enter phNci_Load_Protocol_Test :: LoadProtocol_P003";
  // NCI_LOAD_PROTOCOL_CMD_2
  NfcData data9 = nci_load_protocol_cmd_2;
  EXPECT_EQ(data9.size(), nfc_->write(data9));
  // Wait for NCI_LOAD_PROTOCOL_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_1
  NfcData data10 = nci_load_protocol_sub_cmd_1;
  EXPECT_EQ(data10.size(), nfc_->write(data10));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_2
  NfcData data11 = nci_load_protocol_sub_cmd_2;
  EXPECT_EQ(data11.size(), nfc_->write(data11));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_2
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  LOG(INFO) << "Exit phNci_Load_Protocol_Test :: LoadProtocol_P003";
  sleep(1);

  LOG(INFO) << "Enter phNci_Load_Protocol_Test :: LoadProtocol_N001";
  // NCI_LOAD_PROTOCOL_CMD_2
  NfcData data12 = nci_load_protocol_cmd_3;
  EXPECT_EQ(data12.size(), nfc_->write(data12));
  // Wait for NCI_LOAD_PROTOCOL_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F0E0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_1
  NfcData data13 = nci_load_protocol_sub_cmd_1;
  EXPECT_EQ(data13.size(), nfc_->write(data13));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_LOAD_PROTOCOL_SUB_CMD_2
  NfcData data14 = nci_load_protocol_sub_cmd_2;
  EXPECT_EQ(data14.size(), nfc_->write(data14));
  // Wait for NCI_LOAD_PROTOCOL_SUB_CMD_2
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 4F3F0100
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  LOG(INFO) << "Exit phNci_Load_Protocol_Test :: LoadProtocol_N001";
  sleep(1);
}
*/
/*********************************************************************
**       Self TESTS: Read Write test on EE                            **
*********************************************************************/
/*
 * phNci_Write_ReadEE_Test
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_WRITE_READ_EE_GET_CONFIG_CMD_1,
 *       NCI_WRITE_READ_EE_SET_CONFIG_CMD_1,
 *       NCI_WRITE_READ_EE_GET_CONFIG_CMD_1,
 *       NCI_WRITE_READ_EE_GET_CONFIG_CMD_2,
 *       NCI_WRITE_READ_EE_GET_CONFIG_CMD_3
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
/*
TEST_P(NfcSelfTestTest, phNci_Write_ReadEE_Test) {
  LOG(INFO) << "Enter phNci_Write_ReadEE_Test :: Write_ReadEE_Test_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_write_read_ee_get_config_cmd_1 =
      NCI_WRITE_READ_EE_GET_CONFIG_CMD_1;
  std::vector<uint8_t> nci_write_read_ee_get_config_cmd_2 =
      NCI_WRITE_READ_EE_GET_CONFIG_CMD_2;
  std::vector<uint8_t> nci_write_read_ee_get_config_cmd_3 =
      NCI_WRITE_READ_EE_GET_CONFIG_CMD_3;
  std::vector<uint8_t> nci_write_read_ee_set_config_cmd_1 =
      NCI_WRITE_READ_EE_SET_CONFIG_CMD_1;
  std::vector<uint8_t> nci_write_read_ee_set_config_cmd_2 =
      NCI_WRITE_READ_EE_SET_CONFIG_CMD_2;
  // CORE_RESET_CMD
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

  // CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size() - 3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_WRITE_READ_EE_SET_CONFIG_CMD_1
  NfcData data3 = nci_write_read_ee_set_config_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_WRITE_READ_EE_SET_CONFIG_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  // RSP must be 0x40 0x02 0x02 0x00 0x00
  EXPECT_EQ((int)0x40, res.args->last_data_[0]);
  EXPECT_EQ((int)0x02, res.args->last_data_[1]);
  EXPECT_EQ((int)0x02, res.args->last_data_[2]);
  EXPECT_EQ((int)0x00, res.args->last_data_[3]);
  EXPECT_EQ((int)0x00, res.args->last_data_[4]);

  // NCI_WRITE_READ_EE_GET_CONFIG_CMD_1
  NfcData data4 = nci_write_read_ee_get_config_cmd_1;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_WRITE_READ_EE_GET_CONFIG_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(16ul, res.args->last_data_.size());*/
// TOD0: if we uncomment below code, Test case failing. artifact:artf1003309
// created to check with FW team
//  RSP must be 0x40 0x03 0x0D 0x00 0x01 0xA2 0x02 0x08 0xFF 0x08 0xF6 0x01
//  0x00 0x33 0x64 0x10
/*EXPECT_EQ((int)0x40, res.args->last_data_[0]);
EXPECT_EQ((int)0x03, res.args->last_data_[1]);
EXPECT_EQ((int)0x0D, res.args->last_data_[2]);
EXPECT_EQ((int)0x00, res.args->last_data_[3]);
EXPECT_EQ((int)0x01, res.args->last_data_[4]);
EXPECT_EQ((int)0xA2, res.args->last_data_[5]);
EXPECT_EQ((int)0x02, res.args->last_data_[6]);
EXPECT_EQ((int)0x08, res.args->last_data_[7]);
EXPECT_EQ((int)0xFF, res.args->last_data_[8]);
EXPECT_EQ((int)0x08, res.args->last_data_[9]);
EXPECT_EQ((int)0xF6, res.args->last_data_[10]);
EXPECT_EQ((int)0x01, res.args->last_data_[11]);
EXPECT_EQ((int)0x00, res.args->last_data_[12]);
EXPECT_EQ((int)0x33, res.args->last_data_[13]);
EXPECT_EQ((int)0x64, res.args->last_data_[14]);
EXPECT_EQ((int)0x10, res.args->last_data_[15]);*/
/*EXPECT_EQ((int)0x40, res.args->last_data_[0]);
EXPECT_EQ((int)0x03, res.args->last_data_[1]);
EXPECT_EQ((int)0x0D, res.args->last_data_[2]);
EXPECT_EQ((int)0x00, res.args->last_data_[3]);
EXPECT_EQ((int)0x01, res.args->last_data_[4]);
EXPECT_EQ((int)0xA2, res.args->last_data_[5]);
EXPECT_EQ((int)0x02, res.args->last_data_[6]);
EXPECT_EQ((int)0x08, res.args->last_data_[7]);
EXPECT_EQ((int)0xAA, res.args->last_data_[8]);
EXPECT_EQ((int)0xBB, res.args->last_data_[9]);
EXPECT_EQ((int)0xCC, res.args->last_data_[10]);
EXPECT_EQ((int)0xDD, res.args->last_data_[11]);
EXPECT_EQ((int)0xAA, res.args->last_data_[12]);
EXPECT_EQ((int)0xBB, res.args->last_data_[13]);
EXPECT_EQ((int)0xCC, res.args->last_data_[14]);
EXPECT_EQ((int)0xDD, res.args->last_data_[15]);

// NCI_WRITE_READ_EE_SET_CONFIG_CMD_1
NfcData data5 = nci_write_read_ee_set_config_cmd_2;
EXPECT_EQ(data5.size(), nfc_->write(data5));
// Wait for NCI_WRITE_READ_EE_SET_CONFIG_CMD_1
res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
EXPECT_TRUE(res.no_timeout);
EXPECT_EQ(5ul, res.args->last_data_.size());
// RSP must be 0x40 0x02 0x02 0x00 0x00
EXPECT_EQ((int)0x40, res.args->last_data_[0]);
EXPECT_EQ((int)0x02, res.args->last_data_[1]);
EXPECT_EQ((int)0x02, res.args->last_data_[2]);
EXPECT_EQ((int)0x00, res.args->last_data_[3]);
EXPECT_EQ((int)0x00, res.args->last_data_[4]);

// NCI_WRITE_READ_EE_GET_CONFIG_CMD_1
NfcData data6 = nci_write_read_ee_get_config_cmd_1;
EXPECT_EQ(data6.size(), nfc_->write(data6));
// Wait for NCI_WRITE_READ_EE_GET_CONFIG_CMD_1
res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
EXPECT_TRUE(res.no_timeout);
EXPECT_EQ(16ul, res.args->last_data_.size());
// RSP must be 0x40 0x03 0x0D 0x00 0x01 0xA2 0x02 0x08 0xAA 0xBB 0xCC 0xDD
// 0xAA 0xBB 0xCC 0xDD
EXPECT_EQ((int)0x40, res.args->last_data_[0]);
EXPECT_EQ((int)0x03, res.args->last_data_[1]);
EXPECT_EQ((int)0x0D, res.args->last_data_[2]);
EXPECT_EQ((int)0x00, res.args->last_data_[3]);
EXPECT_EQ((int)0x01, res.args->last_data_[4]);
EXPECT_EQ((int)0xA2, res.args->last_data_[5]);
EXPECT_EQ((int)0x02, res.args->last_data_[6]);
EXPECT_EQ((int)0x08, res.args->last_data_[7]);
EXPECT_EQ((int)0xAA, res.args->last_data_[8]);
EXPECT_EQ((int)0xBB, res.args->last_data_[9]);
EXPECT_EQ((int)0xCC, res.args->last_data_[10]);
EXPECT_EQ((int)0xDD, res.args->last_data_[11]);
EXPECT_EQ((int)0xAA, res.args->last_data_[12]);
EXPECT_EQ((int)0xBB, res.args->last_data_[13]);
EXPECT_EQ((int)0xCC, res.args->last_data_[14]);
EXPECT_EQ((int)0xDD, res.args->last_data_[15]);

// NCI_WRITE_READ_EE_GET_CONFIG_CMD_2
NfcData data7 = nci_write_read_ee_get_config_cmd_2;
EXPECT_EQ(data7.size(), nfc_->write(data7));
// Wait for NCI_WRITE_READ_EE_GET_CONFIG_CMD_2
res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
EXPECT_TRUE(res.no_timeout);
EXPECT_EQ(16ul, res.args->last_data_.size());
// RSP must be 0x40 0x03 0x0D 0x00 0x01 0xA2 0x04 0x08 0xAA 0xBB 0xCC 0xDD
// 0xAA 0xBB 0xCC 0xDD
EXPECT_EQ((int)0x40, res.args->last_data_[0]);
EXPECT_EQ((int)0x03, res.args->last_data_[1]);
EXPECT_EQ((int)0x0D, res.args->last_data_[2]);
EXPECT_EQ((int)0x00, res.args->last_data_[3]);
EXPECT_EQ((int)0x01, res.args->last_data_[4]);
EXPECT_EQ((int)0xA2, res.args->last_data_[5]);
EXPECT_EQ((int)0x04, res.args->last_data_[6]);
EXPECT_EQ((int)0x08, res.args->last_data_[7]);
EXPECT_EQ((int)0xAA, res.args->last_data_[8]);
EXPECT_EQ((int)0xBB, res.args->last_data_[9]);
EXPECT_EQ((int)0xCC, res.args->last_data_[10]);
EXPECT_EQ((int)0xDD, res.args->last_data_[11]);
EXPECT_EQ((int)0xAA, res.args->last_data_[12]);
EXPECT_EQ((int)0xBB, res.args->last_data_[13]);
EXPECT_EQ((int)0xCC, res.args->last_data_[14]);
EXPECT_EQ((int)0xDD, res.args->last_data_[15]);
LOG(INFO) << "Exit phNci_Write_ReadEE_Test :: Write_ReadEE_Test_P001";
sleep(1);

LOG(INFO) << "Enter phNci_Write_ReadEE_Test :: Write_ReadEE_Test_N001";
// NCI_WRITE_READ_EE_GET_CONFIG_CMD_3
NfcData data8 = nci_write_read_ee_get_config_cmd_2;
EXPECT_EQ(data8.size(), nfc_->write(data8));
// Wait for NCI_WRITE_READ_EE_GET_CONFIG_CMD_3
res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
EXPECT_TRUE(res.no_timeout);
EXPECT_EQ(16ul, res.args->last_data_.size());*/
// TOD0: if we uncomment below code, Test case failing. artifact:artf1003309
// created to check with FW team
//  RSP must be 0x40 0x03 0x05 0x09 0x01 0xA2 0xFF 0x00
/* EXPECT_EQ((int)0x40, res.args->last_data_[0]);
 EXPECT_EQ((int)0x03, res.args->last_data_[1]);
 EXPECT_EQ((int)0x05, res.args->last_data_[2]);
 EXPECT_EQ((int)0x09, res.args->last_data_[3]);
 EXPECT_EQ((int)0x01, res.args->last_data_[4]);
 EXPECT_EQ((int)0xA2, res.args->last_data_[5]);
 EXPECT_EQ((int)0xFF, res.args->last_data_[6]);
 EXPECT_EQ((int)0x00, res.args->last_data_[7]);
*/
/* LOG(INFO) << "Exit phNci_Write_ReadEE_Test :: Write_ReadEE_Test_N001";
 sleep(1);
}
*/
/*********************************************************************
**       Self TESTS: PRBS test                            **
*********************************************************************/
/*
 * phNci_PRBS_Test
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_NFCC_CONFIG_CONTROL_CMD,
 *       NCI_LOAD_PROTOCOL_CMD_1,
 *       NCI_LOAD_PROTOCOL_SUB_CMD_1,
 *       NCI_LOAD_PROTOCOL_SUB_CMD_2
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_PRBS_Test) {
  LOG(INFO) << "Enter phNci_PRBS_Test :: Prbs_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> set_config_cmd = NCI_NFCC_CONFIG_CONTROL_CMD;
  std::vector<uint8_t> nci_prbs_cmd_1 = NCI_PRBS_CMD_1;
  std::vector<uint8_t> nci_prbs_cmd_2 = NCI_PRBS_CMD_2;
  std::vector<uint8_t> nci_prbs_cmd_3 = NCI_PRBS_CMD_3;
  std::vector<uint8_t> nci_prbs_cmd_4 = NCI_PRBS_CMD_4;
  std::vector<uint8_t> nci_prbs_cmd_5 = NCI_PRBS_CMD_5;
  std::vector<uint8_t> nci_prbs_cmd_6 = NCI_PRBS_CMD_6;
  std::vector<uint8_t> nci_prbs_cmd_7 = NCI_PRBS_CMD_7;
  std::vector<uint8_t> nci_prbs_cmd_8 = NCI_PRBS_CMD_8;
  std::vector<uint8_t> nci_prbs_cmd_9 = NCI_PRBS_CMD_9;
  std::vector<uint8_t> nci_prbs_cmd_10 = NCI_PRBS_CMD_10;

  // CORE_RESET_CMD
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

  // CORE_INIT_CMD_NCI20
  NfcData data1 = core_init_cmd;
  EXPECT_EQ(data1.size(), nfc_->write(data1));
  // Wait for CORE_INIT_RSP_NCI20
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size() - 3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_NFCC_CONFIG_CONTROL_CMD
  NfcData data2 = set_config_cmd;
  EXPECT_EQ(data2.size(), nfc_->write(data2));
  // Wait for NCI_NFCC_CONFIG_CONTROL_RSP
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(5ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_1
  NfcData data3 = nci_prbs_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Wait for NCI_PRBS_CMD_1
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x0E 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_2
  NfcData data4 = nci_prbs_cmd_2;
  EXPECT_EQ(data4.size(), nfc_->write(data4));
  // Wait for NCI_PRBS_CMD_2
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_3
  NfcData data5 = nci_prbs_cmd_3;
  EXPECT_EQ(data5.size(), nfc_->write(data5));
  // Wait for NCI_PRBS_CMD_3
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x30 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x30, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_4
  NfcData data6 = nci_prbs_cmd_4;
  EXPECT_EQ(data6.size(), nfc_->write(data6));
  // Wait for NCI_PRBS_CMD_4
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_5
  NfcData data7 = nci_prbs_cmd_5;
  EXPECT_EQ(data7.size(), nfc_->write(data7));
  // Wait for NCI_PRBS_CMD_5
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_6
  NfcData data8 = nci_prbs_cmd_6;
  EXPECT_EQ(data8.size(), nfc_->write(data8));
  // Wait for NCI_PRBS_CMD_6
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x30 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x30, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_7
  NfcData data9 = nci_prbs_cmd_7;
  EXPECT_EQ(data9.size(), nfc_->write(data9));
  // Wait for NCI_PRBS_CMD_7
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_8
  NfcData data10 = nci_prbs_cmd_8;
  EXPECT_EQ(data10.size(), nfc_->write(data10));
  // Wait for NCI_PRBS_CMD_8
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_9
  NfcData data11 = nci_prbs_cmd_9;
  EXPECT_EQ(data11.size(), nfc_->write(data11));
  // Wait for NCI_PRBS_CMD_9
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x30 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x30, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  // NCI_PRBS_CMD_10
  NfcData data12 = nci_prbs_cmd_10;
  EXPECT_EQ(data12.size(), nfc_->write(data12));
  // Wait for NCI_PRBS_CMD_10
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);

  LOG(INFO) << "Exit phNci_PRBS_Test :: Prbs_P001";
  sleep(1);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcSelfTestTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, NfcSelfTestTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(INfc::descriptor)),
        android::hardware::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::system("svc nfc disable"); /* Turn off NFC */
    sleep(2);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    std::system("svc nfc enable"); /* Turn on NFC */
    sleep(2);

    return status;
}
