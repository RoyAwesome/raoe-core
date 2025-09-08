#version 460 core
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
#include "core/shaders/common.glsl"

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV0;
layout(location=2) in vec4 inColor0;
layout(location=3) in vec3 inNormal;


void main() {
    gl_Position = camera.proj_cam * vec4(inPosition, 1);
    attributes.aNorm0 = inNormal;
    attributes.aUV0 = inUV0;
    attributes.aColor0 = inColor0;
}
