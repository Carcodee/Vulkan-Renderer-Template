#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_EXT_debug_printf : enable


#include "../Utils/uRendering.glsl"
#include "../Utils/uStructs.glsl"
#include "../Utils/uMath.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 textCoord;

layout(set = 0, binding = 0) uniform sampler2D gCol;
layout(set = 0, binding = 1) uniform sampler2D gNormals;
layout(set = 0, binding = 2) uniform sampler2D gDepth;
layout(set = 0, binding = 3, scalar) uniform CameraProperties{
    mat4 invProj;
    mat4 invView;
}cProps;
layout (set = 0, binding = 4, scalar) buffer PointLights{
    u_PointLight[] pointLights;
};
layout (set = 0, binding = 5, scalar) buffer LightMap{
    u_ArrayIndexer[] lightMap;
};

vec3 EvalPointLight(u_PointLight light, vec3 col, vec3 pos, vec3 normal, out bool evaluated){
    float d;
    evaluated = SDF_Sphere(light.radius, light.pos, pos, d);
    vec3 lightDir = light.pos - pos;
    float diff = max(0.01, dot(lightDir, normal));
    float attenuation =  1.0/(1.0 + light.lAttenuation * d + light.qAttenuation * pow(d, 2.0));
    vec3 finalCol = col* diff * light.col * attenuation * light.intensity;
    return finalCol;
}

void main() {



    vec2 tilePos = u_Remap(vec2(0), vec2(1024.0), vec2(0), vec2(32), vec2(gl_FragCoord));
    tilePos.x = floor(tilePos.x);
    tilePos.y = floor(tilePos.y);
//    tilePos /= vec2(32);
    int mapIndex = int(tilePos.x) * 32 + int(tilePos.y);
    
   float lightsInTile = lightMap[mapIndex].size; 
    
    vec2 fragCoord = vec2(textCoord.x , textCoord.y);
    float depth = texture(gDepth, textCoord).r;
    vec4 col = texture(gCol, textCoord);
    vec4 norm = texture(gNormals, textCoord);
    if(norm == vec4(0.0)){

        discard;
    }

    vec3 pos = u_ScreenToWorld(cProps.invProj, cProps.invView, depth, fragCoord);
    
    vec3 lightPos = vec3(0.0, 4.0, 0.0);
    vec3 lightDir = normalize(lightPos - pos);
    vec3 lightCol = vec3(1.0, 1.0, 1.0);
    vec3 ambientCol = vec3(0.0, 0.0, 0.0);
    vec3 finalCol = col.xyz * dot(lightDir, norm.xyz) * lightCol  * 3.0f + ambientCol;
    
    int evalCounter = 0;
    for (int i = 0; i< 1000 ; i++) {
        bool addValue = false;
        vec3 pointLightCol = EvalPointLight(pointLights[i], finalCol, pos, norm.xyz, addValue);
        if (addValue){
            evalCounter++;
//            finalCol += (pointLightCol);
        }
    }
    if(evalCounter > 0){
//        finalCol /= evalCounter;
    }
    if(lightsInTile>0){
    }

    finalCol += vec3(float(lightsInTile), 0.0, 0.0)/1022.0;
    outColor = vec4(finalCol, 1.0);

}