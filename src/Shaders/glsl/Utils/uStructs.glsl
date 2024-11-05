#ifndef R_STRUCTS 
#define R_STRUCTS 

//Lights

struct u_PointLight{
    vec3 pos;
    vec3 col;
    float radius;
    float intensity;
    float lAttenuation;
    float qAttenuation;
};

struct u_DirLight{
    vec3 pos;
    vec3 col;
    float intensity;
};

//End Lights

//Util

struct u_ArrayIndexer{
    float offset;
    float size;
};

//End Util

#endif 