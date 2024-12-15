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
}pc;

layout (set = 0, binding = 0) uniform sampler2D Cascades[];
layout (set = 0, binding = 1, rgba8) uniform image2D PaintingLayers[];

#define CASCADE_SIZE 4

vec4 MergeIntervals(vec4 near, vec4 far){
    vec3 radiance = near.rgb + (far.rgb * near.a);
    return vec4(radiance, near.rgb * far.a);
}

vec4 CastInterval(vec2 intervalStart, vec2 intervalEnd){
    vec4 accumulatedRadiance = vec4(0.0);
    vec2 dir = intervalEnd - intervalStart;
    int steps = 30;
    vec2 stepSize = vec2(dir)/float(steps);
    bool occluded = false;
    for (int i = 0; i < steps; ++i){
        vec2 pos = intervalStart + (stepSize * float(i));
        ivec2 screenPos = ivec2(pos * vec2(pc.fWidth, pc.fHeight));
        vec4 sampleCol= imageLoad(PaintingLayers[0], screenPos);
        if(sampleCol != vec4(0.0)){
            occluded = true;
            
        }else{
            sampleCol = vec4(0.01, 0.01, 0.01, 1.0);
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
    vec2 cascades[CASCADE_SIZE];
    for(int i= 0; i < CASCADE_SIZE; i++ ){
        cascades[i] = texture(Cascades[i], textCoord).xy;
    }

    vec2 texelDir[CASCADE_SIZE];
    int interval= 4;
    int gridSize = 16;
    vec4 radiances[CASCADE_SIZE];
    ivec2 probeIndexTest = ivec2(textCoord * gridSize) + 1;
    vec2 probePosTest = (vec2(probeIndexTest) / float(gridSize));
    for(int i= 0; i < CASCADE_SIZE; i++ ){
        
        uint dirCount = interval * interval;
        ivec2 textIdx = ivec2(cascades[i].xy * float(interval));
        uint dirIndex = textIdx.x + textIdx.y * interval;
        float angle = 2.0 * 3.1415 * ((float(dirIndex) + 0.5) / float(dirCount));
        texelDir[i] = vec2(cos(angle), sin(angle));
        
        ivec2 probeIndex = ivec2(textCoord * gridSize) + 1;
        vec2 probePos = (vec2(probeIndex) / float(gridSize));
        
        vec2 intervalRange = IntervalRange(i, 0.01);
        vec2 intervalStart = probePos + texelDir[i] * intervalRange.x;
        vec2 intervalEnd = probePos + texelDir[i] * intervalRange.y;
        radiances[i] = CastInterval(intervalStart, intervalEnd);
        interval = interval * 2;
        gridSize = gridSize / 2;
    }

    vec4 radiance = radiances[0]; 
    for(int i= 1; i < CASCADE_SIZE; i++ ){
        radiance = MergeIntervals(radiance, radiances[i]);
    }

    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 storageArr = imageLoad(PaintingLayers[0], coord);

    if(distance(probePosTest, textCoord) < 0.01){

        outColor = vec4(probePosTest, 0.0, 1.0);
    }else{
        outColor = vec4(0.0);
    }
//    outColor = radiance;

}