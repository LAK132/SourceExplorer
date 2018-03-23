#include "imgui.h"
#include <imgui_impl_glfw_gl3.h>
#include <imgui_memory_editor.h>
#include <stdio.h>
#include <iostream>
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#include "explorer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif
#include <stb_image_write.h>

#ifndef SCREXPLR_H
#define SCREXPLR_H

#endif