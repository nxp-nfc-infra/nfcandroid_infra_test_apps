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
#include <iostream>
#include <stdexcept>

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcSelfTestTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, NfcSelfTestTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(INfc::descriptor)),
        android::hardware::PrintInstanceNameToString);

int main(int argc, char** argv) {
  try {
    ::testing::InitGoogleTest(&argc, argv);
    std::system("svc nfc disable"); /* Turn off NFC */
    sleep(2);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    std::system("svc nfc enable"); /* Turn on NFC */
    sleep(2);
    return status;
  } catch (const std::length_error &e) {
    return 1; // Exit with an error code
  } catch (const std::exception &e) {
    return 1; // Exit with an error code
  }
}