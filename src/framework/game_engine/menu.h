#ifndef __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MENU_H
#define __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MENU_H

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>


namespace RetroCore {

namespace GameEngine {

class MenuEntry {
    public:
        using UniquePtr = std::unique_ptr<MenuEntry>;

        static constexpr uint32_t kInvalidEntryID = std::numeric_limits<uint32_t>::max();

        MenuEntry() = default;
        MenuEntry(std::string title, std::function<void()> pAction = nullptr): mTitle(std::move(title)), mAction(std::move(pAction)) { }
        virtual ~MenuEntry() = default;

        void execute() {
            if (mAction) {
                mAction();
            }
        }

        void setAction(std::function<void()> pAction) { mAction = pAction; }
        
        virtual bool isMenu() const { return false; }

        const std::string& getTitle() const { return mTitle; }
        void changeTitle(const std::string& title) { mTitle = title; }

    protected:
        std::string mTitle;
        std::function<void()> mAction = nullptr; 
};

class Menu: public MenuEntry {
    public:
        using Entry = MenuEntry;

        Menu(): MenuEntry() { }
        Menu(std::string title, std::function<void()> pAction = nullptr) : MenuEntry(std::move(title), std::move(pAction)) { }

        uint32_t getEntriesCount() const { return static_cast<uint32_t>(mEntries.size()); }
        std::vector<MenuEntry::UniquePtr>& getEntries() { return mEntries; }

        uint32_t addEntry(Menu&& entry) {
            uint32_t entryID = mEntries.size();
            mEntries.push_back(std::make_unique<Menu>(std::move(entry)));
            return entryID;
        }

        uint32_t addEntry(std::unique_ptr<MenuEntry> pEntry) {
            uint32_t entryID = mEntries.size();
            mEntries.push_back(std::move(pEntry));
            return entryID;
        }
        
        [[nodiscard]] MenuEntry& getEntry(uint32_t entryID) { 
            assert(entryID < mEntries.size());
            return *mEntries[entryID]; 
        }
        
        void changeEntryTitle(uint32_t entryID, const std::string& title) {
            assert(entryID < mEntries.size());
            mEntries[entryID]->changeTitle(title);
        }

        bool isMenu() const override { return true; }

        bool isEntrySelected(uint32_t entryID) const { return mCurrentEntryID == entryID; }
        
        [[nodiscard]] MenuEntry& getCurrentEntry() const {
            assert(mCurrentEntryID < static_cast<uint32_t>(mEntries.size()));
            return *mEntries[mCurrentEntryID];
        }

        void executeCurrentEntry() {
            assert(mCurrentEntryID < mEntries.size());
            mEntries[mCurrentEntryID]->execute();
        }

        uint32_t getCurrentEntryID() const {
            return mCurrentEntryID;
        }

        // Navigate menu "up". When wrap == true cursor jumps to the end of mEntries list if was on frist entry.
        // Return true if cursor was moved.
        bool moveCursorUp(bool wrap = false) const {
            uint32_t prev_entry_id = mCurrentEntryID;
            if(mCurrentEntryID == 0) {
                if(wrap) mCurrentEntryID = getEntriesCount() - 1; 
            } else {
                mCurrentEntryID--;
            }
            return prev_entry_id != mCurrentEntryID;
        }

        // Navigate menu "down". When wrap == true cursor jumps to the mEntries list start if was on last entry.
        // Return true if cursor was moved.
        bool moveCursorDown(bool wrap = false) const{
            uint32_t prev_entry_id = mCurrentEntryID;
            if(mCurrentEntryID == getEntriesCount() - 1) {
                if(wrap) mCurrentEntryID = 0; 
            } else {
                mCurrentEntryID++;
            }
            return prev_entry_id != mCurrentEntryID;
        }

    private:
        std::vector<MenuEntry::UniquePtr> mEntries;
        mutable uint32_t mCurrentEntryID = 0;
};


}  // namespace GameEngine

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_GAME_ENGINE_MENU_H