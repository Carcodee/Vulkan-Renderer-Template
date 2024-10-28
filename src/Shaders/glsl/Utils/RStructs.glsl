#ifndef R_STRUCTS 
#define R_STRUCTS 

//Lights

struct PointLight{
    vec3 pos;
    vec3 col;
    float radius;
    float intensity;
};

struct DirLight{
    vec3 pos;
    vec3 col;
    float intensity;
};

//End Lights

//Util

struct ArrayIndexor{
    int offset;
    int size;
};

//End Util

#endif 