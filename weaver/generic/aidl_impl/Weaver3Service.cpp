/******************************************************************************
 *
 *  Copyright 2026 NXP
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
#include "Weaver3.h"
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

using Weaver3 = ::aidl::android::hardware::weaver::Weaver3;

int main() {
  try {
    ALOGI("Staring up Weaver3 service");
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Weaver3> weaver3 =
        ndk::SharedRefBase::make<Weaver3>();

    const std::string instance =
        std::string() + Weaver3::descriptor + "/default";
    binder_status_t status =
        AServiceManager_addService(weaver3->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
  } catch (std::__1::ios_base::failure& e) {
      ALOGE("ios failure Exception occurred = %s ", e.what());
  } catch (std::__1::system_error& e) {
      ALOGE("system error Exception occurred = %s ", e.what());
  } catch (std::bad_array_new_length& e) {
      ALOGE("bad_array_new_length error Exception occurred = %s ", e.what());
  } catch (std::length_error& e) {
      ALOGE("length_error Exception occurred = %s ", e.what());
  }
  return -1; // Should never be reached
}
