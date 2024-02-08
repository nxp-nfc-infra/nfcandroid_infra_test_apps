/*
 * Copyright (C) 2016 The Android Open Source Project
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
#define LOG_TAG "SmcuSwitchV2_0"
#include <android-base/logging.h>
#include <iostream>
#include <cutils/properties.h>

#include <android/hardware/nfc/1.2/INfc.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <android/hardware/nfc/1.0/types.h>
#include <hardware/nfc.h>

#include <string>//memcpy
#include <vendor/nxp/nxpnfc/2.0/INxpNfc.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::vendor::nxp::nxpnfc::V2_0::INxpNfc;
using ::android::hardware::nfc::V1_2::INfc;
using ::android::hardware::nfc::V1_0::INfcClientCallback;
using ::android::hardware::nfc::V1_0::NfcEvent;
using ::android::hardware::nfc::V1_0::NfcStatus;
using ::android::hardware::nfc::V1_0::NfcData;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;
using namespace std;
/* NCI Commands */
class NfcClientCallback;
constexpr char kCallbackNameSendEvent[] = "sendEvent";
constexpr char kCallbackNameSendData[] = "sendData";

bool isFwDnldExit = false;
static sp<INxpNfc> gp_nxpnfc_;
static sp<INfc> gp_nfc_;
static sp<NfcClientCallback> gp_nfc_cb_;

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
    Return<void> sendEvent(NfcEvent event, NfcStatus event_status) override {
        NfcClientCallbackArgs args;
        args.last_event_ = event;
        args.last_status_ = event_status;
        NotifyFromCallback(kCallbackNameSendEvent, args);
        return Void();
    };

    /* sendData callback function. Records the data and notifies the TEST*/
    Return<void> sendData(const NfcData& data) override {
        NfcClientCallbackArgs args;
        args.last_data_ = data;
        NotifyFromCallback(kCallbackNameSendData, args);
        return Void();
    };
};

// Test environment for Nfc HIDL HAL.
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
};

// The main test class for DUAL CPU Mode Switch HAL.
class NxpNfc_DualCpuTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    virtual void SetUp() override {
      nfc_ = ::testing::VtsHalHidlTargetTestBase::getService<INfc>(
          NfcHidlEnvironment::Instance()->getServiceName<INfc>());
      ASSERT_NE(nfc_, nullptr);

      nxpnfc_ = ::testing::VtsHalHidlTargetTestBase::getService<INxpNfc>(
              NfcHidlEnvironment::Instance()->getServiceName<INxpNfc>());
      ASSERT_NE(nxpnfc_, nullptr);

      nfc_cb_ = new NfcClientCallback();
      ASSERT_NE(nfc_cb_, nullptr);

      EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
      // Wait for OPEN_CPLT event
      auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
      EXPECT_TRUE(res.no_timeout);

      /* Clear the flag */
      isFwDnldExit = false;
      gp_nxpnfc_ = nxpnfc_;
      gp_nfc_cb_ = nfc_cb_;
      gp_nfc_ = nfc_;
    }

    virtual void TearDown() override {
      if(isFwDnldExit == false) {
        EXPECT_EQ(NfcStatus::OK, nfc_->close());
        // Wait for CLOSE_CPLT event
        auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
        EXPECT_TRUE(res.no_timeout);
      }
    }

    /**
     * @brief  Listens for user interrupt and exits the application
     *
     **/
    static void signal_callback_handler(int signum) {
      auto res = 0;
      cout << "SMCU test App abort requested, signum:" << signum << endl;
      res = gp_nxpnfc_->setEseUpdateState(
          (vendor::nxp::nxpnfc::V2_0::NxpNfcHalEseState)0x02);
      EXPECT_TRUE(res);
      gp_nfc_->close();
      gp_nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
      std::system("svc nfc enable"); /* Turn on NFC */
      sleep(2);
      exit(signum);
    }

    /* NCI version the device supports
     * 0x11 for NCI 1.1, 0x20 for NCI 2.0 and so forth */
    uint8_t nci_version;
    sp<INfc> nfc_;
    sp<INxpNfc> nxpnfc_;
    sp<NfcClientCallback> nfc_cb_;
};

/*
 * NxpNfc_DualCpuTest: Test DUAL CPU Mode Switch.
 */

TEST_F(NxpNfc_DualCpuTest, NxpNfc_DualCpu_modeSwitch) {
  LOG(INFO) << "Enter NxpNfc_DualCpu_modeSwitch";
  int userInput = 0;
  auto res = 0;
  int prevState = 2;
  signal(SIGINT, signal_callback_handler);  /* Handle Ctrl+C*/
  signal(SIGTSTP, signal_callback_handler); /* Handle Ctrl+Z*/

  while(1)
  {
    cout << "Select the option\n";
    cout << " 1. Switch to EMVCo Mode (Host: SMCU)\n";
    cout << " 2. Switch to NFC Mode  (Host: Android)\n";
    cout << " 3. Switch to Secure FW Dnld  (Host: SMCU)\n";
    cout << " Please Select : ";
    cin >> userInput;

    cout << " You selected : " << userInput << endl;
    if(prevState == userInput)
    {
      if (userInput == 0x02) {
        goto nfcModeExit;
      }
      cout << "ERROR : Already in this mode. No action needed...." << endl;
      continue;
    }
    else if ((userInput != 0x01) && (userInput != 0x02) && (userInput != 0x03))
    {
      cin.clear(); // Clear the error state
      cin.ignore(std::numeric_limits<std::streamsize>::max(),
                 '\n'); // Clear the input buffer
      cout << "ERROR : Invalid Selection. Retry..." << endl;
      continue;
    }

    prevState = userInput;
    if(userInput == 0x03) {
      cout << " **** Switch to SMCU Host for FW DNLD. Wait for Download to be Completed **** \n" << endl;
    }
    res = nxpnfc_->setEseUpdateState((vendor::nxp::nxpnfc::V2_0::NxpNfcHalEseState)userInput);
    EXPECT_TRUE(res);

  nfcModeExit:
    if(userInput == 0x02) {
      cout << "\n **** Restarting the NFC stack **** \n" << endl;
      break;
    } else if(userInput == 0x03) {
      if(res == true) {
        cout << " **** FW DNLD Completed. Restarting the NFC stack **** \n" << endl;
        TearDown();
        std::system("svc nfc enable");
        isFwDnldExit = true;
      } else {
        cout << " **** ERROR : FW DNLD Wait timer expired**** " << endl;
      }
      break;
    }
  }
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(NfcHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  NfcHidlEnvironment::Instance()->init(&argc, argv);

  char valueStr[PROPERTY_VALUE_MAX] = {0};
  int len = property_get("persist.vendor.nxp.i2cms.enabled", valueStr, "");

  if (len > 0) {
    int getVal = 0;
    sscanf(valueStr, "%d", &getVal);

    if (getVal == 0x00) {
      cout << "\n*** Error : Running this app is not allowed ***"
           << "\n";
      cout << "****  HW is configured as a Signle CPU  ****"
           << "\n\n";
      return -1;
    } else if ((getVal != 0x00) && (getVal != 0x01)) {
      cout << "\n*** Error : Invalid configuration ***"
           << "\n\n";
      return -1;
    }
  } else {
    std::cerr << "Failed to get the property.\n";
    return -1;
  }

  std::system("svc nfc disable"); /* Turn off NFC */
  sleep(2);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;

  if(isFwDnldExit != true)
    std::system("svc nfc enable"); /* Turn on NFC */
  sleep(2);

  return status;
}
