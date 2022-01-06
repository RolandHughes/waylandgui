/*
    waylandgui/opengl.h -- Pulls in GLFW and NanoVG header files

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#define GLFW_INCLUDE_ES32

#include <waylandgui/vector.h>

#include <GLFW/glfw3.h>

#include <nanovg.h>

NAMESPACE_BEGIN(waylandgui)

/// Allows for conversion between waylandgui::Color and the NanoVG NVGcolor class.
inline Color::operator const NVGcolor &() const {
    return reinterpret_cast<const NVGcolor &>(*(this->v));
}

/**
 * \brief Determine whether an icon ID is a texture loaded via ``nvg_image_icon``.
 *
 * \rst
 * The implementation defines all ``value < 1024`` as image icons, and
 * everything ``>= 1024`` as an Entypo icon (see :ref:`file_waylandgui_entypo.h`).
 * The value ``1024`` exists to provide a generous buffer on how many images
 * may have been loaded by NanoVG.
 * \endrst
 *
 * \param value
 *     The integral value of the icon.
 *
 * \return
 *     Whether or not this is an image icon.
 */
inline bool nvg_is_image_icon(int value) { return value < 1024; }

/**
 * \brief Determine whether an icon ID is a font-based icon (e.g. from ``entypo.ttf``).
 *
 * \rst
 * See :func:`waylandgui::nvg_is_image_icon` for details.
 * \endrst
 *
 * \param value
 *     The integral value of the icon.
 *
 * \return
 *     Whether or not this is a font icon (from ``entypo.ttf``).
 */
inline bool nvg_is_font_icon(int value) { return value >= 1024; }


/// Check for OpenGL errors and warn if one is found (returns 'true' in that case')
extern WAYLANDGUI_EXPORT bool waylandgui_check_glerror(const char *cmd);

NAMESPACE_END(waylandgui)
