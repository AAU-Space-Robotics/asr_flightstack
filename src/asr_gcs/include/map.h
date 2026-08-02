#pragma once


#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include "imgui_internal.h"
#include <opencv2/opencv.hpp>

#include <cmath>
#include <unordered_map>
#include <string>
#include <list>  
#include <vector>


#include "state_manager.h"
#include "app_window.h"






// Projects a point `north_m`/`east_m` metres from `home_lat`/`home_lon` into an absolute lat/lon, via WGS84.
void LocalOffsetToLatLon(double home_lat, double home_lon, double north_m, double east_m,
                          double &out_lat, double &out_lon);
float map_value(float value, float in_min, float in_max, float out_min, float out_max);
class Location {
    public:
        GLuint display_map(const char* path, float scale);
        ImVec2 latLonToTileOffset(double lat, double lon, int zoom);
        GLuint loadTileCached(int zoom, int x, int y);
        ImVec2 MapWidget(double lat, double lon, float width, float height, float scale, int zoom = 12, GLuint placeholdetTile = 0, bool theme = 0);
        void NoSatMap(DroneInformation Info, float width, float height, float scale, int zoom, bool theme);

        // Screen position of `lat`/`lon` within a MapWidget -- `widgetPos` must be exactly what that call returned.
        ImVec2 latLonToScreenPos(double lat, double lon, double centerLat, double centerLon,
                                  ImVec2 widgetPos, float width, float height, float scale, int zoom);

    private:
        struct TileCoord { int x, y; };
        TileCoord latLonToTile(double lat, double lon, int zoom);
        std::unordered_map<std::string, GLuint> tileCache;
        std::list<std::string> tileLRU;
        std::unordered_map<std::string, std::list<std::string>::iterator> lruPos;
        static constexpr size_t MAX_CACHED_TILES = 300; 
        
    
};