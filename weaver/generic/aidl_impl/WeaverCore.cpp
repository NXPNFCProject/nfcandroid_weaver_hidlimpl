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
#include "WeaverCore.h"

#include <log/log.h>

namespace aidl {
namespace android {
namespace hardware {
namespace weaver {

using ::aidl::android::hardware::weaver::IWeaver;
using ::ndk::ScopedAStatus;
using std::vector;

// Methods from ::android::hardware::weaver::IWeaver follow.

WeaverCore::WeaverCore() {
  ALOGD("WeaverCore Constructor");
  pInterface = WeaverImpl::getInstance();
  if (pInterface != NULL) {
    pInterface->Init();
  } else {
    ALOGE("Failed to get WeaverCore Interface");
  }
}

ScopedAStatus WeaverCore::getConfig(WeaverConfig* out_config) {
  ALOGD("WeaverCore::getConfig");

  if (out_config == NULL) {
    ALOGE("Invalid param");
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Null pointer passed");
  }
  memset(out_config, 0, sizeof(WeaverConfig));

  if (pInterface == NULL) {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Weaver interface not defined");
  }
  SlotInfo slotInfo;
  Status_Weaver status = pInterface->GetSlots(slotInfo);
  if (status == WEAVER_STATUS_OK) {
    out_config->slots = slotInfo.slots;
    out_config->keySize = slotInfo.keySize;
    out_config->valueSize = slotInfo.valueSize;
    ALOGD("WeaverCore Success for getSlots Slots :(%d)", out_config->slots);
    return ScopedAStatus::ok();
  } else {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Failed to retrieve slots info");
  }
}

ScopedAStatus WeaverCore::read(int32_t in_slotId, const vector<uint8_t>& in_key,
                               WeaverReadResponse* out_response) {
  ALOGD("WeaverCore::read slot %d", in_slotId);

  if (out_response == NULL) {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Null pointer passed");
  }
  if (in_key.empty()) {
    out_response->status = WeaverReadStatus::FAILED;
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Empty key input passed");
  }
  if (pInterface == NULL) {
    out_response->status = WeaverReadStatus::FAILED;
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Weaver interface not defined");
  }
  memset(out_response, 0, sizeof(WeaverReadResponse));

  ReadRespInfo readInfo;
  ScopedAStatus retStatus;
  Status_Weaver status;

  status = pInterface->Read(in_slotId, in_key, readInfo);
  switch (status) {
    case WEAVER_STATUS_OK:
      ALOGD("Read slot %d OK", in_slotId);
      out_response->value = readInfo.value;
      out_response->status = WeaverReadStatus::OK;
      retStatus = ScopedAStatus::ok();
      break;
    case WEAVER_STATUS_INCORRECT_KEY:
      ALOGE("Read Incorrect Key");
      out_response->value.resize(0);
      out_response->timeout = readInfo.timeout;
      out_response->status = WeaverReadStatus::INCORRECT_KEY;
      retStatus = ScopedAStatus::ok();
      break;
    case WEAVER_STATUS_THROTTLE:
      ALOGE("Read WEAVER_THROTTLE");
      out_response->value.resize(0);
      out_response->timeout = readInfo.timeout;
      out_response->status = WeaverReadStatus::THROTTLE;
      retStatus = ScopedAStatus::ok();
      break;
    default:
      out_response->timeout = 0;
      out_response->value.resize(0);
      out_response->status = WeaverReadStatus::FAILED;
      retStatus = ScopedAStatus::ok();
      break;
  }
  return retStatus;
}

ScopedAStatus WeaverCore::write(int32_t in_slotId,
                                const vector<uint8_t>& in_key,
                                const vector<uint8_t>& in_value) {
  ALOGD("WeaverCore::write slot %d", in_slotId);
  if (in_key.empty() || in_value.empty()) {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Invalid parameters passed");
  }
  if (pInterface == NULL) {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "WeaverCore interface not defined");
  }
  if (pInterface->Write(in_slotId, in_key, in_value) == WEAVER_STATUS_OK) {
    ALOGD("Write slot %d OK", in_slotId);
    return ScopedAStatus::ok();
  } else {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
        IWeaver::STATUS_FAILED, "Unknown error");
  }
}

ScopedAStatus WeaverCore::warmUp() {
    ALOGI("warmup");

    pInterface->setSessionTimeoutValue(WARMUP_IDLE_TIMEOUT_VALUE);

    // Reusing getConfig() to open the session/channel
    WeaverConfig out_config;
    auto status = getConfig(&out_config);

    // let transport layer use the default timeout value
    pInterface->setSessionTimeoutValue(std::nullopt);
    return status;
}

ScopedAStatus WeaverCore::getTimeout(int32_t in_slotId, int64_t* _aidl_return) {

    if(pInterface->getSlotThrottleValue(in_slotId, _aidl_return) != WEAVER_STATUS_OK) {
      return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return ScopedAStatus::ok();
}

}  // namespace weaver
}  // namespace hardware
}  // namespace android
}  // namespace aidl
