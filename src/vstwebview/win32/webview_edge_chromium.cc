#include "vstwebview/win32/webview_edge_chromium.h"

#include <winrt/base.h>
namespace winrt::impl {
template <typename Async>
auto wait_for(Async const &async, Windows::Foundation::TimeSpan const &timeout);
}
#include <objbase.h>
#include <winrt/Windows.Foundation.h>

using Microsoft::WRL::Callback;

extern "C" {
static int SignatureFunctionForDLL = 0;
}

namespace vstwebview {

EdgeChromiumBrowser::EdgeChromiumBrowser(HWND parent_window, bool debug,
                                         WebviewCreatedCallback created_cb)
    : WebviewWin32(parent_window, debug, created_cb) {
  SetFilePaths();
  controller_completed_handler_ = Microsoft::WRL::Callback<
      ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
      this, &EdgeChromiumBrowser::OnControllerCreated);
  environment_completed_handler_ =
      Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          this, &EdgeChromiumBrowser::OnEnvironmentCreated);
  message_received_handler_ =
      Callback<ICoreWebView2WebMessageReceivedEventHandler>(
          this, &EdgeChromiumBrowser::OnWebMessageReceived);
  permission_requested_handler_ =
      Callback<ICoreWebView2PermissionRequestedEventHandler>(
          this, &EdgeChromiumBrowser::OnPermissionRequested);
}

EdgeChromiumBrowser::~EdgeChromiumBrowser() {
  wv2_controller_->Release();
  webview2_->Release();
  settings_->Release();
}

HRESULT EdgeChromiumBrowser::OnWebMessageReceived(
    ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) {
  LPWSTR message;
  args->TryGetWebMessageAsString(&message);
  OnBrowserMessage(winrt::to_string(message));
  sender->PostWebMessageAsString(message);

  CoTaskMemFree(message);
  return S_OK;
}

HRESULT EdgeChromiumBrowser::OnPermissionRequested(
    ICoreWebView2 *sender, ICoreWebView2PermissionRequestedEventArgs *args) {
  COREWEBVIEW2_PERMISSION_KIND kind;
  args->get_PermissionKind(&kind);
  if (kind == COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ) {
    args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
  }
  return S_OK;
}

HRESULT EdgeChromiumBrowser::OnEnvironmentCreated(
    HRESULT result, ICoreWebView2Environment *environment) {
  if (!SUCCEEDED(result)) {
    return E_FAIL;
  }
  environment->CreateCoreWebView2Controller(
      window_, controller_completed_handler_.Get());
  return S_OK;
}

HRESULT EdgeChromiumBrowser::OnControllerCreated(
    HRESULT result, ICoreWebView2Controller *controller) {
  if (!SUCCEEDED(result) || !controller) {
    return E_FAIL;
  }

  wv2_controller_ = controller;
  wv2_controller_->AddRef();
  if (wv2_controller_->get_CoreWebView2(&webview2_) != S_OK) {
    return E_FAIL;
  }

  ICoreWebView2_10 *webview_2_10;
  if (webview2_->QueryInterface(IID_PPV_ARGS(&webview_2_10)) != S_OK) {
    return E_FAIL;
  }

  if (webview2_->get_Settings(&settings_) != S_OK) {
    return E_FAIL;
  }

  if (debug_) {
    if (webview2_->OpenDevToolsWindow() != S_OK) {
      return E_FAIL;
    }
  }

  ::EventRegistrationToken token;
  if (webview2_->add_WebMessageReceived(message_received_handler_.Get(),
                                        &token) != S_OK) {
    return E_FAIL;
  };
  if (webview2_->add_PermissionRequested(permission_requested_handler_.Get(),
                                         &token) != S_OK) {
    return E_FAIL;
  }

  webview2_->AddRef();

  OnDocumentCreate(
      "window.external={invoke:s=>window.chrome.webview."
      "postMessage("
      "s)}");
  created_cb_(this);
  return S_OK;
}

bool EdgeChromiumBrowser::Embed() {
  // Start callback chain here.
  HRESULT res = CreateCoreWebView2EnvironmentWithOptions(
      nullptr, user_data_path_, nullptr, environment_completed_handler_.Get());
  if (!SUCCEEDED(res)) {

    return false;
  }

  return true;
}

void EdgeChromiumBrowser::Resize() {
  if (wv2_controller_ == nullptr) {
    return;
  }
  RECT bounds;
  GetClientRect(window_, &bounds);
  wv2_controller_->put_Bounds(bounds);
}

void EdgeChromiumBrowser::Navigate(const std::string &url) {
  auto wurl = winrt::to_hstring(url);
  webview2_->Navigate(wurl.c_str());
}

void EdgeChromiumBrowser::OnDocumentCreate(const std::string &js) {
  auto wjs = winrt::to_hstring(js);
  webview2_->AddScriptToExecuteOnDocumentCreated(wjs.c_str(), nullptr);
}

void EdgeChromiumBrowser::EvalJS(const std::string &js, ResultCallback rs) {
  auto wjs = winrt::to_hstring(js);

  auto complete_handler = [rs](HRESULT errorCode,
                               LPCWSTR resultObjectAsJson) -> HRESULT {
    if (!SUCCEEDED(errorCode)) {
      return E_FAIL;
    }
    std::wstring ws(resultObjectAsJson);
    std::string json_str(ws.begin(), ws.end());
    auto j = nlohmann::json::parse(json_str);
    rs(j);
  };

  webview2_->ExecuteScript(
      wjs.c_str(),
      Callback<ICoreWebView2ExecuteScriptCompletedHandler>(complete_handler)
          .Get());
}

bool EdgeChromiumBrowser::SetFilePaths() {
  if (!SUCCEEDED(
          SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, user_data_path_))) {
    return false;
  }

  return true;
}

void EdgeChromiumBrowser::DispatchIn(DispatchFunction f) { f(); }

}  // namespace vstwebview
