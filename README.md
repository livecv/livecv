![Live CV](/doc/src/images/logo-dark.png)

 * **Version**: 1.2.0
 * **License**: LGPL
 * **Website**: [livecv.dinusv.com](http://livecv.dinusv.com)
 * **Demo**: [www.youtube.com/watch?v=uEnJE6Jawfw](https://www.youtube.com/watch?v=uEnJE6Jawfw)
 * **Documentation**: [livecv.dinusv.com/documentation](http://livecv.dinusv.com/documentation)
 * **Build Requirements**:
   * Qt 5.3 or higher
   * Open CV 2.4 or higher

Live CV is a computer vision coding environment that displays results while you code. It can be used to interact, link and adjust algorithms 
in order to create quick solutions within it's domain. 

The application is built using QtQuick, and uses Open CV library in the back to do the actual processing, kind of like a QML wrapper for the 
Open CV library. 

An API is also available to link already built algorithms to Live CV's interface by simply exposing configurable variables to Qt's meta object system.

## Installation and getting started

To get started, simply download the application from Live CV's [website](http://livecv.dinusv.com/download.html), extract the archive, and you're ready 
to go. Step by step instructions are also found in the download section.

## Compiling

Live CV requires qt and open cv libraries to compile. You can use qmake to generate the makefile, then run make or nmake to build:

```
cd build
qmake -r
make (or nmake)
```

The executable and plugins will be found in the application directory.

## Contributing

There's an [organization on github](http://github.com/livecv) created for Live CV plugins and projects, so anyone looking to contribute is always welcome. 
Live CV is 90% based on plugins, the main application providing just the code editor and result viewer, therefore plugins can be developed either from within
the application or separtely and integrated after. Computer vision based plugins link to live cv library, which provides the interface between live cv and qml.
This [article](http://livecv.dinusv.com/documentation/api-configuration.html) describes how to build a live cv c++ plugin from scratch.

There's a forum available for discussions on [live cv's website](http://livecv.dinusv.com/forum).

## Roadmap

The feature detection module is to be added in version 1.2, after which the following versions are scheduled:

 * Version 1.3 - Scripting module
    * Posibility to execute qml scripts with live cv: ```livecv extract-background.qml```
    * Project files
    * VideoWriter and ImWrite
    * Text files ins and outs
    * Editor search functionality and file navigation
    * Argument forwarding to qml
 * Version 1.4 - Photography module
    * Exposure and contrast configuration
    * Photo stiching
    * HDR Merging
    * Levels and histograms

