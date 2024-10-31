#ifndef R_UTIL 
#define R_UTIL 

vec3 GetPositionFromDepth(mat4 invProj, mat4 invView, float depth, vec2 screenPos){
    vec4 clipSpacePos = vec4(1.0f);
    clipSpacePos.x = 2 * screenPos.x - 1.0f;
    clipSpacePos.y = 2 * screenPos.y - 1.0f;
    clipSpacePos.z = depth;
    
    vec4 viewPos = invProj * clipSpacePos;
    viewPos = viewPos / viewPos.w;
    vec4 worldPos = invView * viewPos;
    
    return vec3(worldPos.xyz);
}
#endif 