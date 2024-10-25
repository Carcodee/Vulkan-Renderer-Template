#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 textCoord;

layout(set = 0 , binding = 0) uniform sampler2D gCol;
layout(set = 0 , binding = 1) uniform sampler2D gNormals;
layout(set = 0 , binding = 2) uniform sampler2D gDepth;


void main() {

}