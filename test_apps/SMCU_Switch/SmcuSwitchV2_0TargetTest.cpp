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
 *  Copyright 2023-2024 NXP
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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/nfc/BnNfc.h>
#include <aidl/android/hardware/nfc/BnNfcClientCallback.h>
#include <aidl/android/hardware/nfc/INfc.h>
#include <android-base/stringprintf.h>
#include <android/binder_auto_utils.h>
#include <android/binder_enums.h>
#include <android/binder_ibinder.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <hardware/nfc.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <hardware/nfc.h>

#include <string>//memcpy
#include <vendor/nxp/nxpnfc/2.0/INxpNfc.h>

#include <chrono>
#include <future>
#include <mutex>
#include <string>  //memcpy
#include <vector>

using aidl::android::hardware::nfc::INfc;
using ::vendor::nxp::nxpnfc::V2_0::INxpNfc;
using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using android::getAidlHalInstanceNames;
using android::PrintInstanceNameToString;
using android::base::StringPrintf;
using namespace std;
using aidl::android::hardware::nfc::INfcClientCallback;
using aidl::android::hardware::nfc::NfcCloseType;
using aidl::android::hardware::nfc::NfcEvent;
using aidl::android::hardware::nfc::NfcStatus;

std::mutex mtx;
std::string NFC_AIDL_HAL_SERVICE_NAME = "android.hardware.nfc.INfc/default";
constexpr static int kCallbackTimeoutMs = 10000;

/* NCI Commands */
bool isFwDnldExit = false;
static sp<INxpNfc> gp_nxpnfc_;
static std::shared_ptr<INfc> gp_nfc_;


static std::promise<void> open_cb_promise;
static std::future<void> open_cb_future = open_cb_promise.get_future();
static std::promise<void> close_cb_promise;
static std::future<void> close_cb_future = close_cb_promise.get_future();
static std::chrono::milliseconds timeout{kCallbackTimeoutMs};

class NfcClientCallback
    : public aidl::android::hardware::nfc::BnNfcClientCallback {
 public:
  NfcClientCallback(
      const std::function<void(NfcEvent, NfcStatus)>& on_hal_event_cb,
      const std::function<void(const std::vector<uint8_t>&)>& on_nci_data_cb)
      : on_nci_data_cb_(on_nci_data_cb), on_hal_event_cb_(on_hal_event_cb) {}
  virtual ~NfcClientCallback() = default;

  /* sendEvent callback function - Records the Event & Status
   * and notifies the TEST
   **/
  ::ndk::ScopedAStatus sendEvent(NfcEvent event,
                                 NfcStatus event_status) override {
    on_hal_event_cb_(event, event_status);
    return ::ndk::ScopedAStatus::ok();
  };

  /* sendData callback function. Records the data and notifies the TEST*/
  ::ndk::ScopedAStatus sendData(const std::vector<uint8_t>& data) override {
    on_nci_data_cb_(data);
    return ::ndk::ScopedAStatus::ok();
  };

 private:
  std::function<void(const std::vector<uint8_t>&)> on_nci_data_cb_;
  std::function<void(NfcEvent, NfcStatus)> on_hal_event_cb_;
};


// The main test class for DUAL CPU Mode Switch HAL.
class NxpNfc_DualCpuTest : public testing::TestWithParam<std::string> {
  public:
    uint8_t nci_version;
    std::shared_ptr<INfc> nfc_;

    /**
     * @brief  Listens for user interrupt and exits the application
     *
     **/
    static void signal_callback_handler(int signum) {
      auto res = 0;
      std::vector<std::promise<void>> send_cmd_cb_promise;
      cout << "SMCU test App abort requested, signum:" << signum << endl;
      res = gp_nxpnfc_->setEseUpdateState(
          (vendor::nxp::nxpnfc::V2_0::NxpNfcHalEseState)0x02);
      EXPECT_TRUE(res);
      // Close and wait for CLOSE_CPLT
      LOG(INFO) << "close HOST_SWITCHED_OFF";
      EXPECT_TRUE(gp_nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
      EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
      std::system("svc nfc enable"); /* Turn on NFC */
      sleep(2);
      exit(signum);
    }

    /* NCI version the device supports
     * 0x11 for NCI 1.1, 0x20 for NCI 2.0 and so forth */
    sp<INxpNfc> nxpnfc_;
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
      ::ndk::SpAIBinder binder(
          AServiceManager_waitForService(NFC_AIDL_HAL_SERVICE_NAME.c_str()));
      nfc_ = INfc::fromBinder(binder);
      ASSERT_NE(nfc_, nullptr);
      nxpnfc_ = INxpNfc::getService();
      ASSERT_NE(nxpnfc_, nullptr);

      std::vector<uint8_t> rsp_data;
      LOG(INFO) << StringPrintf("%s ", __func__);
      std::shared_ptr<INfcClientCallback> mCallback =
          ndk::SharedRefBase::make<NfcClientCallback>(
              [](
                auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                LOG(INFO) << StringPrintf("stack callback %s,%d ", __func__, event);
                  if (event == NfcEvent::OPEN_CPLT) {
                    open_cb_promise.set_value();
                  }
                  if (event == NfcEvent::CLOSE_CPLT) {
                      try
                       {
                          close_cb_promise.set_value();
                       }
                       catch (const std::future_error& e)
                       {
                          LOG(INFO) << StringPrintf("stack callback %s, %s ", __func__, e.what());
                       }
                }
                },
                [ &rsp_data](auto& in_data) {
                  rsp_data.clear();
                  rsp_data = in_data;
                });
      // Open and wait for OPEN_CPLT
      LOG(INFO) << "open";
      EXPECT_TRUE(nfc_->open(mCallback).isOk());
      EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

      /* Clear the flag */
      isFwDnldExit = false;
      gp_nxpnfc_ = nxpnfc_;
      gp_nfc_ = nfc_;
    
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
    res = gp_nxpnfc_->setEseUpdateState((vendor::nxp::nxpnfc::V2_0::NxpNfcHalEseState)userInput);
    EXPECT_TRUE(res);

  nfcModeExit:
    if(userInput == 0x02) {
      std::vector<std::promise<void>> send_cmd_cb_promise;
        // Close and wait for CLOSE_CPLT
      LOG(INFO) << "close DISABLE";
      EXPECT_TRUE(gp_nfc_->close(NfcCloseType::DISABLE).isOk());
 
      cout << "\n **** Restarting the NFC stack **** \n" << endl;
      break;
    } else if(userInput == 0x03) {
      if(res == true) {
        cout << " **** FW DNLD Completed. Restarting the NFC stack **** \n" << endl;
        std::vector<std::promise<void>> send_cmd_cb_promise;
        if(isFwDnldExit == false) {
          // Close and wait for CLOSE_CPLT
          LOG(INFO) << "close DISABLE";
          EXPECT_TRUE(gp_nfc_->close(NfcCloseType::DISABLE).isOk());
          LOG(INFO) << "not waiting for close";
          EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
        }
   
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
  ABinderProcess_startThreadPool();
  ::testing::InitGoogleTest(&argc, argv);

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
