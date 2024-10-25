#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tang;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec2 textCoord;
layout (location = 1) out vec3 norm;

layout(push_constant)uniform pushConstants{
    mat4 model;
    mat4 projView;
}pc;

void main() {
    gl_Position = pc.projView * pc.model * vec4(pos, 1.0f);
    norm = normal;
    textCoord = uv;
}