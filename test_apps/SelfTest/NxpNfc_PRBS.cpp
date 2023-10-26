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
 *  Copyright 2022-2023 NXP
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
**       Self TESTS: PRBS test                            **
*********************************************************************/
/*
 * phNci_PRBS_Test
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
 *       NCI_PRBS_CMD_1,
 *       NCI_PRBS_CMD_2,
 *       NCI_PRBS_CMD_3,
 *       NCI_PRBS_CMD_4,
 *       NCI_PRBS_CMD_5,
 *       NCI_PRBS_CMD_6,
 *       NCI_PRBS_CMD_7,
 *       NCI_PRBS_CMD_8,
 *       NCI_PRBS_CMD_9,
 *       NCI_PRBS_CMD_10,
 * Waits for respective Response and Notifications. Validate the same.
 *
 */
TEST_P(NfcSelfTestTest, phNci_PRBS_Test) {
  LOG(INFO) << "Enter phNci_PRBS_Test :: Prbs_P001";
  std::vector<uint8_t> core_reset_cmd = CORE_RESET_CMD_CONFIG_RESET;
  std::vector<uint8_t> core_init_cmd = CORE_INIT_CMD_NCI20;
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
