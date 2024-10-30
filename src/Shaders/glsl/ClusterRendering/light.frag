#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_EXT_debug_printf : enable


#include "../Utils/RenderingUtil.glsl"
#include "../Utils/RStructs.glsl"

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
    PointLight[] pointLights;
};
layout (set = 0, binding = 5, scalar) buffer LightMap{
    ArrayIndexor[] lightMap;
};

bool SDF_Sphere(float radius, vec3 spherePos, vec3 pos, out float d){
    d = distance(spherePos, pos);
    return (d < radius);
}

vec3 EvalPointLight(PointLight light, vec3 col, vec3 pos, out bool evaluated){
    float d;
    evaluated = SDF_Sphere(light.radius, light.pos, pos, d);
    vec3 finalCol = col * light.col * (1.0/(1.0 + pow(d, 2.0))) * light.intensity;
    return finalCol;
}
float inverseLerp(float a, float b, float value) {
    return (value - a) / (b - a);
}

void main() {

    uvec2 fragCoord = uvec2(textCoord.x, textCoord.y);
    float depth = texture(gDepth, textCoord).r;
    vec4 col = texture(gCol, textCoord);
    vec4 norm = texture(gNormals, textCoord);

    
    //vec3 pos = GetPositionFromDepth(cProps.invProj, cProps.invView, depth, fragCoord);
    
    vec3 lightPos = vec3(0.0, 0.1, 0.0);
    vec3 lightDir = normalize(lightPos - col.xyz);
    vec3 finalCol = dot(lightDir, norm.xyz) * vec3(0.0, 0.01, 0.0) + vec3(0.01, 0.01, 0.01);
    
    int evalCounter = 0;
    for (int i = 0; i< 100 ; i++) {
        bool addValue = false;
        vec3 pointLightCol = EvalPointLight(pointLights[i], finalCol, col.xyz, addValue);
        if (addValue){
            evalCounter++;
            finalCol += (pointLightCol);
        }
        
        
    }
    if(evalCounter > 0){
        finalCol /= evalCounter;
    }
    outColor = vec4(finalCol, 1.0);

}