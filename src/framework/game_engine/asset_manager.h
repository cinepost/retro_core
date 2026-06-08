#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_ASSET_MANAGER_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_ASSET_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>

namespace RetroCore {

namespace GameEngine {

class AssetManager {
    public:
        struct AssetRecord {
            const uint8_t* dataPointer = nullptr;
            size_t sizeInBytes = 0;
            bool isDiskLoaded = false; // Tracks if memory is owned by a unique_ptr
        };

        AssetManager() = default;

        // Parses a unified binary bundle loaded by retro_load_game
        bool loadBundleFromMemory(const uint8_t* bundleBuffer, size_t totalSize) {
            if (!bundleBuffer || totalSize < sizeof(uint32_t)) return false;

            // Example structural format layout:
            // [4 Bytes: Number of files]
            // For each file:
            //   [String: File Name (null-terminated)]
            //   [4 Bytes: File Size]
            //   [X Bytes: Raw binary payload payload]

            size_t offset = 0;
            uint32_t fileCount = 0;
            std::memcpy(&fileCount, bundleBuffer + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            for (uint32_t i = 0; i < fileCount; ++i) {
                // Read null-terminated file key name
                std::string fileName(reinterpret_cast<const char*>(bundleBuffer + offset));
                offset += fileName.length() + 1;

                // Read payload size data
                uint32_t fileSize = 0;
                std::memcpy(&fileSize, bundleBuffer + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);

                // Bind memory indices into virtual table
                AssetRecord record;
                record.dataPointer = bundleBuffer + offset;
                record.sizeInBytes = fileSize;
                mRegistry[fileName] = record;

                offset += fileSize;
                if (offset > totalSize) return false; // Safety bounds overflow fallback
            }
            return true;
        }

        // Fetches pointer to memory sector containing asset data
        const uint8_t* getFile(const std::string& name) {
            auto it = mRegistry.find(name);
            if (it != mRegistry.end()) {
                return it->second.dataPointer;
            }

             // Fallback: Attempt to load from the system disk if not found in memory bundle
            if (loadFileFromDisk(name)) {
                return mRegistry[name].dataPointer;
            }

            return nullptr;
        }

        // Fetches the exact size metric of the target file
        size_t getFileSize(const std::string& name) {
            if (getFile(name) != nullptr) {
                return mRegistry[name].sizeInBytes;
            }
            return 0;
        }

    private:
        bool loadFileFromDisk(const std::string& filepath) {
            // Open file in binary mode at the end of the file to quickly capture size
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                return false;
            }

            std::streamsize size = file.tellg();
            if (size <= 0) {
                return false;
            }

            file.seekg(0, std::ios::beg);

            // Allocate block and wrap inside a unique_ptr managed internally by our cache vector
            auto fileBuffer = std::make_unique<uint8_t[]>(static_cast<size_t>(size));
            if (!file.read(reinterpret_cast<char*>(fileBuffer.get()), size)) {
                return false;
            }

            // Register the new dynamic asset records
            AssetRecord record;
            record.dataPointer = fileBuffer.get();
            record.sizeInBytes = static_cast<size_t>(size);
            record.isDiskLoaded = true;
            
            mRegistry[filepath] = record;
            
            // Push raw ownership down to the cache vector so it survives the scope of this function execution
            mDiskCache.push_back(std::move(fileBuffer));
            
            return true;
        }

    private:
        std::unordered_map<std::string, AssetRecord> mRegistry;

        // Holds heap allocation lifetimes for all filesystem fallback assets loaded at runtime
        std::vector<std::unique_ptr<uint8_t[]>> mDiskCache; 
};


}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_ASSET_MANAGER_H