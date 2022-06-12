# vstwebview

This package allows one to write VST3 plugin user interfaces using an embedded webview, allowing HTML/CSS/JavaScript 
UIs instead of Steinberg's bespoke VSTGUI framework. 

It includes JS binding facilities for piping EditController functionality through to the web layer. 

So: Build your plugin in C++, and then use vstwebview to write your UI layer.

## Why?

VSTGUI is Steinberg's answer to the need for a cross platform UI framework for developing VST3 plugins. It has its own implementation of a bunch of standard controls, its own UI markup language, and its own WYSIWYG editing tool.

However it's all bespoke, non-standard, non-native, and is missing a lot of common UI things. It has no accessibility aspects at all. One inevitably has to write custom components. And when one does that one ends up having to use Steinberg's custom cross-platform graphics drawing library.

And then frustration arises.

Frankly, in this day and age, despite its numerous warts, browser tech is arguably the most crossplatform UI framework.

Using an embedded webview is:

  * Performant
  * Maintained
  * Standard / lots of people know it
  * Plenty of JS etc. framework options
  * Actually supports accessibility.

(aside ... In my past life @ Google, I worked a bit in the Chromium codebase and I've seen how the sausage is made. I personally think browser tech is solid miracle engineering at this point.)

Implementations exist for all 3 supported VST3 SDK platform:
  * Win32 using Edge/Chromium 
  * Linux (using webkit2 GTK 4.1)
  * and OS X (embedded Safari/WebKit)

In the demo/ directory I have ported the 'panner' VST3 sample VST as a very minimal and ugly example of how things work.

More elaborate UIs will generally require a JS framework of some kind, which generally will involve integrating NPM, 
etc. into your build. I will leave this as an exercise to the reader. 

For my own synthesizer, I integrated NPM and TypeScript into my CMake build and built a whole UI around this and created a series of TypeScript bindings to wrap the parameter model in VST3. As I clean this up and improve it, I will likely bring some of that over to this repository.

All of this is a bit early and rough. Contributions and testing welcome.
