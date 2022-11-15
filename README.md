# Adaptive Path Space Filtering

Master Thesis by Alexander Schipek: [Download](https://drive.google.com/file/d/1eUGLRqkoe9PO2kzLCRhN5evFtdWreO8m/view?usp=share_link)

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

## Assets

The assets for most predefined Scenes can be found [here](https://developer.nvidia.com/orca).
