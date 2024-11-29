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
        MaterialPackedData materialPackedData;

        std::map<TextureType, int> texturesOffsets{
            {ALBEDO, -1},
            {NORMAL, -1},
            {EMISSION, -1},
            {TRANSMISSION, -1},
            {ROUGHNESS, -1},
            {METALLIC, -1},
            {METALLIC_ROUGHNESS, -1},
        };

        void SetTexture(TextureType type, int offset)
        {
            assert(texturesOffsets.contains(type) && "Texture type dont exist");
            texturesOffsets.at(type) = offset;
            switch (type) {
            case ALBEDO:
                materialPackedData.albedoOffset = offset;
                break;
            case NORMAL:
                materialPackedData.normalOffset = offset;
                break;
            case EMISSION:
                materialPackedData.albedoOffset = offset;
                break;
            case TRANSMISSION:
                materialPackedData.transOffset = offset;
                break;
            case ROUGHNESS:
                materialPackedData.roughnessOffset = offset;
                break;
            case METALLIC:
                materialPackedData.metallicOffset = offset;
                break;
            case METALLIC_ROUGHNESS:
                materialPackedData.metRoughnessOffset = offset;
                break;
            }
        
        }
        
        


        
        
        

        
        

        
    };
}

#endif //MATERIAL_HPP
