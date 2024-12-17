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

layout (set = 0, binding = 0) uniform sampler2D Cascades[];
layout (set = 0, binding = 1, rgba8) uniform image2D PaintingLayers[];

#define CASCADE_SIZE 4

vec4 MergeIntervals(vec4 near, vec4 far){
    vec3 radiance = near.rgb + (far.rgb * near.a);
    return vec4(radiance, near.a * far.a);
}

vec4 CastInterval(vec2 intervalStart, vec2 intervalEnd, int cascadeIndex){
    vec4 accumulatedRadiance = vec4(0.0);
    vec2 dir = intervalEnd - intervalStart;
    int steps = (10 << (cascadeIndex));
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
        
        if(cascadeIndex == 1 ){
        }
        imageStore(PaintingLayers[2], screenPos, vec4(cascadeCol, 1.0));
        
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

vec4 BilinearWeights(vec2 ratio) {
    return vec4(
    (1.0 - ratio.x) * (1.0 - ratio.y),
    ratio.x * (1.0 - ratio.y),
    (1.0 - ratio.x) * ratio.y,
    ratio.x * ratio.y
    );
}

void BilinearSamples(vec2 dest_center, vec2 bilinearSize, out vec4 weights, out ivec2 baseIndex) {
    const vec2 base_coord = (dest_center / bilinearSize) - vec2(0.5, 0.5);
    const vec2 ratio = fract(base_coord); 
    weights = BilinearWeights(ratio);
    baseIndex = ivec2(floor(base_coord));
}
void main() {
    vec2 cascades[CASCADE_SIZE];
    for(int i= 0; i < CASCADE_SIZE; i++ ){
        cascades[i] = texture(Cascades[i], textCoord).xy;
    }

    int intervalGrid= 2;
    int gridSize = 16;
    vec4 radiances[CASCADE_SIZE];
    //debug
    vec2 texelDir[CASCADE_SIZE];
    vec2 probePositions[CASCADE_SIZE];
    vec2 startPositions[CASCADE_SIZE];

   
    for(int i= 0; i < CASCADE_SIZE; i++ ){
        
        uint dirCount = intervalGrid * intervalGrid;
        ivec2 textIdx = ivec2(cascades[i].xy * float(intervalGrid));
        uint dirIndex = textIdx.x + textIdx.y * intervalGrid;
        
        float angle = 2.0 * PI * ((float(dirIndex) + 0.5) / float(dirCount));
        texelDir[i] = vec2(cos(angle), sin(angle));


        ivec2 probeStart = ivec2(textCoord * gridSize);
        ivec2 probeEnd = ivec2(textCoord * gridSize) + 1;

        vec2 probeStartPos = (vec2(probeStart) / float(gridSize));
        vec2 probeEndPos = (vec2(probeEnd) / float(gridSize));
        vec2 deltaStartEndPos = (probeEndPos - probeStartPos)/2.0;

        vec2 probePos = probeStartPos + deltaStartEndPos;

        probePositions[i] = probePos;

        vec2 intervalRange = IntervalRange(i, pc.baseIntervalLength);
        vec2 intervalStart = probePos + texelDir[i] * intervalRange.x;
        vec2 intervalEnd = probePos + texelDir[i] * intervalRange.y;
        radiances[i] = CastInterval(intervalStart, intervalEnd, i);
        startPositions[i] = intervalStart; 
        
        intervalGrid = intervalGrid * 2;
        gridSize = gridSize / 2;
    }

    vec4 radiance = radiances[0]; 
    for(int i= 1; i < CASCADE_SIZE; i++ ){
        radiance = MergeIntervals(radiance, radiances[i]);
    }

    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 storageArr = imageLoad(PaintingLayers[0], coord);
    vec4 debugImg = imageLoad(PaintingLayers[2], coord);
//    
//    if(distance(startPositions[0], textCoord) < 0.001){
//        outColor = vec4(0.0);
//    }else{
//        outColor = vec4(probePositions[0], 0.0, 1.0);
//    }
//      outColor = storageArr + radiance;
    outColor = storageArr + radiances[0];
//    outColor = debugImg;

}