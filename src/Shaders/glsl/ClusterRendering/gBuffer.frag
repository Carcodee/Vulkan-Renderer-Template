#version 450

layout(location = 0) out vec4 colors;
layout(location = 1) out vec4 normals;

layout(location = 0) in vec2 textCoord;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec4 fragPos;

layout (set = 0, binding = 0) uniform sampler2D color;
layout (set = 0, binding = 1) uniform sampler2D normal;

void main() {
    //colors = texture(color, textCoord);
    colors = fragPos;
    normals = vec4(norm, 1.0f);
    
}