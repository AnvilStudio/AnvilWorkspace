![Anvil Banner](AnvilEngine/AnvilBanner.png "Anvil Banner")

# Anvil Engine 
- `Anvil Engine` is built with a powerful, flexible toolset designed to empower developers in creating immersive experiences, whether you're building games, interactive simulations, or lightweight applications. <br> 

- Anvil provides a solid foundation with multi-window support, a range of customizable components
and a versatile renderer that balances performance with simplicity. <br>

- With Anvil, you’ll have the freedom to focus on crafting your vision without unnecessary overhead, thanks to our lightweight, hardware-focused renderer. Our mission is to support developers by offering both high-level tools for rapid development and fine-tuned, API-specific functionality for those who want to dive deeper.

Thank you for choosing Anvil Engine—let’s get started building something incredible!
## Table of contents
1. [Overview](#overview)    
2. [Installation](#installation)
3. [Structure](#structure)
4. [Naming Convention](#naming-convention)
5. [API Documentation](#api-documentation)

---

## Overview

Anvil Engine is a modular, developer-friendly engine built for flexibility, offering everything needed to create dynamic applications and games. It’s designed with two primary use cases in mind: high-performance game development and lightweight application development. Anvil’s structure includes a high-level layer of classes that streamline development with intuitive components and a lower-level API-specific layer for developers who need greater control over rendering and functionality.

### Key Features

- **Multi-Window Support**: Manage multiple windows seamlessly for complex applications and dynamic interfaces.
- **Customizable Renderer**: Choose between an optimized hardware renderer for performance-driven applications and a stripped-down version for lightweight projects.
- **Component-Based Architecture**: A library of components provides bare-bones elements for app development without the overhead of a full game-driven renderer.
- **High-Level and Low-Level Access**: High-level tools accelerate your workflow, while lower-level API-specific access gives you the freedom to customize as needed.
- **Built-In GLFW Support**: Integrated GLFW ensures cross-platform compatibility and handles system-level details, so you can focus on what matters most—your application’s functionality and experience.

Anvil Engine is built to serve as a comprehensive toolset that grows with your project. Whether you’re building a game or a simple app, Anvil has the flexibility and power to bring your ideas to life.

---
## Installation
Installation is meant to be as simple as possible. 

### Prereq
- `c++20`
- `Microsoft Visual Studio 2022`

1. Clone the dev repository.<br>
```git clone --recursive -b dev https://github.com/AnvilStudio/AnvilEngine.git```
2. In the  root folder, run... <br>
```.\vendor\premake.exe vs2022```
3. Open the `AnvilWorkspace.sln` file
4. Build and run!
    
---

## Structure

Anvil follows a specific file structure known as a `Library/Application` structure.<br>

the files are as follows

```
    AnvilEngine/ (static lib)
        L include/ (client headers)
        L src/ (main library implimentation)
        |    L Core/
        |        L High Level code (i.e. - App class, api - agnostic)
        |    L Render/ (All Rendering + Renderer)
        |    |    L Context/ (api - agnostic)
        |    |    L Platform/ (api - specific)
        |    |    |    L Vulkan/ (api implimentation)
        |    |    |    L OtherAPI/
        |    L Util/ (Engine Utility like macros/logging)
        L vendor/ (3rd party libs like GLFW)

        |
        | Engine turns into a static lib for client to use
        |
        V

    Client/
        L src/  
```

---

## Naming Convention

when contributing to Anvil, you MUST follow the naming convention!


#### Tests
- All Tests should start with `tst_`

#### Files
- All File names should be `Upper Camel Case` like so: `MyCppFile.cpp`

#### Macros
- All Macros should start with `ANV_` and `USE_ALL_CAPS_WITH_UNDERS`

#### Global
Avoid using globals
- All global vars should start with `g_` then be `Upper Camel Case`

#### Namespace 
rules only apply if the function is not apart of the `anv` namespace
- All namespace functions should start with the `first 3 letters` of the namespace, underscore, then `Upper Camel Case` for the function name like so:
``` cpp
namespace MyNamespc
{
    int MYN_AddTwoNumbers();
}
```
- All nested namespaces within namespace `anv` should be lowercase

#### Functions
applys to constructors and deconstructors
- All parameter names should have an `underscore` at the beginning and be `Lower Camel Case` like so:
`void MyFn(int _myInt);`

#### Classes
- All Class Names should follow `Upper Camel Case`
- All non static member variables must start with `m_<VarName>`
- All static member variables must start with `s_<StaticVarName>`
- All public and static member Functions/Constructors/Deconstructors must be `Upper Cammel Case` like so: `public void MyFunctionIsGreat();`
- All private member Functions/Constructors/Deconstructors should be `Lower Camel Case with Underscores` like so: `private void my_function_is_great();`

#### Structs
- All Struct names should be `Upper Camel Case` 
- All Members should be `Lower Camel Case` like so: `int myInt;`
- All pointer types should start with a `p` like so: `int* pMyPointer;`
- Fns/Constr/DeConst should all be `Upper Camel Case` 

#### Enums
- All Enum names should be `Upper Camel Case`
- All Enum Values should be `ALL_CAPS_WITH_UNDERS`


---

## API Documentation

under  construction