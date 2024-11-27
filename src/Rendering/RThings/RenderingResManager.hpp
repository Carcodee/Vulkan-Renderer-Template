//

// Created by carlo on 2024-11-25.
//

#ifndef ENGINERESMANAGER_HPP
#define ENGINERESMANAGER_HPP

namespace Rendering
{
    class RenderingResManager
    {
    public:

        Material* PushMaterial(std::string name)
        {

            Material* material;
            if (!materialsNames.contains(name))
            {
                materials.emplace_back(std::make_unique<Material>());
                material = materials.back().get();
            }else
            {
                SYSTEMS::Logger::GetInstance()->SetLogPreferences(SYSTEMS::LogLevel::L_WARN);
                SYSTEMS::Logger::GetInstance()->LogMessage("Material with name: \"" + name + "\" already exist returning that material");
                material = GetMaterialFromName(name);
            }
            return material;
        }

        static RenderingResManager* GetInstance()
        {
            if (instance == nullptr)
            {
                instance = new RenderingResManager();
            }
            return instance;
        }

        Material* GetMaterialFromName(std::string name)
        {
            if (!materialsNames.contains(name))
            {
                SYSTEMS::Logger::GetInstance()->SetLogPreferences(SYSTEMS::LogLevel::L_ERROR);
                SYSTEMS::Logger::GetInstance()->LogMessage("Material with name: \"" +name +"\" does not exist");
                return nullptr;
            }
            return materials.at(materialsNames.at(name)).get();
        }
        
        int GetMaterialId(std::string name)
        {
             if (!materialsNames.contains(name))
            {
                SYSTEMS::Logger::GetInstance()->SetLogPreferences(SYSTEMS::LogLevel::L_ERROR);
                SYSTEMS::Logger::GetInstance()->LogMessage("Material with name: \"" +name +"\" does not exist");
                return -1;
            }
            return materialsNames.at(name);    
        }
        std::map<std::string, int> materialsNames;
        std::map<std::string, int> modelsNames;
        std::map<std::string, int> texturesNames;
        
        std::vector<std::unique_ptr<Material>> materials;
        std::vector<std::unique_ptr<Model>> models;

        RenderingResManager() = default;
        ~RenderingResManager() = default;
    private:
        static RenderingResManager* instance;
    };
    
    RenderingResManager* RenderingResManager::instance = nullptr;
    
}
#endif //ENGINERESMANAGER_HPP
