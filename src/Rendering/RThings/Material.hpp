//
// Created by carlo on 2024-11-25.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

namespace Rendering
{
    enum TextureType
    {
        ALBEDO,
        NORMAL,
        EMISSION,
        TRANSMISSION,
        ROUGHNESS,
        METALLIC,
        METALLIC_ROUGHNESS,
    };
    class Material 
    {
    public:
        glm::vec4 diff = glm::vec4(1.0);
        float albedoFactor = 1.0;
        float normalFactor = 0.0;
        float roughnessFactor = 0.0;
        float metallicFactor = 0.0;
        float alphaCutoff = 0.0;
        std::map<TextureType, int> texturesOffsets{
            {ALBEDO, -1},
            {NORMAL, -1},
            {EMISSION, -1},
            {TRANSMISSION, -1},
            {ROUGHNESS, -1},
            {METALLIC, -1},
            {METALLIC_ROUGHNESS, -1},
        };

        void SetTexture(TextureType type, int index)
        {
            assert(texturesOffsets.contains(type) && "Texture type dont exist");
            texturesOffsets.at(type) = index;
        }


        
        
        

        
        

        
    };
}

#endif //MATERIAL_HPP
