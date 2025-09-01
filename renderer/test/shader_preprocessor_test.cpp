/*
    Copyright 2022-2025 Roy Awesome's Open Engine (RAOE)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

 */

#include "core/core.hpp"
#include "spdlog/spdlog.h"

#include <catch2/catch_test_macros.hpp>

#include "render/render.hpp"
#include "render/shader.hpp"

TEST_CASE("Include Test", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#include "test_include.glsl"
void main()
{

}
    )GLSL";

    std::string test_include = R"GLSL(#version 330 core
//This is a test include
uniform mat4 test;
)GLSL";

    std::function<std::string(std::string)> load_file_callback = [&test_include](const std::string& source) {
        return test_include;
    };

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback);

    std::string combined = R"GLSL(#version 330 core
#version 330 core
#line 2 1
//This is a test include
uniform mat4 test;
#line 3 0
void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Recursive Test", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#include "test_include.glsl"
void main()
{

}
    )GLSL";

    std::string test_include = R"GLSL(#version 330 core
//This is a test include
uniform mat4 test;
#include "test_include_2.glsl"
)GLSL";

    std::string test_include_2 = R"GLSL(uniform mat4 test2;)GLSL";

    std::function<std::string(std::string)> load_file_callback = [&test_include,
                                                                  &test_include_2](const std::string& source) {
        if(source == "test_include.glsl")
        {
            return test_include;
        }
        if(source == "test_include_2.glsl")
        {
            return test_include_2;
        }
        return std::string();
    };

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback);

    std::string combined = R"GLSL(#version 330 core
#version 330 core
#line 2 1
//This is a test include
uniform mat4 test;
#line 1 2
uniform mat4 test2;
#line 6 1
#line 3 0
void main()
{

}
    )GLSL";
    REQUIRE(preprocessed == combined);
}

TEST_CASE("Pragma Once Test", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#include "test_include.glsl"
#include "test_include.glsl"
void main()
{

}
    )GLSL";

    std::string test_include = R"GLSL(#pragma once
#version 330 core
//This is a test include
uniform mat4 test;
)GLSL";

    const std::function<std::string(std::string)> load_file_callback =
        [&test_include](const std::string& source) -> std::string { return test_include; };

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback);

    std::string combined = R"GLSL(#version 330 core
#version 330 core
#line 2 1
//This is a test include
uniform mat4 test;
#line 3 0

void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Injection Test", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#inject <_RAOE_COMMON_GLSL>
void main()
{

}
    )GLSL";

    std::function<std::string(std::string)> load_file_callback = [](const std::string& source) { return ""; };
    const std::unordered_map<std::string, std::string> injections = {
        {"_RAOE_COMMON_GLSL", R"GLSL(#define _RAOE_COMMON 1)GLSL"}
    };

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback, injections);

    std::string combined = R"GLSL(#version 330 core
#define _RAOE_COMMON 1
void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Common Injections For Fragment shader", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#inject <_RAOE_COMMON_DEFINES>
void main()
{

}
    )GLSL";

    std::function<std::string(std::string)> load_file_callback = [](const std::string& source) { return ""; };
    std::unordered_map<std::string, std::string> injections;
    raoe::render::shader::glsl::injections_for_shader_type(injections, raoe::render::shader::shader_type::fragment);

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback, injections);

    std::string combined = R"GLSL(#version 330 core
#define _RAOE_STAGE_VERTEX 0
#define _RAOE_STAGE_FRAGMENT 1
#define _RAOE_STAGE_GEOMETRY 0
#define _RAOE_STAGE_TESSELLATION_CONTROL 0
#define _RAOE_STAGE_TESSELLATION_EVALUATION 0
#define _RAOE_STAGE_MESH 0
#define _RAOE_STAGE_COMPUTE 0
void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Injection Not Provided is Removed", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#inject <_RAOE_COMMON_DEFINES>
void main()
{

}
    )GLSL";

    std::function<std::string(std::string)> load_file_callback = [](const std::string& source) { return ""; };
    const std::unordered_map<std::string, std::string> injections = {
        {"_RAOE_SOME_OTHER_INJECTION", R"GLSL(#define _RAOE_COMMON 1)GLSL"}
    };
    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback, injections);

    std::string combined = R"GLSL(#version 330 core

void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Injection Not Provided is Removed MultiInjection", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#inject <_RAOE_COMMON_DEFINES>
#inject <_RAOE_ANOTHER_NOT_DEFINED>
void main()
{

}
    )GLSL";

    std::function<std::string(std::string)> load_file_callback = [](const std::string& source) { return ""; };
    const std::unordered_map<std::string, std::string> injections = {
        {"_RAOE_SOME_OTHER_INJECTION", R"GLSL(#define _RAOE_COMMON 1)GLSL"}
    };
    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback, injections);

    std::string combined = R"GLSL(#version 330 core


void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Include With Injection Test", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#include "test_include.glsl"
void main()
{

}
    )GLSL";

    std::string test_include = R"GLSL(#version 330 core
//This is a test include
uniform mat4 test;
#inject <_RAOE_COMMON_GLSL>
)GLSL";

    std::function<std::string(std::string)> load_file_callback = [&test_include](const std::string& source) {
        return test_include;
    };
    const std::unordered_map<std::string, std::string> injections = {
        {"_RAOE_COMMON_GLSL", R"GLSL(#define _RAOE_COMMON 1)GLSL"}
    };

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback, injections);

    std::string combined = R"GLSL(#version 330 core
#version 330 core
#line 2 1
//This is a test include
uniform mat4 test;
#define _RAOE_COMMON 1
#line 3 0
void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}

TEST_CASE("Include With Injection Test And Extra Inject", "[SHADER][PREPROCESSOR]")
{
    std::string shader_source = R"GLSL(#version 330 core
#include "test_include.glsl"
void main()
{

}
    )GLSL";

    std::string test_include = R"GLSL(#version 330 core
//This is a test include
uniform mat4 test;
#inject <_RAOE_COMMON_GLSL>
#inject <_RAOE_SOME_OTHER_INJECTION>
)GLSL";

    std::function<std::string(std::string)> load_file_callback = [&test_include](const std::string& source) {
        return test_include;
    };
    const std::unordered_map<std::string, std::string> injections = {
        {"_RAOE_COMMON_GLSL", R"GLSL(#define _RAOE_COMMON 1)GLSL"}
    };

    std::string preprocessed = raoe::render::shader::glsl::preprocess(shader_source, load_file_callback, injections);

    std::string combined = R"GLSL(#version 330 core
#version 330 core
#line 2 1
//This is a test include
uniform mat4 test;
#define _RAOE_COMMON 1

#line 3 0
void main()
{

}
    )GLSL";

    REQUIRE(preprocessed == combined);
}