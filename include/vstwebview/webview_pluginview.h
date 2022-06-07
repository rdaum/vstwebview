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