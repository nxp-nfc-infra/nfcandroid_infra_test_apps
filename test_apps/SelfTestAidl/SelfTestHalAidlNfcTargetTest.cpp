/******************************************************************************
 *
 *  Copyright 2024-2025 NXP
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
#define LOG_TAG "nfc_aidl_hal_test"
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/nfc/BnNfc.h>
#include <aidl/android/hardware/nfc/BnNfcClientCallback.h>
#include <aidl/android/hardware/nfc/INfc.h>
#include <aidl/vendor/nxp/nxpnfc_aidl/BnNxpNfc.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/binder_auto_utils.h>
#include <android/binder_enums.h>
#include <android/binder_ibinder.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <mutex>
#include <string>  //memcpy
#include <vector>

using aidl::android::hardware::nfc::INfc;
using ::aidl::vendor::nxp::nxpnfc_aidl::INxpNfc;
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
std::string NXPNFC_AIDL_HAL_SERVICE_NAME =
    "vendor.nxp.nxpnfc_aidl.INxpNfc/default";
constexpr static int kCallbackTimeoutMs = 10000;

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
  { 0x2F, 0x3D, 0x02, 0x20, 0x00}

#define TEST_FW_PRBS_A_106_CMD \
  { 0x2F, 0x30, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01 }

#define LOOP_BACK_HEADER_SIZE 3
#define SYNTAX_ERROR 5
#define NUMBER_LOOPS 3922
#define NCI_VERSION_1_1 0x11
#define NCI_VERSION_2 0x20
#define TIMEOUT_PERIOD 5
#define MODE_ESE_COLD_RESET 0x03

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

// The main test class for NFC SelfTest  HAL.
class NfcSelfTestTest : public testing::TestWithParam<std::string> {
 public:
  void core_reset_command(void);
  void core_init_command(void);
  void nci_prop_command(void);
  /* NCI version the device supports
   * 0x11 for NCI 1.1, 0x20 for NCI 2.0 and so forth */
  uint8_t nci_version;
  std::shared_ptr<INfc> nfc_;
  std::shared_ptr<INxpNfc> nxpnfc_;

  void SetUp() override {
    ::ndk::SpAIBinder nfcBinder(
        AServiceManager_waitForService(NFC_AIDL_HAL_SERVICE_NAME.c_str()));
    nfc_ = INfc::fromBinder(nfcBinder);
    ASSERT_NE(nfc_, nullptr);
    ::ndk::SpAIBinder nxpNfcBinder(
      AServiceManager_waitForService(NXPNFC_AIDL_HAL_SERVICE_NAME.c_str()));
    nxpnfc_ = INxpNfc::fromBinder(nxpNfcBinder);
    ASSERT_NE(nxpnfc_, nullptr);
  }
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  std::vector<uint8_t> rsp_data;
  auto open2_cb_future = open2_cb_promise.get_future();
  auto close_cb_future = close_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_A_106_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto close_cb_future = close_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto open2_cb_future = open2_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_A_212_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[6] = 0x01;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_A_424_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[6] = 0x02;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto open2_cb_future = open2_cb_promise.get_future();
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_A_848_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[6] = 0x03;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_B_848_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[5] = 0x01;
  data4[6] = 0x03;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_B_424_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[5] = 0x01;
  data4[6] = 0x02;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::promise<void> open2_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> close_cb_promise;
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_B_212_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[5] = 0x01;
  data4[6] = 0x01;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(10 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(10 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_B_106_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[5] = 0x01;
  data4[6] = 0x00;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  LOG(INFO) << StringPrintf("%s ", __func__);
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_F_212_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[5] = 0x02;
  data4[6] = 0x01;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            rsp_data.clear();
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_FW_PRBS_F_424_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[5] = 0x02;
  data4[6] = 0x02;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_HW_PRBS9_A_106_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[3] = 0x01;
  data4[4] = 0x00;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_HW_PRBS9_A_212_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[3] = 0x01;
  data4[4] = 0x00;
  data4[6] = 0x01;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_HW_PRBS9_A_424_CMD
  rsp_data.resize(0);
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[3] = 0x01;
  data4[4] = 0x00;
  data4[6] = 0x02;
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_HW_PRBS9_B_848_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[3] = 0x01;
  data4[4] = 0x00;
  data4[6] = 0x03;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_HW_PRBS15_B_848_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[3] = 0x01;
  data4[4] = 0x01;
  data4[5] = 0x01;
  data4[6] = 0x03;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
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
  int open_count = 0;
  int send_count = 0;
  int SEND_MAX_TIMES = 10;
  std::vector<uint8_t> rsp_data;
  std::vector<std::promise<void>> send_cmd_cb_promise;
  std::vector<std::future<void>> send_cmd_cb_future;
  std::promise<void> open_cb_promise;
  std::promise<void> open2_cb_promise;
  std::promise<void> close_cb_promise;
  auto open2_cb_future = open2_cb_promise.get_future();
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  auto close_cb_future = close_cb_promise.get_future();
  for (int i = 0; i < SEND_MAX_TIMES; i++) {
    std::promise<void> promise;
    send_cmd_cb_future.push_back(promise.get_future());
    send_cmd_cb_promise.push_back(std::move(promise));
  }
  std::shared_ptr<INfcClientCallback> mCallback =
      ndk::SharedRefBase::make<NfcClientCallback>(
          [&open_cb_promise, &open2_cb_promise, &open_count, &close_cb_promise](
              auto event, auto status) {
            EXPECT_EQ(status, NfcStatus::OK);
            LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
            if (event == NfcEvent::OPEN_CPLT) {
              open_count == 0 ? open_cb_promise.set_value()
                              : open2_cb_promise.set_value();
              open_count++;
            }
            if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
          },
          [&send_cmd_cb_promise, &rsp_data, &send_count](auto& in_data) {
            mtx.lock();
            rsp_data = in_data;
            send_cmd_cb_promise.at(send_count).set_value();
          });
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  // Open and wait for OPEN_CPLT
  LOG(INFO) << "open";
  EXPECT_TRUE(nfc_->open(mCallback).isOk());
  EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  // CORE_RESET_CMD
  std::vector<uint8_t> data = core_reset_cmd;
  int ret;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data, &ret).isOk());
  // Wait for CORE_RESET_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // Wait for CORE_RESET_NTF
  rsp_data.resize(0);
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  // Check if reset trigger was due to CORE_RESET_CMD
  EXPECT_LE(8ul, rsp_data.size());
  EXPECT_EQ(2ul, rsp_data[3]);
  EXPECT_GE(1ul, rsp_data[4]);
  send_count++;
  mtx.unlock();

  // CORE_INIT_CMD_NCI20
  std::vector<uint8_t> data1 = core_init_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data1, &ret).isOk());
  // Wait for CORE_INIT_RSP_NCI20
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(timeout),
            std::future_status::ready);
  EXPECT_LE(4ul, rsp_data.size());
  EXPECT_EQ((int)(rsp_data.size() - 3), rsp_data[2]);
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_PROPRIETARY_ACT_CMD
  std::vector<uint8_t> data2 = prop_act_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data2, &ret).isOk());
  // Wait for NCI_PROPRIETARY_ACT_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(8ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // NCI_DISABLE_STANDBY_MODE_CMD
  std::vector<uint8_t> data3 = nci_disable_standby_cmd;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data3, &ret).isOk());
  // Wait for NCI_DISABLE_STANDBY_MODE_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  // TEST_HW_PRBS15_B_424_CMD
  std::vector<uint8_t> data4 = test_prbs_fw_a_106_cmd;
  data4[3] = 0x01;
  data4[4] = 0x01;
  data4[5] = 0x01;
  rsp_data.resize(0);
  EXPECT_TRUE(nfc_->write(data4, &ret).isOk());
  // Wait for TEST_ANNTENA_RF_FIELD_ON_RSP
  EXPECT_EQ(send_cmd_cb_future.at(send_count).wait_for(3 * timeout),
            std::future_status::ready);
  EXPECT_EQ(4ul, rsp_data.size());
  EXPECT_EQ((int)NfcStatus::OK, rsp_data[3]);
  send_count++;
  mtx.unlock();

  nfc_->powerCycle();
  // Wait for OPEN_CPLT event from power_cycle_complete callback
  LOG(INFO) << "wait for open";
  EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);

  // Close and wait for CLOSE_CPLT
  LOG(INFO) << "close HOST_SWITCHED_OFF";
  EXPECT_TRUE(nfc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
  LOG(INFO) << "wait for close";
  EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
}

/*
 * HalApi_SelfTest: Test all NXPNFC HAL APIs.
 */

TEST_P(NfcSelfTestTest, INxpNfcApi_selfTest) {
/*********************************SET_TRANSIT_CONFIG************************************/
bool ret;
  //std::string transitconf = "This is dummy data\n";
  std::string transitconf;
  EXPECT_TRUE(nxpnfc_->setNxpTransitConfig(transitconf, &ret).isOk());
  EXPECT_TRUE(ret);
  /*********************************NCI_TRANSCEIVE***************************************/
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcSelfTestTest);
INSTANTIATE_TEST_SUITE_P(
    PerInstance, NfcSelfTestTest,
    testing::ValuesIn(::android::getAidlHalInstanceNames(INfc::descriptor)),
    ::android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  std::system("svc nfc disable"); /* Turn off NFC */
  usleep(300000);

  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;

  std::system("svc nfc enable"); /* Turn on NFC */
  sleep(2);

  return status;
}
