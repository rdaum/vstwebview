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

#include <public.sdk/source/vst/vsteditcontroller.h>

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "vstwebview/bindings.h"
#include "vstwebview/webview.h"

namespace vstwebview {

class WebviewControllerBindings;

class WebviewPluginView : public Steinberg::Vst::EditorView {
 public:
  WebviewPluginView(Steinberg::Vst::EditController *controller,
                    const std::vector<vstwebview::Bindings *> &bindings,
                    Steinberg::ViewRect *size = nullptr);

  // EditorView overrides
  Steinberg::tresult isPlatformTypeSupported(
      Steinberg::FIDString type) override;

  void attachedToParent() override;
  void removedFromParent() override;
  Steinberg::tresult setFrame(Steinberg::IPlugFrame *frame) override;
  Steinberg::tresult onFocus(Steinberg::TBool a_bool) override;

  Steinberg::tresult canResize() override;
  Steinberg::tresult onSize(Steinberg::ViewRect *newSize) override;

 private:
  std::mutex webview_mutex_;
  std::unique_ptr<vstwebview::Webview> webview_handle_;
  std::vector<vstwebview::Bindings *> bindings_;
};

}  // namespace vstwebview