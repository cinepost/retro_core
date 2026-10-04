#include "game_world.h"
#include "game_objects.h"

namespace KnightGame {

GameWorld::GameWorld(const V99x8& ppu, const AssetManager& assetManager, const SoundEngine& soundEngine): mPPU(ppu), 
   mpPlayers({nullptr, nullptr}), mIsInitialized(false), mMapWidth(64), mMapHeight(256), mAssetManager(assetManager), mSoundPlayer(soundEngine) 
{
   mTiles.resize(mMapWidth * mMapHeight); // default size
   mExtraTiles.resize(mTiles.size() / 4); // default size

   mFreezeTimer.start(0.0f);
}

bool GameWorld::init(const std::string& mapFilePath) {
   mIsInitialized = false;

   tmx::Map worldMap;
   if(!tmx::Loader::loadMap(mapFilePath, worldMap, &mAssetManager)) {
      std::cerr << "Error loading map " << mapFilePath << std::endl;
      return false;
   }

   assert(worldMap.width == 64);
   assert(worldMap.height > 34 && worldMap.height % 2 == 0);
   assert(worldMap.tileWidth == kTileSize && worldMap.tileHeight == kTileSize);

   mMapWidth = worldMap.width;
   mMapHeight = worldMap.height;

   static const int sMaxMapPixelCoordX = static_cast<int>(mMapWidth) * 8 - 1;
   static const int sMaxMapPixelCoordY = static_cast<int>(mMapHeight) * 8 - 1;


   assert(mMapWidth % 2 == 0); // to fit extras we need map width to be multple of 2

   // Set up tile layers, power-up objects, weapon upgrade objects

   mTiles.resize(mMapWidth * mMapHeight);
   mExtraTiles.resize((mMapWidth >> 1) * (mMapHeight >> 1));

   // compose background tiles from layers
   for(const tmx::Layer& layer: worldMap.layers) {
      assert(layer.width == mMapWidth);
      assert(layer.height == mMapHeight);
      assert(layer.width * layer.height == layer.data.size());
      for(int x = 0; x < layer.width; ++x) {
         for(int y = 0; y < layer.height; ++y) {
            const uint32_t tile_index = layer.data[x + y * layer.width];
            if(tile_index == 0) continue;

            Tile::Flags flags = Tile::Flags::None;
            for (const auto& [name, value] : layer.properties) {
               if(name == "obstacle" && value == "true") flags = flags | Tile::Flags::Obstacle;
               else if(name == "water" && value == "true") flags = flags | Tile::Flags::Water;
            }

            mTiles[x + y * layer.width] = {tile_index - 1 /* Tiled editor indices are 1-based */, flags};
         }
      }
   }

   auto addExtras = [&](const tmx::ObjectLayer& objLayer) {
      for(const tmx::Object& obj: objLayer.objects) {
         assert(obj.width == kExtraTileSize && obj.height == kExtraTileSize);
         int x = std::max(0, obj.getPosX<int>());
         int y = std::max(0, obj.getPosY<int>());
         int width = std::max(0, obj.getWidth<int>());
         int height = std::max(0, obj.getHeight<int>());

         if (x % 16 != 0 || y % 16 != 0) {
            std::cerr << "World map extra object " << obj.id << " position should be multiple of 16px !" << std::endl;
            continue;
         }

         if (width != kExtraTileSize || height != kExtraTileSize) {
            std::cerr << "World map extra object " << obj.id << " width and height should be 16px !" << std::endl;
            continue;
         }

         const std::string* pTypeStr = nullptr;

         if(obj.type.empty()) {
            const tmx::Tileset* pTileset = worldMap.getTilesetForGid(obj.gid);
            if(pTileset) {
               const tmx::PropertyMap* pProps = pTileset->getPropertiesForGid(obj.gid);
               if(pProps) {
                  pTypeStr = &pProps->getAsString("type");
               }
            }
         } else {
            pTypeStr = &obj.type;
         }

         if(!pTypeStr) {
            std::cerr << "Error parsing objects layer " << objLayer.name << std::endl;
            continue;
         }

         auto& extra = mExtraTiles[(x >> 4) + (y >> 4) * (mMapWidth >> 1)];
         extra.state = Extra::State::Unknown;
         extra.type = Extra::typeFromString(*pTypeStr);   
      }
      return true;
   };

   auto addPowerUps = [&](const tmx::ObjectLayer& objLayer) {
      for(const tmx::Object& obj: objLayer.objects) {
         int x = std::min(sMaxMapPixelCoordX, std::max(0, obj.getPosX<int>()));
         int y = std::min(sMaxMapPixelCoordY, std::max(0, obj.getPosY<int>()));
         
         mGameObjects.push_back(std::make_unique<PowerUp>(x, y));   
      }
      return true;
   };

   auto addWeaponUps = [&](const tmx::ObjectLayer& objLayer) {
      for(const tmx::Object& obj: objLayer.objects) {
         int x = std::min(sMaxMapPixelCoordX, std::max(0, obj.getPosX<int>()));
         int y = std::min(sMaxMapPixelCoordY, std::max(0, obj.getPosY<int>()));
         
         mGameObjects.push_back(std::make_unique<WeaponUp>(x, y));   
      }
      return true;
   };

   auto addEnemies = [&](const tmx::ObjectLayer& objLayer) {
      for(const tmx::Object& obj: objLayer.objects) {
         int x = std::min(sMaxMapPixelCoordX, std::max(0, obj.getPosX<int>()));
         int y = std::min(sMaxMapPixelCoordY, std::max(0, obj.getPosY<int>()));
         mGameObjects.push_back(std::make_unique<Slime>(x, y));   
      }
      return true;
   };

   for(const tmx::ObjectLayer& objLayer: worldMap.objectLayers) {
      if(objLayer.name == "Extras") {
         addExtras(objLayer);
      } else if(objLayer.name == "PowerUps") {
         addPowerUps(objLayer);
      } else if(objLayer.name == "WeaponUps") {
         addWeaponUps(objLayer);
      } else if(objLayer.name == "Enemies") {
         addEnemies(objLayer);
      }
   }


   // Set up camera
   mCamera.setHeight(34); // height in 8px tiles. default value
   mCamera.setPosY(mMapHeight * kTileSize - mCamera.getHeight() * kTileSize); // set to map bottom
   //mCamera.stop();

   // Spawn player(s)
   static const float sTwoPlayersSpawnShift = 48.0f;

   auto player0 = std::make_unique<Player>(248.0f - sTwoPlayersSpawnShift, 256.0f * 8.0f - 32.0f, 7);
   mpPlayers[0] = player0.get(); 
   mGameObjects.push_back(std::move(player0));

   auto player1 = std::make_unique<Player>(248.0f + sTwoPlayersSpawnShift, 256.0f * 8.0f - 32.0f, 13);
   mpPlayers[1] = player1.get(); 
   mGameObjects.push_back(std::move(player1));

   mIsInitialized = true;
   return true;
}

void GameWorld::update(float dt) {
   assert(mIsInitialized);

   mFreezeTimer.update(dt);

   const int prevCamY = mCamera.getPosY();
   mCamera.update(dt);

   // move player as it follows camera movement
   movePlayer(0, 0, mCamera.getPosY() - prevCamY);
   movePlayer(1, 0, mCamera.getPosY() - prevCamY);

   int cameraMinY = mCamera.getPosY();
   int cameraMaxY = cameraMinY + mCamera.getHeight() * kTileSize;

   mActiveGameObjects.clear();
   using EntityType = GameObject::EntityType;

   // update active objects
   for(auto& pObject: mGameObjects) {
      
      // Skip inactive objects and frozen characters
      if(!pObject->isActive()){
         continue;
      }

      // Tricky part. Need some more research. Skip objects that are out of camera top edge except non projectile weapons as they may need to fly back
      const bool isNonProjectileWeapon = pObject->getType() == EntityType::Weapon;

      if (!isNonProjectileWeapon && !isObjectLowerEdgeWithinCameraView(pObject.get(), 0)) {
         continue;
      }

      // update all objects except frozen enemies etc...
      const int update_frames = (isWorldFrozen() && pObject->isFreezable()) ? 0 : 1;

      pObject->update(update_frames, *this);
      
      // Check once agais as update(...) may inactivate object iself.
      if(pObject->isActive()) mActiveGameObjects.push_back(pObject.get());
   }

   // process collisions and deactivate objects by camera bounds if needed
   for(auto& pObject: mActiveGameObjects) {
      const auto& aabb = pObject->getCollisionAABB();
      int objMinX = pObject->getPosX() + aabb.x;
      int objMinY = pObject->getPosY() + aabb.y;
      int objMaxX = objMinX + aabb.w;
      int objMaxY = objMinY + aabb.h;

      // skip objects that are too far from camera bounds.
      // we use horizontal camera bounds here. for vertical we add object height as a padding
      if(objMaxX < 0 || objMinX >= 512 || objMaxY < (cameraMinY - aabb.h) || objMinY >= (cameraMaxY + aabb.h)) {
         continue;
      }

      switch(pObject->getType()) {
         case EntityType::Weapon:
            {
               Weapon& weapon = *reinterpret_cast<Weapon*>(pObject);
               
               // deactivate projectiles that are out of screen
               if(weapon.isProjectile() && (objMaxX < 0 || objMinX >= 512 || objMaxY < cameraMinY || objMinY >= cameraMaxY)) {
                  pObject->setActive(false);
                  break;
               }

               if(weapon.getOwner() == Weapon::OwnerType::Player) {
                  processPlayerWeaponCollisions(weapon);
               } else {
                  processEnemyWeaponCollisions(weapon);
               }
            }
            break;
         case EntityType::Enemy:
            if(objMinY >= cameraMaxY) {
               pObject->setActive(false); // enemies spawns and moves top to bottom. remove if goes below camera view
            } else {
               processPlayerEnemyCollision(mpPlayers[(mFrameNumber + 0) % 2], *reinterpret_cast<Enemy*>(pObject));
               processPlayerEnemyCollision(mpPlayers[(mFrameNumber + 1) % 2], *reinterpret_cast<Enemy*>(pObject));
            }
            break;
         case EntityType::PowerUp:
         case EntityType::WeaponUp:
            if(objMinY >= cameraMaxY) {
               pObject->setActive(false);
               break;
            }
            // shuffle players from frame to frame
            playerRedeemUpgrade(mpPlayers[(mFrameNumber + 0) % 2], *reinterpret_cast<Upgrade*>(pObject));
            playerRedeemUpgrade(mpPlayers[(mFrameNumber + 1) % 2], *reinterpret_cast<Upgrade*>(pObject));
            break;
         default:
            break;
      }
   }

   playerRedeemExtra(mpPlayers[(mFrameNumber + 0) % 2]);
   playerRedeemExtra(mpPlayers[(mFrameNumber + 0) % 2]);

   // GC erasure pass
   size_t prev_objects_count = mGameObjects.size();
   mGameObjects.erase(
      std::remove_if(mGameObjects.begin(), mGameObjects.end(),
         [this](const std::unique_ptr<GameObject>& obj) {
            if (!obj->isActive() && obj.get() == this->mpPlayers[0]) this->mpPlayers[0] = nullptr;
            if (!obj->isActive() && obj.get() == this->mpPlayers[1]) this->mpPlayers[1] = nullptr; 
            
            return !obj->isActive();
         }),
      mGameObjects.end()
   );
   size_t curr_objects_count = mGameObjects.size();
   if(curr_objects_count != prev_objects_count) {
      std::cout << (prev_objects_count - curr_objects_count) << " inactive objects removed.\n";
   }


   // push pending objects
   while (!mPendingObjects.empty()) {
      mGameObjects.push_back(std::move(mPendingObjects.back()));
      mPendingObjects.pop_back(); // Shrinks the size of the vector one by one
   }

   assert(mPendingObjects.empty());

   // final pass before rendering

   mActiveGameObjects.clear();

   // update active objects
   for(auto& pObject: mGameObjects) {
      if(!pObject->isActive()) continue; // we can probably skip this check as GC pass removed inacive objects already
      mActiveGameObjects.push_back(pObject.get());
   }

   // sort objects based on their collsion aabb lower edge
   std::sort(mActiveGameObjects.begin(), mActiveGameObjects.end(), [](const GameObject* pObjA, const GameObject* pObjB) {
     return (pObjA->getPosY() + pObjA->getCollisionAABB().y + pObjA->getCollisionAABB().h) < (pObjB->getPosY() + pObjB->getCollisionAABB().y + pObjB->getCollisionAABB().h); 
   });

   mFrameNumber++;
}

void GameWorld::getContext(WorldContext& ctx) const {

}

}  // namespace KnightGame