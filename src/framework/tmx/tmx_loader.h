#ifndef __RETRO_CORE_FRAMEWORK_TMX_TMX_LOADER_H
#define __RETRO_CORE_FRAMEWORK_TMX_TMX_LOADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "framework/game_engine/asset_manager.h"

namespace RetroCore {

namespace tmx {

class PropertyMap: public std::map<std::string, std::string> {
    public:
        PropertyMap() = default;

        const std::string& getAsString(const std::string& key) const {
            static const std::string sEmptyString;
            auto it = find(key);
            if(it == end()) {
                return sEmptyString;
            }
            return it->second;
        }
};

struct Tileset {
    std::string name;
    unsigned int firstGid = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    int spacing = 0;
    int margin = 0;
    int tileCount = 0;
    int columns = 0;
    
    std::string imageSource; 
    int imageWidth = 0;
    int imageHeight = 0;

    PropertyMap properties;                          // Global tileset properties
    std::map<unsigned int, PropertyMap> tileProperties; // Tile-specific properties (Key: Local Tile ID)

    const PropertyMap* getPropertiesForGid(unsigned int gid) const {
        if(gid < firstGid) return nullptr;
        auto it = tileProperties.find(gid - firstGid);
        if(it != tileProperties.end()) {
            return &it->second;
        }
        return nullptr;
    }

};

struct Layer {
    std::string name;
    int width = 0;
    int height = 0;
    std::vector<unsigned int> data; 
    PropertyMap properties;
};

struct Object {
    int id = 0;
    std::string name;
    std::string type; 
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    unsigned int gid = 0; 
    PropertyMap properties;
};

struct ObjectLayer {
    int id = 0;
    std::string name;
    std::vector<Object> objects;
    PropertyMap properties;
};

struct Map {
    int width = 0;
    int height = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    
    std::vector<Tileset> tilesets;
    std::vector<Layer> layers;
    std::vector<ObjectLayer> objectLayers;
    PropertyMap properties;

    const Tileset* getTilesetForGid(unsigned int gid) const {
        const Tileset* result = nullptr;
        unsigned int maxFirstGid = 0;
        for (const auto& ts : tilesets) {
            if (gid >= ts.firstGid && ts.firstGid >= maxFirstGid) {
                maxFirstGid = ts.firstGid;
                result = &ts;
            }
        }
        return result;
    }
};

namespace internal {
    inline std::string get_attr(const std::string& tag, const std::string& attr) {
        std::string target = attr + "=\"";
        size_t start = tag.find(target);
        if (start == std::string::npos) {
            return "";
        }
        
        start += target.length();
        size_t end = tag.find("\"", start);
        if (end == std::string::npos) {
            return "";
        }

        return tag.substr(start, end - start);
    }

    inline std::string load_file_string(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        return buffer.str();
    }

    inline std::string load_file_string_from_asset_manager(const std::string& path, const AssetManager& assetManager) {
        const Asset& asset = assetManager.getAsset(path);
        if(!asset.isValid()) {
            std::cerr << "Error loading file " << path << std::endl;
        }
        return asset.getDataAsString();
    }

    inline PropertyMap parse_properties(const std::string& block, size_t startPos, size_t endPos) {
        PropertyMap props;
        size_t propsStart = block.find("<properties", startPos);
        if (propsStart == std::string::npos || propsStart > endPos) {
            return props;
        }

        size_t propsEnd = block.find("</properties>", propsStart);
        
        size_t propPos = propsStart;
        while ((propPos = block.find("<property", propPos)) != std::string::npos && propPos < propsEnd) {
            size_t tagEnd = block.find(">", propPos);
            std::string propTag = block.substr(propPos, tagEnd - propPos);
            std::string name = get_attr(propTag, "name");
            std::string value = get_attr(propTag, "value");
            if (value.empty() && propTag.find("/>") == std::string::npos) {
                size_t closeTag = block.find("</property>", tagEnd);
                value = block.substr(tagEnd + 1, closeTag - (tagEnd + 1));
            }
            if (!name.empty()) {
                props[name] = value;
            }
            propPos = tagEnd;
        }
        return props;
    }

    inline void populate_tileset_fields(Tileset& ts, const std::string& tagBody) {
        if (!get_attr(tagBody, "name").empty()) ts.name = get_attr(tagBody, "name");
        if (!get_attr(tagBody, "tilewidth").empty()) ts.tileWidth = std::stoi(get_attr(tagBody, "tilewidth"));
        if (!get_attr(tagBody, "tileheight").empty()) ts.tileHeight = std::stoi(get_attr(tagBody, "tileheight"));
        
        std::string spacingStr = get_attr(tagBody, "spacing");
        if (!spacingStr.empty()) ts.spacing = std::stoi(spacingStr);
        
        std::string marginStr = get_attr(tagBody, "margin");
        if (!marginStr.empty()) ts.margin = std::stoi(marginStr);
        
        std::string countStr = get_attr(tagBody, "tilecount");
        if (!countStr.empty()) ts.tileCount = std::stoi(countStr);
        
        std::string colsStr = get_attr(tagBody, "columns");
        if (!colsStr.empty()) ts.columns = std::stoi(colsStr);

        size_t imgPos = tagBody.find("<image");
        if (imgPos != std::string::npos) {
            size_t imgEnd = tagBody.find(">", imgPos);
            std::string imgTag = tagBody.substr(imgPos, imgEnd - imgPos);
            ts.imageSource = get_attr(imgTag, "source");
            std::string iw = get_attr(imgTag, "width");
            std::string ih = get_attr(imgTag, "height");
            ts.imageWidth = iw.empty() ? 0 : std::stoi(iw);
            ts.imageHeight = ih.empty() ? 0 : std::stoi(ih);
        }

        // Parse individual tile-specific properties
        size_t tilePos = 0;
        while ((tilePos = tagBody.find("<tile ", tilePos)) != std::string::npos) {
            size_t tileTagEnd = tagBody.find(">", tilePos);
            size_t tileBlockEnd = tagBody.find("</tile>", tilePos);
            if (tileTagEnd == std::string::npos || tileBlockEnd == std::string::npos) {
                // prase self closing tile
                tileTagEnd = tagBody.find("/>", tilePos);
                if(tileTagEnd == std::string::npos) break;
                tilePos += 6;
                std::string tileTag = tagBody.substr(tilePos, tileTagEnd - tilePos);

                unsigned int localId = std::stoul(get_attr(tileTag, "id"));
                std::string typeStr = get_attr(tileTag, "type");
                PropertyMap& tileProperties = ts.tileProperties[localId];
                tileProperties["type"] = typeStr;

                tilePos = tileTagEnd + 2;
                continue;
            }

            std::string tileTag = tagBody.substr(tilePos, tileTagEnd - tilePos);
            unsigned int localId = std::stoul(get_attr(tileTag, "id"));

            std::string tileBody = tagBody.substr(tilePos, tileBlockEnd - tilePos);
            PropertyMap tileProps = parse_properties(tileBody, 0, tileBody.length());
            
            if (!tileProps.empty()) {
                ts.tileProperties[localId] = tileProps;
            }
            tilePos = tileBlockEnd + 7;
        }
    }

}  // namespace

class Loader {
    public:
        static bool loadMap(const std::string& filePath, Map& outMap, const AssetManager* pAssetManager = nullptr) {
            std::string content = pAssetManager ? internal::load_file_string_from_asset_manager(filePath, *pAssetManager) : internal::load_file_string(filePath);
            if (content.empty()) return false;

            std::string baseDir = "";
            size_t lastSlash = filePath.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                baseDir = filePath.substr(0, lastSlash + 1);
            }

            // 1. Map Global Setup
            size_t mapPos = content.find("<map");
            if (mapPos == std::string::npos) return false;
            size_t mapEnd = content.find(">", mapPos);
            std::string mapTag = content.substr(mapPos, mapEnd - mapPos);

            outMap.width = std::stoi(internal::get_attr(mapTag, "width"));
            outMap.height = std::stoi(internal::get_attr(mapTag, "height"));
            outMap.tileWidth = std::stoi(internal::get_attr(mapTag, "tilewidth"));
            outMap.tileHeight = std::stoi(internal::get_attr(mapTag, "tileheight"));
            outMap.properties = internal::parse_properties(content, mapPos, content.find("</map>"));

            // 2. Parse Tilesets (Inline or External TSX Layout Files)
            size_t tsPos = 0;
            while ((tsPos = content.find("<tileset", tsPos)) != std::string::npos) {
                size_t tsTagEnd = content.find(">", tsPos);
                std::string tsTag = content.substr(tsPos, tsTagEnd - tsPos);
                size_t tsBlockEnd = content.find("</tileset>", tsPos);

                Tileset ts;
                ts.firstGid = std::stoul(internal::get_attr(tsTag, "firstgid"));
                std::string extSource = internal::get_attr(tsTag, "source"); 

                if (!extSource.empty()) {
                    std::string tsxPath = baseDir + extSource;
                    std::string tsxContent = pAssetManager ? internal::load_file_string_from_asset_manager(tsxPath, *pAssetManager) : internal::load_file_string(tsxPath);
                    if (!tsxContent.empty()) {
                        size_t tsxStart = tsxContent.find("<tileset");
                        size_t tsxEnd = tsxContent.find("</tileset>");
                        std::string tsxBody = tsxContent.substr(tsxStart, tsxEnd - tsxStart + 10);
                        
                        internal::populate_tileset_fields(ts, tsxBody);
                        ts.properties = internal::parse_properties(tsxBody, 0, tsxBody.length());
                    }
                } else {
                    if (tsBlockEnd != std::string::npos) {
                        std::string tsBody = content.substr(tsPos, tsBlockEnd - tsPos + 10);
                        internal::populate_tileset_fields(ts, tsBody);
                        ts.properties = internal::parse_properties(tsBody, 0, tsBody.length());
                    }
                }

                outMap.tilesets.push_back(ts);
                tsPos = (tsBlockEnd != std::string::npos) ? tsBlockEnd : tsTagEnd;
            }

            // 3. Parse Standard Tile Grid Layers
            size_t layerPos = 0;
            while ((layerPos = content.find("<layer", layerPos)) != std::string::npos) {
                size_t layerTagEnd = content.find(">", layerPos);
                size_t layerBlockEnd = content.find("</layer>", layerPos);
                std::string layerTag = content.substr(layerPos, layerTagEnd - layerPos);

                Layer layer;
                layer.name = internal::get_attr(layerTag, "name");
                layer.width = std::stoi(internal::get_attr(layerTag, "width"));
                layer.height = std::stoi(internal::get_attr(layerTag, "height"));
                
                if (layerBlockEnd != std::string::npos) {
                    std::string layerBody = content.substr(layerPos, layerBlockEnd - layerPos);
                    layer.properties = internal::parse_properties(layerBody, 0, layerBody.length());
                }

                size_t dataStart = content.find("<data encoding=\"csv\">", layerTagEnd);
                if (dataStart == std::string::npos || dataStart > layerBlockEnd) {
                    dataStart = content.find("<data>", layerTagEnd);
                    if (dataStart == std::string::npos || dataStart > layerBlockEnd) {
                        layerPos = layerTagEnd;
                        continue; 
                    }
                    dataStart += 6;
                } else {
                    dataStart += 21;
                }

                size_t dataEnd = content.find("</data>", dataStart);
                std::string csvData = content.substr(dataStart, dataEnd - dataStart);
                std::stringstream ss(csvData);
                std::string tileId;
                layer.data.reserve(layer.width * layer.height);

                while (std::getline(ss, tileId, ',')) {
                    tileId.erase(std::remove_if(tileId.begin(), tileId.end(), ::isspace), tileId.end());
                    if (!tileId.empty()) {
                        layer.data.push_back(std::stoul(tileId));
                    }
                }

                outMap.layers.push_back(layer);
                layerPos = (layerBlockEnd != std::string::npos) ? layerBlockEnd : layerTagEnd;
            }

            // 4. Parse Object Groups
            size_t objGroupPos = 0;
            while ((objGroupPos = content.find("<objectgroup", objGroupPos)) != std::string::npos) {
                size_t openTagEnd = content.find(">", objGroupPos);
                size_t groupEnd = content.find("</objectgroup>", objGroupPos);
                if (groupEnd == std::string::npos) break;

                std::string groupTag = content.substr(objGroupPos, openTagEnd - objGroupPos);
                ObjectLayer objLayer;
                objLayer.id = std::stoi(internal::get_attr(groupTag, "id"));
                objLayer.name = internal::get_attr(groupTag, "name");
                
                openTagEnd += 1;
                std::string groupBody = content.substr(openTagEnd, groupEnd - openTagEnd);

                objLayer.properties = internal::parse_properties(groupBody, 0, groupBody.length());
                size_t objPos = 0;

                while ((objPos = groupBody.find("<object", objPos)) != std::string::npos) {
                    size_t objTagEnd = groupBody.find(">", objPos);
                    size_t objBlockEnd = groupBody.find("", objPos);
                    bool selfClosing = (groupBody.substr(objPos, objTagEnd - objPos).find("/>") != std::string::npos);
                    size_t stepPos = selfClosing ? objTagEnd : objBlockEnd;
                    if (stepPos == std::string::npos) break;

                    std::string objectTag = groupBody.substr(objPos, objTagEnd - objPos);
                    Object obj;
                    obj.id = std::stoi(internal::get_attr(objectTag, "id"));
                    obj.name = internal::get_attr(objectTag, "name");
                    obj.type = internal::get_attr(objectTag, "class");

                    if (obj.type.empty()) obj.type = internal::get_attr(objectTag, "type");
                    std::string attrX = internal::get_attr(objectTag, "x");
                    std::string attrY = internal::get_attr(objectTag, "y");
                    std::string attrW = internal::get_attr(objectTag, "width");
                    std::string attrH = internal::get_attr(objectTag, "height");
                    std::string attrG = internal::get_attr(objectTag, "gid");
                    obj.x = attrX.empty() ? 0.0f : std::stof(attrX);
                    obj.y = attrY.empty() ? 0.0f : std::stof(attrY);
                    obj.width = attrW.empty() ? 0.0f : std::stof(attrW);
                    obj.height = attrH.empty() ? 0.0f : std::stof(attrH);
                    obj.gid = attrG.empty() ? 0 : std::stoul(attrG);

                    if (!selfClosing) {
                        std::string objBody = groupBody.substr(objPos, objBlockEnd - objPos);
                        obj.properties = internal::parse_properties(objBody, 0, objBody.length());
                    }
                    objLayer.objects.push_back(obj);
                    objPos = stepPos + 1;
                } 
                outMap.objectLayers.push_back(objLayer);
                objGroupPos = groupEnd + 1;
            }

            return true;
        }
};

}  // namespace tmx

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_TMX_TMX_LOADER_H
