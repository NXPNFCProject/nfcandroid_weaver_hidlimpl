/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <aidl/android/hardware/weaver/IWeaver.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <future>
#include <string>
#include <thread>

using aidl::android::hardware::weaver::IWeaver;
using aidl::android::hardware::weaver::WeaverConfig;

constexpr char SECURE_ELEMENT_SERVICE_NAME[]
                                = "android.se.omapi.ISecureElementService/default";
constexpr char WEAVER_SERVICE_NAME[] = "android.hardware.weaver.IWeaver/default";

// Helper function to run a command and capture its standard output.
// Returns true on success (exit code 0), false otherwise.
static bool RunCommand(const std::string& command, std::string& stdout_str) {
    stdout_str.clear();
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        PLOG(ERROR) << "Failed to popen command: " << command;
        return false;
    }

    if (!android::base::ReadFdToString(fileno(pipe), &stdout_str)) {
        LOG(ERROR) << "Failed to read command output.";
        return false;
    }

    int status = pclose(pipe);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// Gets the PID of the Secure Element service using dumpsys.
// Returns the PID as an integer, or <=0 if the service is not running or on error.
static int GetSecureElementServicePid() {
    std::string pid_str;
    const std::string command = "dumpsys --pid " + std::string(SECURE_ELEMENT_SERVICE_NAME);
    if (RunCommand(command, pid_str)) {
        pid_str.erase(pid_str.find_last_not_of(" \n\r\t") + 1);
        if (!pid_str.empty() && std::all_of(pid_str.begin(), pid_str.end(), ::isdigit)) {
            return atoi(pid_str.c_str());
        }
    }
    return -1;
}

// Verifies that the Weaver HAL is responsive by calling its getConfig method.
// Returns an AssertionResult to indicate success or failure.
testing::AssertionResult CheckWeaverHalIsResponding() {
    LOG(INFO) << "Checking if Weaver HAL is responding..";
    ndk::SpAIBinder binder(AServiceManager_waitForService(WEAVER_SERVICE_NAME));
    std::shared_ptr<IWeaver> weaver_service = IWeaver::fromBinder(binder);
    if (weaver_service == nullptr) {
        return testing::AssertionFailure() << "Failed to get Weaver AIDL service.";
    }

    WeaverConfig config;
    ndk::ScopedAStatus status = weaver_service->getConfig(&config);
    if (!status.isOk()) {
        return testing::AssertionFailure()
                    << "Weaver HAL's getConfig() returned an error: "
                    << status.getMessage();
    }
    LOG(INFO) << "Weaver HAL is responding without errors.";
    return testing::AssertionSuccess();
}

static void OnBinderDied(void* promise_cookie) {
    LOG(INFO) << "Received a binder died notification for the service.";
    // Set the promise the test is waiting on.
    std::promise<void>* promise = static_cast<std::promise<void>*>(promise_cookie);
    promise->set_value();
}

static void OnBinderUnlinked(void* promise_cookie) {
    LOG(INFO) << "Cleaning-up the cookie on unlinking death recipient..";
    delete static_cast<std::promise<void>*>(promise_cookie);
}

class WeaverKillSecureElementParamTest : public ::testing::TestWithParam<int> {};

TEST_P(WeaverKillSecureElementParamTest, WeaverRecoversAfterSecureElementServiceRestarts) {
    SCOPED_TRACE(::testing::Message() << "Iteration " << GetParam());
    LOG(INFO) << "Starting iteration " << GetParam();

    // Get the initial PID of the Secure Element Service
    const int initial_pid = GetSecureElementServicePid();
    ASSERT_GT(initial_pid, 0) << "The Secure Element Service is not running.";
    LOG(INFO) << "The Secure Element Service's PID: " << initial_pid;

    // Register a death recipient with the Secure Element Service to confirm
    // that the service triggers a death notification after it is killed
    ndk::SpAIBinder se_binder(AServiceManager_getService(SECURE_ELEMENT_SERVICE_NAME));
    ASSERT_NE(se_binder.get(), nullptr)
        << "Failed to get the Secure Element Service binder.";
    // Use promise/future objects to communicate if a death notification is received.
    // The death recipient will own and set the promise object while the test will
    // wait on the associated future object.
    std::promise<void> *death_promise = new std::promise<void>();
    auto death_future = death_promise->get_future();
    auto death_recipient = ndk::ScopedAIBinder_DeathRecipient(
                                    AIBinder_DeathRecipient_new(OnBinderDied));
    AIBinder_DeathRecipient_setOnUnlinked(death_recipient.get(), OnBinderUnlinked);
    auto link_status = AIBinder_linkToDeath(se_binder.get(), death_recipient.get(),
                                            static_cast<void*>(death_promise));
    ASSERT_EQ(link_status, STATUS_OK) << "AIBinder_linkToDeath() failed.";

    // Kill the Secure Element Service
    LOG(INFO) << "Killing the Secure Element Service..";
    ASSERT_EQ(kill(initial_pid, SIGKILL), 0) << "Failed to kill the service.";

    // Verify that a death notification is received
    LOG(INFO) << "Waiting for a death notification..";
    const int kDeathNotificationTimeoutSec = 5;
    auto future_status = death_future.wait_for(
                            std::chrono::seconds(kDeathNotificationTimeoutSec));
    ASSERT_EQ(future_status, std::future_status::ready)
        << "Did not receive a death notification within "
        << kDeathNotificationTimeoutSec << " seconds.";
    LOG(INFO) << "Successfully received a death notification.";

    // Confirm that the Secure Element Service recovers with a new PID
    int new_pid = 0;
    const int kMaxRetries = 100;  // 100 * 100ms = 10 seconds timeout
    constexpr auto kRetryDelay = std::chrono::milliseconds(100);
    for (int i = 0; i < kMaxRetries; ++i) {
        std::this_thread::sleep_for(kRetryDelay);
        new_pid = GetSecureElementServicePid();
        if (new_pid > 0 && new_pid != initial_pid) {
            break;
        }
    }
    ASSERT_GT(new_pid, 0) << "The Secure Element Service did not restart.";
    ASSERT_NE(new_pid, initial_pid)
            << "The Secure Element Service did not restart (has the same PID).";
    LOG(INFO) << "The Secure Element Service recovered with new PID: " << new_pid;

    // Check that the Weaver HAL is still responding, with a 5-second timeout.
    // A small delay to allow Weaver to process the death notification.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const int kWeaverHalTimeoutSec = 5;
    auto future = std::async(std::launch::async, CheckWeaverHalIsResponding);
    if (future.wait_for(std::chrono::seconds(kWeaverHalTimeoutSec))
                                            == std::future_status::timeout) {
        FAIL() << "The Weaver HAL did not respond for too long (timeout="
                        << kWeaverHalTimeoutSec << "seconds)";
    } else {
        ASSERT_TRUE(future.get());
    }
}

INSTANTIATE_TEST_SUITE_P(Repeat5Times, WeaverKillSecureElementParamTest, ::testing::Range(1, 6));

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
