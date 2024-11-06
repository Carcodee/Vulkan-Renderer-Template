#ifndef R_UTIL 
#define R_UTIL 

vec3 u_ScreenToWorld(mat4 invProj, mat4 invView, float depth, vec2 screenPos){
    vec4 clipSpacePos = vec4(1.0f);
    clipSpacePos.x = 2 * screenPos.x - 1.0f;
    clipSpacePos.y = 2 * screenPos.y - 1.0f;
    clipSpacePos.z = depth;
    
    vec4 viewPos = invProj * clipSpacePos;
    viewPos = viewPos / viewPos.w;
    
    vec4 worldPos = invView * viewPos;
    return vec3(worldPos.xyz);
}

vec3 u_ScreenToView(mat4 invProj, float depth, vec2 screenPos){
    vec4 clipSpacePos = vec4(1.0f);
    clipSpacePos.x = 2 * screenPos.x - 1.0f;
    clipSpacePos.y = 2 * screenPos.y - 1.0f;
    clipSpacePos.z = depth;

    vec4 viewPos = invProj * clipSpacePos;
    viewPos = viewPos / viewPos.w;

    return vec3(viewPos.xyz);
}
float u_SDF_Sphere(float radius, vec3 spherePos, vec3 pos){
    
    return distance(spherePos, pos);
}
bool u_SDF_AABB(vec2 minAABB, vec2 maxAABB, vec2 pos){
    bool xCheck = bool(pos.x > minAABB.x && pos.x < maxAABB.x);
    bool yCheck = bool(pos.y > minAABB.y && pos.y < maxAABB.y);
    return xCheck && yCheck;
}
bool u_SDF_AABB(vec3 minAABB, vec3 maxAABB, vec3 pos){
    bool xCheck = bool(pos.x > minAABB.x && pos.x < maxAABB.x);
    bool yCheck = bool(pos.y > minAABB.y && pos.y < maxAABB.y);
    bool zCheck = bool(pos.z > minAABB.z && pos.z < maxAABB.z);
    return xCheck && yCheck;
}

bool u_SDF_AABB_Sphere(vec2 min, vec2 max, vec2 lightPos, float lightRadius){
    
    float closestX = clamp(lightPos.x, min.x, max.x);
    float closestY = clamp(lightPos.y, min.y, max.y);
    
    float dx = lightPos.x - closestX;
    float dy = lightPos.y - closestY;
    
    float distanceSqr = dx * dx + dy * dy;
    
    return distanceSqr < (lightRadius * lightRadius);
}
bool u_SDF_AABB_Sphere(vec3 min, vec3 max, vec3 lightPos, float lightRadius){

    float closestX = clamp(lightPos.x, min.x, max.x);
    float closestY = clamp(lightPos.y, min.y, max.y);
    float closestZ = clamp(lightPos.z, min.z, max.z);

    float dx = lightPos.x - closestX;
    float dy = lightPos.y - closestY;
    float dz = lightPos.z - closestZ;

    float distanceSqr = dx * dx + dy * dy + dz * dz ;

    return distanceSqr < (lightRadius * lightRadius);
}


#endif 