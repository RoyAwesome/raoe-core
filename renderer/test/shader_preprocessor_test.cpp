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

TEST_CASE("Preprocessor Test", "[SHADER][PREPROCESSOR]")
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
