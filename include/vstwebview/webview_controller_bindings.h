/*
 * Copyright 2022 Ryan Daum
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

#pragma once

#include <public.sdk/source/common/threadchecker.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <nlohmann/json.hpp>

#include "vstwebview/bindings.h"
#include "vstwebview/webview.h"

using nlohmann::json;

namespace vstwebview {

/**
 * Implementation of JS bindings for proxying the functionality of the VST3
 * EditController through to the webview.
 */
class WebviewControllerBindings : public vstwebview::Bindings {
 public:
  explicit WebviewControllerBindings(
      Steinberg::Vst::EditControllerEx1 *controller);

  void Bind(vstwebview::Webview *webview) override;

 private:
  void DeclareJSBinding(const std::string &name,
                        vstwebview::Webview::FunctionBinding binding);

  using CallbackFn = json (WebviewControllerBindings::*)(
      vstwebview::Webview *webview, const json &);
  std::function<const json(vstwebview::Webview *webview, int seq,
                           const std::string &, const json &)>
  BindCallback(CallbackFn fn);

  json GetParameterObject(vstwebview::Webview *webview, const json &in);
  json GetParameterObjects(vstwebview::Webview *webview, const json &in);
  json SetParameterNormalized(vstwebview::Webview *webview, const json &in);
  json NormalizedParamToPlain(vstwebview::Webview *webview, const json &in);
  json GetParamNormalized(vstwebview::Webview *webview, const json &in);
  json BeginEdit(vstwebview::Webview *webview, const json &in);
  json PerformEdit(vstwebview::Webview *webview, const json &in);
  json EndEdit(vstwebview::Webview *webview, const json &in);
  json GetParameterCount(vstwebview::Webview *webview, const json &in);
  json GetSelectedUnit(vstwebview::Webview *webview, const json &in);
  json SelectUnit(vstwebview::Webview *webview, const json &in);
  json SubscribeParameter(vstwebview::Webview *webview, const json &in);
  json DoSendMessage(vstwebview::Webview *webview, const json &in);

  std::unique_ptr<Steinberg::Vst::ThreadChecker> thread_checker_;
  std::vector<std::pair<std::string, vstwebview::Webview::FunctionBinding>>
      bindings_;
  std::unique_ptr<Steinberg::IDependent> param_dep_proxy_;
  Steinberg::Vst::EditControllerEx1 *controller_;
};

}  // namespace vstwebview