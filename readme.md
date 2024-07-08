# RAOE Core

This is my core C++ library that I share across multiple files.  

There is no rhyme or reason to why things go into core, other than each of these functions have been used at least once.  


corelib dependencies:

Current Dependencies:
    * spdlog
    * libassert
    * ctre

the CMakeLists exports the library as `raoe::core`, so simply link against that target to use it.



## CMake Library - Project layout

CMake is kind of annoying, so I created some helper macros for common tasks.  

My project layout is traditionally:

```
/{workspace}
  /game (or project-name)
    /src
    /third_party
        CMakeLists.txt [4]
    /test
        CMakeLists.txt [4]
    CMakeLists.txt [3]
  /modules
    /core (this repo as a git submodule)
    /other git submodule libraries...
    CMakeLists.txt [2]
CMakeLists.txt [1]
```



In CMakeLists.txt \[1] (the one in the project root), I set up some basics and begin the whole project bootstrapping.  I am a big fan of the very excellent [CMake Package Manager](https://github.com/cpm-cmake/CPM.cmake).  I spin this up and use it to acquire github projects as dependencies.  

```cmake
cmake_minimum_required(VERSION 3.26)

project("{project_name}")

#CPM Boilerplate goes here, edited out for brevity.  Copy the CMake Code in the CPM repo here.


add_subdirectory("modules")
add_subdirectory("game")

```
In CMakeLists.txt \[2], I generally just forward include to this repo, raoe-core.  I stick this repo in as a git submodule and just

```cmake
add_subdirectory("raoe-core")
```

in that file.  If I am including other third party libraries i build myself, i include them here as well (such as glad, or other raoe libraries i wrote)


in CMakeLists.txt \[3], I use the add_module cmake lists to define two modules:

```cmake
cmake_minimum_required(VERSION 3.26)

raoe_add_module(
    STATIC
    NAME "project-name"
    NAMESPACE "raoe"
    INCLUDE_DIRECTORIES
    PUBLIC
        "src"
    CPP_SOURCE_FILES
        "src/game.cpp"
        #and so on...
    DEPENDENCIES
    PUBLIC
        raoe::core
        #and other modules defined in the cmake project....       
)

raoe_add_module(
    EXECUTABLE
    NAME "project-name-exe"
    NAMESPACE "raoe"
    INCLUDE_DIRECTORIES
    PUBLIC
        "src"
    CPP_SOURCE_FILES
        "src/main.cpp"
    DEPENDENCIES
    PUBLIC
        raoe::project-name #note: generally i only include project-name here/ 
)
```

I create a static library with all my project's code, and an executable that links to that static library.  I do this because this makes my game code testable (more on that below).  You don't have to do this though.  you can put everything in the executable module, that works too.


CMakeLists.txt \[4]: `raoe_add_module` automatically includes the `third_party` and `test` libraries if they exist and have a CMakeLists.txt in them.  In the `third_party` CMakeLists.txt, I simply use CPM to include github dependencies I want for a given project (eg: just `CPMAddPackage("gh:g-truc/glm#master")`).  In `test` I use the `raoe_add_test` macro to define tests (more on that below).  It's best to export your project as a static library if you plan on using tests, as you can link your project's lib to the test suite and just call functions in your project.  


## CMake raoe_add_module

The macro `raoe_add_module` is a helper macro for bringing together a bunch of different elements and creating either a library or an executable

Libraries are always aliased as `NAMESPACE::NAME` (ie, if NAMESPACE = "raoe" and NAME = "core", the resulting project will export as `raoe::core`)


`raoe_add_module` has the following parameters:

#### Option modes:
    * STATIC        - Builds the module as a static library
    * SHARED        - Builds the module as a shared library (.dll or .so) (NOT IMPLEMENTED)
    * MODULE        - Builds the module as a cpp20+ module (NOT TESTED)
    * HEADERONLY    - Builds the module as a Interface or Header-Only library (Notes: see below)
    * EXECUTABLE    - Builds the module as an executable
 
#### One Value Args:
    * NAME              - REQUIRED.  The name of this module.   
    * NAMESPACE         - The namespace of this module.  Defaults to "raoe" if omitted
    * CXX_STANDARD      - The C++ standard version for this module.  Defaults to the project's CXX_STANDARD if omitted, or 20 if CXX_STANDARD  is not set.
    * TARGET_VARIABLE   - The CMake variable the target name will be stored into.  Useful for getting the real target name for various cmake commands. 

#### Multivalue Args:
    * CPP_SOURCE_FILES          - REQUIRED if not HEADERONLY.  The source files to include into the module.  If HEADERONLY, must be omitted
    * INCLUDE_DIRECTORIES       - The include directories for this module.  Requires the use of PUBLIC and PRIVATE.  Ignored if HEADERONLY or MODULE
    * COMPILE_DEFINITIONS       - The definitions to include with this module.  Requires the use of PUBLIC and PRIVATE
    * DEPENDENCIES              - Additional libraries to link.  See note about HEADERONLY
    * COPY_DIRECTORY            - Directories to copy to output folder.  Requires directories relative to the current source directory.  



### Examples

Some examples of setups i've used before:

```cmake

raoe_add_module(
    STATIC
    NAME "game-core"
    NAMESPACE "raoe"
    INCLUDE_DIRECTORIES
    PUBLIC
        "src"
    CPP_SOURCE_FILES
        "src/game.cpp"
        "src/render/render.cpp"
        "src/sys/window.cpp"
        "src/render/mesh.cpp"
        "src/render/shader.cpp"
    DEPENDENCIES
    PUBLIC
        spdlog
        flecs::flecs_static
        raoe::core
        glm::glm
        glfw
        nlohmann_json::nlohmann_json
        glad
    COMPILE_DEFINITIONS
    PUBLIC
        "NO_ASSERT_RELEASE_EVAL"
        "GLM_ENABLE_EXPERIMENTAL"
        "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING"
)

raoe_add_module(
    EXECUTABLE
    NAME "glstuff"
    NAMESPACE "raoe"
    INCLUDE_DIRECTORIES
    PUBLIC
        "src"
    CPP_SOURCE_FILES
        "src/main.cpp"
    DEPENDENCIES
    PUBLIC
        raoe::game-core
)
```

This is a standard setup for static module + executable

## Tests

Included is also a helper macro for adding tests to the project using `Catch2`.  If you use `raoe_add_module` in a folder, and that folder contains a subdirectory named `test` with a CMakeLists.txt in it, then that subdirectory will be automatically added.  This is useful for defining tests.

The macro `raoe_add_test` is a simplified call to `raoe_add_module` in executable mode.  It ensures that catch2 is downloaded and linked against the test executable, and that testing is enabled.  

I often just throw the .cpp files for my tests in the test folder with the CMakeLists.txt.  You can be more creative with test layout, but I've found it not worth doing for small projects.  

### Parameters:

#### OneValueArgs
    * NAME              - REQUIRED.  The name of the test.  
    * TARGET_VARIABLE   - Optional. The CMake variable the target name will be stored into.  Useful for getting the real target name for various cmake commands. 

#### MultiValue Args
    CPP_SOURCE_FILES        - REQUIRED.  The source files for testing
    INCLUDE_DIRECTORIES     - Optional.  The include directories for the test.  Often excluded.  
    COMPILE_DEFINITIONS     - Optional.  Compile definitions set for the test
    DEPENDENCIES            - Optional, but virtually required.  The dependencies to add to the test.  You probably want to add your project's module to this!


