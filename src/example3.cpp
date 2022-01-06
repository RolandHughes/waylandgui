/*
    src/example3.cpp -- C++ version of an example application that shows
    how to use waylandgui in an application with an already created and managed
    GLFW context.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

// GLFW
//
#  define GLFW_INCLUDE_ES2

#include <GLFW/glfw3.h>
#include <waylandgui/waylandgui.h>
#include <iostream>

using namespace waylandgui;

enum test_enum {
    Item1 = 0,
    Item2,
    Item3
};

bool bvar = true;
int ivar = 12345678;
double dvar = 3.1415926;
float fvar = (float)dvar;
std::string strval = "A string";
test_enum enumval = Item2;
Color colval(0.5f, 0.5f, 0.7f, 1.f);

Screen *screen = nullptr;

int main(int /* argc */, char ** /* argv */) {
    glfwInit();
    glfwSetTime(0);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    // Create a GLFWwindow object
    GLFWwindow* window = glfwCreateWindow(500, 700, "example3", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Create a waylandgui screen and pass the glfw pointer to initialize
    screen = new Screen();
    screen->initialize(window, true);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSwapInterval(0);
    glfwSwapBuffers(window);

    // Create waylandgui gui
    bool enabled = true;
    FormHelper *gui = new FormHelper(screen);
    ref<Window> waylandgui_window = gui->add_window(Vector2i(10, 10), "Form helper example 3");
    gui->add_group("Basic types");
    gui->add_variable("bool", bvar)->set_tooltip("Test tooltip.");
    gui->add_variable("string", strval);

    gui->add_group("Validating fields");
    gui->add_variable("int", ivar)->set_spinnable(true);
    gui->add_variable("float", fvar)->set_tooltip("Test.");
    gui->add_variable("double", dvar)->set_spinnable(true);

    gui->add_group("Complex types");
    gui->add_variable("Enumeration", enumval, enabled)->set_items({ "Item 1", "Item 2", "Item 3" });
    gui->add_variable("Color", colval);

    gui->add_group("Other widgets");
    gui->add_button("A button", []() { std::cout << "Button pressed." << std::endl; })
       ->set_tooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");;

    screen->set_visible(true);
    screen->perform_layout();
    waylandgui_window->center();
    screen->clear();
    screen->draw_all();

    glfwSetCursorPosCallback(window,
            [](GLFWwindow *, double x, double y) {
            screen->cursor_pos_callback_event(x, y);
        }
    );

    glfwSetMouseButtonCallback(window,
        [](GLFWwindow *, int button, int action, int modifiers) {
            screen->mouse_button_callback_event(button, action, modifiers);
        }
    );

    glfwSetKeyCallback(window,
        [](GLFWwindow *, int key, int scancode, int action, int mods) {
            screen->key_callback_event(key, scancode, action, mods);
        }
    );

    glfwSetCharCallback(window,
        [](GLFWwindow *, unsigned int codepoint) {
            screen->char_callback_event(codepoint);
        }
    );

    glfwSetDropCallback(window,
        [](GLFWwindow *, int count, const char **filenames) {
            screen->drop_callback_event(count, filenames);
        }
    );

    glfwSetScrollCallback(window,
        [](GLFWwindow *, double x, double y) {
            screen->scroll_callback_event(x, y);
       }
    );

    glfwSetFramebufferSizeCallback(window,
        [](GLFWwindow *, int width, int height) {
            screen->resize_callback_event(width, height);
        }
    );

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        // Draw waylandgui
        screen->draw_setup();
        screen->clear(); // glClear
        screen->draw_contents();
        screen->draw_widgets();
        screen->draw_teardown();

        glfwSwapBuffers(window);
    }

    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();

    return 0;
}
