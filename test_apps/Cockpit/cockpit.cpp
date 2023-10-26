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

#include "common.h"

struct Parameter {
  std::string name;
  std::string offset;
  std::string value;
};

struct Protocol {
  std::string name;
  std::string index;
  std::string offset;
  std::vector<Parameter> registers;
};

struct Region {
  std::string name;
  std::string access;
  std::string type;
  std::string offset;
  std::vector<Parameter> parameters;
  std::vector<Protocol> protocols;
};

struct EEPROM {
  std::vector<Region> regions;
};

uint8_t getDigit(char num1, char num2) {
  uint8_t upperNibble = 0;
  uint8_t lowerNibble = 0;

  /* Convert ASCII to HEX Numeric value */
  if (num1 >= '0' && num1 <= '9') {
    upperNibble = num1 - '0';
  } else if (num1 >= 'A' && num1 <= 'F') {
    upperNibble = 10 + (num1 - 'A');
  } else if (num1 >= 'a' && num1 <= 'f') {
    upperNibble = 10 + (num1 - 'a');
  }

  if (num2 >= '0' && num2 <= '9') {
    lowerNibble = num2 - '0';
  } else if (num2 >= 'A' && num2 <= 'F') {
    lowerNibble = 10 + (num2 - 'A');
  } else if (num2 >= 'a' && num2 <= 'f') {
    lowerNibble = 10 + (num2 - 'a');
  }

  return ((upperNibble << 4) | lowerNibble);
}

TEST_P(NfcSelfTestTest, NfcCockPitTest) {

  pugi::xml_document doc;
  if (!doc.load_file(DUMMP_XML_FILE_PATH)) {
    ALOGE("Error loading %s file", DUMMP_XML_FILE_PATH);
    return;
  } else {
    ALOGE("Opened %s", DUMMP_XML_FILE_PATH);
  }

  EEPROM eeprom;
  std::vector<std::string> eeprom_value_stng;
  std::vector<uint8_t> eeprom_value_hex;
  std::vector<std::string> protocol_value_stng;
  std::vector<uint8_t> protocol_value_hex;
  NfcData eepromNciPkt;
  NfcData protocolNciPkt;
  Region region;
  Parameter param;
  Protocol protocol;
  char *ptr = nullptr;
  std::string output;
  int cIndex = 0;
  int pIndex = -1;

  std::vector<uint8_t> eeprom_data = {0x20, 0x02, 0x0C, 0x01, 0xA2,
                                      0x00, 0x08, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00};

  std::vector<uint8_t> eeprom_get_data = {0x20, 0x03, 0x03, 0x01, 0xA2, 0x00};

  std::vector<uint8_t> protocol_data = {0x20, 0x02, 0x0A, 0x01, 0xA0,
                                        0x0D, 0x06, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00};

  std::vector<uint8_t> protocol_get_data = {0x20, 0x03, 0x04, 0x01,
                                            0xA0, 0x0D, 0x00};

  auto eeprom_node = doc.child("EEPROM");
  for (auto region_node : eeprom_node.children("Region")) {

    region.name = region_node.attribute("RegionName").value();
    region.access = region_node.attribute("RegionAccess").value();
    region.type = region_node.attribute("RegionType").value();
    region.offset = region_node.attribute("RegionOffset").value();

    eeprom_value_stng.clear();
    for (auto param_node : region_node.children("Parameter")) {

      param.name = param_node.attribute("Name").value();
      param.offset = param_node.attribute("Offset").value();
      param.value = param_node.attribute("Value").value();

      region.parameters.push_back(param);
      /* Remove '0x' prefix from string */
      if (param.value.substr(0, 2) == "0x") {
        param.value.erase(0, 2);
      }

      /* Convert the little endian into big endian and push into string buffer
       */
      std::reverse(param.value.begin(), param.value.end());
      output.clear();
      for (int i = 0; i < param.value.size(); i += 2) {
        std::string byte = param.value.substr(i, 2);
        std::reverse(byte.begin(), byte.end());
        output += byte;
      }
      /* Push the string into a string buffer */
      eeprom_value_stng.push_back(output);
    }

    /* Convert the string into HEX Numeric value */
    for (auto str : eeprom_value_stng) {
      ptr = (char *)str.c_str();
      uint8_t byte = 0;

      for (int i = 0; i < (strlen(str.c_str()) - 1); i += 2) {
        byte = getDigit(ptr[i], ptr[i + 1]);
        eeprom_value_hex.push_back(byte);
      }
    }

    for (auto protocol_node : region_node.children("Protocol")) {

      protocol.name = protocol_node.attribute("ProtocolName").value();
      protocol.index = protocol_node.attribute("ProtocolIndex").value();
      protocol.offset = protocol_node.attribute("ProtocolOffset").value();

      /* Remove '0x' prefix from string */
      if (protocol.index.substr(0, 2) == "0x") {
        protocol.index.erase(0, 2);
      }

      protocol_value_stng.clear();
      for (auto reg_node : protocol_node.children("Register")) {
        Parameter reg;
        reg.name = reg_node.attribute("RegisterName").value();
        reg.offset = reg_node.attribute("RegisterLogicalAddress").value();
        reg.value = reg_node.attribute("RegisterValue").value();
        protocol.registers.push_back(reg);

        protocol_value_stng.push_back(protocol.index);

        /* Remove '0x' prefix from string */
        if (reg.offset.substr(0, 2) == "0x") {
          reg.offset.erase(0, 2);
        }

        protocol_value_stng.push_back(reg.offset);

        /* Remove '0x' prefix from string */
        if (reg.value.substr(0, 2) == "0x") {
          reg.value.erase(0, 2);
        }
        /* Convert the little endian into big endian and push into string buffer
         */
        std::reverse(reg.value.begin(), reg.value.end());
        output.clear();
        for (int i = 0; i < reg.value.size(); i += 2) {
          std::string byte = reg.value.substr(i, 2);
          std::reverse(byte.begin(), byte.end());
          output += byte;
        }
        protocol_value_stng.push_back(output);
      }

      /* Convert the string into HEX and push into buffer */
      for (auto pstr : protocol_value_stng) {
        ptr = (char *)pstr.c_str();
        uint8_t pbyte = 0;

        for (int i = 0; i < (strlen(pstr.c_str()) - 1); i += 2) {
          pbyte = getDigit(ptr[i], ptr[i + 1]);
          protocol_value_hex.push_back(pbyte);
        }
      }
      region.protocols.push_back(protocol);
    }
    eeprom.regions.push_back(region);
  }

  /* Prepare NCI packet send to NFCC for EEPROM Data */
  for (int k = 0; k < eeprom_value_hex.size(); k += 8) {
    uint8_t offset = k / 8;
    std::copy(&offset, &offset + 1, eeprom_data.begin() + 5);
    std::copy(eeprom_value_hex.begin() + k, eeprom_value_hex.begin() + (k + 8),
              eeprom_data.begin() + 7);

    eeprom_get_data[5] = offset;
    EXPECT_EQ(eeprom_get_data.size(), nfc_->write(eeprom_get_data));
    // Wait for CORE_GET_CONFIG_RSP
    auto get_rsp = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(get_rsp.no_timeout);
    EXPECT_EQ((int)0x40, get_rsp.args->last_data_[0]);
    EXPECT_EQ((int)0x03, get_rsp.args->last_data_[1]);
    EXPECT_EQ((int)0x0D, get_rsp.args->last_data_[2]);
    EXPECT_EQ((int)NfcStatus::OK, get_rsp.args->last_data_[3]);

    eepromNciPkt = eeprom_data;
    EXPECT_EQ(eepromNciPkt.size(), nfc_->write(eepromNciPkt));
    // Wait for CORE_SET_CONFIG_RSP
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(res.no_timeout);

    EXPECT_EQ(5ul, res.args->last_data_.size());
    // RSP must be 4002020000
    EXPECT_EQ((int)0x40, res.args->last_data_[0]);
    EXPECT_EQ((int)0x02, res.args->last_data_[1]);
    EXPECT_EQ((int)0x02, res.args->last_data_[2]);
    EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  }

  /* Prepare NCI packet send to NFCC for Protocol Data */
  for (int z = 0; z < protocol_value_hex.size(); z += 6) {
    std::copy(protocol_value_hex.begin() + z,
              protocol_value_hex.begin() + (z + 6), protocol_data.begin() + 7);

    protocolNciPkt = protocol_data;
    cIndex = protocolNciPkt[7]; // Get the ProtocolIndex

    /* Send Get config command only if ProtocolIndex is different then previous
     */
    if (pIndex != cIndex) {
      pIndex = cIndex;
      protocol_get_data[6] = cIndex;
      EXPECT_EQ(protocol_get_data.size(), nfc_->write(protocol_get_data));
      // Wait for CORE_GET_CONFIG_RSP
      auto get_res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
      EXPECT_TRUE(get_res.no_timeout);
      EXPECT_EQ((int)0x40, get_res.args->last_data_[0]);
      EXPECT_EQ((int)0x03, get_res.args->last_data_[1]);
      EXPECT_EQ((int)NfcStatus::OK, get_res.args->last_data_[3]);
    }

    EXPECT_EQ(protocolNciPkt.size(), nfc_->write(protocolNciPkt));
    // Wait for CORE_SET_CONFIG_RSP
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(res.no_timeout);

    EXPECT_EQ(5ul, res.args->last_data_.size());
    // RSP must be 4002020000
    EXPECT_EQ((int)0x40, res.args->last_data_[0]);
    EXPECT_EQ((int)0x02, res.args->last_data_[1]);
    EXPECT_EQ((int)0x02, res.args->last_data_[2]);
    EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  }
}
