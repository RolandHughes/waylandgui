Contributing
========================================================================================

Thank you for your interest in this project! Please refer to the following sections on
how to contribute code and bug reports.

Reporting bugs
----------------------------------------------------------------------------------------

At the moment, this project is run in the spare time of a single person
(Roland Hughes) with very limited resources for issue tracker tickets. Thus,
before submitting a question or bug report, please take a moment of your time and
ensure that your issue isn't already discussed in the project documentation elsewhere
on this site.

Feature requests are generally closed unless they come with a pull request
that implements the desired functionality.

Assuming that you have identified a previously unknown problem or an important question,
it's essential that you submit a self-contained and minimal piece of code that
reproduces the problem. In other words: no external dependencies, isolate the
function(s) that cause breakage, submit matched and complete C++ project that can be
built on the Linux command line via cmake.

Pull requests
----------------------------------------------------------------------------------------
Contributions are submitted, reviewed, and accepted using Github pull requests. Please
refer to `this article <https://help.github.com/articles/using-pull-requests>`_ for
details and adhere to the following rules to make the process as smooth as possible:

- Make a new branch for every feature you're working on.
- Make small and clean pull requests that are easy to review but make sure they do add
  value by themselves.
- Make sure you have tested any new functionality (e.g. if you made a new Widget).
- This project has a strong focus on providing general solutions using a minimal amount
  of code, thus small pull requests are greatly preferred.
- Read the remainder of this document, adhering to the bindings and documentation
  requirements.
- If making a purely documentation PR, please prefix the commit with ``[docs]``

  - E.g. ``[docs] Adding documentation for class X.``


Specific activities for contributions
----------------------------------------------------------------------------------------

In order of importance:

- Tracking down the Shader issue for examples 1 an 4.
- Getting all C++ source and header files up to the .astylerc coding standard.
- A TextDisplayWidget that understands lines and automatically adds horizontal/vertical
  scroll bars as needed. Understanding lines is important and needs to support a
  scrollback limit. Such that 8000 holds at most 8000 lines. When the next line is added
  the very first line is removed.
- Text translation such that either a tr() macro or some function call using the original
  text as a key looks up the language from a table for the current language. If not found
  the original text key is returned.
- ToolButton or other graphical button. Just a button with an SVG or BMP image like a
  toolbar button. Current buttons support image with text but not an image only button.
- Completely free to use international standard images for things like Open, Close, Exit,
  Exit Application, etc.
- Themes! Better support for themes per widget such that the button theme can be
  completely different from a window/screen theme. A standard set of themes to mimic the
  look and feel of many desktop environments. Tutorial for beginner needing their own
  Theme.
- Keep in mind the target market for this library is embedded systems with touch screen
  devices. Thinks like "hover" messages and function key/click "help" are not getting
  added. An HtmlDisplayWidget that can handle HTML formatting with some images would be
  useful for displaying application level help text.

A WaylandGUIMedia sub-library:

- AudioPlay widget that supports non-compressed WAV files via ALSA lib.
- SimpleVideoPlayer widget. No GLStreamer. Think of those annoying gas pumps that play
  advertisements while you fill your vehicle or small help videos contained in medical
  devices. Nothing high end or complex.
