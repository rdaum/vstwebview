# vstwebview

This package allows one to write VST3 plugin user interfaces using an an embedded webview, allowing HTML/CSS/JavaScript 
UIs instead of the VSTGUI framework. 

It includes JS binding facilities for piping EditController functionality through to the web layer. 

HTML is arguably a better crossplatform UI framework for the following reasons:

  * Performant
  * Maintained
  * Standard / lots of people know it
  * Plenty of framework options
  * Actually supports accessibility

Implementations exist for all 3 supported VST3 SDK platform:
  * Win32 using Edge/Chromium 
  * Linux (using webkit2 GTK 4.1)
  * and OS X (embedded Safari/WebKit)

In the demo/ directory I have ported the 'panner' VST3 sample VST as a very minimal and ugly example of how things work.

More elaborate UIs will generally require a JS framework of some kind, which generally will involve integrating NPM, 
etc. into your build. I will leave this as an exercise to the reader. 

For my own synthesizer, I integrated NPM and TypeScript into my CMake build and built a whole UI around this and created a series of TypeScript bindings to wrap the parameter model in VST3. As I clean this up and improve it, I will likely bring some of that over to this repository.

All of this is a bit early and rough. Contributions and testing welcome.
