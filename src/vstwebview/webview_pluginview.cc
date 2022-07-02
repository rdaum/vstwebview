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

#include "vstwebview/webview_pluginview.h"

#include "vstwebview/webview_controller_bindings.h"

namespace vstwebview {

WebviewPluginView::WebviewPluginView(
    Steinberg::Vst::EditController *controller,
    const std::string &title,
    const std::vector<vstwebview::Bindings *> &bindings,
    Steinberg::ViewRect *size,
    const std::string uri)
    : Steinberg::Vst::EditorView(controller, size), title_(title),
      bindings_(bindings) {}

Steinberg::tresult WebviewPluginView::isPlatformTypeSupported(
    Steinberg::FIDString type) {
  return Steinberg::kResultTrue;
}

Steinberg::tresult WebviewPluginView::onSize(Steinberg::ViewRect *newSize) {
  {
    std::lock_guard<std::mutex> webview_lock(webview_mutex_);
    if (webview_handle_) {
      webview_handle_->SetViewSize(newSize->getWidth(), newSize->getHeight(),
                                   vstwebview::Webview::SizeHint::kNone);
    }
  }
  return Steinberg::Vst::EditorView::onSize(newSize);
}

void WebviewPluginView::attachedToParent() {
  if (!webview_handle_) {
    auto init_function = [this](vstwebview::Webview *webview) {
      for (auto binding : bindings_) {
        binding->Bind(webview);
      }
      webview->SetTitle(title_);
      webview->SetViewSize(rect.getWidth(), rect.getHeight(),
                           vstwebview::Webview::SizeHint::kFixed);

      if (uri) {
          webview->Navigate(uri);
      } else {
          webview->Navigate(webview->ContentRootURI() + "/index.html");
      }
    };

    webview_handle_ =
        vstwebview::MakeWebview(true, plugFrame, systemWindow, init_function);
  }

  EditorView::attachedToParent();
}

Steinberg::tresult WebviewPluginView::setFrame(Steinberg::IPlugFrame *frame) {
  return CPluginView::setFrame(frame);
}

Steinberg::tresult WebviewPluginView::onFocus(Steinberg::TBool a_bool) {
  return Steinberg::kResultOk;
}

void WebviewPluginView::removedFromParent() {
  if (webview_handle_) {
    webview_handle_->Terminate();
  }

  EditorView::removedFromParent();
}

Steinberg::tresult WebviewPluginView::canResize() {
  return Steinberg::kResultFalse;
}

}  // namespace vstwebview
