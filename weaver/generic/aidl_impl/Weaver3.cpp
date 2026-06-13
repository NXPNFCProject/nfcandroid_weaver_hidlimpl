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
#include <log/log.h>

namespace aidl {
namespace android {
namespace hardware {
namespace weaver {

using ::ndk::ScopedAStatus;
using std::vector;

ScopedAStatus Weaver3::getConfig(WeaverConfig *out_config) {
  return weaverImpl_->getConfig(out_config);
}

ScopedAStatus Weaver3::read(int32_t in_slotId, const vector<uint8_t> &in_key,
                           WeaverReadResponse *out_response) {
  return weaverImpl_->read(in_slotId, in_key, out_response);
}

ScopedAStatus Weaver3::write(int32_t in_slotId, const vector<uint8_t> &in_key,
                            const vector<uint8_t> &in_value) {
  return weaverImpl_->write(in_slotId, in_key, in_value);
}
  ScopedAStatus Weaver3::warmUp() {
    return weaverImpl_->warmUp();
  }
  ScopedAStatus Weaver3::getTimeout(int32_t in_slotId, int64_t* _aidl_return) {
    return weaverImpl_->getTimeout(in_slotId, _aidl_return);
  }

} // namespace weaver
} // namespace hardware
} // namespace android
} // namespace aidl
