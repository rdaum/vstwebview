# vstwebview

This package allows one to write of VST3 plugin user interfaces using an an embedded webview using HTML/CSS/JavaScript 
instead of the VSTGUI framework. 

It includes JS binding facilities for piping EditController functionality through to the web layer. 

Implementations exist for all 3 supported VST3 SDK platform:
  * Win32 using Edge/Chromium 
  * Linux (using webkit2 GTK 4.1)
  * and OS X (embedded Safari/WebKit)

In the demo/ directory I have ported the 'panner' VST3 sample VST as a very minimal and ugly example of how things work.

More elaborate UIs will generally require a JS framework of some kind, which generally will involve integrating NPM, 
etc. into your build. I will leave this as an exercise to the reader.

This is still a bit early and rough. Contributions and testing welcome.
