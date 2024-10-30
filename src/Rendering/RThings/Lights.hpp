//
// Created by carlo on 2024-10-28.
//

#ifndef LIGHTS_HPP
#define LIGHTS_HPP

struct DirectionalLight
{
    glm::vec3 pos;
    glm::vec3 col;
    float intensity;
};
struct PointLight
{
    glm::vec3 pos;
    glm::vec3 col;
    float radius;
    float intensity;
    float lAttenuation;
    float qAttenuation;
};


#endif //LIGHTS_HPP
