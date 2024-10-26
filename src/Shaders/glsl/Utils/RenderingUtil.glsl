
#ifndef RENDERING_UTIL 
#define RENDERING_UTIL 

vec3 GetPositionFromDepth(mat4 invView, mat4 invProj, float depth, uvec2 fragPos){
    vec4 ndcCoords = vec4(1.0f);
    ndcCoords.x = 2 * fragPos.x - 1.0f;
    ndcCoords.y = 2 * fragPos.y - 1.0f;
    ndcCoords.z = 2 * depth - 1.0f;
    
    ndcCoords = invProj * ndcCoords;
    ndcCoords = ndcCoords / ndcCoords.w;
    
    //world space
    ndcCoords = invView * ndcCoords;
    
    return ndcCoords.xyz;
}

#endif 