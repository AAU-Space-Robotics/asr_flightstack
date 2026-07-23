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


#include "statemanager.h"





class Location {
    public:
        GLuint display_map(const char* path, float scale);
        ImVec2 latLonToTileOffset(double lat, double lon, int zoom);
        GLuint loadTileCached(int zoom, int x, int y);
        void MapWidget(double lat, double lon, float width, float height, float scale, int zoom = 12, GLuint placeholdetTile = 0, bool theme = 0);

    private:
        struct TileCoord { int x, y; };
        TileCoord latLonToTile(double lat, double lon, int zoom);
        std::unordered_map<std::string, GLuint> tileCache;
        std::list<std::string> tileLRU;
        std::unordered_map<std::string, std::list<std::string>::iterator> lruPos;
        static constexpr size_t MAX_CACHED_TILES = 300; 
    
};