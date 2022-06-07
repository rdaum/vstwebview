#include "vstwebview/webview_controller_bindings.h"

#include <public.sdk/source/vst/utility/stringconvert.h>

#include <codecvt>
#include <locale>

#include "pluginterfaces/base/ustring.h"

namespace vstwebview {

namespace {

json SerializeParameter(Steinberg::Vst::Parameter *param) {
  auto &info = param->getInfo();
  json j = {
      {"normalized", param->getNormalized()},
      {"precision", param->getUnitID()},
      {"unitID", param->getUnitID()},
      {"info",
       {
           {"id", info.id},
           {"title", VST3::StringConvert::convert(info.title)},
           {"stepCount", info.stepCount},
           {"flags", info.flags},
           {"defaultNormalizedValue", info.defaultNormalizedValue},
           {"units", info.units},
           {"shortTitle", VST3::StringConvert::convert(info.shortTitle)},
       }},
  };
  bool isRangeParameter =
      (param->isA(Steinberg::Vst::RangeParameter::getFClassID()));
  j["isRangeParameter"] = isRangeParameter;
  if (isRangeParameter) {
    auto *range_param = dynamic_cast<Steinberg::Vst::RangeParameter *>(param);
    j["min"] = range_param->getMin();
    j["max"] = range_param->getMax();
  }
  return j;
}

// Proxy IDependent through the webview for parameter object changes.
class ParameterDependenciesProxy : public Steinberg::FObject {
 public:
  explicit ParameterDependenciesProxy(vstwebview::Webview *webview)
      : webview_(webview) {}

  void update(FUnknown *changedUnknown, Steinberg::int32 message) override {
    if (!webview_ || message != IDependent::kChanged) return;

    Steinberg::Vst::Parameter *changed_param;
    auto query_result = changedUnknown->queryInterface(
        Steinberg::Vst::RangeParameter::iid, (void **)&changed_param);

    if (query_result != Steinberg::kResultOk) return;

    webview_->EvalJS("notifyParameterChange(" +
                         SerializeParameter(changed_param).dump() + ");",
                     [](const json &r) {});
  }

 private:
  vstwebview::Webview *webview_;
};

}  // namespace

void WebviewMessageListener::Subscribe(
    const std::string &receiver, const std::string &message_id,
    const std::vector<MessageAttribute> &attributes) {
  subscriptions_[message_id] = {message_id, attributes, receiver};
}

json WebviewMessageListener::SerializeMessage(
    Steinberg::Vst::IMessage *message,
    const WebviewMessageListener::MessageDescriptor &descriptor) {
  auto attributes = message->getAttributes();

  json j = {{"messageId", message->getMessageID()}};
  for (const auto &attr : descriptor.attributes) {
    switch (attr.type) {
      case MessageAttribute::Type::INT:
        Steinberg::int64 i;
        if (attributes->getInt(attr.name.c_str(), i) ==
            Steinberg::kResultTrue) {
          j[attr.name] = i;
        }
        break;
      case MessageAttribute::Type::FLOAT:
        double f;
        if (attributes->getFloat(attr.name.c_str(), f) ==
            Steinberg::kResultTrue) {
          j[attr.name] = f;
        }
        break;
      case MessageAttribute::Type::STRING:
        Steinberg::Vst::TChar str[128];
        if (attributes->getString(attr.name.c_str(), str,
                                  128 * sizeof(Steinberg::Vst::TChar)) ==
            Steinberg::kResultTrue) {
          j[attr.name] = str;
        }
        break;
      case MessageAttribute::Type::BINARY:
        const void *addr;
        Steinberg::uint32 size;
        if (attributes->getBinary(attr.name.c_str(), addr, size) ==
            Steinberg::kResultTrue) {
          std::vector<double> data(size / sizeof(double));
          std::memcpy(data.data(), addr, size);
          j[attr.name] = data;
        }
        break;
    }
  }
  return j;
}

Steinberg::tresult WebviewMessageListener::Notify(
    Steinberg::Vst::IMessage *message) {
  auto *msg_id = message->getMessageID();

  const auto &it = subscriptions_.find(msg_id);
  if (it == subscriptions_.end()) return Steinberg::kResultFalse;

  auto json = SerializeMessage(message, it->second.descriptor);
  auto send_message_js = it->second.notify_function + "(" + json.dump() + ");";
  webview_->EvalJS(send_message_js, [](const nlohmann::json &res) {});
  return Steinberg::kResultOk;
}

WebviewControllerBindings::WebviewControllerBindings(
    Steinberg::Vst::EditControllerEx1 *controller)
    : thread_checker_(Steinberg::Vst::ThreadChecker::create()),
      controller_(controller) {
  DeclareJSBinding(
      "getParameterObject",
      BindCallback(&WebviewControllerBindings::GetParameterObject));
  DeclareJSBinding(
      "getParameterObjects",
      BindCallback(&WebviewControllerBindings::GetParameterObjects));
  DeclareJSBinding(
      "subscribeParameter",
      BindCallback(&WebviewControllerBindings::SubscribeParameter));
  DeclareJSBinding(
      "setParamNormalized",
      BindCallback(&WebviewControllerBindings::SetParameterNormalized));
  DeclareJSBinding(
      "normalizedParamToPlain",
      BindCallback(&WebviewControllerBindings::NormalizedParamToPlain));
  DeclareJSBinding(
      "getParamNormalized",
      BindCallback(&WebviewControllerBindings::GetParamNormalized));
  DeclareJSBinding("beginEdit",
                   BindCallback(&WebviewControllerBindings::BeginEdit));
  DeclareJSBinding("performEdit",
                   BindCallback(&WebviewControllerBindings::PerformEdit));
  DeclareJSBinding("endEdit",
                   BindCallback(&WebviewControllerBindings::EndEdit));
  DeclareJSBinding("getParameterCount",
                   BindCallback(&WebviewControllerBindings::GetParameterCount));
  DeclareJSBinding("getSelectedUnit",
                   BindCallback(&WebviewControllerBindings::GetSelectedUnit));
  DeclareJSBinding("selectUnit",
                   BindCallback(&WebviewControllerBindings::SelectUnit));
  DeclareJSBinding("sendMessage",
                   BindCallback(&WebviewControllerBindings::DoSendMessage));
}

void WebviewControllerBindings::Bind(vstwebview::Webview *webview) {
  for (auto &binding : bindings_) {
    webview->BindFunction(binding.first, binding.second);
  }
}

vstwebview::Webview::FunctionBinding WebviewControllerBindings::BindCallback(
    CallbackFn fn) {
  return std::bind(fn, this, std::placeholders::_1, std::placeholders::_4);
}

// TODO parameter validation on all these JSON argument retrievals. Or crash.

json WebviewControllerBindings::GetParameterObject(vstwebview::Webview *webview,
                                                   const json &in) {
  thread_checker_->test();
  int id = in[0];
  auto *param = controller_->getParameterObject(id);
  if (!param) return json();
  return SerializeParameter(param);
}

json WebviewControllerBindings::GetParameterObjects(
    vstwebview::Webview *webview, const json &in) {
  thread_checker_->test();
  json out;
  for (int id : in[0]) {
    auto *param = controller_->getParameterObject(id);
    if (param) {
      out[id] = SerializeParameter(param);
    }
  }
  return out;
}

json WebviewControllerBindings::SetParameterNormalized(
    vstwebview::Webview *webview, const json &in) {
  thread_checker_->test();
  int tag = in[0];
  double value = in[1];
  json out =
      controller_->setParamNormalized(tag, value) == Steinberg::kResultOk;
  return out;
}

json WebviewControllerBindings::NormalizedParamToPlain(
    vstwebview::Webview *webview, const json &in) {
  thread_checker_->test();
  int tag = in[0];
  double value = in[1];
  json out = controller_->normalizedParamToPlain(tag, value);
  return out;
}

json WebviewControllerBindings::GetParamNormalized(vstwebview::Webview *webview,
                                                   const json &in) {
  thread_checker_->test();
  int tag = in[0];
  json out = controller_->getParamNormalized(tag);
  return out;
}

json WebviewControllerBindings::BeginEdit(vstwebview::Webview *webview,
                                          const json &in) {
  thread_checker_->test();
  int tag = in[0];
  json out = controller_->beginEdit(tag) == Steinberg::kResultOk;
  return out;
}

json WebviewControllerBindings::PerformEdit(vstwebview::Webview *webview,
                                            const json &in) {
  thread_checker_->test();
  int tag = in[0];
  double value = in[1];
  json out = controller_->performEdit(tag, value) == Steinberg::kResultOk;
  return out;
}

json WebviewControllerBindings::EndEdit(vstwebview::Webview *webview,
                                        const json &in) {
  thread_checker_->test();
  int tag = in[0];
  json out = controller_->endEdit(tag) == Steinberg::kResultOk;
  return out;
}

json WebviewControllerBindings::GetParameterCount(vstwebview::Webview *webview,
                                                  const json &in) {
  thread_checker_->test();
  json out = controller_->getParameterCount();
  return out;
}

json WebviewControllerBindings::GetSelectedUnit(vstwebview::Webview *webview,
                                                const json &in) {
  thread_checker_->test();
  json out = controller_->getSelectedUnit();
  return out;
}

json WebviewControllerBindings::SelectUnit(vstwebview::Webview *webview,
                                           const json &in) {
  thread_checker_->test();
  json out = controller_->selectUnit(in[0]);
  return out;
}

json WebviewControllerBindings::SubscribeParameter(vstwebview::Webview *webview,
                                                   const json &in) {
  thread_checker_->test();
  json out = controller_->getParameterCount();
  int tag = in[0];
  auto *param = controller_->getParameterObject(tag);
  if (!param) return json();
  if (!param_dep_proxy_) {
    param_dep_proxy_ = std::make_unique<ParameterDependenciesProxy>(webview);
  }
  param->addDependent(param_dep_proxy_.get());
  return true;
}

json WebviewControllerBindings::DoSendMessage(vstwebview::Webview *webview,
                                              const json &in) {
  thread_checker_->test();
  std::string messageId = in[0];
  json attributes = in[1];
  if (auto msg = owned(controller_->allocateMessage())) {
    msg->setMessageID(messageId.c_str());
    auto msg_attrs = msg->getAttributes();
    for (const auto &k : attributes.items()) {
      auto type = k.value().type();
      auto attr_id = k.key().c_str();
      if (type == json::value_t::number_float) {
        msg_attrs->setFloat(attr_id, k.value());
      } else if (type == json::value_t::number_integer ||
                 type == json::value_t::number_unsigned ||
                 type == json::value_t::boolean) {
        msg_attrs->setInt(attr_id, k.value());
      } else if (type == json::value_t::string) {
        std::string str = k.value();
        Steinberg::Vst::TChar t_char_str[128];
        VST3::StringConvert::convert(str, t_char_str);
        msg_attrs->setString(attr_id, t_char_str);
      } else if (type == json::value_t::binary) {
        json::binary_t b = k.value();
        msg_attrs->setBinary(attr_id, b.data(), b.size());
      }
    }
    controller_->sendMessage(msg);
  }

  return true;
}

void WebviewControllerBindings::DeclareJSBinding(
    const std::string &name, vstwebview::Webview::FunctionBinding binding) {
  bindings_.push_back({name, binding});
}

}  // namespace vstwebview