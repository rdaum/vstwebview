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
