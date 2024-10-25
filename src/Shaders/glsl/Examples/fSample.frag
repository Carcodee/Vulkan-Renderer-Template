#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 textCoord;


layout(set=0 , binding = 0) uniform sampler2D testImage;

layout (set = 0, binding = 1, rgba8) uniform image2D storageImg;

void main() {

    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 computeImage= imageLoad(storageImg, coord);
    vec4 text = texture(testImage, textCoord);
    outColor = textCoord;
}