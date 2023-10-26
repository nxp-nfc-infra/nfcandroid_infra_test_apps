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
#include "common.h"

/*********************************************************************
**       Self TESTS: CTS and Test Signal configuration test         **
*********************************************************************/
/*
 * phNci_Cts_Test1: Validate the CTS Signal Configuration
 * Sends CORE_RESET_CMD_CONFIG_RESET,
 *       CORE_INIT_CMD_NCI20,
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
