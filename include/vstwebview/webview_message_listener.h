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

#pragma once

#include <string>

#include <public.sdk/source/vst/vsteditcontroller.h>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace vstwebview {

class Webview;

class WebviewMessageListener {
public:
  explicit WebviewMessageListener(vstwebview::Webview *webview)
      : webview_(webview) {}

  struct MessageAttribute {
    std::string name;
    enum class Type { INT, FLOAT, STRING, BINARY };
    Type type;
  };

  void Subscribe(const std::string &receiver, const std::string &message_id,
                 const std::vector<MessageAttribute> &attributes);

  Steinberg::tresult Notify(Steinberg::Vst::IMessage *message);

private:
  struct MessageDescriptor {
    std::string message_id;
    std::vector<MessageAttribute> attributes;
  };

  struct MessageSubscription {
    MessageDescriptor descriptor;
    std::string notify_function;
  };

  json SerializeMessage(Steinberg::Vst::IMessage *message,
                        const MessageDescriptor &descriptor);

  std::unordered_map<std::string, MessageSubscription> subscriptions_;
  vstwebview::Webview *webview_;
};

}  // namespace vstwebview