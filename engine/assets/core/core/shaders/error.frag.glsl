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
layout(location=1, binding=0) uniform sampler2D tex;

out vec4 fragColor;

void main() {
    vec4 object_color = texture(tex, attributes.aUV0);

    float ambient_light = 0.1;
    vec3 diffuse_light = vec3(0.7, 0.7, 0.7);
    #if USE_NORMALS
    vec3 light_direction = normalize(vec3(2.0, 2.0, 2.0) - gl_Position);
    float intensity = max(dot(normalize(attributes.aNorm0), light_direction), 0);
    diffuse_light *= intensity;
    #endif
    object_color.rgb *= (vec3(ambient_light) + diffuse_light);
    fragColor = object_color;
}
