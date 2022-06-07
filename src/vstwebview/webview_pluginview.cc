#include "vstwebview/webview_pluginview.h"

#include "vstwebview/webview_controller_bindings.h"

namespace vstwebview {

WebviewPluginView::WebviewPluginView(
    Steinberg::Vst::EditController *controller,
    const std::vector<vstwebview::Bindings *> &bindings,
    Steinberg::ViewRect *size)
    : Steinberg::Vst::EditorView(controller, size), bindings_(bindings) {}

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
      webview->SetTitle("Sidebands VST Webview");
      webview->SetViewSize(rect.getWidth(), rect.getHeight(),
                           vstwebview::Webview::SizeHint::kFixed);

      webview->Navigate(webview->ContentRootURI() + "/index.html");
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
