/*
    src/example4.cpp -- C++ version of an example application that shows
    how to use the OpenGL widget. For a Python implementation, see
    '../python/example4.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <waylandgui/screen.h>
#include <waylandgui/layout.h>
#include <waylandgui/window.h>
#include <waylandgui/button.h>
#include <waylandgui/canvas.h>
#include <waylandgui/shader.h>
#include <waylandgui/renderpass.h>
#include <GLFW/glfw3.h>
#include <iostream>

using waylandgui::Vector3f;
using waylandgui::Vector2i;
using waylandgui::Shader;
using waylandgui::Canvas;
using waylandgui::ref;

constexpr float Pi = 3.14159f;

class MyCanvas : public Canvas {
public:
    MyCanvas(Widget *parent) : Canvas(parent, 1), m_rotation(0.f) {
        using namespace waylandgui;

        m_shader = new Shader(
            render_pass(),

            // An identifying name
            "a_simple_shader",

            // Vertex shader
            R"(precision highp float;
            uniform mat4 mvp;
            attribute vec3 position;
            attribute vec3 color;
            varying vec4 frag_color;
            void main() {
                frag_color = vec4(color, 1.0);
                gl_Position = mvp * vec4(position, 1.0);
            })",

            // Fragment shader
            R"(precision highp float;
            varying vec4 frag_color;
            void main() {
                gl_FragColor = frag_color;
            })"
        );

        uint32_t indices[3*12] = {
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4,
            4, 0, 3, 3, 7, 4,
            1, 5, 6, 6, 2, 1,
            0, 1, 2, 2, 3, 0,
            7, 6, 5, 5, 4, 7
        };

        float positions[3*8] = {
            -1.f, 1.f, 1.f, -1.f, -1.f, 1.f,
            1.f, -1.f, 1.f, 1.f, 1.f, 1.f,
            -1.f, 1.f, -1.f, -1.f, -1.f, -1.f,
            1.f, -1.f, -1.f, 1.f, 1.f, -1.f
        };

        float colors[3*8] = {
            0, 1, 1, 0, 0, 1,
            1, 0, 1, 1, 1, 1,
            0, 1, 0, 0, 0, 0,
            1, 0, 0, 1, 1, 0
        };

        m_shader->set_buffer("indices", VariableType::UInt32, {3*12}, indices);
        m_shader->set_buffer("position", VariableType::Float32, {8, 3}, positions);
        m_shader->set_buffer("color", VariableType::Float32, {8, 3}, colors);
    }

    void set_rotation(float rotation) {
        m_rotation = rotation;
    }

    virtual void draw_contents() override {
        using namespace waylandgui;

        Matrix4f view = Matrix4f::look_at(
            Vector3f(0, -2, -10),
            Vector3f(0, 0, 0),
            Vector3f(0, 1, 0)
        );

        Matrix4f model = Matrix4f::rotate(
            Vector3f(0, 1, 0),
            (float) glfwGetTime()
        );

        Matrix4f model2 = Matrix4f::rotate(
            Vector3f(1, 0, 0),
            m_rotation
        );

        Matrix4f proj = Matrix4f::perspective(
            float(25 * Pi / 180),
            0.1f,
            20.f,
            m_size.x() / (float) m_size.y()
        );

        Matrix4f mvp = proj * view * model * model2;

        m_shader->set_uniform("mvp", mvp);

        // Draw 12 triangles starting at index 0
        m_shader->begin();
        m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 12*3, true);
        m_shader->end();
    }

private:
    ref<Shader> m_shader;
    float m_rotation;
};

class ExampleApplication : public waylandgui::Screen {
public:
    ExampleApplication() : waylandgui::Screen(Vector2i(800, 600), "NanoGUI Test", false) {
        using namespace waylandgui;

        Window *window = new Window(this, "Canvas widget demo");
        window->set_position(Vector2i(15, 15));
        window->set_layout(new GroupLayout());

        m_canvas = new MyCanvas(window);
        m_canvas->set_background_color({100, 100, 100, 255});
        m_canvas->set_fixed_size({400, 400});

        Widget *tools = new Widget(window);
        tools->set_layout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 5));

        Button *b0 = new Button(tools, "Random Background");
        b0->set_callback([this]() {
            m_canvas->set_background_color(
                Vector4i(rand() % 256, rand() % 256, rand() % 256, 255));
        });

        Button *b1 = new Button(tools, "Random Rotation");
        b1->set_callback([this]() {
            m_canvas->set_rotation((float) Pi * rand() / (float) RAND_MAX);
        });

        perform_layout();
    }

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboard_event(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            set_visible(false);
            return true;
        }
        return false;
    }

    virtual void draw(NVGcontext *ctx) {
        // Draw the user interface
        Screen::draw(ctx);
    }
private:
    MyCanvas *m_canvas;
};

int main(int /* argc */, char ** /* argv */) {
    try {
        waylandgui::init();

        /* scoped variables */ {
            waylandgui::ref<ExampleApplication> app = new ExampleApplication();
            app->draw_all();
            app->set_visible(true);
            waylandgui::mainloop(1 / 60.f * 1000);
        }

        waylandgui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        std::cerr << error_msg << std::endl;
        return -1;
    }

    return 0;
}
