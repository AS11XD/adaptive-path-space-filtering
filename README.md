# Adaptive Path Space Filtering

Master Thesis by Alexander Schipek: [Download](https://drive.google.com/file/d/1eUGLRqkoe9PO2kzLCRhN5evFtdWreO8m/view?usp=share_link)

## Vulkan installation

- download [installer](https://vulkan.lunarg.com/sdk/home#windows)
- follow instructions of the installer

## CUDA Toolkit installation

- download [installer](https://developer.nvidia.com/cuda-downloads)
- follow instructions of the installer

## Visual Studio Setup

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
