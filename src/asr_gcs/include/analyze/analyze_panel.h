#pragma once

#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

#include "analyze/ulog_data.h"

class Location;

namespace analyze {


class AnalyzePanel {
public:
    AnalyzePanel();

    void Draw(float scale, bool theme, float display_w, float display_h,
              Location &location, int &map_zoom, unsigned int placeholder_tile);

private:
    struct Loaded {
        std::unique_ptr<LogFile> log;
        int color_index = 0;
        bool show_track = true;
        std::string align_note;  
    };

    struct PlotSeries {
        size_t file_index;
        std::string topic;
        std::string field;
        int color_index;
        int pane;   
    };


    struct SeriesRef {
        size_t file_index = 0;
        std::string topic;
        std::string field;
        bool valid = false;
    };

    struct XyCurve {
        SeriesRef x, y;
        int color_index;
    };

    enum class RightView { Map, Xy };

    void DrawBrowser(float scale, bool theme, ImVec2 pos, ImVec2 size);
    void DrawLoadedList(float scale, bool theme, ImVec2 pos, ImVec2 size);
    void DrawFieldTree(float scale, bool theme, ImVec2 pos, ImVec2 size);
    void DrawPlots(float scale, bool theme, ImVec2 pos, ImVec2 size);
    void DrawMap(float scale, bool theme, ImVec2 pos, ImVec2 size,
                 Location &location, int &map_zoom, unsigned int placeholder_tile);
    void DrawXyPlot(float scale, bool theme, ImVec2 pos, ImVec2 size);
    bool BuildXySamples(const XyCurve &curve, std::vector<double> &xs, std::vector<double> &ys,
                        std::vector<double> &ts);
    void DrawCursorReadout(float scale, bool theme, ImVec2 pos, ImVec2 size);

   
    bool OpenLog(const std::string &path);
    void OpenEntryWithCompanions(const LogEntry &entry);
    void Rescan();
    void ToggleSeries(size_t file_index, const std::string &topic, const std::string &field,
                      int pane);
    bool IsPlotted(size_t file_index, const std::string &topic, const std::string &field,
                   int pane) const;
    void RemoveSeries(size_t plot_index);
    void AddPane();
    void RemovePane(int pane);
    int NextColor();
    bool TimeRange(double &lo, double &hi) const;
    void FitTimeRange();

    std::vector<Loaded> files_;
    std::vector<PlotSeries> plotted_;
    std::vector<DateGroup> browser_groups_;
    std::string browser_path_;
    std::string browser_status_;
    std::string field_filter_;
    int selected_file_ = -1;
    int active_pane_ = 0;   
    int pane_count_ = 1;
    int color_index_ = 0;
    SeriesRef drag_ref_;
    double cursor_t_ = 0.0;
    bool cursor_valid_ = false;
    bool scanned_once_ = false;
    double x_min_ = 0.0, x_max_ = 0.0;
    bool range_valid_ = false;
    bool y_autofit_ = true;

    RightView right_view_ = RightView::Map;
    std::vector<XyCurve> xy_curves_;
    int xy_pick_x_ = 0, xy_pick_y_ = 1;  
    bool xy_equal_axes_ = true;           
};

}  // namespace analyze
