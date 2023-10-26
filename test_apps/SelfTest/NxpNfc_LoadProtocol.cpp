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
#include "common.h"


/*********************************************************************
**       Self TESTS: LoadProtocol  Test             **
*********************************************************************/
/*
 * phNci_LoadProtocol_Test
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_LOAD_PROTOCOL_CMD_1,
 *       NCI_LOAD_PROTOCOL_CMD_2,
 *       NCI_LOAD_PROTOCOL_CMD_3,

 * Waits for respective Response and Notifications. Validate the same.
 *
 */
void validateRfOnOffRsp(sp<NfcClientCallback> nfc_cb_) {
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  // RSP must be 0x4F 0x3F 0x01 0x00
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x3F, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
}
void testRfOnOff(sp<INfc> nfc_,sp<NfcClientCallback> nfc_cb_) {
  std::vector<uint8_t> nci_rf_off_cmd_1 = NCI_LOAD_PROTOCOL_SUB_CMD_1;
  std::vector<uint8_t> nci_rf_on_cmd_1  = NCI_LOAD_PROTOCOL_SUB_CMD_2;
  NfcData rf_off = nci_rf_off_cmd_1;
  EXPECT_EQ(rf_off.size(), nfc_->write(rf_off));
  validateRfOnOffRsp(nfc_cb_);
  NfcData rf_on = nci_rf_on_cmd_1;
  EXPECT_EQ(rf_on.size(), nfc_->write(rf_on));
  validateRfOnOffRsp(nfc_cb_);
}
void validateNciRsp(sp<NfcClientCallback> nfc_cb_) {
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  // RSP must be 0x4F 0x0E 0x01 0x00
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)0x4F, res.args->last_data_[0]);
  EXPECT_EQ((int)0x0E, res.args->last_data_[1]);
  EXPECT_EQ((int)0x01, res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
}
TEST_P(NfcSelfTestTest, phNci_LoadProtocol_Test) {
  LOG(INFO) << "Enter phNci_LoadProtocol_Test :: LoadProtocol_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd  =  CORE_INIT_CMD_NCI20;
  std::vector<uint8_t> nci_lp_cmd_1 = NCI_LOAD_PROTOCOL_CMD_1;
  std::vector<uint8_t> nci_lp_cmd_2 = NCI_LOAD_PROTOCOL_CMD_2;
  std::vector<uint8_t> nci_lp_cmd_3 = NCI_LOAD_PROTOCOL_CMD_3;
  std::vector<uint8_t> nci_lp_cmd_4 = NCI_LOAD_PROTOCOL_CMD_4;
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
  // Waiting for CORE_INIT_RSP_NCI20 Response
  res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_LE(4ul, res.args->last_data_.size());
  EXPECT_EQ((int)(res.args->last_data_.size() - 3), res.args->last_data_[2]);
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  // NCI_LOAD_PROTOCOL_CMD_1
  //Do Load Protocol with Type A and do RF ON
  NfcData data3 = nci_lp_cmd_1;
  EXPECT_EQ(data3.size(), nfc_->write(data3));
  // Waiting Response for NCI_LoadProtocol_CMD_1
  validateNciRsp(nfc_cb_);
  testRfOnOff(nfc_,nfc_cb_);
  // NCI_LOAD_PROTOCOL_CMD_2
  //Do Load Protocol with Type B and do RF ON
  NfcData data6 = nci_lp_cmd_2;
  EXPECT_EQ(data6.size(), nfc_->write(data6));
  // Wait for NCI_LoadProtocol_CMD_2
  validateNciRsp(nfc_cb_);
  testRfOnOff(nfc_,nfc_cb_);
  //Waiting Response for NCI_LoadProtocol_CMD_3
  //Do Load Protocol with Type F and do RF ON
  NfcData data7 = nci_lp_cmd_3;
  EXPECT_EQ(data7.size(), nfc_->write(data7));
  // Waiting Response for NCI_LoadProtocol_CMD_4
  validateNciRsp(nfc_cb_);
  testRfOnOff(nfc_,nfc_cb_);
  // Do Load Protocol with ISO15693 and do RF ON
  NfcData data8 = nci_lp_cmd_3;
  EXPECT_EQ(data8.size(), nfc_->write(data8));
  validateNciRsp(nfc_cb_);
  testRfOnOff(nfc_,nfc_cb_);
  LOG(INFO) << "Exit phNci_LoadProtocol_Test :: LoadProtocol_P001";
  sleep(1);
}
