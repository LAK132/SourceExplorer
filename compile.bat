@echo off

cl /Fo:bin\imgui.obj /c R:\dear-imgui\imgui.cpp /nologo -EHsc -DNDEBUG /MD /IR:\dear-imgui
cl /Fo:bin/imgui_draw.obj /c R:\dear-imgui\imgui_draw.cpp /nologo -EHsc -DNDEBUG /MD /IR:\dear-imgui
cl /Fo:bin\imgui_demo.obj /c R:\dear-imgui\imgui_demo.cpp /nologo -EHsc -DNDEBUG /MD /IR:\dear-imgui
cl /Fo:bin\gl3w.obj /c lib\gl3w.c /nologo -EHsc -DNDEBUG /MD /Isrc /Iinclude
cl /Fo:bin\imgui_glfw.obj /c lib\imgui_impl_glfw_gl3.cpp /nologo -EHsc -DNDEBUG /MD /Isrc /Iinclude /IR:\dear-imgui

cl /Fo:bin\srcexplr.obj /c src\srcexplr.cpp /nologo -EHsc -DNDEBUG /MD /Isrc /Iinclude /IR:\dear-imgui

link /nologo /out:out\srcexplr.exe bin\srcexplr.obj bin\imgui_glfw.obj bin\imgui.obj bin\imgui_demo.obj bin\imgui_draw.obj bin\gl3w.obj lib\glfw3dll.lib lib\glfw3.lib