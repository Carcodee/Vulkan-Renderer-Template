#ifndef U_MATH 
#define U_MATH 
float u_Lerp(float start, float end, float v){
    return start + v * (start - end);
}
float u_InvLerp(float start, float end, float v){
    return (v - start)/ (end - start);
}
float u_Remap(float iStart, float iEnd, float oStart, float oEnd, float v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}

vec2 u_Lerp(vec2 start, vec2 end, vec2 v){
    return start + v * (start - end);
}
vec2 u_InvLerp(vec2 start, vec2 end, vec2 v){
    return (v - start)/ (end - start);
}
vec2 u_Remap(vec2 iStart, vec2 iEnd, vec2 oStart, vec2 oEnd, vec2 v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}

vec3 u_Lerp(vec3 start, vec3 end, vec3 v){
    return start + v * (start - end);
}
vec3 u_InvLerp(vec3 start, vec3 end, vec3 v){
    return (v - start)/ (end - start);
}
vec3 u_Remap(vec3 iStart, vec3 iEnd, vec3 oStart, vec3 oEnd, vec3 v){
    return oStart + (oEnd - oStart) * (v - iStart)/(iEnd - iStart);
}

#endif 