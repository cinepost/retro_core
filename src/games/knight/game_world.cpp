#include "game_world.h"
#include "game_objects.h"

namespace KnightGame {

GameWorld::GameWorld(const SoundEngine& soundEngine): mSoundPlayer(soundEngine), mCamera(kMapHeight) {
   // Position camera to bottom
   mCamera.setHeight(35); // default value
   mCamera.setPosY(kMapHeight * kTileSize - mCamera.getHeight() * kTileSize); // set to map bottom

   // Spawn player(s)
   auto player = std::make_unique<Player>(248.0f, 256.0f * 8.0f - 32.0f);

   mpPlayer = player.get(); 
   mGameObjects.push_back(std::move(player));

   //mGameObjects.push_back(std::make_unique<Arrow>(200.0f,  256.0f * 8.0f - 32.0f, Weapon::OwnerType::Player));
}

void GameWorld::update(float dt) {
   const uint16_t prevCamY = mCamera.getPosY();
   mCamera.update(dt);

   // move player as it follows camera movement
   movePlayer(0.0f, static_cast<float>(mCamera.getPosY() - prevCamY));

   for(auto& pObject: mGameObjects) {
      if(!pObject->isActive()) continue;
      pObject->update(dt, *this);

      if (pObject->getType() == GameObject::EntityType::Projectile) {

      }
   }

   // GC erasure pass
   mGameObjects.erase(
      std::remove_if(mGameObjects.begin(), mGameObjects.end(),
         [this](const std::unique_ptr<GameObject>& obj) {
            if (!obj->isActive() && obj.get() == this->mpPlayer) {
               mpPlayer = nullptr; 
            }
            return !obj->isActive();
         }),
      mGameObjects.end()
    );

   // push pending objects
   while (!mPendingObjects.empty()) {
      mGameObjects.push_back(std::move(mPendingObjects.back()));
      mPendingObjects.pop_back(); // Shrinks the size of the vector one by one
   }

   assert(mPendingObjects.empty());
}

}  // namespace KnightGame