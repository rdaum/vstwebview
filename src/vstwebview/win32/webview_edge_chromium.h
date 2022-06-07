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

#include "vstwebview/win32/webview_win32.h"

#pragma comment(lib, "windowsapp")

#include <wrl.h>

#include <WebView2.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace vstwebview {

//
// Edge/Chromium browser engine
//
class EdgeChromiumBrowser : public WebviewWin32 {
 public:
  EdgeChromiumBrowser(HWND parent_window, bool debug,
                      WebviewCreatedCallback created_cb);
  ~EdgeChromiumBrowser() override;

  bool Embed() override;

  void Navigate(const std::string &url) override;
  void OnDocumentCreate(const std::string &js) override;
  void EvalJS(const std::string &js, ResultCallback rs) override;
  void DispatchIn(DispatchFunction f) override;

 protected:
  void Resize() override;

 private:
  HRESULT OnControllerCreated(HRESULT result,
                              ICoreWebView2Controller *controller);
  HRESULT OnEnvironmentCreated(HRESULT result,
                               ICoreWebView2Environment *environment);
  HRESULT OnWebMessageReceived(ICoreWebView2 *sender,
                               ICoreWebView2WebMessageReceivedEventArgs *args);
  HRESULT OnPermissionRequested(
      ICoreWebView2 *sender, ICoreWebView2PermissionRequestedEventArgs *args);
  bool SetFilePaths();

  wchar_t user_data_path_[MAX_PATH];

  ICoreWebView2 *webview2_ = nullptr;
  ICoreWebView2Controller *wv2_controller_ = nullptr;

  Microsoft::WRL::ComPtr<
      ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
      controller_completed_handler_;
  Microsoft::WRL::ComPtr<
      ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
      environment_completed_handler_;
  Microsoft::WRL::ComPtr<ICoreWebView2WebMessageReceivedEventHandler>
      message_received_handler_;
  Microsoft::WRL::ComPtr<ICoreWebView2PermissionRequestedEventHandler>
      permission_requested_handler_;
  ICoreWebView2Settings *settings_;
};

}  // namespace vstwebview