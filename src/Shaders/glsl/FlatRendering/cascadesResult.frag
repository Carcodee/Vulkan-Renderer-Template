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
    int cascadeIndex;
}pc;

layout (set = 0, binding = 0)uniform sampler2D MergedCascades;
layout (set = 0, binding = 1, rgba8) uniform image2D PaintingLayers[];
layout (set = 0, binding = 2, rgba8) uniform image2D Radiances[];
layout (set = 0, binding = 3) uniform sampler2D TestImage;

layout (set = 0, binding = 4) uniform sampler2D SpriteAnims[];

layout (set = 0, binding = 5, scalar) buffer SpriteInfo{
    u_SpriteAnimationInfo spriteAnimInfo;
};

layout (set = 0, binding = 6) uniform sampler2D NormalMap;
layout (set = 0, binding = 7) uniform sampler2D BaseCol;
layout (set = 0, binding = 8) uniform sampler2D Roughness;
layout (set = 0, binding = 9) uniform sampler2D Height;
layout (set = 0, binding = 10) uniform sampler2D Ao;

layout (set = 0, binding = 11, scalar) uniform LightInfo{
    vec3 pos;
    vec3 col;
    float intensity;
}ubo;



void main() {
    vec3 lightDir = ubo.pos ;
   
    ivec2 coord = ivec2(gl_FragCoord.xy);
    ivec2 fSize = ivec2(pc.fWidth, pc.fHeight);
    
    int intervalCount = pc.intervalCount;
    int probeSizePx = pc.probeSizePx;
    
    vec2 probeCount = floor(vec2(fSize) / float(probeSizePx));
    vec2 baseIndex = ((vec2(gl_FragCoord.xy)) / probeSizePx);
    vec2 cellFrac = fract(baseIndex);

    baseIndex = floor(baseIndex);

    vec2 tlProbePos= baseIndex;
    vec2 trProbePos= baseIndex + vec2(1.0, 0.0);
    vec2 blProbePos= baseIndex + vec2(0.0, 1.0);
    vec2 brProbePos= baseIndex + vec2(1.0, 1.0);

    vec2 tlFragPos = tlProbePos * probeSizePx;
    vec2 trFragPos = trProbePos * probeSizePx;
    vec2 blFragPos = blProbePos * probeSizePx;
    vec2 brFragPos = brProbePos * probeSizePx;
    
    vec4 radiance = vec4(0.0);

    ivec2 currDir = ivec2(0.0);
    for (int i = 0; i < intervalCount * intervalCount; i++){

        currDir = ivec2(tlFragPos) + ivec2(i % probeSizePx, i / probeSizePx);
        radiance+= imageLoad(Radiances[0], currDir);
    }
    radiance /= (intervalCount * intervalCount);
//    
//    imageStore(Radiances[0], ivec2(gl_FragCoord.xy), radiance);
//
//    vec4 tlCol = imageLoad(Radiances[0], ivec2(tlFragPos));
//    vec4 trCol = imageLoad(Radiances[0], ivec2(trFragPos));
//    vec4 blCol = imageLoad(Radiances[0], ivec2(blFragPos));
//    vec4 brCol = imageLoad(Radiances[0], ivec2(brFragPos));
//
//    vec4 interpolated = vec4(0.0);
//    interpolated += mix(mix(tlCol, trCol, 0.0), mix(blCol, brCol, cellFrac.x), cellFrac.y);

//    vec4 baseCol = imageLoad(Radiances[0], ivec2(gl_FragCoord.xy));
    vec4 paintingImage = imageLoad(PaintingLayers[0], coord);
    vec4 testImg = texture(TestImage, textCoord);
    vec4 blackOc= imageLoad(PaintingLayers[1], coord);
    vec4 debug= imageLoad(PaintingLayers[2], coord);

    int size = spriteAnimInfo.rows * spriteAnimInfo.cols;
    vec2 spritePos = u_GetSpriteCoordInAtlas(spriteAnimInfo.currentFrame, spriteAnimInfo.spriteSizePx, spriteAnimInfo.rows, spriteAnimInfo.cols, ivec2(gl_FragCoord.xy), fSize);
    vec2 spritePos1 = u_GetSpriteCoordInAtlas((spriteAnimInfo.currentFrame + 1) % size, spriteAnimInfo.spriteSizePx, spriteAnimInfo.rows, spriteAnimInfo.cols, ivec2(gl_FragCoord.xy), fSize);
    vec4 spriteCol = texture(SpriteAnims[0], vec2(spritePos));
    vec4 spriteCol1 = texture(SpriteAnims[0], vec2(spritePos1));
    
    vec4 finalVal = mix(spriteCol, spriteCol1, spriteAnimInfo.interpVal);

    vec4 baseCol = texture(BaseCol, textCoord);
    vec4 normalMap = texture(NormalMap, textCoord);
    vec4 roughness = texture(Roughness, textCoord);
    vec4 ao = texture(Ao, textCoord);
    vec4 height = texture(Height, textCoord);
    
//    outColor = (baseCol * ao.x * height.x) * vec4(diff, 1.0);
    float normalMapCombined =pow(24.0,(normalMap.x * normalMap.y * normalMap.z));
    float specular = pow(2.0 ,max(0.0, normalMapCombined));
    vec4 finalCol = baseCol * radiance * radiance * normalMapCombined * specular * pow(2.0, roughness.x) * height;
    vec4 dirLightCol = abs(dot(normalMap.xyz, lightDir)) * vec4(0.0, 0.0, 1.0, 1.0) * 0.01;
    outColor = finalCol + dirLightCol;
    if(spriteCol.w > 0.8){
        outColor = spriteCol ;
    }
    if(paintingImage.w > 0.01){
        outColor = paintingImage;
    }

}