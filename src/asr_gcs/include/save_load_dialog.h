#pragma once

// Full-screen modal file browser for save/load workflows: lists files in a
// directory with modified date + size, supports delete, and (in Save mode)
// a name field for a new or overwritten entry. Deals only in filesystem
// paths -- has no knowledge of what the files contain, so it's reusable
// wherever a "pick or name a file in one directory" flow is needed.

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class SaveLoadDialog {
public:
    enum class Mode { Save, Load };

    // `on_confirm` fires once, the frame the user confirms a Save (typed
    // name + Save button) or picks an entry to Load.
    void Open(Mode mode, std::string directory, std::string extension,
              std::function<void(const std::string &path)> on_confirm);

    // No-op if not currently open. Call every frame.
    void Draw(float scale, bool theme);

private:
    struct Entry {
        std::string name;
        std::string path;
        std::uintmax_t size_bytes = 0;
        std::string modified_text;
        std::filesystem::file_time_type modified_time;
    };

    bool open_ = false;
    Mode mode_ = Mode::Load;
    std::string directory_;
    std::string extension_;
    std::function<void(const std::string &)> on_confirm_;
    std::vector<Entry> entries_;
    char name_buffer_[128] = "";

    // Delete is two-stage: first click arms it (button turns into a
    // "Confirm?" prompt), second click on the same entry deletes it.
    int armed_delete_index_ = -1;

    void Refresh();
    void Close();
};
