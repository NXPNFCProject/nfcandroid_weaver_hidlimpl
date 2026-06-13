/******************************************************************************
 *
 *  Copyright 2023, 2025-2026 NXP
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
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include "Weaver2.h"

using Weaver2 = ::aidl::android::hardware::weaver::Weaver2;

int main() {
  try {
    ALOGI("Staring up Weaver2 Service");
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<Weaver2> weaver2_instance =
        ndk::SharedRefBase::make<Weaver2>();

    const std::string instance =
        std::string() + Weaver2::descriptor + "/default";
    binder_status_t status =
        AServiceManager_addService(weaver2_instance->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
  } catch (std::__1::ios_base::failure& e) {
    ALOGE("ios failure Exception occurred = %s ", e.what());
  } catch (std::__1::system_error& e) {
    ALOGE("system error Exception occurred = %s ", e.what());
  }
  return -1;  // Should never be reached
}
