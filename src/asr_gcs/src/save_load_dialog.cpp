#include "save_load_dialog.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>

#include "info_panels.h"

namespace {

void PushThemedButtonStyle(bool theme) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::PushStyleColor(ImGuiCol_Button, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 130, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 130, 30, 255));
}

void PopThemedButtonStyle() {
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
}

std::string FormatSize(std::uintmax_t bytes) {
    char buf[32];
    if (bytes < 1024) {
        std::snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
    } else if (bytes < 1024ull * 1024) {
        std::snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    } else {
        std::snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
    }
    return buf;
}

std::string FormatTime(std::filesystem::file_time_type ftime) {
    const auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
    const std::time_t tt = std::chrono::system_clock::to_time_t(system_time);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&tt));
    return buf;
}

} // namespace

void SaveLoadDialog::Open(Mode mode, std::string directory, std::string extension,
                           std::function<void(const std::string &)> on_confirm)
{
    mode_ = mode;
    directory_ = std::move(directory);
    extension_ = std::move(extension);
    on_confirm_ = std::move(on_confirm);
    armed_delete_index_ = -1;
    if (mode_ == Mode::Save) {
        std::snprintf(name_buffer_, sizeof(name_buffer_), "plan");
    }
    Refresh();
    open_ = true;
    ImGui::OpenPopup("SaveLoadDialog");
}

void SaveLoadDialog::Refresh()
{
    entries_.clear();
    std::error_code ec;
    for (const auto &dir_entry : std::filesystem::directory_iterator(directory_, ec)) {
        if (!dir_entry.is_regular_file() || dir_entry.path().extension() != extension_) { continue; }
        Entry entry;
        entry.name = dir_entry.path().stem().string();
        entry.path = dir_entry.path().string();
        entry.size_bytes = dir_entry.file_size(ec);
        entry.modified_time = dir_entry.last_write_time(ec);
        entry.modified_text = FormatTime(entry.modified_time);
        entries_.push_back(std::move(entry));
    }
    std::sort(entries_.begin(), entries_.end(),
              [](const Entry &a, const Entry &b) { return a.modified_time > b.modified_time; });
}

void SaveLoadDialog::Close()
{
    open_ = false;
    armed_delete_index_ = -1;
    ImGui::CloseCurrentPopup();
}

void SaveLoadDialog::Draw(float scale, bool theme)
{
    if (!open_) { return; }

    // Fixed width narrower than the viewport -- the modal's dim background
    // covers the rest, reading as clear gutters on either side.
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float margin_y = 60.0f * scale;
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520.0f * scale, viewport->Size.y - margin_y * 2.0f), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f * scale, 20.0f * scale));

    if (ImGui::BeginPopupModal("SaveLoadDialog", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::PushFont(winInit.getFont(181));  // bold 18
        ImGui::TextUnformatted(mode_ == Mode::Save ? "SAVE PLAN" : "LOAD PLAN");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();

        if (mode_ == Mode::Save) {
            ImGui::TextUnformatted("Name");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##SaveName", name_buffer_, sizeof(name_buffer_));
            ImGui::Spacing();
            PushThemedButtonStyle(theme);
            const bool can_save = name_buffer_[0] != '\0';
            ImGui::BeginDisabled(!can_save);
            if (ImGui::Button("Save", ImVec2(-1, 0))) {
                on_confirm_(directory_ + "/" + name_buffer_ + extension_);
                Close();
            }
            ImGui::EndDisabled();
            PopThemedButtonStyle();
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color::dwhite_lblack(theme));
            ImGui::TextDisabled(entries_.empty() ? "No existing saves." : "Or overwrite an existing save:");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        // Entries scroll in their own region, leaving room for Cancel below.
        const float footer_h = ImGui::GetFrameHeightWithSpacing() + 8.0f * scale;
        ImGui::BeginChild("SaveLoadEntries", ImVec2(0, ImGui::GetContentRegionAvail().y - footer_h), false);
        for (size_t idx = 0; idx < entries_.size(); ++idx) {
            const Entry &entry = entries_[idx];
            ImGui::PushID(static_cast<int>(idx));
            const float delete_w = 90.0f * scale;

            ImGui::BeginGroup();
            ImGui::PushFont(winInit.getFont(181));
            ImGui::TextUnformatted(entry.name.c_str());
            ImGui::PopFont();
            ImGui::PushStyleColor(ImGuiCol_Text, Color::dwhite_lblack(theme));
            ImGui::Text("%s    %s", entry.modified_text.c_str(), FormatSize(entry.size_bytes).c_str());
            ImGui::PopStyleColor();
            ImGui::EndGroup();
            const bool row_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            if (ImGui::IsItemHovered()) {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 255, 255, 15), 6.0f * scale);
            }

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - delete_w);
            const bool armed = (armed_delete_index_ == static_cast<int>(idx));
            bool deleted = false;
            if (armed) {
                // Always white -- this button's background is a fixed dark red regardless of theme.
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
                if (ImGui::Button("Confirm?", ImVec2(delete_w, 0))) {
                    std::error_code rm_ec;
                    std::filesystem::remove(entry.path, rm_ec);
                    deleted = true;
                }
                ImGui::PopStyleColor(4);
            } else {
                PushThemedButtonStyle(theme);
                if (ImGui::Button("Delete", ImVec2(delete_w, 0))) {
                    armed_delete_index_ = static_cast<int>(idx);
                }
                PopThemedButtonStyle();
            }
            ImGui::PopID();

            if (deleted) {
                armed_delete_index_ = -1;
                Refresh();
                break;  // entries_ was just invalidated
            }
            if (row_clicked) {
                if (mode_ == Mode::Save) {
                    std::snprintf(name_buffer_, sizeof(name_buffer_), "%s", entry.name.c_str());
                } else {
                    on_confirm_(entry.path);
                    Close();
                    break;
                }
            }
            ImGui::Spacing();
        }
        ImGui::EndChild();

        ImGui::Separator();
        PushThemedButtonStyle(theme);
        if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
            Close();
        }
        PopThemedButtonStyle();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(7);
}
