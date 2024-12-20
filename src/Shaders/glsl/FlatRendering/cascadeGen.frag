#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_EXT_debug_printf : enable

#include "../Utils/uRendering.glsl"
#include "../Utils/uStructs.glsl"
#include "../Utils/uMath.glsl"


layout(push_constant)uniform pushConstants{
    int cascadeIndex;
    int intervalSize;
    int gridSize;
}pc;


layout (location = 0) in vec2 textCoord;

layout(location = 0) out vec4 outColor;

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
        ivec2 screenPos = ivec2(pos * vec2(pc.fWidth, pc.fHeight));

        vec4 sampleCol= imageLoad(PaintingLayers[0], screenPos);
        vec3 cascadeCol = u_HSLToRGB(cascadeIndex / 4.0, 0.8, 0.4);
        if(cascadeIndex == 3){
            imageStore(PaintingLayers[2], screenPos, vec4(cascadeCol, 1.0));
        }

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
    return float(1 << (1 * cascadeIndex));
}

vec2 IntervalRange(int cascadeIndex, float baseLenght){
    return baseLenght * vec2(IntervalScale(cascadeIndex), IntervalScale(cascadeIndex + 1));
}

void main() {

    vec2 textCoordPerGrid = vec2(1.0)/ pc.gridSize;
    ivec2 probeId = ivec2(textCoord * pc.gridSize);
    vec2 gridCol = vec2(probeId) / pc.gridSize;
    
    vec2 origin = textCoordPerGrid * vec2(probeId);

    vec2 localUV = ((textCoord) - origin) / textCoordPerGrid;
    
    ivec2 index = ivec2(localUV * (pc.intervalSize));
    vec2 indexCol = vec2(index)/(pc.intervalSize);
    
    int dirCount = pc.intervalSize;
    int dirIndex = index.x + index.y * (pc.intervalSize);
    
    float angle = 2.0 * PI * ((float(dirIndex) + 0.5) / float(dirCount));
    vec2 texelDir = vec2(cos(angle), sin(angle));

    ivec2 probeStart = ivec2(textCoord * pc.gridSize);
    ivec2 probeEnd = ivec2(textCoord * pc.gridSize) + 1;

    vec2 probeStartPos = (vec2(probeStart) / float(pc.gridSize));
    vec2 probeEndPos = (vec2(probeEnd) / float(pc.gridSize));
    vec2 deltaStartEndPos = (probeEndPos - probeStartPos)/2.0;

    vec2 probePos = probeStartPos + deltaStartEndPos;
    probeSizesPx[i] = ivec2(vec2(pc.gridSize) * vec2(pc.fWidth, pc.fHeight));
    probeCentersPositionsPx[i] = ivec2(probePos * vec2(pc.fWidth, pc.fHeight));

    if(distance(probePos, textCoord) < 0.01){
    }

    vec2 intervalRange = IntervalRange(i, pc.baseIntervalLength);
    vec2 intervalStart = probePos + texelDir * intervalRange.x;
    vec2 intervalEnd = probePos + texelDir * intervalRange.y;
    
    outColor = CastInterval(intervalStart, intervalEnd, i);

}