#ifndef U_MATH 
#define U_MATH 
//float
float u_Lerp(float start, float end, float v){
    return start + v * (end - start);
}
float u_InvLerp(float start, float end, float v){
    return (v - start)/ (end - start);
}
float u_Remap(float iStart, float iEnd, float oStart, float oEnd, float v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}

//vec2
vec2 u_Lerp(vec2 start, vec2 end, float v){
    return start + v * (end - start);
}
vec2 u_InvLerp(vec2 start, vec2 end, float v){
    return (v - start)/ (end - start);
}
vec2 u_Remap(vec2 iStart, vec2 iEnd, vec2 oStart, vec2 oEnd, float v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}
vec2 u_Lerp(vec2 start, vec2 end, vec2 v){
    return start + v * (end - start);
}
vec2 u_InvLerp(vec2 start, vec2 end, vec2 v){
    return (v - start)/ (end - start);
}
vec2 u_Remap(vec2 iStart, vec2 iEnd, vec2 oStart, vec2 oEnd, vec2 v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}

//vec3
vec3 u_Lerp(vec3 start, vec3 end, float v){
    return start + v * (end - start);
}
vec3 u_InvLerp(vec3 start, vec3 end, float v){
    return (v - start)/ (end - start);
}
vec3 u_Remap(vec3 iStart, vec3 iEnd, vec3 oStart, vec3 oEnd, float v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}
vec3 u_Lerp(vec3 start, vec3 end, vec3 v){
    return start + v * (end - start);
}
vec3 u_InvLerp(vec3 start, vec3 end, vec3 v){
    return (v - start)/ (end - start);
}
vec3 u_Remap(vec3 iStart, vec3 iEnd, vec3 oStart, vec3 oEnd, vec3 v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}

float u_Min(float v1, float v2){
    return min(v1, v2);
}
vec2 u_Min(vec2 v1, vec2 v2){
    return vec2(min(v1.x, v2.x), min(v1.y, v2.y));
}
vec3 u_Min(vec3 v1,vec3 v2){
    return vec3(min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z));
}

float u_Max(float v1, float v2){
    return max(v1, v2);
}
vec2 u_Max(vec2 v1, vec2 v2){
    return vec2(max(v1.x, v2.x), max(v1.y, v2.y));
}
vec3 u_Max(vec3 v1,vec3 v2){
    return vec3(max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z));
}
#endif 