WaylandGUI
========================================================================================

.. begin_brief_description

WaylandGUI is a minimalistic based on NanoGUI.

https://github.com/mitsuba-renderer/nanogui

That library tried to be all things to all people in a minimalist way which is noble,
but kind of crippled the embedded systems world, especially after Metal support was
added.

This project stripped out most of the unnecessary stuff. It focuses only on SOM
(System On a Module) based ARM devices running Debian 11 (or other Linux of similar
vintage). X11 is being phased out on these platforms and the standard build dependencies
bring in libglvnd which causes major issues with xWayland and Wayland. To save time
during the initial proof of concept the source forces GLES 3. Later versions will be
Wayland EGL only, or at least that is the plan.

It is known that example1 and example4 have issues due to Shader.

This library is being maintained by one person: Roland Hughes, in his spare time. A
client did fund part of the effort because we needed a version of NanoGUI to run on
Variscite SOM hardware. We also wanted one to run on Toradex that didn't need X11. As
such, the only widgets I will feel compelled to code myself will be the basic ones I
need for projects. If you need additional widgets for your projects generate a pull
request with your fully functioning and tested widget code.

I left the existing coding style in place for the first version. I do not like this
style as it violates the BAAR Group coding standard used in most embedded systems.
There is a .astylerc file added to this project and soon all .cpp and .h files will
conform to it.

Minimum compiler coding standard is still C++17.

**Python has been dropped.** Embedded systems for safety critical environments use C/C++.

.. _BARR Group: https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard
.. _NanoVG: https://github.com/memononen/NanoVG
.. _NanoGUI: https://github.com/wjakob/waylandgui
.. _Entypo: http://www.entypo.com
.. _FontAwesome: https://github.com/FortAwesome/Font-Awesome

.. end_brief_description

.. contents:: Contents
   :local:
   :backlinks: none

Example screenshot
----------------------------------------------------------------------------------------

.. image:: https://github.com/wjakob/waylandgui/raw/master/resources/screenshot.png
   :alt: Screenshot of Example 1.
   :align: center

Description
----------------------------------------------------------------------------------------

.. begin_long_description

This library only cares about embedded Debian based systems. It will not be
accepting changes to support Windows, Apple, or anything else. Most of the Yocto builds
out there are "Debian-like" or "Debian-based" so the Yocto people should be able to
use this too.

Quite simply many devices, particularly in medical and system control worlds don't
need complex GUIs with intense graphics capabilities.

As much of the codebase as could be removed was for this first version:

- Python was dropped.
- Apple was dropped.
- Windows was dropped.
- GLES 3 is now used for the GLFW layer, soon that it will be EGL only.
- We don't include GLFW, you must build from the official repository.  

The goal here is to support a single set of platforms well. The more libraries that can
be removed the easier it is to get your project through regulatory approval. At some point
this project will look at using EGL directly in Wayland removing one more library. It may
also look into using Vulcan.

NanoGUI was a fantastic effort, don't get me wrong. The problem was the build dependencies
pulled in libglvnd which doesn't play nice in the Wayland world.

.. _GLFW: http://www.glfw.org/

.. end_long_description

Creating widgets
----------------------------------------------------------------------------------------

Previous NanoGUI documentation currently still applies for widget creation.

.. code-block:: cpp

   Button *b = new Button(window, "Plain button");
   b->set_callback([] { cout << "pushed!" << endl; });


The following lines from the example application create the coupled
slider and text box on the bottom of the second window (see the screenshot).

.. code-block:: cpp

   /* Create an empty panel with a horizontal layout */
   Widget *panel = new Widget(window);
   panel->set_layout(new BoxLayout(BoxLayout::Horizontal, BoxLayout::Middle, 0, 20));

   /* Add a slider and set defaults */
   Slider *slider = new Slider(panel);
   slider->set_value(0.5f);
   slider->set_fixed_width(80);

   /* Add a textbox and set defaults */
   TextBox *tb = new TextBox(panel);
   tb->set_fixed_size(Vector2i(60, 25));
   tb->set_value("50");
   tb->set_units("%");

   /* Propagate slider changes to the text box */
   slider->set_callback([tb](float value) {
       tb->set_value(std::to_string((int) (value * 100)));
   });



Compiling
----------------------------------------------------------------------------------------

On a perfectly clean Debian 11 VM (or SD card on your target) with plenty of disk
available. (If you are not **root** then you know you need sudo in front of most of this.)

.. code-block:: bash

   apt-get update
   apt-get install build-essential dkms linux-headers-$(uname -r)
   apt-get upgrade
   apt-get install cmake unzip zip git patchelf pkg-config extra-cmake-modules
   apt-get install libwayland-dev libxkbcommon-dev wayland-protocols 
   apt-get install waylandpp-dev libvulkan-dev

   mkdir Projects
   cd Projects
   git clone https://github.com/glfw/glfw.git
   cd glfw
   
   mkdir build && cd build
   
   cmake -DGLFW_USE_WAYLAND=ON -DBUILD_SHARED_LIBS=ON -DGLFW_BUILD_EXAMPLES=OFF \
   -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=ON -DGLFW_BUILD_COCOA=OFF \
   -DGLFW_BUILD_X11=OFF -DUSE_MSVC_RUNTIME_LIBRARY_DLL=OFF ..

   make install

   cd ~/Projects
   git clone --recursive https://github.com/RolandHughes/waylandgui.git
   cd waylandgui
   mkdir build && cd build
   cmake -Wno-depricated -Wno-error=depricated ..
   make install
   ./example3
   

License
----------------------------------------------------------------------------------------

.. begin_license

The License is the same as NanoGUI. It has a BSD-style license you can find in
the LICENSE_ file. By using, distributing, or contributing to this project, you agree to the
terms and conditions of this license.

.. _LICENSE: https://github.com/RolandHughes/waylandgui/blob/master/LICENSE.txt

Note that WaylandGUI ships with several fonts that use different (though similarly
unencumbered) licenses, in particular `Roboto
<https://github.com/google/roboto/>`_, `Inconsolata
<https://github.com/googlefonts/Inconsolata>`_, and the free version of the
`Font Awesome icon font <https://github.com/FortAwesome/Font-Awesome>`_
(v5.10.1). The latter two are distributed under the `SIL Open Font License
Version 1.1 <https://opensource.org/licenses/OFL-1.1>`_, while Roboto is
distributed under the `Apache 2.0
<https://opensource.org/licenses/Apache-2.0>`_ license.

.. end_license
