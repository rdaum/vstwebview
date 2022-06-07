#pragma once

#include <public.sdk/source/common/threadchecker.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <nlohmann/json.hpp>

#include "vstwebview/bindings.h"
#include "vstwebview/webview.h"

using nlohmann::json;

namespace vstwebview {

class WebviewMessageListener;
class WebviewControllerBindings : public vstwebview::Bindings {
 public:
  explicit WebviewControllerBindings(
      Steinberg::Vst::EditControllerEx1 *controller);

  void Bind(vstwebview::Webview *webview) override;

 private:
  void DeclareJSBinding(const std::string &name,
                        vstwebview::Webview::FunctionBinding binding);

  using CallbackFn = json (WebviewControllerBindings::*)(
      vstwebview::Webview *webview, const json &);
  std::function<const json(vstwebview::Webview *webview, int seq,
                           const std::string &, const json &)>
  BindCallback(CallbackFn fn);

  json GetParameterObject(vstwebview::Webview *webview, const json &in);
  json GetParameterObjects(vstwebview::Webview *webview, const json &in);
  json SetParameterNormalized(vstwebview::Webview *webview, const json &in);
  json NormalizedParamToPlain(vstwebview::Webview *webview, const json &in);
  json GetParamNormalized(vstwebview::Webview *webview, const json &in);
  json BeginEdit(vstwebview::Webview *webview, const json &in);
  json PerformEdit(vstwebview::Webview *webview, const json &in);
  json EndEdit(vstwebview::Webview *webview, const json &in);
  json GetParameterCount(vstwebview::Webview *webview, const json &in);
  json GetSelectedUnit(vstwebview::Webview *webview, const json &in);
  json SelectUnit(vstwebview::Webview *webview, const json &in);
  json SubscribeParameter(vstwebview::Webview *webview, const json &in);
  json DoSendMessage(vstwebview::Webview *webview, const json &in);

  std::unique_ptr<Steinberg::Vst::ThreadChecker> thread_checker_;
  std::vector<std::pair<std::string, vstwebview::Webview::FunctionBinding>>
      bindings_;
  std::unique_ptr<Steinberg::IDependent> param_dep_proxy_;
  Steinberg::Vst::EditControllerEx1 *controller_;
};

class WebviewMessageListener {
 public:
  struct MessageAttribute {
    std::string name;
    enum class Type { INT, FLOAT, STRING, BINARY };
    Type type;
  };

  explicit WebviewMessageListener(vstwebview::Webview *webview)
      : webview_(webview) {}

  void Subscribe(const std::string &receiver, const std::string &message_id,
                 const std::vector<MessageAttribute> &attributes);

  Steinberg::tresult Notify(Steinberg::Vst::IMessage *message);

 private:
  struct MessageDescriptor {
    std::string message_id;
    std::vector<MessageAttribute> attributes;
  };

  struct MessageSubscription {
    MessageDescriptor descriptor;
    std::string notify_function;
  };

  json SerializeMessage(Steinberg::Vst::IMessage *message,
                        const MessageDescriptor &descriptor);

  std::unordered_map<std::string, MessageSubscription> subscriptions_;
  vstwebview::Webview *webview_;
};

}  // namespace vstwebview