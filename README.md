
# ShatterXorps - Vulkan Setup for Visual Studio

Posting the repository and opening a project from it in Visual Studio should work

## Project Map:

CTRL + F > Search for: tutorialEnd - skips the Full tutorial on how to install Vulkan and initialize Visual Studio for it

CTRL + F > Search for: iWillHelp - some throubleshooting with project initialization

CTRL + F > Search for: theLegacy - history of the project

# FULL TUTORIAL:

This repository contains all necessary files to start Vulkan development with Visual Studio. The project already includes pre-configured Visual Studio files and Vulkan headers and classes within the `vulkan` folder. The only requirement is configuring your path variables in Visual Studio.

---

## Prerequisites
1. A Windows PC with Visual Studio installed (2019 or later recommended).
2. A GPU that supports Vulkan (check compatibility (https://vulkan.lunarg.com/)).
3. Vulkan SDK installed for system-wide Vulkan runtime support. Download it (https://vulkan.lunarg.com/sdk/home).

## Project Structure

Starting a Vulkan project in Visual Studio involves several steps, including setting up the Vulkan SDK, configuring your project settings, and writing some initial code to verify everything is working correctly. Below is a comprehensive guide to help you get started:

## 1. **Install the Vulkan SDK**

Before you begin, you need to install the Vulkan Software Development Kit (SDK), which provides all the necessary tools, libraries, and headers.

- **Download the Vulkan SDK:**
  - Visit the [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home) website.
  - Download the appropriate installer for your operating system.

- **Install the SDK:**
  - Run the installer and follow the on-screen instructions.
  - By default, the SDK will be installed in `C:\VulkanSDK\<version>` on Windows.

- **Set Environment Variables:**
  - The installer typically sets the necessary environment variables (`VULKAN_SDK`, `PATH`, etc.) automatically.
  - To verify, open a Command Prompt and run `echo %VULKAN_SDK%`. It should display the path to your Vulkan SDK installation.

## 2. **Set Up Your Visual Studio Project**

### a. **Create a New Project**

1. **Open Visual Studio:**
   - Launch Visual Studio.

2. **Create a New Project:**
   - Go to `File` > `New` > `Project`.

3. **Select Project Template:**
   - Choose `Console App` under C++ templates.
   - Click `Next`.

4. **Configure Project:**
   - **Name:** Enter a project name, e.g., `VulkanApp`.
   - **Location:** Choose a suitable directory.
   - **Solution Name:** You can keep it the same as the project name.
   - Click `Create`.

### b. **Configure Project Properties**

1. **Open Project Properties:**
   - In the Solution Explorer, right-click on your project and select `Properties`.

2. **Set C++ Language Standard (Optional but Recommended):**
   - Navigate to `Configuration Properties` > `C/C++` > `Language`.
   - Set `C++ Language Standard` to `ISO C++17 Standard (/std:c++17)` or higher.

3. **Include Vulkan Headers:**
   - Navigate to `Configuration Properties` > `C/C++` > `General`.
   - Find `Additional Include Directories` and add the Vulkan include path:
     ```
     $(VULKAN_SDK)\Include
     ```
     *Note: `$(VULKAN_SDK)` is an environment variable set by the Vulkan SDK installer.*

4. **Link Vulkan Libraries:**
   - Navigate to `Configuration Properties` > `Linker` > `General`.
   - Find `Additional Library Directories` and add:
     ```
     $(VULKAN_SDK)\Lib
     ```
   - Then, go to `Configuration Properties` > `Linker` > `Input`.
   - Find `Additional Dependencies` and add:
     ```
     vulkan-1.lib
     ```
   
5. **Copy Vulkan DLL to Output Directory (Optional for Debugging):**
   - To ensure your executable can find `vulkan-1.dll` at runtime, you can copy it to your project's output directory.
   - Alternatively, make sure the Vulkan SDK's `Bin` directory is in your system's `PATH`.

### **Resources for Learning Vulkan:**

- **Official Vulkan Documentation:** [Khronos Vulkan](https://www.khronos.org/vulkan/)
- **Vulkan Tutorial:** [Vulkan Tutorial](https://vulkan-tutorial.com/) – A step-by-step guide to learning Vulkan.
- **Books:**
  - *Vulkan Programming Guide* by Graham Sellers, John Kessenich, and Dave Shreiner.
  - *Vulkan Cookbook* by Pawel Lapinski.

## 3. **Troubleshooting Tips**

- **Missing `vulkan-1.dll`:**
  - Ensure the Vulkan SDK's `Bin` directory is in your system `PATH`, or copy `vulkan-1.dll` to your project's output directory.

iWillHelp - and ofc I will:

## Troubleshooting
 
1  - Install the Vulkan SDK from [LunarG](https://vulkan.lunarg.com/sdk/home).
2  - Add the SDK path to your system's environment variables:
3  - Change Your Project's Target Platform to the same of Vulkan's SDK library
    VULKAN_SDK = C:\VulkanSDK\<version>
4  - Verify and Configure Linker Settings:
    a. Include Vulkan Libraries
    Navigate to Linker Input:
    
    In the Project Properties, go to:
    Configuration Properties > Linker > Input
    In the Additional Dependencies field, ensure that vulkan-1.lib is listed. If not, add it:
    vulkan-1.lib
    b. Specify Library Directories
    Navigate to Linker General:
    
    Go to:
    Configuration Properties > Linker > General
    In the Additional Library Directories field, ensure the Vulkan SDK's Lib directory is included:
    $(VULKAN_SDK)\Lib
    Note: $(VULKAN_SDK) is an environment variable set by the Vulkan SDK installer. It typically points to something like C:\VulkanSDK\1.x.x.x.
5  - Ensure Correct Include Directories
      Navigate to C/C++ General Settings:
      
    In the Project Properties, go to:
    Configuration Properties > C/C++ > General
    In the Additional Include Directories field, ensure the Vulkan SDK's Include directory is added:
    $(VULKAN_SDK)\Include
6  - Confirm Environment Variables
    Open a Command Prompt and run:

    echo %VULKAN_SDK%
    It should display the path to your Vulkan SDK installation (e.g., C:\VulkanSDK\1.x.x.x).
    Ensure PATH Includes Vulkan Bin:
    The Vulkan SDK's Bin directory should be in your system's PATH. This allows the executable to locate vulkan-1.dll at runtime.
    To verify, in the Command Prompt, run:
    where vulkan-1.dll
    If not found, you may need to add $(VULKAN_SDK)\Bin to your system's PATH environment variable.

tutorialEnd - but what is this?!

It's a GitHub for a Vulkan Voxel Engine, where I decided to keep it open source, so others like me, can learn.

## Additional Resources

- [Vulkan Documentation](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html)
- [Vulkan Samples and Tutorials](https://github.com/KhronosGroup/Vulkan-Samples)
- [Visual Studio Documentation](https://learn.microsoft.com/en-us/visualstudio/)

---

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

```

theLegacy
2024-10-29 - Triangle 2D - not on Github
2024-11-10 - Triangle 3D RGB - not on Gitub
2024-11-17 - 1st Github conmit - and some code optimizations
2024-11-17 - 4th Github commit - window resizing, dynamic DWM thumbnail
