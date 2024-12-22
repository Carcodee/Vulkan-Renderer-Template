#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_EXT_debug_printf : enable

#include "../Utils/uRendering.glsl"
#include "../Utils/uStructs.glsl"
#include "../Utils/uMath.glsl"

layout (location = 0) in vec2 textCoord;

layout(location = 0) out vec4 outColor;


layout(push_constant) uniform pushConstants{
    int cascadesCount;
    int fWidth;
    int fHeight;
    float baseIntervalLength;
}pc;

layout (set = 0, binding = 0)uniform sampler2D MergedCascades;
layout (set = 0, binding = 1, rgba8) uniform image2D PaintingLayers[];


void main() {

    int intervalCount = 4;
    int gridSize = 240;
    ivec2 coord = ivec2(gl_FragCoord.xy);
    ivec2 fSize = ivec2(pc.fWidth, pc.fHeight);
    ivec2 probeSizePx = ivec2(vec2(fSize) / float(gridSize));
    
    vec2 gridStartPos = vec2(ivec2(textCoord * gridSize)) / float(gridSize);
    
    ivec2 tlProbePos= ivec2(gridStartPos * fSize);
    ivec2 trProbePos= tlProbePos + ivec2(probeSizePx.x, 0.0);
    ivec2 blProbePos= tlProbePos + ivec2(0.0, probeSizePx.y);
    ivec2 brProbePos= tlProbePos + probeSizePx;
    
    vec2 tlPosTextCoords = vec2(vec2(tlProbePos) / vec2(fSize));
    vec2 trPosTextCoords = vec2(vec2(trProbePos) / vec2(fSize));
    vec2 blPosTextCoords = vec2(vec2(blProbePos) / vec2(fSize));
    vec2 brPosTextCoords = vec2(vec2(brProbePos) / vec2(fSize));

    vec4 baseCol = texture(MergedCascades, textCoord);
    vec4 tlCol = texture(MergedCascades, tlPosTextCoords);
    vec4 trCol = texture(MergedCascades, trPosTextCoords);
    vec4 blCol = texture(MergedCascades, blPosTextCoords);
    vec4 brCol = texture(MergedCascades, brPosTextCoords);
    
    vec2 cellFrac = (textCoord * gridSize) - floor(textCoord * gridSize);
    
    vec4 col = mix(mix(tlCol, trCol, cellFrac.x), mix(blCol, brCol, cellFrac.x), cellFrac.y);

    vec4 paintingImage = imageLoad(PaintingLayers[0], coord);
    
//    outColor = vec4(pos, 0.0, 1.0) + paintingImage;
    outColor = baseCol + paintingImage;

}