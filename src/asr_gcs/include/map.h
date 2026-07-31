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





// Projects a point `north_m`/`east_m` metres from `home_lat`/`home_lon`
// (degrees) into an absolute lat/lon (degrees), via the WGS84 ellipsoid --
// home is converted to ECEF, the local offset is rotated into ECEF using
// home's ENU basis, then the result is converted back to geodetic lat/lon.
// Plan tasks store goto/spin positions as local NED offsets from home
// (x = north, y = east, in metres); this is what turns one of those into
// something the map's lat/lon tile math can place.
void LocalOffsetToLatLon(double home_lat, double home_lon, double north_m, double east_m,
                          double &out_lat, double &out_lon);

class Location {
    public:
        GLuint display_map(const char* path, float scale);
        ImVec2 latLonToTileOffset(double lat, double lon, int zoom);
        GLuint loadTileCached(int zoom, int x, int y);
        ImVec2 MapWidget(double lat, double lon, float width, float height, float scale, int zoom = 12, GLuint placeholdetTile = 0, bool theme = 0);

        // Screen position of `lat`/`lon` within a MapWidget that's centered
        // on `centerLat`/`centerLon` at `zoom` -- `widgetPos` must be
        // exactly what that MapWidget call returned, so the two line up.
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