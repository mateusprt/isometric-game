
# Isometric Game
Project developed in C++ using OpenGL, GLAD, and GLFW.

## Environment Setup

The libraries used by the program are already included in the project. Therefore, there is no need to download them. Below, we will configure our environment to run C++ code on Windows. To do this:

- Download VS Code: https://code.visualstudio.com/download
- Download the C++ extension: https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools
- Download and install MSys2: https://github.com/msys2/msys2-installer/releases/download/2024-01-13/msys2-x86_64-20240113.exe
- After installation, in the MSys2 terminal, type: 
`pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`

- Add to path with this command (via terminal):
`set PATH=%PATH%;C:\msys64\ucrt64\bin`

That's it! Now you can run the project. \o/



## Features

- 15x15 tile map
- Maps loaded from configuration files
- Character controlled via keyboard in 8 possible directions on the isometric tilemap (N, S, E, W, NE, NW, SE, SW)
- Logic for walkable and non-walkable tiles
- Objective to win the game
- Logic for positioning objects in configuration files
- Logic for walkable tiles in configuration files
