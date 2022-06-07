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

#include <atomic>
#include <cstring>
#include <functional>
#include <future>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

#include "pluginterfaces/gui/iplugview.h"

namespace vstwebview {

using DispatchFunction = std::function<void()>;

// Abstract webview parent.
class Webview {
 public:
  virtual ~Webview() = default;

  /**
   * Create a JavaScript function ('name') that invokes native function 'f'
   * and returns a Promise with its results.
   */
  using FunctionBinding = std::function<const nlohmann::json(
      Webview *webview, int seq, const std::string &, const nlohmann::json &)>;
  void BindFunction(const std::string &name, FunctionBinding f);

  /**
   * Unbind a previously-bound JavaScript function.
   */
  void UnbindFunction(const std::string &name);

  /**
   * Set the webview document title.
   */
  virtual void SetTitle(const std::string &title) = 0;

  /*
   * Adjust the size of the webview.
   */
  enum class SizeHint {
    kNone,  // Width and height are default size
    kMin,   // Width and height are minimum bounds
    kMax,   // Width and height are maximum bounds
    kFixed  // Window size can not be changed by a user
  };
  virtual void SetViewSize(int width, int height,
                           SizeHint hints = SizeHint::kNone) = 0;

  virtual std::string ContentRootURI() const = 0;

  /**
   * Navigate the webview to a URL.
   */
  virtual void Navigate(const std::string &url) = 0;

  /*
   * Evaluate a fragment of JS in the webview.
   */
  using ResultCallback = std::function<void(const nlohmann::json &)>;
  virtual void EvalJS(const std::string &js, ResultCallback rs) = 0;

  /*
   * Set a fragment of JS to execute when the webview first loads a document.
   */
  virtual void OnDocumentCreate(const std::string &js) = 0;

  /*
   * Returns the handle for the platform window hosting the webview.
   */
  virtual void *PlatformWindow() const = 0;

  /*
   * Terminate the webview execution.
   */
  virtual void Terminate() = 0;

 protected:
  void OnBrowserMessage(const std::string &msg);
  virtual void DispatchIn(DispatchFunction f) = 0;

 private:
  void ResolveFunctionDispatch(int seq, int status,
                               const nlohmann::json &result);

  std::map<std::string, FunctionBinding> bindings_;
  ;
};

using WebviewCreatedCallback = std::function<void(Webview *)>;
std::unique_ptr<Webview> MakeWebview(bool debug,
                                     Steinberg::IPlugFrame *plug_frame,
                                     void *window,
                                     WebviewCreatedCallback created_cb);

}  // namespace vstwebview
