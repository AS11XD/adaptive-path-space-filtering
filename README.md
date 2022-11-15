# Master Thesis Alexander Schipek

## Windows Vulkan installation

- download installer from https://vulkan.lunarg.com/sdk/home#windows
- follow instructions of the installer

## Windows Visual Studio GLSL language integration

- download from https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL
- follow instructions of the installer
- open Visual Studio
    - Tools -> Options... -> GLSL language integration -> Configuration -> General
    - External compiler executable file path = %VK_SDK_PATH%\Bin\glslangValidator.exe --target-env=vulkan1.2

## Windows Visual Studio installation

- open Visual Studio
    - File -> Open -> CMake...
- wait until loading has finished
- right-click on CMakelists.txt:
    - Set as Startup Item
- right-click on CMakelists.txt
    - Add Debug Configuration
    - Default
    - expand configurations by adding
```
"currentDir": "${workspaceRoot}"
```
