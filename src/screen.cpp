/*
    src/screen.cpp -- Top-level widget and interface between NanoGUI and GLFW

    A significant redesign of this code was contributed by Christian Schueller.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <waylandgui/screen.h>
#include <waylandgui/theme.h>
#include <waylandgui/opengl.h>
#include <waylandgui/window.h>
#include <waylandgui/popup.h>
#include <map>
#include <iostream>

#define NANOVG_GLES3_IMPLEMENTATION
#include <nanovg_gl.h>
# include "opengl_check.h"

#if !defined(GL_RGBA_FLOAT_MODE)
#  define GL_RGBA_FLOAT_MODE 0x8820
#endif

#define WAYLANDGUI_GLES_VERSION 3

NAMESPACE_BEGIN(waylandgui)

std::map<GLFWwindow *, Screen *> __waylandgui_screens;

/* Calculate pixel ratio for hi-dpi devices. */
static float get_pixel_ratio(GLFWwindow *window) {
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    return xscale;
}


Screen::Screen()
    : Widget(nullptr), m_glfw_window(nullptr), m_nvg_context(nullptr),
      m_cursor(Cursor::Arrow), m_background(0.3f, 0.3f, 0.32f, 1.f),
      m_shutdown_glfw(false), m_fullscreen(false), m_depth_buffer(false),
      m_stencil_buffer(false), m_redraw(false) {
    memset(m_cursors, 0, sizeof(GLFWcursor *) * (size_t) Cursor::CursorCount);
}

Screen::Screen(const Vector2i &size, const std::string &caption, bool resizable,
               bool fullscreen, bool depth_buffer, bool stencil_buffer,
               bool float_buffer, unsigned int gl_major, unsigned int gl_minor)
    : Widget(nullptr), m_glfw_window(nullptr), m_nvg_context(nullptr),
      m_cursor(Cursor::Arrow), m_background(0.3f, 0.3f, 0.32f, 1.f), m_caption(caption),
      m_shutdown_glfw(false), m_fullscreen(fullscreen), m_depth_buffer(depth_buffer),
      m_stencil_buffer(stencil_buffer), m_redraw(false) {
    memset(m_cursors, 0, sizeof(GLFWcursor *) * (int) Cursor::CursorCount);

    // Was GLFW_OPENGL_ES_API but that causes wayland windows to not display
    //
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, WAYLANDGUI_GLES_VERSION);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    int color_bits = 8, depth_bits = 0, stencil_bits = 0;

    if (stencil_buffer && !depth_buffer)
        throw std::runtime_error(
            "Screen::Screen(): stencil_buffer = True requires depth_buffer = True");
    if (depth_buffer)
        depth_bits = 32;
    if (stencil_buffer) {
        depth_bits = 24;
        stencil_bits = 8;
    }

    glfwWindowHint(GLFW_RED_BITS, color_bits);
    glfwWindowHint(GLFW_GREEN_BITS, color_bits);
    glfwWindowHint(GLFW_BLUE_BITS, color_bits);
    glfwWindowHint(GLFW_ALPHA_BITS, color_bits);
    glfwWindowHint(GLFW_STENCIL_BITS, stencil_bits);
    glfwWindowHint(GLFW_DEPTH_BITS, depth_bits);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, resizable ? GL_TRUE : GL_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    for (int i = 0; i < 2; ++i) {
        if (fullscreen) {
            GLFWmonitor *monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            m_glfw_window = glfwCreateWindow(mode->width, mode->height,
                                             caption.c_str(), monitor, nullptr);
        } else {
            m_glfw_window = glfwCreateWindow(size.x(), size.y(),
                                             caption.c_str(), nullptr, nullptr);
        }
    }

    if (!m_glfw_window) {
        (void) gl_major; (void) gl_minor;
        throw std::runtime_error("Could not create a OpenGL " +
                                 std::to_string(gl_major) + "." +
                                 std::to_string(gl_minor) + " context!");
    }

    glfwMakeContextCurrent(m_glfw_window);

    glfwSetInputMode(m_glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);


    glfwGetFramebufferSize(m_glfw_window, &m_fbsize[0], &m_fbsize[1]);

    CHK(glViewport(0, 0, m_fbsize[0], m_fbsize[1]));
    CHK(glClearColor(m_background[0], m_background[1],
                     m_background[2], m_background[3]));
    CHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT));

    glfwSwapInterval(0);
    glfwSwapBuffers(m_glfw_window);

    /* Propagate GLFW events to the appropriate Screen instance */
    glfwSetCursorPosCallback(m_glfw_window,
        [](GLFWwindow *w, double x, double y) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;
            if (!s->m_process_events)
                return;
            s->cursor_pos_callback_event(x, y);
        }
    );

    glfwSetMouseButtonCallback(m_glfw_window,
        [](GLFWwindow *w, int button, int action, int modifiers) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;
            if (!s->m_process_events)
                return;
            s->mouse_button_callback_event(button, action, modifiers);
        }
    );

    glfwSetKeyCallback(m_glfw_window,
        [](GLFWwindow *w, int key, int scancode, int action, int mods) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;
            if (!s->m_process_events)
                return;
            s->key_callback_event(key, scancode, action, mods);
        }
    );

    glfwSetCharCallback(m_glfw_window,
        [](GLFWwindow *w, unsigned int codepoint) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;
            if (!s->m_process_events)
                return;
            s->char_callback_event(codepoint);
        }
    );

    glfwSetDropCallback(m_glfw_window,
        [](GLFWwindow *w, int count, const char **filenames) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;
            if (!s->m_process_events)
                return;
            s->drop_callback_event(count, filenames);
        }
    );

    glfwSetScrollCallback(m_glfw_window,
        [](GLFWwindow *w, double x, double y) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;
            if (!s->m_process_events)
                return;
            s->scroll_callback_event(x, y);
        }
    );

    /* React to framebuffer size events -- includes window
       size events and also catches things like dragging
       a window from a Retina-capable screen to a normal
       screen on Mac OS X */
    glfwSetFramebufferSizeCallback(m_glfw_window,
        [](GLFWwindow* w, int width, int height) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen *s = it->second;

            if (!s->m_process_events)
                return;

            s->resize_callback_event(width, height);
        }
    );

    // notify when the screen has lost focus (e.g. application switch)
    glfwSetWindowFocusCallback(m_glfw_window,
        [](GLFWwindow *w, int focused) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;

            Screen *s = it->second;
            // focus_event: 0 when false, 1 when true
            s->focus_event(focused != 0);
        }
    );

    glfwSetWindowContentScaleCallback(m_glfw_window,
        [](GLFWwindow* w, float, float) {
            auto it = __waylandgui_screens.find(w);
            if (it == __waylandgui_screens.end())
                return;
            Screen* s = it->second;

            s->m_pixel_ratio = get_pixel_ratio(w);
            s->resize_callback_event(s->m_size.x(), s->m_size.y());
        }
    );

    initialize(m_glfw_window, true);
}

void Screen::initialize(GLFWwindow *window, bool shutdown_glfw) {
    m_glfw_window = window;
    m_shutdown_glfw = shutdown_glfw;
    glfwGetWindowSize(m_glfw_window, &m_size[0], &m_size[1]);
    glfwGetFramebufferSize(m_glfw_window, &m_fbsize[0], &m_fbsize[1]);

    m_pixel_ratio = get_pixel_ratio(window);

    if (m_pixel_ratio != 1 && !m_fullscreen)
        glfwSetWindowSize(window, m_size.x() * m_pixel_ratio,
                                  m_size.y() * m_pixel_ratio);
    int flags = NVG_ANTIALIAS;
    if (m_stencil_buffer)
       flags |= NVG_STENCIL_STROKES;
#if !defined(NDEBUG)
    flags |= NVG_DEBUG;
#endif

    m_nvg_context = nvgCreateGLES3(flags);

    if (!m_nvg_context)
        throw std::runtime_error("Could not initialize NanoVG!");

    m_visible = glfwGetWindowAttrib(window, GLFW_VISIBLE) != 0;
    set_theme(new Theme(m_nvg_context));
    m_mouse_pos = Vector2i(0);
    m_mouse_state = m_modifiers = 0;
    m_drag_active = false;
    m_last_interaction = glfwGetTime();
    m_process_events = true;
    m_redraw = true;
    __waylandgui_screens[m_glfw_window] = this;

    for (size_t i = 0; i < (size_t) Cursor::CursorCount; ++i)
        m_cursors[i] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR + (int) i);

    /// Fixes retina display-related font rendering issue (#185)
    nvgBeginFrame(m_nvg_context, m_size[0], m_size[1], m_pixel_ratio);
    nvgEndFrame(m_nvg_context);
}

Screen::~Screen() {
    __waylandgui_screens.erase(m_glfw_window);
    for (size_t i = 0; i < (size_t) Cursor::CursorCount; ++i) {
        if (m_cursors[i])
            glfwDestroyCursor(m_cursors[i]);
    }

    if (m_nvg_context) {
        nvgDeleteGLES3(m_nvg_context);
    }

    if (m_glfw_window && m_shutdown_glfw)
        glfwDestroyWindow(m_glfw_window);
}

void Screen::set_visible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;

        if (visible)
            glfwShowWindow(m_glfw_window);
        else
            glfwHideWindow(m_glfw_window);
    }
}

void Screen::set_caption(const std::string &caption) {
    if (caption != m_caption) {
        glfwSetWindowTitle(m_glfw_window, caption.c_str());
        m_caption = caption;
    }
}

void Screen::set_size(const Vector2i &size) {
    Widget::set_size(size);

    glfwSetWindowSize(m_glfw_window, size.x(), size.y());
}

void Screen::clear() {
    CHK(glClearColor(m_background[0], m_background[1], m_background[2], m_background[3]));
    CHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

void Screen::draw_setup() {
    glfwMakeContextCurrent(m_glfw_window);

    glfwGetFramebufferSize(m_glfw_window, &m_fbsize[0], &m_fbsize[1]);
    glfwGetWindowSize(m_glfw_window, &m_size[0], &m_size[1]);

    m_fbsize = m_size;
    m_size = Vector2i(Vector2f(m_size) / m_pixel_ratio);

    CHK(glViewport(0, 0, m_fbsize[0], m_fbsize[1]));
}

void Screen::draw_teardown() {
    glfwSwapBuffers(m_glfw_window);
}

void Screen::draw_all() {
    if (m_redraw) {
        m_redraw = false;
        draw_setup();
        draw_contents();
        draw_widgets();
        draw_teardown();
    }
}

void Screen::draw_contents() {
    clear();
}

void Screen::nvg_flush() {
    NVGparams *params = nvgInternalParams(m_nvg_context);
    params->renderFlush(params->userPtr);
    params->renderViewport(params->userPtr, m_size[0], m_size[1], m_pixel_ratio);
}

void Screen::draw_widgets() {
    nvgBeginFrame(m_nvg_context, m_size[0], m_size[1], m_pixel_ratio);

    draw(m_nvg_context);

    double elapsed = glfwGetTime() - m_last_interaction;

    if (elapsed > 0.5f) {
        /* Draw tooltips */
        const Widget *widget = find_widget(m_mouse_pos);
        if (widget && !widget->tooltip().empty()) {
            int tooltip_width = 150;

            float bounds[4];
            nvgFontFace(m_nvg_context, "sans");
            nvgFontSize(m_nvg_context, 15.0f);
            nvgTextAlign(m_nvg_context, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgTextLineHeight(m_nvg_context, 1.1f);
            Vector2i pos = widget->absolute_position() +
                           Vector2i(widget->width() / 2, widget->height() + 10);

            nvgTextBounds(m_nvg_context, pos.x(), pos.y(),
                          widget->tooltip().c_str(), nullptr, bounds);

            int h = (bounds[2] - bounds[0]) / 2;
            if (h > tooltip_width / 2) {
                nvgTextAlign(m_nvg_context, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgTextBoxBounds(m_nvg_context, pos.x(), pos.y(), tooltip_width,
                                widget->tooltip().c_str(), nullptr, bounds);

                h = (bounds[2] - bounds[0]) / 2;
            }
            int shift = 0;

            if (pos.x() - h - 8 < 0) {
                /* Keep tooltips on screen */
                shift = pos.x() - h - 8;
                pos.x() -= shift;
                bounds[0] -= shift;
                bounds[2] -= shift;
            }

            nvgGlobalAlpha(m_nvg_context,
                           std::min(1.0, 2 * (elapsed - 0.5f)) * 0.8);

            nvgBeginPath(m_nvg_context);
            nvgFillColor(m_nvg_context, Color(0, 255));
            nvgRoundedRect(m_nvg_context, bounds[0] - 4 - h, bounds[1] - 4,
                           (int) (bounds[2] - bounds[0]) + 8,
                           (int) (bounds[3] - bounds[1]) + 8, 3);

            int px = (int) ((bounds[2] + bounds[0]) / 2) - h + shift;
            nvgMoveTo(m_nvg_context, px, bounds[1] - 10);
            nvgLineTo(m_nvg_context, px + 7, bounds[1] + 1);
            nvgLineTo(m_nvg_context, px - 7, bounds[1] + 1);
            nvgFill(m_nvg_context);

            nvgFillColor(m_nvg_context, Color(255, 255));
            nvgFontBlur(m_nvg_context, 0.0f);
            nvgTextBox(m_nvg_context, pos.x() - h, pos.y(), tooltip_width,
                       widget->tooltip().c_str(), nullptr);
        }
    }

    nvgEndFrame(m_nvg_context);
}

bool Screen::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (m_focus_path.size() > 0) {
        for (auto it = m_focus_path.rbegin() + 1; it != m_focus_path.rend(); ++it)
            if ((*it)->focused() && (*it)->keyboard_event(key, scancode, action, modifiers))
                return true;
    }

    return false;
}

bool Screen::keyboard_character_event(unsigned int codepoint) {
    if (m_focus_path.size() > 0) {
        for (auto it = m_focus_path.rbegin() + 1; it != m_focus_path.rend(); ++it)
            if ((*it)->focused() && (*it)->keyboard_character_event(codepoint))
                return true;
    }
    return false;
}

bool Screen::resize_event(const Vector2i& size) {
    if (m_resize_callback)
        m_resize_callback(size);
    m_redraw = true;
    draw_all();
    return true;
}

void Screen::redraw() {
    if (!m_redraw) {
        m_redraw = true;
        glfwPostEmptyEvent();
    }
}

void Screen::cursor_pos_callback_event(double x, double y) {
    Vector2i p((int) x, (int) y);

    p = Vector2i(Vector2f(p) / m_pixel_ratio);

    m_last_interaction = glfwGetTime();
    try {
        p -= Vector2i(1, 2);

        bool ret = false;
        if (!m_drag_active) {
            Widget *widget = find_widget(p);
            if (widget != nullptr && widget->cursor() != m_cursor) {
                m_cursor = widget->cursor();
                glfwSetCursor(m_glfw_window, m_cursors[(int) m_cursor]);
            }
        } else {
            ret = m_drag_widget->mouse_drag_event(
                p - m_drag_widget->parent()->absolute_position(), p - m_mouse_pos,
                m_mouse_state, m_modifiers);
        }

        if (!ret)
            ret = mouse_motion_event(p, p - m_mouse_pos, m_mouse_state, m_modifiers);

        m_mouse_pos = p;
        m_redraw |= ret;
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event handler: " << e.what() << std::endl;
    }
}

void Screen::mouse_button_callback_event(int button, int action, int modifiers) {
    m_modifiers = modifiers;
    m_last_interaction = glfwGetTime();

    #if defined(__APPLE__)
        if (button == GLFW_MOUSE_BUTTON_1 && modifiers == GLFW_MOD_CONTROL)
            button = GLFW_MOUSE_BUTTON_2;
    #endif

    try {
        if (m_focus_path.size() > 1) {
            const Window *window =
                dynamic_cast<Window *>(m_focus_path[m_focus_path.size() - 2]);
            if (window && window->modal()) {
                if (!window->contains(m_mouse_pos))
                    return;
            }
        }

        if (action == GLFW_PRESS)
            m_mouse_state |= 1 << button;
        else
            m_mouse_state &= ~(1 << button);

        auto drop_widget = find_widget(m_mouse_pos);
        if (m_drag_active && action == GLFW_RELEASE &&
            drop_widget != m_drag_widget) {
            m_redraw |= m_drag_widget->mouse_button_event(
                m_mouse_pos - m_drag_widget->parent()->absolute_position(), button,
                false, m_modifiers);
        }

        if (drop_widget != nullptr && drop_widget->cursor() != m_cursor) {
            m_cursor = drop_widget->cursor();
            glfwSetCursor(m_glfw_window, m_cursors[(int) m_cursor]);
        }

        bool btn12 = button == GLFW_MOUSE_BUTTON_1 || button == GLFW_MOUSE_BUTTON_2;

        if (!m_drag_active && action == GLFW_PRESS && btn12) {
            m_drag_widget = find_widget(m_mouse_pos);
            if (m_drag_widget == this)
                m_drag_widget = nullptr;
            m_drag_active = m_drag_widget != nullptr;
            if (!m_drag_active)
                update_focus(nullptr);
        } else if (m_drag_active && action == GLFW_RELEASE && btn12) {
            m_drag_active = false;
            m_drag_widget = nullptr;
        }

        m_redraw |= mouse_button_event(m_mouse_pos, button,
                                       action == GLFW_PRESS, m_modifiers);
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event handler: " << e.what() << std::endl;
    }
}

void Screen::key_callback_event(int key, int scancode, int action, int mods) {
    m_last_interaction = glfwGetTime();
    try {
        m_redraw |= keyboard_event(key, scancode, action, mods);
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event handler: " << e.what() << std::endl;
    }
}

void Screen::char_callback_event(unsigned int codepoint) {
    m_last_interaction = glfwGetTime();
    try {
        m_redraw |= keyboard_character_event(codepoint);
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event handler: " << e.what() << std::endl;
    }
}

void Screen::drop_callback_event(int count, const char **filenames) {
    std::vector<std::string> arg(count);
    for (int i = 0; i < count; ++i)
        arg[i] = filenames[i];
    m_redraw |= drop_event(arg);
}

void Screen::scroll_callback_event(double x, double y) {
    m_last_interaction = glfwGetTime();
    try {
        if (m_focus_path.size() > 1) {
            const Window *window =
                dynamic_cast<Window *>(m_focus_path[m_focus_path.size() - 2]);
            if (window && window->modal()) {
                if (!window->contains(m_mouse_pos))
                    return;
            }
        }
        m_redraw |= scroll_event(m_mouse_pos, Vector2f(x, y));
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event handler: " << e.what() << std::endl;
    }
}

void Screen::resize_callback_event(int, int) {
    Vector2i fb_size, size;
    glfwGetFramebufferSize(m_glfw_window, &fb_size[0], &fb_size[1]);
    glfwGetWindowSize(m_glfw_window, &size[0], &size[1]);
    if (fb_size == Vector2i(0, 0) || size == Vector2i(0, 0))
        return;
    m_fbsize = fb_size; m_size = size;

    m_size = Vector2i(Vector2f(m_size) / m_pixel_ratio);

    m_last_interaction = glfwGetTime();

    try {
        resize_event(m_size);
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event handler: " << e.what() << std::endl;
    }
    redraw();
}

void Screen::update_focus(Widget *widget) {
    for (auto w: m_focus_path) {
        if (!w->focused())
            continue;
        w->focus_event(false);
    }
    m_focus_path.clear();
    Widget *window = nullptr;
    while (widget) {
        m_focus_path.push_back(widget);
        if (dynamic_cast<Window *>(widget))
            window = widget;
        widget = widget->parent();
    }
    for (auto it = m_focus_path.rbegin(); it != m_focus_path.rend(); ++it)
        (*it)->focus_event(true);

    if (window)
        move_window_to_front((Window *) window);
}

void Screen::dispose_window(Window *window) {
    if (std::find(m_focus_path.begin(), m_focus_path.end(), window) != m_focus_path.end())
        m_focus_path.clear();
    if (m_drag_widget == window)
        m_drag_widget = nullptr;
    remove_child(window);
}

void Screen::center_window(Window *window) {
    if (window->size() == 0) {
        window->set_size(window->preferred_size(m_nvg_context));
        window->perform_layout(m_nvg_context);
    }
    window->set_position((m_size - window->size()) / 2);
}

void Screen::move_window_to_front(Window *window) {
    m_children.erase(std::remove(m_children.begin(), m_children.end(), window), m_children.end());
    m_children.push_back(window);
    /* Brute force topological sort (no problem for a few windows..) */
    bool changed = false;
    do {
        size_t base_index = 0;
        for (size_t index = 0; index < m_children.size(); ++index)
            if (m_children[index] == window)
                base_index = index;
        changed = false;
        for (size_t index = 0; index < m_children.size(); ++index) {
            Popup *pw = dynamic_cast<Popup *>(m_children[index]);
            if (pw && pw->parent_window() == window && index < base_index) {
                move_window_to_front(pw);
                changed = true;
                break;
            }
        }
    } while (changed);
}

bool Screen::tooltip_fade_in_progress() const {
    double elapsed = glfwGetTime() - m_last_interaction;
    if (elapsed < 0.25f || elapsed > 1.25f)
        return false;
    /* Temporarily increase the frame rate to fade in the tooltip */
    const Widget *widget = find_widget(m_mouse_pos);
    return widget && !widget->tooltip().empty();
}

Texture::PixelFormat Screen::pixel_format() const {
    return Texture::PixelFormat::RGBA;
}

Texture::ComponentFormat Screen::component_format() const {
    return Texture::ComponentFormat::UInt8;
}


NAMESPACE_END(waylandgui)
