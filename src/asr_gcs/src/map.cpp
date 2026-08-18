#include "map.h"
#include <ament_index_cpp/get_package_share_directory.hpp>

float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}
static GLuint UploadImageToGL(const cv::Mat& img, const char* path)
{
    if (img.empty()) {
        return 0;
    }

    cv::Mat rgba;
    switch (img.channels()) {
        case 1: cv::cvtColor(img, rgba, cv::COLOR_GRAY2RGBA); break;
        case 3: cv::cvtColor(img, rgba, cv::COLOR_BGR2RGBA);  break;
        case 4: cv::cvtColor(img, rgba, cv::COLOR_BGRA2RGBA); break;
        default:
            std::cerr << "Unexpected channel count (" << img.channels() << ") in " << path << std::endl;
            return 0;
    }

    constexpr int TILE_SIZE = 256;
    cv::Mat canvas(TILE_SIZE, TILE_SIZE, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    int copyW = std::min(TILE_SIZE, rgba.cols);
    int copyH = std::min(TILE_SIZE, rgba.rows);
    if (copyW > 0 && copyH > 0) {
        rgba(cv::Rect(0, 0, copyW, copyH)).copyTo(canvas(cv::Rect(0, 0, copyW, copyH)));
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TILE_SIZE, TILE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, canvas.data);

    return tex;
}

void LocalOffsetToLatLon(double home_lat, double home_lon, double north_m, double east_m,
                          double &out_lat, double &out_lon)
{
    const double a = 6378137.0;            // WGS84 semi-major axis, metres
    const double f = 1.0 / 298.257223563;  // WGS84 flattening
    const double e2 = 2.0 * f - f * f;     // first eccentricity squared

    const double lat = home_lat * M_PI / 180.0;
    const double lon = home_lon * M_PI / 180.0;

    // Home, geodetic -> ECEF (height 0 -- only horizontal offsets are projected).
    const double N = a / std::sqrt(1.0 - e2 * std::sin(lat) * std::sin(lat));
    const double x0 = N * std::cos(lat) * std::cos(lon);
    const double y0 = N * std::cos(lat) * std::sin(lon);
    const double z0 = N * (1.0 - e2) * std::sin(lat);

    // Rotate the local north/east offset into ECEF using home's ENU basis
    const double dx = -east_m * std::sin(lon) - north_m * std::sin(lat) * std::cos(lon);
    const double dy =  east_m * std::cos(lon) - north_m * std::sin(lat) * std::sin(lon);
    const double dz =  north_m * std::cos(lat);

    const double x = x0 + dx;
    const double y = y0 + dy;
    const double z = z0 + dz;

    // ECEF -> geodetic
    const double b = a * (1.0 - f);
    const double e2_prime = (a * a - b * b) / (b * b);
    const double p = std::sqrt(x * x + y * y);
    const double theta = std::atan2(z * a, p * b);

    const double result_lat = std::atan2(z + e2_prime * b * std::pow(std::sin(theta), 3),
                                          p - e2 * a * std::pow(std::cos(theta), 3));
    const double result_lon = std::atan2(y, x);

    out_lat = result_lat * 180.0 / M_PI;
    out_lon = result_lon * 180.0 / M_PI;
}

GLuint Location::display_map(const char* path, float scale)
{
    cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
    return UploadImageToGL(img, path);
}

Location::TileCoord Location::latLonToTile(double lat, double lon, int zoom)
{
    int n = 1 << zoom;
    int x = (int)((lon + 180.0) / 360.0 * n);
    double latRad = lat * M_PI / 180.0;
    int y = (int)((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n);
    return {x, y};
}

ImVec2 Location::latLonToTileOffset(double lat, double lon, int zoom)
{
    int n = 1 << zoom;
    double xFrac = (lon + 180.0) / 360.0 * n;
    double latRad = lat * M_PI / 180.0;
    double yFrac = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n;
    return { (float)((xFrac - floor(xFrac)) * 256),
             (float)((yFrac - floor(yFrac)) * 256) };
}

ImVec2 Location::latLonToScreenPos(double lat, double lon, double centerLat, double centerLon,
                                    ImVec2 widgetPos, float width, float height, float scale, int zoom)
{
    TileCoord pointTile = latLonToTile(lat, lon, zoom);
    ImVec2 pointOffset = latLonToTileOffset(lat, lon, zoom);
    TileCoord centerTile = latLonToTile(centerLat, centerLon, zoom);
    ImVec2 centerOffset = latLonToTileOffset(centerLat, centerLon, zoom);

    float dxPixels = (pointTile.x - centerTile.x) * 256.0f + (pointOffset.x - centerOffset.x);
    float dyPixels = (pointTile.y - centerTile.y) * 256.0f + (pointOffset.y - centerOffset.y);

    return ImVec2(widgetPos.x + width / 2.0f + dxPixels * scale,
                   widgetPos.y + height / 2.0f + dyPixels * scale);
}

GLuint Location::loadTileCached(int zoom, int x, int y)
{
    std::string key = std::to_string(zoom) + "/" + std::to_string(x) + "/" + std::to_string(y);

    auto it = tileCache.find(key);
    if (it != tileCache.end()) {
        tileLRU.splice(tileLRU.begin(), tileLRU, lruPos[key]);
        return it->second;
    }
    std::string package_path = ament_index_cpp::get_package_share_directory("asr_gcs");
    std::string path = package_path + "/tiles/" + key + ".png";

    GLuint tex = display_map(path.c_str(), 1.0f);
    if (!tex) return 0;

    tileCache[key] = tex;
    tileLRU.push_front(key);
    lruPos[key] = tileLRU.begin();

    if (tileCache.size() > MAX_CACHED_TILES) {
        std::string oldestKey = tileLRU.back();
        GLuint oldTex = tileCache[oldestKey];
        glDeleteTextures(1, &oldTex);
        tileCache.erase(oldestKey);
        lruPos.erase(oldestKey);
        tileLRU.pop_back();
    }

    return tex;
}

ImVec2 Location::MapWidget(double lat, double lon, float width, float height, float scale, int zoom, GLuint placeholderTile, bool theme)
{
    ImGui::BeginChild("MapWidget", ImVec2(width, height), false,
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();

    float tileSize = 256.0f * scale;

    TileCoord center = latLonToTile(lat, lon, zoom);
    ImVec2 offset = latLonToTileOffset(lat, lon, zoom);

    int tilesX = (int)ceil(width  / tileSize / 2) + 1;
    int tilesY = (int)ceil(height / tileSize / 2) + 1;

    for (int dy = -tilesY; dy <= tilesY; dy++)
    for (int dx = -tilesX; dx <= tilesX; dx++)
    {
        int tx = center.x + dx;
        int ty = center.y + dy;
        float sx = pos.x + width  / 2.0f + dx * tileSize - offset.x * scale;
        float sy = pos.y + height / 2.0f + dy * tileSize - offset.y * scale;
        GLuint tex = loadTileCached(zoom, tx, ty);
        if (tex) {
            draw->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(sx, sy),
                ImVec2(sx + tileSize, sy + tileSize));
        }
    }

    float roundRadius = 12.0f * scale;
    ImU32 maskColor = ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme));
    ImVec2 p_min = pos;
    ImVec2 p_max = ImVec2(pos.x + width, pos.y + height);

    // Top-left
    draw->PathClear();
    draw->PathLineTo(p_min);
    draw->PathLineTo(ImVec2(p_min.x + roundRadius, p_min.y));
    draw->PathArcTo(ImVec2(p_min.x + roundRadius, p_min.y + roundRadius), roundRadius, -IM_PI * 0.5f, -IM_PI, 8);
    draw->PathFillConvex(maskColor);

    // Top-right
    draw->PathClear();
    draw->PathLineTo(ImVec2(p_max.x, p_min.y));
    draw->PathLineTo(ImVec2(p_max.x, p_min.y + roundRadius));
    draw->PathArcTo(ImVec2(p_max.x - roundRadius, p_min.y + roundRadius), roundRadius, 0.0f, -IM_PI * 0.5f, 8);
    draw->PathFillConvex(maskColor);

    // Bottom-left
    draw->PathClear();
    draw->PathLineTo(ImVec2(p_min.x, p_max.y));
    draw->PathLineTo(ImVec2(p_min.x + roundRadius, p_max.y));
    draw->PathArcTo(ImVec2(p_min.x + roundRadius, p_max.y - roundRadius), roundRadius, IM_PI * 0.5f, IM_PI, 8);
    draw->PathFillConvex(maskColor);

    // Bottom-right
    draw->PathClear();
    draw->PathLineTo(p_max);
    draw->PathLineTo(ImVec2(p_max.x, p_max.y - roundRadius));
    draw->PathArcTo(ImVec2(p_max.x - roundRadius, p_max.y - roundRadius), roundRadius, 0.0f, IM_PI * 0.5f, 8);
    draw->PathFillConvex(maskColor);

    ImGui::EndChild();
    return pos;
}

void Location::NoSatMap(const DroneInformation& Info, float width, float height, float scale, int zoom, bool theme){
    ImGui::BeginChild("NoSatMapWidget", ImVec2(width, height), false,
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 center = ImVec2(pos.x + width * 0.5f, pos.y + height * 0.5f);
    
    float world_min = -50.0f;
    float world_max = 50.0f;
    float world_range = world_max - world_min; 

    float pixels_per_meter_x = width  / world_range;
    float pixels_per_meter_y = height / world_range;
    float grid_step_meters = 10.0f;

    ImVec2 dot_pos;
    dot_pos.x = map_value(Info.xyz_pos[1], world_min, world_max, pos.x, pos.x + width); //flipped x and y to match satmap
    dot_pos.y = map_value(-Info.xyz_pos[0], world_min, world_max, pos.y, pos.y + height);

    

     for (int i = -5; i <= 5; i++) {   // -50, -40, ..., 0, ..., 40, 50
        float x_offset = i * grid_step_meters * pixels_per_meter_x;
        float line_x = center.x + x_offset;
        draw->AddLine(
            ImVec2(line_x, pos.y),
            ImVec2(line_x, pos.y + height),
            Color::panelBorder(theme), 1.0f);
    }

    
    for (int i = -5; i <= 5; i++) {
        float y_offset = i * grid_step_meters * pixels_per_meter_y;
        float line_y = center.y + y_offset;
        draw->AddLine(
            ImVec2(pos.x, line_y),
            ImVec2(pos.x + width, line_y),
            Color::panelBorder(theme), 1.0f);
    }

    draw->AddCircleFilled(dot_pos, 6.0f * scale, IM_COL32(255, 50, 50, 255));


    float roundRadius = 12.0f * scale;
    ImU32 maskColor = ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme));
    ImVec2 p_min = pos;
    ImVec2 p_max = ImVec2(pos.x + width, pos.y + height);

    // Top-left
    draw->PathClear();
    draw->PathLineTo(p_min);
    draw->PathLineTo(ImVec2(p_min.x + roundRadius, p_min.y));
    draw->PathArcTo(ImVec2(p_min.x + roundRadius, p_min.y + roundRadius), roundRadius, -IM_PI * 0.5f, -IM_PI, 8);
    draw->PathFillConvex(maskColor);

    // Top-right
    draw->PathClear();
    draw->PathLineTo(ImVec2(p_max.x, p_min.y));
    draw->PathLineTo(ImVec2(p_max.x, p_min.y + roundRadius));
    draw->PathArcTo(ImVec2(p_max.x - roundRadius, p_min.y + roundRadius), roundRadius, 0.0f, -IM_PI * 0.5f, 8);
    draw->PathFillConvex(maskColor);

    // Bottom-left
    draw->PathClear();
    draw->PathLineTo(ImVec2(p_min.x, p_max.y));
    draw->PathLineTo(ImVec2(p_min.x + roundRadius, p_max.y));
    draw->PathArcTo(ImVec2(p_min.x + roundRadius, p_max.y - roundRadius), roundRadius, IM_PI * 0.5f, IM_PI, 8);
    draw->PathFillConvex(maskColor);

    // Bottom-right
    draw->PathClear();
    draw->PathLineTo(p_max);
    draw->PathLineTo(ImVec2(p_max.x, p_max.y - roundRadius));
    draw->PathArcTo(ImVec2(p_max.x - roundRadius, p_max.y - roundRadius), roundRadius, 0.0f, IM_PI * 0.5f, 8);
    draw->PathFillConvex(maskColor);


    ImGui::EndChild();
}