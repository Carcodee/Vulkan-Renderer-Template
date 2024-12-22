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
    int probeSizePx;
    int intervalCount;
    int fWidth;
    int fHeight;
    int baseIntervalLength;
}pc;

layout (set = 0, binding = 0)uniform sampler2D MergedCascades;
layout (set = 0, binding = 1, rgba8) uniform image2D PaintingLayers[];


void main() {
   
    ivec2 coord = ivec2(gl_FragCoord.xy);
    ivec2 fSize = ivec2(pc.fWidth, pc.fHeight);
    
    int intervalCount = pc.intervalCount;
    int probeSizePx = pc.probeSizePx;
    
    vec2 probeCount = floor(vec2(fSize) / float(probeSizePx));

    vec2 baseIndex = (vec2(gl_FragCoord.xy) / probeSizePx);
    vec2 cellFrac = fract(baseIndex);
    
    baseIndex = floor(baseIndex);
    
    vec2 tlProbePos= baseIndex;
    vec2 trProbePos= baseIndex + vec2(1.0, 0.0);
    vec2 blProbePos= baseIndex + vec2(0.0, 1.0);
    vec2 brProbePos= baseIndex + vec2(1.0, 1.0);
    
    vec2 tlPosTextCoords = tlProbePos / probeCount;
    vec2 trPosTextCoords = trProbePos / probeCount;
    vec2 blPosTextCoords = blProbePos / probeCount;
    vec2 brPosTextCoords = brProbePos / probeCount;

    vec4 baseCol = texture(MergedCascades, textCoord);
    vec4 tlCol = texture(MergedCascades, tlPosTextCoords);
    vec4 trCol = texture(MergedCascades, trPosTextCoords);
    vec4 blCol = texture(MergedCascades, blPosTextCoords);
    vec4 brCol = texture(MergedCascades, brPosTextCoords);
    
    vec4 col = mix(mix(tlCol, trCol, cellFrac.x), mix(blCol, brCol, cellFrac.x), cellFrac.y);

    vec4 paintingImage = imageLoad(PaintingLayers[0], coord);
    vec4 debug= imageLoad(PaintingLayers[2], coord);
    
//    outColor = vec4(pos, 0.0, 1.0) + paintingImage;
    outColor = col + paintingImage;

}