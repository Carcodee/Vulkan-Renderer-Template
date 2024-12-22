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

layout (set = 0, binding = 0) uniform sampler2D Cascades[];
layout (set = 0, binding = 1, rgba8) uniform image2D PaintingLayers[];
layout (set = 0, binding = 2, rgba8) uniform image2D Radiances[];

#define CASCADE_SIZE 4

vec4 MergeIntervals(vec4 near, vec4 far){
    vec3 radiance = near.rgb + (far.rgb * near.a);
    return vec4(radiance, near.a * far.a);
}

vec4 CastInterval(vec2 intervalStart, vec2 intervalEnd, int cascadeIndex){
    vec4 accumulatedRadiance = vec4(0.0);
    vec2 dir = intervalEnd - intervalStart;
    int steps = (30 << (cascadeIndex));
//    int steps = 10;
    vec2 stepSize = vec2(dir)/float(steps);
    bool occluded = false;
    
    for (int i = 0; i < steps; i++){
        vec2 pos = intervalStart + (stepSize * float(i));
//        if (pos.x < 0 || pos.x >= 1.0 || pos.y < 0 || pos.y >= 1.0) {
//            continue;
//        }
//        ivec2 screenPos = ivec2(pos * vec2(pc.fWidth, pc.fHeight));
        ivec2 screenPos = ivec2(pos);
        
        vec4 sampleCol= imageLoad(PaintingLayers[0], screenPos);
        vec3 cascadeCol = u_HSLToRGB(cascadeIndex / 4.0, 0.8, 0.4);
        if(cascadeIndex == 3){
            imageStore(PaintingLayers[2], screenPos, vec4(cascadeCol, 1.0));
        }       
   
        if(sampleCol != vec4(0.0)){
            occluded = true;
        }else{
            sampleCol = vec4(0.0, 0.0, 0.0, 1.0);
        }
        accumulatedRadiance+= sampleCol;
    }
    accumulatedRadiance = accumulatedRadiance / float(steps);
    if(occluded){
        accumulatedRadiance.w = 0.0;
    }
    return accumulatedRadiance;
}

float IntervalScale(int cascadeIndex){
    if(cascadeIndex<= 0){
        return 0.0;
    }
    return float(1 << (2 * cascadeIndex));
}

vec2 IntervalRange(int cascadeIndex, float baseLenght){
    return baseLenght * vec2(IntervalScale(cascadeIndex), IntervalScale(cascadeIndex + 1));
}


void main() {
    float aspect = float(pc.fWidth)/float(pc.fHeight);
    vec2 normalizedTextCoords = vec2(textCoord.x, textCoord.y);
    ivec2 coord = ivec2(gl_FragCoord.xy);
    
    int intervalGrid= pc.intervalCount;
    int gridSize = pc.probeSizePx;
    //debug
    vec2 possi;
   
    for(int i= 0; i < CASCADE_SIZE; i++ ){

        vec2 cascades = texture(Cascades[i], textCoord).xy;
        uint dirCount = intervalGrid * intervalGrid;
        ivec2 textIdx = ivec2(cascades.xy * float(intervalGrid));
        int dirIndex = textIdx.x + textIdx.y * intervalGrid;
        
        float angle = 2.0 * PI * ((float(dirIndex) + 0.5) / float(dirCount));
        vec2 texelDir = vec2(cos(angle), sin(angle));

        vec2 probeCenterGrid = floor(vec2(gl_FragCoord.xy) / float(gridSize)) + vec2(0.5);

        vec2 probePos = (probeCenterGrid * (gridSize));
//        probePos.y /= aspect;
        
        possi = probePos;

        if(distance(probePos, normalizedTextCoords) < 0.01){
        }
        vec2 intervalRange = IntervalRange(i, pc.baseIntervalLength);
        vec2 intervalStart = probePos + texelDir * intervalRange.x;
        vec2 intervalEnd = probePos + texelDir * intervalRange.y;
        vec4 radiance = CastInterval(intervalStart, intervalEnd, i);
        
        imageStore(Radiances[i], coord, radiance);

        intervalGrid *= 2;
        gridSize *= 2;
    }

    vec4 storageArr = imageLoad(PaintingLayers[0], coord);
    

}