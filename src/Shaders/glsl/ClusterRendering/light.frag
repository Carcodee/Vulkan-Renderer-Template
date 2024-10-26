#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout :enable

#include "../Utils/RenderingUtil.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 textCoord;

layout(set = 0, binding = 0) uniform sampler2D gCol;
layout(set = 0, binding = 1) uniform sampler2D gNormals;
layout(set = 0, binding = 2) uniform sampler2D gDepth;

layout(set = 0, binding = 3, std140) uniform CameraProperties{
    mat4 invProj;
    mat4 invView;
}cProps;


void main() {
    
    uvec2 fragPos = uvec2(textCoord.x, textCoord.y);
    float depth = texture(gDepth, textCoord).r;
    vec4 col = texture(gCol, textCoord);
    vec4 norm = texture(gNormals, textCoord);
    
    vec3 pos = GetPositionFromDepth(cProps.invView, cProps.invProj, depth, fragPos);
    
    vec3 lightPos = vec3(0.0, 10.0, 0.0);
    vec3 finalCol = max(0.01,dot(lightPos, norm.xyz)) * col.xyz * 3.0f;
    
    outColor = vec4(finalCol, 1.0);

}