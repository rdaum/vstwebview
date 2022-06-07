#include "vstwebview/webview.h"

#include <utility>

namespace vstwebview {

void Webview::BindFunction(const std::string &name,
                           Webview::FunctionBinding f) {
  auto js = "(function() { var name = '" + name + "';" + R"(
     var RPC = window._rpc = (window._rpc || {nextSeq: 1});
     window[name] = function() {
       var seq = RPC.nextSeq++;
       var promise = new Promise(function(resolve, reject) {
         RPC[seq] = {
           resolve: resolve,
           reject: reject,
         };
       });
       window.external.invoke(JSON.stringify({
         id: seq,
         method: name,
         params: Array.prototype.slice.call(arguments),
       }));
       return promise;
     }
   })())";
  OnDocumentCreate(js);
  bindings_[name] = FunctionBinding(std::move(f));
}

void Webview::UnbindFunction(const std::string &name) {
  if (bindings_.find(name) != bindings_.end()) {
    auto js = "delete window['" + name + "'];";
    OnDocumentCreate(js);
    EvalJS(js, [](const nlohmann::json &j) {});
    bindings_.erase(name);
  }
}

void Webview::ResolveFunctionDispatch(int seq, int status,
                                      const nlohmann::json &result) {
  DispatchIn([this, status, seq, result]() {
    if (status == 0) {
      EvalJS("window._rpc[" + std::to_string(seq) + "].resolve(" +
                 result.dump() + "); delete window._rpc[" +
                 std::to_string(seq) + "]",
             [](const nlohmann::json &j) {});
    } else {
      EvalJS("window._rpc[" + std::to_string(seq) + "].reject(" +
                 result.dump() + "); delete window._rpc[" +
                 std::to_string(seq) + "]",
             [](const nlohmann::json &j) {});
    }
  });
}

void Webview::OnBrowserMessage(const std::string &msg) {
  nlohmann::json msg_parsed = nlohmann::json::parse(msg);
  int seq = msg_parsed["id"];
  std::string name = msg_parsed["method"];
  nlohmann::json args = msg_parsed["params"];
  const auto &it = bindings_.find(name);
  if (it == bindings_.end()) {
    return;
  }
  auto result = it->second(this, seq, name, args);
  ResolveFunctionDispatch(seq, 0, result);
}

}  // namespace vstwebview