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
