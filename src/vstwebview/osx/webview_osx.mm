/*
 * Copyright 2022 Noah Schairer
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

#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include "base/source/fobject.h"
#include "vstwebview/webview.h"

// Largely a port of https://github.com/webview/webview
// Extended to run in an existing NSApplication

// OSX Imports
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc-runtime.h>

#define NSBackingStoreBuffered 2
#define NSWindowStyleMaskResizable 8
#define NSWindowStyleMaskMiniaturizable 4
#define NSWindowStyleMaskTitled 1
#define NSWindowStyleMaskClosable 2
#define NSApplicationActivationPolicyRegular 0
#define WKUserScriptInjectionTimeAtDocumentStart 0

id operator"" _cls(const char *s, std::size_t) { return (id)objc_getClass(s); }
SEL operator"" _sel(const char *s, std::size_t) { return sel_registerName(s); }
id operator"" _str(const char *s, std::size_t) {
  return ((id(*)(id, SEL, const char *))objc_msgSend)("NSString"_cls, "stringWithUTF8String:"_sel,
                                                      s);
}

namespace vstwebview {
class WebviewOSX : public vstwebview::Webview {
 public:
  WebviewOSX(bool debug, Steinberg::IPlugFrame *plug_frame, id parentView,
             WebviewCreatedCallback created) {
    Class cls = objc_getClass("WebviewDelegate");
    if (cls == NULL) {
      cls = objc_allocateClassPair((Class) "NSResponder"_cls, "WebviewDelegate", 0);
      class_addMethod(cls, "userContentController:didReceiveScriptMessage:"_sel,
                      (IMP)(+[](id self, SEL, id, id msg) {
                        auto w = (WebviewOSX *)objc_getAssociatedObject(self, "webview");
                        assert(w);
                        w->OnBrowserMessage(((const char *(*)(id, SEL))objc_msgSend)(
                            ((id(*)(id, SEL))objc_msgSend)(msg, "body"_sel), "UTF8String"_sel));
                      }),
                      "v@:@@");
      objc_registerClassPair(cls);
    }

    auto delegate = ((id(*)(id, SEL))objc_msgSend)((id)cls, "new"_sel);
    objc_setAssociatedObject(delegate, "webview", (id)this, OBJC_ASSOCIATION_ASSIGN);

    // Webview config
    auto config = ((id(*)(id, SEL))objc_msgSend)("WKWebViewConfiguration"_cls, "new"_sel);

    m_manager = ((id(*)(id, SEL))objc_msgSend)(config, "userContentController"_sel);

    ((void (*)(id, SEL, id, id))objc_msgSend)(m_manager, "addScriptMessageHandler:name:"_sel,
                                              delegate, "external"_str);

    if (debug) {
      ((id(*)(id, SEL, id, id))objc_msgSend)(
          ((id(*)(id, SEL))objc_msgSend)(config, "preferences"_sel), "setValue:forKey:"_sel,
          ((id(*)(id, SEL, BOOL))objc_msgSend)("NSNumber"_cls, "numberWithBool:"_sel, 1),
          "developerExtrasEnabled"_str);
    }
    // [[config preferences] setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
    ((id(*)(id, SEL, id, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)(config, "preferences"_sel), "setValue:forKey:"_sel,
        ((id(*)(id, SEL, BOOL))objc_msgSend)("NSNumber"_cls, "numberWithBool:"_sel, 1),
        "javaScriptCanAccessClipboard"_str);

    // [[config preferences] setValue:@YES forKey:@"DOMPasteAllowed"];
    ((id(*)(id, SEL, id, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)(config, "preferences"_sel), "setValue:forKey:"_sel,
        ((id(*)(id, SEL, BOOL))objc_msgSend)("NSNumber"_cls, "numberWithBool:"_sel, 1),
        "DOMPasteAllowed"_str);

    // Create webview
    webview_ = ((id(*)(id, SEL))objc_msgSend)("WKWebView"_cls, "alloc"_sel);

    OnDocumentCreate(R"script(
                          window.external = {
                            invoke: function(s) {
                              window.webkit.messageHandlers.external.postMessage(s);
                            },
                          };
                         )script");

    ((void (*)(id, SEL, CGRect, id))objc_msgSend)(webview_, "initWithFrame:configuration:"_sel,
                                                  CGRectMake(0, 0, 100, 100), config);

    ((void (*)(id, SEL, id))objc_msgSend)(parentView, "addSubview:"_sel, webview_);

    window_ = parentView;

    created(this);
  }
  static char *plugin_path(void) {
    Dl_info info;
    if (dladdr((const char *)plugin_path, &info) != 0) {
      return strdup(info.dli_fname);
    } else {
      return nullptr;
    }
  }
  std::string ContentRootURI() const override {
    std::string path = plugin_path();
    std::string resources = "Resources";

    CFURLRef url = CFURLCreateWithString(
        NULL, CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingUTF8), NULL);
    url = CFURLCreateCopyDeletingLastPathComponent(NULL, url);
    url = CFURLCreateCopyDeletingLastPathComponent(NULL, url);
    url = CFURLCreateCopyAppendingPathComponent(
        NULL, url, CFStringCreateWithCString(NULL, resources.c_str(), kCFStringEncodingUTF8),
        false);

    auto utf16length = CFStringGetLength(CFURLGetString(url));
    CFIndex maxUtf8len = CFStringGetMaximumSizeForEncoding(utf16length, kCFStringEncodingUTF8);

    char buffer[maxUtf8len];
    CFStringGetCString(CFURLGetString(url), buffer, maxUtf8len, kCFStringEncodingUTF8);

    // XXX relative path maybe?
    return "file://" + std::string(buffer);
  }
  void OnDocumentCreate(const std::string &js) override {
    ((void (*)(id, SEL, id))objc_msgSend)(
        m_manager, "addUserScript:"_sel,
        ((id(*)(id, SEL, id, long, BOOL))objc_msgSend)(
            ((id(*)(id, SEL))objc_msgSend)("WKUserScript"_cls, "alloc"_sel),
            "initWithSource:injectionTime:forMainFrameOnly:"_sel,
            ((id(*)(id, SEL, const char *))objc_msgSend)("NSString"_cls,
                                                         "stringWithUTF8String:"_sel, js.c_str()),
            WKUserScriptInjectionTimeAtDocumentStart, 1));
  }
  void SetTitle(const std::string &title) override {
    // XXX TODO - I don't think this is necessary since not creating a new window
  }
  void SetViewSize(int width, int height, SizeHint hints) override {
    // XXX TODO - make use of size hints -- again might not be necessary since its a subview..
    CGSize size = CGSizeMake(width, height);
    ((id(*)(id, SEL, CGSize))objc_msgSend)(webview_, "setFrameSize:"_sel, size);
  }
  void Navigate(const std::string &url) override {
    auto nsurl = ((id(*)(id, SEL, id))objc_msgSend)(
        "NSURL"_cls, "URLWithString:"_sel,
        ((id(*)(id, SEL, const char *))objc_msgSend)("NSString"_cls, "stringWithUTF8String:"_sel,
                                                     url.c_str()));

    ((void (*)(id, SEL, id))objc_msgSend)(
        webview_, "loadRequest:"_sel,
        ((id(*)(id, SEL, id))objc_msgSend)("NSURLRequest"_cls, "requestWithURL:"_sel, nsurl));
  }
  void *PlatformWindow() const override { return window_; };
  void Terminate() override {
    // XXX TODO - Might not be necessary since a sub view not an actual window
  }
  void EvalJS(const std::string &js, ResultCallback rs) override {
    auto foo = ^(id ret, id err) {
      if (ret == NULL) {
        /* Could be useful!
        char * errorText =
        ((char*(*)(id, SEL))objc_msgSend)(
            ((id(*)(id, SEL))objc_msgSend)(err, "localizedDescription"_sel),
                                          "UTF8String"_sel);
         */
        return;
      } else {
        //XXX TODO - Parse JSON correctly based on Obj-C type, using NSJSONSerialization
        //char *value = ((char *(*)(id, SEL))objc_msgSend)(ret, "UTF8String"_sel);
        //rs(value);
      }
    };

    ((void (*)(id, SEL, id, dispatch_block_t))objc_msgSend)(
        webview_, "evaluateJavaScript:completionHandler:"_sel,
        ((id(*)(id, SEL, const char *))objc_msgSend)("NSString"_cls, "stringWithUTF8String:"_sel,
                                                     js.c_str()),
        (dispatch_block_t)foo);
  }

 protected:
  void DispatchIn(vstwebview::DispatchFunction f) override { f(); }

 private:
  id window_;
  id webview_;
  id m_manager;
};

std::unique_ptr<Webview> MakeWebview(bool debug, Steinberg::IPlugFrame *plug_frame, void *window,
                                     WebviewCreatedCallback created_cb) {
  auto parentView = reinterpret_cast<id>(window);
  auto webview = std::make_unique<WebviewOSX>(debug, plug_frame, parentView, created_cb);
  return std::move(webview);
}
}
