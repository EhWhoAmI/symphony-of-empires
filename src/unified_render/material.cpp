#include <algorithm>
#include <fstream>
#include <string>

#include "unified_render\path.hpp"
#include "unified_render/print.hpp"
#include "unified_render/material.hpp"
#include "unified_render/texture.hpp"

using namespace UnifiedRender;

std::vector<std::pair<Material*, std::string>> MaterialManager::load_wavefront(const std::string& path) {
    std::ifstream file(path);
    std::string line;

    print_info("Loading material file %s", path.c_str());

    std::vector<std::pair<Material*, std::string>> tmp_mat;
    Material* curr_mat = nullptr;
    while(std::getline(file, line)) {
        // Skip whitespace
        size_t len = line.find_first_not_of(" \t");
        if(len != std::string::npos)
            line = line.substr(len, line.length() - len);

        // Comment
        if(line[0] == '#' || line.empty()) continue;
        
        std::istringstream sline(line);
        std::string cmd;
        sline >> cmd;
        if(cmd != "newmtl" && curr_mat == nullptr) continue;

        if(cmd == "newmtl") {
            std::string name;
            sline >> name;
            curr_mat = new Material();
            tmp_mat.push_back(std::make_pair(curr_mat, name));
        } else if(cmd == "Ka") {
            glm::vec3 color;
            sline >> color.x >> color.y >> color.z;
            curr_mat->ambient_color = color;
        } else if(cmd == "Kd") {
            glm::vec3 color;
            sline >> color.x >> color.y >> color.z;
            curr_mat->diffuse_color = color;
        } else if(cmd == "Ks") {
            glm::vec3 color;
            sline >> color.x >> color.y >> color.z;
            curr_mat->specular_color = color;
        } else if(cmd == "Ns") {
            sline >> curr_mat->specular_exp;
        } else if(cmd == "Ni") {
            sline >> curr_mat->optical_density;
        } else if(cmd == "map_Kd") {
            std::string map_path;
            sline >> map_path;

            if(map_path[0] == '.') continue;
            curr_mat->diffuse_map = &g_texture_manager->load_texture(Path::get("3d/" + map_path));
        } else {
            print_info("Command %s not implemented", cmd.c_str());
        }
    }

    for(const auto& mat: tmp_mat) {
        materials.insert(mat);
    }
    return tmp_mat;
}

const Material& MaterialManager::loadMaterial(const std::string& path) {
find:
    auto it = std::find_if(materials.begin(), materials.end(), [&path](const std::pair<Material*, std::string>& element) {
        return (element.second == path);
    });
    if(it != materials.end())
        return *((*it).first);
    
    // Create a new material
    Material* mat = new Material();
    mat->ambient_map = &g_texture_manager->load_texture(Path::get("default.png"));
    materials.insert(std::make_pair(mat, path));
    goto find;
}

MaterialManager* g_material_manager;