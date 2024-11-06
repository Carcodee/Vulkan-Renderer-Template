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
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 col = glm::vec3(1.0f);
    float radius = 0.0f;
    float intensity = 1.0f;
    float lAttenuation = 0.01f;
    float qAttenuation = 1.0f;
    PointLight(glm::vec3 pos, glm::vec3 col, float intensity, float lAttenuation, float qAttenuation)
    {
        this->pos = pos;
        this->col = col;
        this->intensity = intensity;
        this->lAttenuation = lAttenuation;
        this->qAttenuation = qAttenuation;
        CalculateRadiusFromAttenuation();
    }

    PointLight(glm::vec3 pos, glm::vec3 col, float radius, float intensity, float lAttenuation, float qAttenuation)
    {
        this->pos = pos;
        this->col = col;
        this->intensity = intensity;
        this->lAttenuation = lAttenuation;
        this->radius = radius;
        CalculateQAttenuationFromRadius();
    }
    
    
    void CalculateRadiusFromAttenuation()
    {
        //lAtt * r + qAtt * r *r + c + 100= 0;
        float a = lAttenuation;
        float b = qAttenuation;
        float c =  1 - (1 / 0.01f);
        float discriminant = static_cast<float>((b * b) - (4 * a * c));
        if (discriminant < 0)
        {
            std::cout<< "Invalid parameters for attenuation";
            radius = -1.0;
            return;
        }
        float r1 = (-b + sqrt(discriminant))/ (2.0f * a);
        float r2 = (-b - sqrt(discriminant))/ (2.0f * a);
        radius = std::max(r1, r2);
    }
    void CalculateQAttenuationFromRadius()
    {
        float threshold = 0.01f; // Define the threshold
        qAttenuation = (1.0f / threshold - 1.0f - lAttenuation * radius) / (radius * radius);
    }
    
};


#endif //LIGHTS_HPP
