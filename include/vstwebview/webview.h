#pragma once

/*
 * MIT License
 *
 * Copyright (c) 2017 Serge Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
