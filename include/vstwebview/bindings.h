#pragma once

namespace vstwebview {

class Webview;

/**
 * Interface for bindings implementations.
 * Each instance is called on webview initialization, to permit the installation
 * of custom JS bindings into the webview.
 */
class Bindings {
 public:
  virtual void Bind(vstwebview::Webview *webview) = 0;
};
}  // namespace vstwebview
