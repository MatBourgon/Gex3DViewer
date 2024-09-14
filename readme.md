# What is this?
This project is the beginning of a level viewer, and eventually, level editor for Gex 2: Enter the Gecko.
The engine is currently set up to work with the PC build and its files.

# How to use
Open up the level viewer, and pick a level file (.dfx). Make sure its matching graphics file (.vfx) is in the same directory, as the program will automatically load it up at the same time.

## Keys
To move around, use WASD.
To look around, right click in the window and move your cursor, or use the mouse wheel at any time.

## Hotkeys:
Ctrl+X: Toggle Wireframe
- ON: enable vertex colors and textures
- OFF: disable vertex colors and textures

# Building
This project is built using Premake and VCPKG (and thus with a bias for Windows and Visual Studio).
The project can be generated and re-generated with the provided batch file. C++20 is used.
The required VCPKG packages are:
- glfw:x86-windows
- glm:x86-windows
- imgui:x86-windows
- imgui[glfw-binding]:x86-windows
- imgui[opengl3-binding]:x86-windows
- imgui[win32-binding]:x86-windows