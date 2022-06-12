// Copyright 2022 Ryan Daum
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vstwebview/webview_message_listener.h"

#include <public.sdk/source/vst/utility/stringconvert.h>

#include "vstwebview/webview.h"

namespace vstwebview {

void WebviewMessageListener::Subscribe(
    const std::string &receiver, const std::string &message_id,
    const std::vector<MessageAttribute> &attributes) {
  subscriptions_[message_id] = {message_id, attributes, receiver};
}

json WebviewMessageListener::SerializeMessage(
    Steinberg::Vst::IMessage *message,
    const WebviewMessageListener::MessageDescriptor &descriptor) {
  auto attributes = message->getAttributes();

  json j = {{"messageId", message->getMessageID()}};
  for (const auto &attr : descriptor.attributes) {
    switch (attr.type) {
    case MessageAttribute::Type::INT:
      Steinberg::int64 i;
      if (attributes->getInt(attr.name.c_str(), i) ==
          Steinberg::kResultTrue) {
        j[attr.name] = i;
      }
      break;
    case MessageAttribute::Type::FLOAT:
      double f;
      if (attributes->getFloat(attr.name.c_str(), f) ==
          Steinberg::kResultTrue) {
        j[attr.name] = f;
      }
      break;
    case MessageAttribute::Type::STRING:
      Steinberg::Vst::TChar str[128];
      if (attributes->getString(attr.name.c_str(), str,
                                128 * sizeof(Steinberg::Vst::TChar)) ==
          Steinberg::kResultTrue) {
        j[attr.name] = str;
      }
      break;
    case MessageAttribute::Type::BINARY:
      const void *addr;
      Steinberg::uint32 size;
      if (attributes->getBinary(attr.name.c_str(), addr, size) ==
          Steinberg::kResultTrue) {
        std::vector<double> data(size / sizeof(double));
        std::memcpy(data.data(), addr, size);
        j[attr.name] = data;
      }
      break;
    }
  }
  return j;
}

Steinberg::tresult WebviewMessageListener::Notify(
    Steinberg::Vst::IMessage *message) {
  auto *msg_id = message->getMessageID();

  const auto &it = subscriptions_.find(msg_id);
  if (it == subscriptions_.end()) return Steinberg::kResultFalse;

  auto json = SerializeMessage(message, it->second.descriptor);
  auto send_message_js = it->second.notify_function + "(" + json.dump() + ");";
  webview_->EvalJS(send_message_js, [](const nlohmann::json &res) {});
  return Steinberg::kResultOk;
}

}  // namespace vstwebview