#include "analyze/analyze_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <implot.h>

#include "info_panels.h"
#include "map.h"

namespace analyze {

namespace {

const ImVec4 kPaletteDark[] = {
    ImVec4(0.98f, 0.51f, 0.12f, 1.0f),  // orange -- matches the GCS accent
    ImVec4(0.31f, 0.71f, 0.95f, 1.0f),  // blue
    ImVec4(0.42f, 0.82f, 0.42f, 1.0f),  // green
    ImVec4(0.93f, 0.40f, 0.55f, 1.0f),  // pink
    ImVec4(0.72f, 0.58f, 0.94f, 1.0f),  // violet
    ImVec4(0.95f, 0.83f, 0.30f, 1.0f),  // yellow
    ImVec4(0.36f, 0.85f, 0.80f, 1.0f),  // teal
};
const ImVec4 kPaletteLight[] = {
    ImVec4(0.80f, 0.36f, 0.02f, 1.0f),
    ImVec4(0.08f, 0.40f, 0.72f, 1.0f),
    ImVec4(0.10f, 0.48f, 0.16f, 1.0f),
    ImVec4(0.75f, 0.12f, 0.34f, 1.0f),
    ImVec4(0.42f, 0.22f, 0.68f, 1.0f),
    ImVec4(0.58f, 0.46f, 0.02f, 1.0f),
    ImVec4(0.02f, 0.45f, 0.45f, 1.0f),
};
constexpr int kPaletteSize = static_cast<int>(sizeof(kPaletteDark) / sizeof(kPaletteDark[0]));

ImVec4 PaletteColor(int index, bool theme)
{
    const int i = ((index % kPaletteSize) + kPaletteSize) % kPaletteSize;
    return theme ? kPaletteDark[i] : kPaletteLight[i];
}

ImVec4 AccentColor(bool theme)
{
    return theme ? ImVec4(1.0f, 0.51f, 0.12f, 1.0f) : ImVec4(0.85f, 0.40f, 0.04f, 1.0f);
}
ImVec4 HintColor(bool theme)
{
    return theme ? ImVec4(0.62f, 0.62f, 0.70f, 1.0f) : ImVec4(0.36f, 0.36f, 0.42f, 1.0f);
}
ImVec4 WarnColor(bool theme)
{
    return theme ? ImVec4(1.0f, 0.66f, 0.25f, 1.0f) : ImVec4(0.70f, 0.40f, 0.0f, 1.0f);
}
ImVec4 GoodColor(bool theme)
{
    return theme ? ImVec4(0.42f, 0.82f, 0.42f, 1.0f) : ImVec4(0.10f, 0.48f, 0.16f, 1.0f);
}
ImVec4 BadgeColor(Source s, bool theme)
{
    switch (s) {
        case Source::Uav: return PaletteColor(0, theme);
        case Source::Fc:  return PaletteColor(1, theme);
        case Source::Gcs: return PaletteColor(2, theme);
        default:          return HintColor(theme);
    }
}

void PushPlotTheme(bool theme)
{
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_PlotBorder, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisText, Color::white_black(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisGrid, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisTick, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_LegendText, Color::white_black(theme));
}

void PopPlotTheme() { ImPlot::PopStyleColor(8); }

void PushPanelStyle(bool theme, const UiScale& scale)
{
    const ImVec4 surface = ImGui::ColorConvertU32ToFloat4(Color::dBlue_lGrey(theme));
    const ImVec4 border = ImGui::ColorConvertU32ToFloat4(Color::panelBorder(theme));
    const ImVec4 text = ImGui::ColorConvertU32ToFloat4(Color::white_black(theme));
    const ImVec4 accent = AccentColor(theme);

    ImVec4 hover = surface;
    hover.x += (accent.x - surface.x) * 0.28f;
    hover.y += (accent.y - surface.y) * 0.28f;
    hover.z += (accent.z - surface.z) * 0.28f;

    ImVec4 sel = accent;
    sel.w = 0.32f;
    ImVec4 sel_hover = accent;
    sel_hover.w = 0.48f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7 * scale.x, 3 * scale.y));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6 * scale.x, 5 * scale.y));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f * scale.uniform());

    ImGui::PushStyleColor(ImGuiCol_Text, text);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, HintColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Button, surface);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, surface);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, hover);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, accent);
    ImGui::PushStyleColor(ImGuiCol_Header, sel);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sel_hover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, sel_hover);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, border);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, accent);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Separator, border);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, sel_hover);
}

void PopPanelStyle()
{
    ImGui::PopStyleColor(20);
    ImGui::PopStyleVar(7);
}

bool TrackPosAt(const Track &tr, double t, double &lat, double &lon)
{
    if (tr.t.size() < 2 || t < tr.t.front() || t > tr.t.back()) return false;
    const auto it = std::lower_bound(tr.t.begin(), tr.t.end(), t);
    const size_t i = static_cast<size_t>(it - tr.t.begin());
    if (i == 0) {
        lat = tr.lat.front();
        lon = tr.lon.front();
        return true;
    }
    const double t0 = tr.t[i - 1], t1 = tr.t[i];
    const double f = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0;
    lat = tr.lat[i - 1] + f * (tr.lat[i] - tr.lat[i - 1]);
    lon = tr.lon[i - 1] + f * (tr.lon[i] - tr.lon[i - 1]);
    return true;
}

// Linear interpolation of a series at time t; false when t is outside its span.
bool SampleAt(const Series &s, double t, double &out)
{
    if (s.t.size() < 2 || t < s.t.front() || t > s.t.back()) return false;
    const auto it = std::lower_bound(s.t.begin(), s.t.end(), t);
    const size_t i = static_cast<size_t>(it - s.t.begin());
    if (i == 0) { out = s.v.front(); return true; }
    const double t0 = s.t[i - 1], t1 = s.t[i];
    const double f = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0;

    if (s.angle && std::abs(s.v[i] - s.v[i - 1]) > M_PI) {
        out = (f < 0.5) ? s.v[i - 1] : s.v[i];
        return true;
    }
    out = s.v[i - 1] + f * (s.v[i] - s.v[i - 1]);
    return true;
}

}  // namespace

AnalyzePanel::AnalyzePanel() : browser_path_("~/asr_workspace/flight_logs") {}

int AnalyzePanel::NextColor() { return color_index_++; }

bool AnalyzePanel::TimeRange(double &lo, double &hi) const
{
    bool any = false;
    for (const auto &f : files_) {
        if (!f.log) continue;
        if (!any) {
            lo = f.log->t_begin();
            hi = f.log->t_end();
            any = true;
        } else {
            lo = std::min(lo, f.log->t_begin());
            hi = std::max(hi, f.log->t_end());
        }
    }
    return any;
}

bool AnalyzePanel::OpenLog(const std::string &path)
{
    for (const auto &f : files_) {
        if (f.log && f.log->path() == path) return false;
    }

    auto log = std::make_unique<LogFile>();
    if (!log->Load(path)) {
        browser_status_ = "failed: " + (log->warnings().empty() ? std::string("unknown error")
                                                               : log->warnings().front());
        return false;
    }

    Loaded entry;
    entry.color_index = NextColor();
    entry.log = std::move(log);
    files_.push_back(std::move(entry));
    selected_file_ = static_cast<int>(files_.size()) - 1;
    FitTimeRange();
    return true;
}

void AnalyzePanel::FitTimeRange()
{
    double lo, hi;
    if (!TimeRange(lo, hi)) {
        range_valid_ = false;
        cursor_valid_ = false;
        return;
    }
    if (hi <= lo) hi = lo + 1.0;             
    const double pad = (hi - lo) * 0.02;
    x_min_ = lo - pad;
    x_max_ = hi + pad;
    range_valid_ = true;

    if (!cursor_valid_) {
        cursor_t_ = lo + (hi - lo) * 0.5;
        cursor_valid_ = true;
    } else {
        cursor_t_ = std::clamp(cursor_t_, lo, hi);
    }
}

void AnalyzePanel::OpenEntryWithCompanions(const LogEntry &entry)
{
    const bool opened = OpenLog(entry.path);
    // The clicked log stays selected even if companions load after it.
    const int primary = files_.empty() ? -1 : static_cast<int>(files_.size()) - 1;

    size_t companions = 0;
    for (const LogEntry *c : FindCompanions(browser_groups_, entry)) {
        if (OpenLog(c->path)) companions++;
    }
    if (primary >= 0) selected_file_ = primary;

    if (!opened && companions == 0) {
        browser_status_ = entry.name + " already open";
    } else if (companions > 0) {
        browser_status_ = "opened " + entry.name + " + " + std::to_string(companions) +
                          " matching log(s)";
    } else {
        browser_status_ = "opened " + entry.name + " (no matching logs from other sources)";
    }
}

void AnalyzePanel::Rescan()
{
    browser_groups_ = ScanGrouped(browser_path_);
    size_t n = 0;
    for (const auto &g : browser_groups_) n += g.entries.size();
    browser_status_ = std::to_string(n) + " log(s) across " +
                      std::to_string(browser_groups_.size()) + " day(s)";
    scanned_once_ = true;
}

bool AnalyzePanel::IsPlotted(size_t file_index, const std::string &topic,
                              const std::string &field, int pane) const
{
    return std::any_of(plotted_.begin(), plotted_.end(), [&](const PlotSeries &p) {
        return p.file_index == file_index && p.topic == topic && p.field == field &&
               p.pane == pane;
    });
}

void AnalyzePanel::ToggleSeries(size_t file_index, const std::string &topic,
                                 const std::string &field, int pane)
{
    const auto it = std::find_if(plotted_.begin(), plotted_.end(), [&](const PlotSeries &p) {
        return p.file_index == file_index && p.topic == topic && p.field == field &&
               p.pane == pane;
    });
    if (it != plotted_.end()) {
        plotted_.erase(it);
        return;
    }
    plotted_.push_back({file_index, topic, field, NextColor(), pane});
}

void AnalyzePanel::AddPane()
{
    pane_count_++;
    active_pane_ = pane_count_ - 1;
}

void AnalyzePanel::RemovePane(int pane)
{
    if (pane_count_ <= 1) return;   // always keep one to draw into
    plotted_.erase(std::remove_if(plotted_.begin(), plotted_.end(),
                                   [&](const PlotSeries &p) { return p.pane == pane; }),
                    plotted_.end());
    for (auto &p : plotted_) {
        if (p.pane > pane) p.pane--;
    }
    pane_count_--;
    active_pane_ = std::clamp(active_pane_, 0, pane_count_ - 1);
}

void AnalyzePanel::RemoveSeries(size_t plot_index)
{
    if (plot_index < plotted_.size()) {
        plotted_.erase(plotted_.begin() + static_cast<long>(plot_index));
    }
}

// ---------------------------------------------------------------------------

void AnalyzePanel::DrawBrowser(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size)
{
    BeginFixedPanel("AnalyzeBrowser", pos, size, scale, theme, 0, ImVec2(10, 8), true);
    PushPanelStyle(theme, scale);
    ImGui::PushFont(winInit.getFont(18));
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::white_black(theme)), "Open flight log");
    ImGui::PopFont();
    ImGui::PushFont(winInit.getFont(14));

    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s", browser_path_.c_str());
    ImGui::SetNextItemWidth(size.x - 90 * scale.x);
    if (ImGui::InputText("##analyze_path", buf, sizeof(buf),
                          ImGuiInputTextFlags_EnterReturnsTrue)) {
        browser_path_ = buf;
        Rescan();
    } else {
        browser_path_ = buf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Scan##analyze")) Rescan();

    if (!browser_status_.empty()) {
        ImGui::TextColored(HintColor(theme), "%s", browser_status_.c_str());
    }

    ImGui::BeginChild("##analyze_results", ImVec2(0, 0), false);
    if (!scanned_once_) {
        ImGui::TextWrapped("Press Scan to index the log directory. Logs are grouped by flight "
                            "date; opening one also opens the other sources' recording of the "
                            "same flight.");
    }

    for (size_t gi = 0; gi < browser_groups_.size(); ++gi) {
        const auto &group = browser_groups_[gi];
        char header[64];
        std::snprintf(header, sizeof(header), "%s  (%zu)", group.date.c_str(),
                      group.entries.size());
  
        ImGui::SetNextItemOpen(gi == 0, ImGuiCond_Once);
        if (!ImGui::TreeNode(header)) continue;

        for (const auto &entry : group.entries) {
            const bool open_already =
                std::any_of(files_.begin(), files_.end(), [&](const Loaded &f) {
                    return f.log && f.log->path() == entry.path;
                });

            ImGui::TextColored(BadgeColor(entry.source, theme), "%-3s",
                                SourceName(entry.source));
            ImGui::SameLine(0, 6 * scale.x);

            char label[256];
            std::snprintf(label, sizeof(label), "%s  %.0fs##%s",
                          FormatTime(entry.t_begin, entry.absolute).c_str(),
                          entry.t_end - entry.t_begin, entry.path.c_str());
            if (ImGui::Selectable(label, open_already)) {
                OpenEntryWithCompanions(entry);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", entry.name.c_str());
                ImGui::Text("%s", entry.path.c_str());
                const auto companions = FindCompanions(browser_groups_, entry);
                if (companions.empty()) {
                    ImGui::TextColored(HintColor(theme),
                                        "no matching logs from other sources");
                } else {
                    ImGui::Text("opens with:");
                    for (const LogEntry *c : companions) {
                        ImGui::BulletText("[%s] %s", SourceName(c->source), c->name.c_str());
                    }
                }
                ImGui::EndTooltip();
            }
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();

    ImGui::PopFont();
    PopPanelStyle();
    EndFixedPanel();
}

void AnalyzePanel::DrawLoadedList(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size)
{
    BeginFixedPanel("AnalyzeLoaded", pos, size, scale, theme, 0, ImVec2(10, 8), true);
    PushPanelStyle(theme, scale);
    ImGui::PushFont(winInit.getFont(18));
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::white_black(theme)), "Loaded");
    ImGui::PopFont();
    ImGui::PushFont(winInit.getFont(14));

    int to_close = -1;
    for (size_t i = 0; i < files_.size(); ++i) {
        auto &f = files_[i];
        ImGui::PushID(static_cast<int>(i));

        ImGui::ColorButton("##color", PaletteColor(f.color_index, theme),
                            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                            ImVec2(12 * scale.uniform(), 12 * scale.uniform()));
        ImGui::SameLine();

        bool selected = (selected_file_ == static_cast<int>(i));
        if (ImGui::Selectable(f.log->name().c_str(), selected)) {
            selected_file_ = static_cast<int>(i);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", f.log->path().c_str());
            ImGui::Text("source: %s", f.log->sys_name().empty() ? "unknown"
                                                                : f.log->sys_name().c_str());
            ImGui::Text("span:   %s .. %s",
                        FormatTime(f.log->t_begin(), f.log->absolute_time()).c_str(),
                        FormatTime(f.log->t_end(), f.log->absolute_time()).c_str());
            ImGui::Text("topics: %zu", f.log->topics().size());
            if (!f.log->absolute_time()) {
                ImGui::TextColored(WarnColor(theme), "boot-relative time");
            }
            ImGui::EndTooltip();
        }

        ImGui::Indent(18 * scale.x);
        if (f.log->track().valid()) {
            ImGui::Checkbox("track", &f.show_track);
        } else {
            ImGui::TextColored(HintColor(theme), "no GPS in this log");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("close")) to_close = static_cast<int>(i);

        // The first log is the time reference; the others can be pulled onto its clock.
        if (i == 0) {
            ImGui::TextColored(HintColor(theme), "time reference");
        } else {
            if (ImGui::SmallButton("align")) {
                const TimeAlignment a = EstimateTimeOffset(*files_[0].log, *f.log);
                if (a.ok) {
                    char msg[192];
                    std::snprintf(msg, sizeof(msg), "aligned %+.2fs (r=%.3f, via %s)", a.offset,
                                  a.correlation, a.signal.c_str());
                    f.align_note = msg;
                    FitTimeRange();
                } else {
                    f.align_note = "no confident match against the reference log";
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Line this log up with %s by correlating a shared signal.\n"
                                   "Use when the clocks disagree -- the onboard clock can step\n"
                                   "mid-session, putting logs minutes away from GPS time.",
                                   files_[0].log->name().c_str());
            }
            if (f.log->time_offset() != 0.0) {
                ImGui::SameLine();
                if (ImGui::SmallButton("reset")) {
                    f.log->set_time_offset(0.0);
                    f.align_note.clear();
                    FitTimeRange();
                }
                ImGui::SameLine();
                ImGui::TextColored(GoodColor(theme), "%+.2fs", f.log->time_offset());
            }
            if (!f.align_note.empty()) {
                ImGui::TextColored(GoodColor(theme), "%s", f.align_note.c_str());
            }
        }
        ImGui::Unindent(18 * scale.x);

        for (const auto &w : f.log->warnings()) {
            ImGui::PushStyleColor(ImGuiCol_Text, WarnColor(theme));
            ImGui::TextWrapped("! %s", w.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }

    if (files_.empty()) {
        ImGui::TextColored(HintColor(theme), "nothing loaded yet");
    }

    if (to_close >= 0) {
        // Drop this file's series, and renumber the ones pointing past it.
        plotted_.erase(std::remove_if(plotted_.begin(), plotted_.end(),
                                       [&](const PlotSeries &p) {
                                           return p.file_index == static_cast<size_t>(to_close);
                                       }),
                        plotted_.end());
        for (auto &p : plotted_) {
            if (p.file_index > static_cast<size_t>(to_close)) p.file_index--;
        }
        files_.erase(files_.begin() + to_close);
        selected_file_ = files_.empty() ? -1 : 0;
        FitTimeRange();
    }

    ImGui::PopFont();
    PopPanelStyle();
    EndFixedPanel();
}

void AnalyzePanel::DrawFieldTree(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size)
{
    BeginFixedPanel("AnalyzeFields", pos, size, scale, theme, 0, ImVec2(10, 8), true);
    PushPanelStyle(theme, scale);
    ImGui::PushFont(winInit.getFont(18));
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::white_black(theme)), "Fields");
    ImGui::PopFont();
    ImGui::PushFont(winInit.getFont(14));

    ImGui::TextColored(HintColor(theme),
                        "click adds to plot %d -- or drag onto any plot", active_pane_ + 1);

    char filter[128];
    std::snprintf(filter, sizeof(filter), "%s", field_filter_.c_str());
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##field_filter", "filter...", filter, sizeof(filter))) {
        field_filter_ = filter;
    }

    ImGui::BeginChild("##analyze_tree", ImVec2(0, 0), false);
    if (files_.empty()) {
        ImGui::TextColored(HintColor(theme), "open a log to see its fields");
    }

    for (size_t fi = 0; fi < files_.size(); ++fi) {
        auto &f = files_[fi];
        ImGui::PushID(static_cast<int>(fi));

        char file_header[320];
        std::snprintf(file_header, sizeof(file_header), "[%s] %s",
                      SourceName(f.log->source()), f.log->name().c_str());
        
        ImGui::SetNextItemOpen(!field_filter_.empty() || fi == 0,
                               field_filter_.empty() ? ImGuiCond_Once : ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_Text, PaletteColor(f.color_index, theme));
        const bool file_open = ImGui::TreeNode(file_header);
        ImGui::PopStyleColor();

        if (file_open) {
            for (const auto &topic : f.log->topics()) {
               
                std::vector<const std::string *> matching;
                for (const auto &field : topic.fields) {
                    if (field_filter_.empty() ||
                        field.find(field_filter_) != std::string::npos ||
                        topic.name.find(field_filter_) != std::string::npos) {
                        matching.push_back(&field);
                    }
                }
                if (matching.empty()) continue;

                char header[256];
                std::snprintf(header, sizeof(header), "%s  (%zu)", topic.name.c_str(),
                              topic.sample_count);
                if (!field_filter_.empty()) ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                if (ImGui::TreeNode(header)) {
                    if (topic.boot_clock) {
                        ImGui::TextColored(WarnColor(theme), "shifted clock");
                    }
                    for (const std::string *field : matching) {
                
                        const bool on = IsPlotted(fi, topic.name, *field, active_pane_);
                        if (ImGui::Selectable(field->c_str(), on)) {
                            ToggleSeries(fi, topic.name, *field, active_pane_);
                        }
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                            drag_ref_ = {fi, topic.name, *field, true};
                        
                            ImGui::SetDragDropPayload("ANALYZE_FIELD", &fi, sizeof(fi));
                            ImGui::Text("%s.%s", topic.name.c_str(), field->c_str());
                            ImGui::EndDragDropSource();
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::PopFont();
    PopPanelStyle();
    EndFixedPanel();
}

void AnalyzePanel::DrawPlots(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size)
{
    BeginFixedPanel("AnalyzePlots", pos, size, scale, theme, 0, ImVec2(6, 6));

    // Absolute-time formatting only makes sense if every loaded log has it.
    bool absolute = !files_.empty();
    for (const auto &f : files_) {
        if (!f.log->absolute_time()) absolute = false;
    }

    // Toolbar.
    PushPanelStyle(theme, scale);
    ImGui::PushFont(winInit.getFont(14));
    if (ImGui::Button("Fit")) FitTimeRange();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("reset the time axis to all loaded logs");
    ImGui::SameLine();
    if (ImGui::Button("Add plot")) AddPane();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("add another stacked plot");
    ImGui::SameLine();
    if (ImGui::Button("Clear")) plotted_.clear();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("remove every plotted field");
    ImGui::SameLine();
    ImGui::Checkbox("Auto Y", &y_autofit_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Y rescales to what's in view, so scroll/drag zoom time only.\n"
                           "Uncheck to zoom and pan Y by hand.");
    }
    if (plotted_.empty()) {
        ImGui::SameLine(0, 12 * scale.x);
        ImGui::TextColored(HintColor(theme), "click or drag a field onto a plot");
    }
    ImGui::PopFont();

    PushPlotTheme(theme);

    const float toolbar_h = ImGui::GetCursorPosY();
    const float avail_h = size.y - toolbar_h - 12 * scale.y;
    const float header_h = 20 * scale.y;
    const float min_plot_h = 150 * scale.y;
    const float plot_h =
        std::max(min_plot_h, avail_h / std::max(1, pane_count_) - header_h - 4 * scale.y);

    ImGui::BeginChild("##analyze_panes", ImVec2(0, 0), false);

    int pane_to_remove = -1;
    for (int pane = 0; pane < pane_count_; ++pane) {
        ImGui::PushID(pane);
        ImGui::PushFont(winInit.getFont(14));


        const bool is_active = (active_pane_ == pane);
       
        if (is_active) ImGui::PushStyleColor(ImGuiCol_Button, AccentColor(theme));
        char pane_label[32];
        std::snprintf(pane_label, sizeof(pane_label), "Plot %d", pane + 1);
        if (ImGui::Button(pane_label)) active_pane_ = pane;
        if (is_active) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("send clicked fields to this plot");

        int to_remove = -1;
        for (size_t i = 0; i < plotted_.size(); ++i) {
            if (plotted_[i].pane != pane) continue;
            ImGui::SameLine();
            ImGui::PushID(static_cast<int>(i));
           
            ImVec4 chip = PaletteColor(plotted_[i].color_index, theme);
            chip.w = 0.22f;
            ImVec4 chip_hover = PaletteColor(plotted_[i].color_index, theme);
            chip_hover.w = 0.42f;
            ImGui::PushStyleColor(ImGuiCol_Button, chip);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chip_hover);
            ImGui::PushStyleColor(ImGuiCol_Border, PaletteColor(plotted_[i].color_index, theme));
            char label[256];
            std::snprintf(label, sizeof(label), "%s.%s  x", plotted_[i].topic.c_str(),
                          plotted_[i].field.c_str());
            if (ImGui::Button(label)) to_remove = static_cast<int>(i);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s -- click to remove",
                                  files_[plotted_[i].file_index].log->name().c_str());
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
        if (pane_count_ > 1) {
            ImGui::SameLine(0, 14 * scale.x);
            if (ImGui::Button("Remove")) pane_to_remove = pane;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("remove this plot and its fields");
        }
        ImGui::PopFont();
        if (to_remove >= 0) RemoveSeries(static_cast<size_t>(to_remove));

        char plot_id[32];
        std::snprintf(plot_id, sizeof(plot_id), "##analyze_plot_%d", pane);
        if (ImPlot::BeginPlot(plot_id, ImVec2(-1, plot_h),
                               ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText)) {
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoHighlight);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr,
                              ImPlotAxisFlags_NoHighlight |
                                  (y_autofit_ ? ImPlotAxisFlags_AutoFit : 0));
            if (absolute) ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
           
            if (range_valid_) ImPlot::SetupAxisLinks(ImAxis_X1, &x_min_, &x_max_);

            for (auto &p : plotted_) {
                if (p.pane != pane) continue;
                const Series *s = files_[p.file_index].log->GetSeries(p.topic, p.field);
                if (!s || s->t.empty()) continue;
                char label[300];
                std::snprintf(label, sizeof(label), "%s.%s [%s]", p.topic.c_str(),
                              p.field.c_str(), files_[p.file_index].log->name().c_str());
                ImPlotSpec spec;
                spec.LineColor = PaletteColor(p.color_index, theme);
                spec.LineWeight = 1.6f * scale.uniform();
                ImPlot::PlotLine(label, s->t.data(), s->v.data(),
                                 static_cast<int>(s->t.size()), spec);
            }

            
            if (cursor_valid_) {
                ImPlot::DragLineX(pane, &cursor_t_, AccentColor(theme),
                                   1.5f * scale.uniform());
            }

            if (ImPlot::BeginDragDropTargetPlot()) {
                if (ImGui::AcceptDragDropPayload("ANALYZE_FIELD") && drag_ref_.valid) {
                    if (!IsPlotted(drag_ref_.file_index, drag_ref_.topic, drag_ref_.field,
                                    pane)) {
                        plotted_.push_back({drag_ref_.file_index, drag_ref_.topic,
                                            drag_ref_.field, NextColor(), pane});
                    }
                    active_pane_ = pane;
                }
                ImPlot::EndDragDropTarget();
            }
            ImPlot::EndPlot();
        }
        ImGui::PopID();
    }

    ImGui::EndChild();
    PopPlotTheme();
    PopPanelStyle();

    if (pane_to_remove >= 0) RemovePane(pane_to_remove);
    EndFixedPanel();
}

void AnalyzePanel::DrawMap(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size,
                            Location &location, int &map_zoom, unsigned int placeholder_tile)
{
    // Centre on the midpoint of the first visible track.
    double center_lat = 0.0, center_lon = 0.0;
    bool have_center = false;
    for (const auto &f : files_) {
        if (!f.show_track || !f.log->track().valid()) continue;
        const auto &tr = f.log->track();
        center_lat = tr.lat[tr.lat.size() / 2];
        center_lon = tr.lon[tr.lon.size() / 2];
        have_center = true;
        break;
    }

    BeginFixedPanel("AnalyzeMap", pos, size, scale, theme, 0, ImVec2(0, 0));

    if (!have_center) {
        const ImVec2 origin = ImGui::GetWindowPos();
        ImDrawList *dl = ImGui::GetForegroundDrawList();
        ImGui::PushFont(winInit.getFont(14));
        const char *lines[] = {"No GPS track in the selected log(s).",
                                "asr_logger records no position fix --",
                                "open the matching flight controller log",
                                "to see the path."};
        for (int i = 0; i < 4; ++i) {
            dl->AddText(ImVec2(origin.x + 16 * scale.x, origin.y + (16 + i * 18) * scale.y),
                        ImGui::ColorConvertFloat4ToU32(HintColor(theme)), lines[i]);
        }
        ImGui::PopFont();
        EndFixedPanel();
        return;
    }

    const ImVec2 map_pos =
        location.MapWidget(center_lat, center_lon, size.x, size.y, scale, map_zoom,
                            placeholder_tile, theme);

    {
        ImDrawList *dl = ImGui::GetForegroundDrawList();
        const ImVec2 clip_min = map_pos;
        const ImVec2 clip_max = ImVec2(map_pos.x + size.x, map_pos.y + size.y);
        dl->PushClipRect(clip_min, clip_max, true);

        const ImVec2 origin = ImGui::GetWindowPos();
        float note_y = size.y - 22 * scale.y;

        for (const auto &f : files_) {
            if (!f.show_track || !f.log->track().valid()) continue;
            const auto &tr = f.log->track();
            const ImVec4 track_col = PaletteColor(f.color_index, theme);
            const ImU32 col = ImGui::ColorConvertFloat4ToU32(track_col);
            ImVec4 faded_col = track_col;
            faded_col.w = 0.28f;
            const ImU32 faded = ImGui::ColorConvertFloat4ToU32(faded_col);

            auto project = [&](size_t i) {
                return location.latLonToScreenPos(tr.lat[i], tr.lon[i], center_lat, center_lon,
                                                   map_pos, size.x, size.y, scale, map_zoom);
            };

            const bool in_span =
                cursor_valid_ && cursor_t_ >= tr.t.front() && cursor_t_ <= tr.t.back();
            ImVec2 prev = project(0);
            for (size_t i = 1; i < tr.lat.size(); ++i) {
                const ImVec2 cur = project(i);
                const bool past = !in_span || tr.t[i] <= cursor_t_;
                dl->AddLine(prev, cur, past ? col : faded, 2.0f * scale.uniform());
                prev = cur;
            }

            double lat, lon;
            if (in_span && TrackPosAt(tr, cursor_t_, lat, lon)) {
                const ImVec2 at =
                    location.latLonToScreenPos(lat, lon, center_lat, center_lon, map_pos,
                                                size.x, size.y, scale, map_zoom);
                const float r = 9.0f * scale.uniform();
                dl->AddLine(ImVec2(at.x - r * 1.9f, at.y), ImVec2(at.x - r * 0.6f, at.y),
                            IM_COL32(255, 255, 255, 220), 1.5f * scale.uniform());
                dl->AddLine(ImVec2(at.x + r * 0.6f, at.y), ImVec2(at.x + r * 1.9f, at.y),
                            IM_COL32(255, 255, 255, 220), 1.5f * scale.uniform());
                dl->AddLine(ImVec2(at.x, at.y - r * 1.9f), ImVec2(at.x, at.y - r * 0.6f),
                            IM_COL32(255, 255, 255, 220), 1.5f * scale.uniform());
                dl->AddLine(ImVec2(at.x, at.y + r * 0.6f), ImVec2(at.x, at.y + r * 1.9f),
                            IM_COL32(255, 255, 255, 220), 1.5f * scale.uniform());
                dl->AddCircleFilled(at, r * 0.55f, col);
                dl->AddCircle(at, r, IM_COL32(255, 255, 255, 230), 16, 1.8f * scale.uniform());
            } else if (cursor_valid_) {
                const double gap = cursor_t_ < tr.t.front() ? tr.t.front() - cursor_t_
                                                             : cursor_t_ - tr.t.back();
                char note[192];
                std::snprintf(note, sizeof(note), "%s: cursor %.0fs %s this track",
                              f.log->name().c_str(), gap,
                              cursor_t_ < tr.t.front() ? "before" : "after");
                ImGui::PushFont(winInit.getFont(14));
                dl->AddText(ImVec2(origin.x + 12 * scale.x, origin.y + note_y),
                            ImGui::ColorConvertFloat4ToU32(WarnColor(theme)), note);
                ImGui::PopFont();
                note_y -= 18 * scale.y;
            }
        }
        dl->PopClipRect();
    }

    EndFixedPanel();
}

bool AnalyzePanel::BuildXySamples(const XyCurve &curve, std::vector<double> &xs,
                                   std::vector<double> &ys, std::vector<double> &ts)
{
    if (!curve.x.valid || !curve.y.valid) return false;
    if (curve.x.file_index >= files_.size() || curve.y.file_index >= files_.size()) return false;

    const Series *sx = files_[curve.x.file_index].log->GetSeries(curve.x.topic, curve.x.field);
    const Series *sy = files_[curve.y.file_index].log->GetSeries(curve.y.topic, curve.y.field);
    if (!sx || !sy || sx->t.size() < 2 || sy->t.size() < 2) return false;

    const double lo = std::max(sx->t.front(), sy->t.front());
    const double hi = std::min(sx->t.back(), sy->t.back());
    if (hi <= lo) return false;   // the two channels never coexist

    // Follow the sparser of the two -- resampling finer invents detail neither recorded.
    const double dx = (sx->t.back() - sx->t.front()) / static_cast<double>(sx->t.size() - 1);
    const double dy = (sy->t.back() - sy->t.front()) / static_cast<double>(sy->t.size() - 1);
    double step = std::max(dx, dy);
    const double kMaxPoints = 20000.0;
    if ((hi - lo) / step > kMaxPoints) step = (hi - lo) / kMaxPoints;

    xs.clear();
    ys.clear();
    ts.clear();
    for (double t = lo; t <= hi; t += step) {
        double vx, vy;
        if (!SampleAt(*sx, t, vx) || !SampleAt(*sy, t, vy)) continue;
        xs.push_back(vx);
        ys.push_back(vy);
        ts.push_back(t);
    }
    return xs.size() > 1;
}

void AnalyzePanel::DrawXyPlot(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size)
{
    BeginFixedPanel("AnalyzeXy", pos, size, scale, theme, 0, ImVec2(8, 6), true);
    ImGui::PushFont(winInit.getFont(14));

    std::vector<std::string> labels;
    labels.reserve(plotted_.size());
    for (const auto &p : plotted_) {
        labels.push_back(p.topic + "." + p.field + " [" +
                         files_[p.file_index].log->name() + "]");
    }

    if (plotted_.size() < 2) {
        ImGui::TextWrapped("Add at least two fields to a time plot, then pair them here -- "
                            "e.g. position.pos_x against position.pos_y for a ground track.");
        ImGui::PopFont();
        EndFixedPanel();
        return;
    }

    xy_pick_x_ = std::clamp(xy_pick_x_, 0, static_cast<int>(labels.size()) - 1);
    xy_pick_y_ = std::clamp(xy_pick_y_, 0, static_cast<int>(labels.size()) - 1);
    PushPanelStyle(theme, scale);

    auto combo = [&](const char *id, int &choice) {
        ImGui::SetNextItemWidth(size.x * 0.36f);
        if (ImGui::BeginCombo(id, labels[static_cast<size_t>(choice)].c_str())) {
            for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
                if (ImGui::Selectable(labels[static_cast<size_t>(i)].c_str(), choice == i)) {
                    choice = i;
                }
            }
            ImGui::EndCombo();
        }
    };
    combo("##xy_x", xy_pick_x_);
    ImGui::SameLine();
    combo("##xy_y", xy_pick_y_);
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        const auto &px = plotted_[static_cast<size_t>(xy_pick_x_)];
        const auto &py = plotted_[static_cast<size_t>(xy_pick_y_)];
        xy_curves_.push_back({{px.file_index, px.topic, px.field, true},
                              {py.file_index, py.topic, py.field, true},
                              NextColor()});
    }

    ImGui::Checkbox("Equal axes", &xy_equal_axes_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("same units per pixel on both axes, so a circular path\n"
                           "looks circular instead of an ellipse");
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##xy")) xy_curves_.clear();

    int remove = -1;
    for (size_t i = 0; i < xy_curves_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImVec4 chip = PaletteColor(xy_curves_[i].color_index, theme);
        chip.w = 0.22f;
        ImVec4 chip_hover = PaletteColor(xy_curves_[i].color_index, theme);
        chip_hover.w = 0.42f;
        ImGui::PushStyleColor(ImGuiCol_Button, chip);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chip_hover);
        ImGui::PushStyleColor(ImGuiCol_Border, PaletteColor(xy_curves_[i].color_index, theme));
        char lbl[256];
        std::snprintf(lbl, sizeof(lbl), "%s vs %s  x", xy_curves_[i].x.field.c_str(),
                      xy_curves_[i].y.field.c_str());
        if (ImGui::Button(lbl)) remove = static_cast<int>(i);
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    if (remove >= 0) xy_curves_.erase(xy_curves_.begin() + remove);
    PopPanelStyle();

    PushPlotTheme(theme);
    const ImPlotFlags flags = ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText |
                              (xy_equal_axes_ ? ImPlotFlags_Equal : 0);
    if (ImPlot::BeginPlot("##analyze_xy", ImVec2(-1, -1), flags)) {
        ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoHighlight);
        ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoHighlight);

        for (const auto &curve : xy_curves_) {
            std::vector<double> xs, ys, ts;
            if (!BuildXySamples(curve, xs, ys, ts)) continue;

            char label[300];
            std::snprintf(label, sizeof(label), "%s vs %s", curve.x.field.c_str(),
                          curve.y.field.c_str());
            ImPlotSpec spec;
            spec.LineColor = PaletteColor(curve.color_index, theme);
            spec.LineWeight = 1.6f * scale.uniform();
            ImPlot::PlotLine(label, xs.data(), ys.data(), static_cast<int>(xs.size()), spec);

            if (cursor_valid_ && cursor_t_ >= ts.front() && cursor_t_ <= ts.back()) {
                const auto it = std::lower_bound(ts.begin(), ts.end(), cursor_t_);
                const size_t k = std::min(static_cast<size_t>(it - ts.begin()), xs.size() - 1);
                const ImVec2 px = ImPlot::PlotToPixels(xs[k], ys[k]);
                ImDrawList *dl = ImPlot::GetPlotDrawList();
                ImPlot::PushPlotClipRect();
                dl->AddCircleFilled(px, 4.5f * scale.uniform(),
                                     ImGui::ColorConvertFloat4ToU32(PaletteColor(curve.color_index, theme)));
                dl->AddCircle(px, 6.5f * scale.uniform(), IM_COL32(255, 255, 255, 230), 16, 1.6f * scale.uniform());
                ImPlot::PopPlotClipRect();
            }
        }
        ImPlot::EndPlot();
    }
    PopPlotTheme();

    ImGui::PopFont();
    EndFixedPanel();
}

void AnalyzePanel::DrawCursorReadout(const UiScale& scale, bool theme, ImVec2 pos, ImVec2 size)
{
    BeginFixedPanel("AnalyzeCursor", pos, size, scale, theme, 0, ImVec2(10, 8), true);
    PushPanelStyle(theme, scale);
    ImGui::PushFont(winInit.getFont(14));

    bool absolute = !files_.empty();
    for (const auto &f : files_) {
        if (!f.log->absolute_time()) absolute = false;
    }

    if (!cursor_valid_) {
        ImGui::TextColored(HintColor(theme), "load a log to place the cursor");
    } else {
        ImGui::PushFont(winInit.getFont(18));
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::white_black(theme)), "t = %s",
                            FormatTime(cursor_t_, absolute).c_str());
        ImGui::PopFont();

        // Where the vehicle was at this instant, for any log that carries a fix.
        for (const auto &f : files_) {
            if (!f.log->track().valid()) continue;
            double lat, lon;
            if (!TrackPosAt(f.log->track(), cursor_t_, lat, lon)) continue;
            ImGui::ColorButton("##t", PaletteColor(f.color_index, theme),
                                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                                ImVec2(10 * scale.uniform(), 10 * scale.uniform()));
            ImGui::SameLine(0, 6 * scale.x);
            ImGui::Text("%.7f, %.7f", lat, lon);
        }
        ImGui::Separator();

        for (const auto &p : plotted_) {
            const Series *s = files_[p.file_index].log->GetSeries(p.topic, p.field);
            if (!s) continue;
            double v;
            ImGui::ColorButton("##c", PaletteColor(p.color_index, theme),
                                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                                ImVec2(10 * scale.uniform(), 10 * scale.uniform()));
            ImGui::SameLine(0, 6 * scale.x);
            if (SampleAt(*s, cursor_t_, v)) {
                ImGui::Text("%s.%s = %.4g", p.topic.c_str(), p.field.c_str(), v);
            } else {
                ImGui::TextColored(HintColor(theme), "%s.%s -- outside span",
                                    p.topic.c_str(), p.field.c_str());
            }
            if (!s->note.empty() && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", s->note.c_str());
            }
        }
    }

    ImGui::PopFont();
    PopPanelStyle();
    EndFixedPanel();
}

// ---------------------------------------------------------------------------

void AnalyzePanel::Draw(const UiScale& scale, bool theme, float display_w, float display_h,
                         Location &location, int &map_zoom, unsigned int placeholder_tile)
{
    const float top_y = 70 * scale.y;
    const float margin_x = 10 * scale.x;
    const float margin_y = 10 * scale.y;
    const float content_h = std::max(200 * scale.y, display_h - top_y - 3 * margin_y);

    const float left_x = 70 * scale.x;
    const float left_w = 400 * scale.x;
    // The tree is the panel that actually needs room; the other two are mostly chrome.
    const float browser_h = content_h * 0.30f;
    const float loaded_h = content_h * 0.20f;
    const float fields_h = content_h - browser_h - loaded_h - 2 * margin_y;


    if (!scanned_once_) Rescan();

    DrawBrowser(scale, theme, ImVec2(left_x, top_y), ImVec2(left_w, browser_h));
    DrawLoadedList(scale, theme, ImVec2(left_x, top_y + browser_h + margin_y),
                   ImVec2(left_w, loaded_h));
    DrawFieldTree(scale, theme, ImVec2(left_x, top_y + browser_h + loaded_h + 2 * margin_y),
                  ImVec2(left_w, fields_h));

    const float right_x = left_x + left_w + margin_x;
    const float right_w = std::max(400 * scale.x, display_w - right_x - margin_x);
    const float map_w = std::min(520 * scale.x, right_w * 0.36f);
    const float plot_w = right_w - map_w - margin_x;

    DrawPlots(scale, theme, ImVec2(right_x, top_y), ImVec2(plot_w, content_h));

    const float map_x = right_x + plot_w + margin_x;
    const float tab_h = 26 * scale.y;
    const float map_h = content_h * 0.62f - tab_h - 4 * scale.y;

    ImGui::SetCursorPos(ImVec2(map_x, top_y));
    ImGui::PushFont(winInit.getFont(14));
    PushPanelStyle(theme, scale);
    const auto view_tab = [&](const char *label, RightView view, const char *tip) {
        const bool on = (right_view_ == view);
        if (on) ImGui::PushStyleColor(ImGuiCol_Button, AccentColor(theme));
        if (ImGui::Button(label)) right_view_ = view;
        if (on) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
    };
    view_tab("Map", RightView::Map, "flight path on satellite imagery -- needs GPS");
    ImGui::SameLine();
    view_tab("XY", RightView::Xy,
             "one channel against another, matched by time.\n"
             "pos_x vs pos_y draws the flown path with no GPS needed.");
    PopPanelStyle();
    ImGui::PopFont();

    const ImVec2 view_pos(map_x, top_y + tab_h);
    const ImVec2 view_size(map_w, map_h);
    if (right_view_ == RightView::Map) {
        DrawMap(scale, theme, view_pos, view_size, location, map_zoom, placeholder_tile);
    } else {
        DrawXyPlot(scale, theme, view_pos, view_size);
    }
    DrawCursorReadout(scale, theme, ImVec2(map_x, view_pos.y + map_h + margin_y),
                       ImVec2(map_w, content_h - tab_h - map_h - margin_y));
}

}  // namespace analyze
