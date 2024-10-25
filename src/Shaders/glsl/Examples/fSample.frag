#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 textCoord;


layout(set=0 , binding = 0) uniform sampler2D testImage;


void main() {
    vec4 text = texture(testImage, textCoord);
    outColor = vec4(text);
}