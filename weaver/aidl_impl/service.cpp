/******************************************************************************
 *
 *  Copyright 2023, 2025 NXP
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
#include "Weaver.h"
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

using ::aidl::android::hardware::weaver::Weaver;

int main() {
  try {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Weaver> weaver = ndk::SharedRefBase::make<Weaver>();

    const std::string instance =
        std::string() + Weaver::descriptor + "/default";
    binder_status_t status =
        AServiceManager_addService(weaver->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
  } catch (std::length_error& e) {
    ALOGE("Length Exception occurred = %s ", e.what());
  } catch (std::__1::ios_base::failure &e) {
    ALOGE("ios failure Exception occurred = %s ", e.what());
  } catch (std::__1::system_error &e) {
    ALOGE("system error Exception occurred = %s ", e.what());
  }
  return -1; // Should never be reached
}
