#pragma once

#include "vstwebview/webview.h"

#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <windows.h>

#include <codecvt>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace vstwebview {

// Abstract parent for both edge-chromium and edge variants.
class WebviewWin32 : public Webview {
 public:
  WebviewWin32(HWND parent_window, bool debug,
               WebviewCreatedCallback created_cb);
  ~WebviewWin32() override = default;

  virtual bool Embed() = 0;

  void *PlatformWindow() const override { return (void *)window_; }
  void Terminate() override;
  void SetTitle(const std::string &title) override;
  void SetViewSize(int width, int height, SizeHint hints) override;

  std::string ContentRootURI() const override;

 protected:
  virtual void Resize(){};

 protected:
  HWND window_;
  bool debug_;
  WebviewCreatedCallback created_cb_;

 private:
  POINT minsz_ = POINT{0, 0};
  POINT maxsz_ = POINT{0, 0};
};

}  // namespace vstwebview
