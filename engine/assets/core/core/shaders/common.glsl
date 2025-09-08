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

#pragma once

#inject <_RAOE_COMMON_DEFINES>
#inject <_RAOE_ENGINE_VERTEX_FORMAT_DEFINES>

layout(std140, binding = 0) uniform EngineUniforms {
    vec2 screen_size;
    float time;
} engine_uniforms;

layout(std140, binding = 1) uniform Camera {
    mat4 camera_matrix;
    mat4 projection_matrix;
    mat4 proj_cam;// projection_matrix * camera_matrix, for convenience
} camera;

#if _RAOE_STAGE_VERTEX || _RAOE_STAGE_FRAGMENT
#if _RAOE_STAGE_VERTEX
#define RASTERIZER_QUALIFER out
#elif _RAOE_STAGE_FRAGMENT
#define RASTERIZER_QUALIFER in
#endif

RASTERIZER_QUALIFER _RASTERIZER_BLOCK {
    vec2 aUV0;
    vec3 aNorm0;
    vec4 aColor0;
} attributes;
#endif