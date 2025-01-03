//
// Created by carlo on 2025-01-02.
//

#ifndef WIDGETS_HPP
#define WIDGETS_HPP

namespace UI{


    struct TextureViewer
    {
        ENGINE::ImageView* currImageView = nullptr;
        std::string labelName;
        ENGINE::ImageView* DisplayTexture(std::string name, ENGINE::ImageView* imageView, ImTextureID textureId, glm::vec2 size)
        {
            ImGui::PushID(name.c_str());
            ImGui::SeparatorText(name.c_str());
		    ImGui::Image(textureId, ImVec2{size.x, size.y});
            ImGui::PopID();
            this->currImageView = imageView;
            return this->currImageView;
        }
        
    };
    
    
}

#endif //WIDGETS_HPP
