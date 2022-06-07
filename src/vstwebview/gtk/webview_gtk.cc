#include <JavaScriptCore/JavaScript.h>
#include <X11/X.h>

#include <thread>
#define GNU_SOURCE
#include <dlfcn.h>
#include <gtk-3.0/gtk/gtk.h>
#include <gtk-3.0/gtk/gtkx.h>
#include <webkit2/webkit2.h>

#include "base/source/fobject.h"
#include "vstwebview/webview.h"

static unsigned int kAddressMarker = 0xcafebabe;

namespace vstwebview {

class WebviewWebkitGTK : public Webview,
                         public Steinberg::Linux::ITimerHandler,
                         public Steinberg::FObject {
 public:
  WebviewWebkitGTK(bool debug, Steinberg::IPlugFrame *plug_frame,
                   Window x11Parent, WebviewCreatedCallback created_callback) {
    // On linux the IPlugFrame is also a "run loop" we can use to schedule
    // timers and file-descriptor triggered events.
    run_loop_ = plug_frame;

    gtk_init_check(nullptr, nullptr);

    window_ = gtk_plug_new(x11Parent);

    g_signal_connect(G_OBJECT(window_), "destroy",
                     G_CALLBACK(+[](GtkWidget *, gpointer arg) {
                       static_cast<WebviewWebkitGTK *>(arg)->Terminate();
                     }),
                     this);

    MakeWebView(debug);
    OnDocumentCreate(
        "window.external={invoke:function(s){window.webkit.messageHandlers."
        "external.postMessage(s);}}");

    gtk_container_add(GTK_CONTAINER(window_), webview_);
    gtk_widget_grab_focus(webview_);
    gtk_widget_show_all(window_);

    created_callback(this);

    // Call into GTK main loop check 60 times a second.
    run_loop_->registerTimer(this, 16);
  }

  std::string ContentRootURI() const override {
    std::string resPath;
    Dl_info info;

    if (dladdr(&kAddressMarker, &info) != 0) {
      auto path =
          std::filesystem::absolute(std::filesystem::path(info.dli_fname));
      auto content_path = path.relative_path().parent_path().parent_path();
      return "file:///" + content_path.generic_string() + "/Resources/";
    }
    return "";
  }

  void OnDocumentCreate(const std::string &js) override {
    WebKitUserContentManager *manager =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_));
    webkit_user_content_manager_add_script(
        manager,
        webkit_user_script_new(js.c_str(), WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
                               WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
                               nullptr, nullptr));
  }

  void SetTitle(const std::string &title) override {
    gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
  }

  void SetViewSize(int width, int height, SizeHint hints) override {
    gtk_window_set_resizable(GTK_WINDOW(window_), hints != SizeHint::kFixed);
    if (hints == SizeHint::kNone) {
      gtk_window_resize(GTK_WINDOW(window_), width, height);
    } else if (hints == SizeHint::kFixed) {
      gtk_widget_set_size_request(window_, width, height);
    } else {
      GdkGeometry g;
      g.min_width = g.max_width = width;
      g.min_height = g.max_height = height;
      GdkWindowHints h =
          (hints == SizeHint::kMin ? GDK_HINT_MIN_SIZE : GDK_HINT_MAX_SIZE);
      // This defines either MIN_SIZE, or MAX_SIZE, but not both:
      gtk_window_set_geometry_hints(GTK_WINDOW(window_), nullptr, &g, h);
    }
  }

  void Navigate(const std::string &url) override {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview_), url.c_str());
  }

  void EvalJS(const std::string &js, ResultCallback rs) override {
    webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(webview_), js.c_str(),
                                   nullptr, nullptr, nullptr);
  }

  void *PlatformWindow() const override { return window_; }
  void Terminate() override { gtk_main_quit(); }

  DELEGATE_REFCOUNT(Steinberg::FObject)
  DEFINE_INTERFACES
  DEF_INTERFACE(Steinberg::Linux::ITimerHandler)
  END_DEFINE_INTERFACES(Steinberg::FObject)

 protected:
  void DispatchIn(DispatchFunction f) override { f(); }

 private:
  void onTimer() override {
    while (gtk_events_pending()) gtk_main_iteration();
  }

  void MakeWebView(bool debug) {
    webview_ = webkit_web_view_new();
    WebKitUserContentManager *manager =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_));

    g_signal_connect(manager, "script-message-received::external",
                     G_CALLBACK(+[](WebKitUserContentManager *,
                                    WebKitJavascriptResult *r, gpointer arg) {
                       auto *w = static_cast<WebviewWebkitGTK *>(arg);
                       JSCValue *value =
                           webkit_javascript_result_get_js_value(r);
                       char *s = jsc_value_to_string(value);
                       w->OnBrowserMessage(s);
                       g_free(s);
                     }),
                     this);
    webkit_user_content_manager_register_script_message_handler(manager,
                                                                "external");

    WebKitSettings *settings = webkit_settings_new();

    webkit_settings_set_allow_file_access_from_file_urls(settings, true);
    webkit_settings_set_allow_universal_access_from_file_urls(settings, true);
    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_allow_modal_dialogs(settings, true);

    if (debug) {
      webkit_settings_set_enable_write_console_messages_to_stdout(settings,
                                                                  true);
      webkit_settings_set_enable_developer_extras(settings, true);
    }
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webview_), settings);
  }

  Steinberg::FUnknownPtr<Steinberg::Linux::IRunLoop> run_loop_;

  GtkWidget *window_;
  GtkWidget *webview_;
};

// static
std::unique_ptr<Webview> MakeWebview(bool debug,
                                     Steinberg::IPlugFrame *plug_frame,
                                     void *window,
                                     WebviewCreatedCallback created_cb) {
  auto x11Parent = reinterpret_cast<XID>(window);

  auto webview = std::make_unique<WebviewWebkitGTK>(debug, plug_frame,
                                                    x11Parent, created_cb);
  return std::move(webview);
}

}  // namespace vstwebview