#!/usr/bin/env python3
import os, sys, urllib.request, urllib.error, time, math
from ament_index_python.packages import get_package_share_directory

TILE_DIR = os.path.join(get_package_share_directory("asr_gcs"), "tiles")

SOURCE = "google"
MAPBOX_TOKEN = os.environ.get("MAPBOX_TOKEN", "")

RADIUS_M = 500
ZOOM_LEVELS = range(13, 21)


def lat_lon_to_tile(lat, lon, zoom):
    n = 2 ** zoom
    x = int((lon + 180.0) / 360.0 * n)
    lat_rad = math.radians(lat)
    y = int((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def meters_to_deg(meters, lat):
    lat_deg = meters / 111320.0
    lon_deg = meters / (111320.0 * math.cos(math.radians(lat)))
    return lat_deg, lon_deg


def tile_url(source, z, x, y):
    if source == "esri":
        return f"https://services.arcgisonline.com/arcgis/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"
    if source == "google":
        return f"https://mt0.google.com/vt/lyrs=s&x={x}&y={y}&z={z}"
    if source == "mapbox":
        if not MAPBOX_TOKEN:
            raise RuntimeError("Set MAPBOX_TOKEN env var to use the mapbox source")
        return f"https://api.mapbox.com/v4/mapbox.satellite/{z}/{x}/{y}.png?access_token={MAPBOX_TOKEN}"
    raise ValueError(f"Unknown source: {source}")


def is_valid_image(data, min_bytes=800):
    if len(data) < min_bytes:
        return False
    is_png = data[:8] == b"\x89PNG\r\n\x1a\n"
    is_jpeg = data[:3] == b"\xff\xd8\xff"
    return is_png or is_jpeg


def build_tile_list(lat, lon, radius_m, zoom_levels):
    lat_deg, lon_deg = meters_to_deg(radius_m, lat)
    lat_min, lat_max = lat - lat_deg, lat + lat_deg
    lon_min, lon_max = lon - lon_deg, lon + lon_deg

    tiles = []
    for z in zoom_levels:
        x_min, y_max = lat_lon_to_tile(lat_min, lon_min, z)
        x_max, y_min = lat_lon_to_tile(lat_max, lon_max, z)
        for x in range(x_min, x_max + 1):
            for y in range(y_min, y_max + 1):
                tiles.append((z, x, y))
    return tiles


def main():
    if len(sys.argv) < 3:
        print("Usage: download_tiles.py <lat> <lon> [radius_m]")
        sys.exit(1)

    lat = float(sys.argv[1])
    lon = float(sys.argv[2])
    radius_m = float(sys.argv[3]) if len(sys.argv) > 3 else RADIUS_M

    tiles = build_tile_list(lat, lon, radius_m, ZOOM_LEVELS)

    missing = [t for t in tiles if not os.path.exists(f"{TILE_DIR}/{t[0]}/{t[1]}/{t[2]}.png")]

    if not missing:
        print(f"CACHE_COMPLETE: all {len(tiles)} tiles already present for ({lat}, {lon})")
        return

    print(f"DOWNLOAD_START: {len(missing)}/{len(tiles)} tiles missing for ({lat}, {lon})")

    total_downloaded = 0
    total_blank = 0
    total_failed = 0

    for (z, x, y) in missing:
        path = f"{TILE_DIR}/{z}/{x}/{y}.png"
        os.makedirs(f"{TILE_DIR}/{z}/{x}", exist_ok=True)
        url = tile_url(SOURCE, z, x, y)

        try:
            req = urllib.request.Request(url, headers={"User-Agent": "ASR-GCS/1.0"})
            with urllib.request.urlopen(req, timeout=10) as response:
                data = response.read()

            if not is_valid_image(data):
                total_blank += 1
                continue

            with open(path, "wb") as f:
                f.write(data)
            total_downloaded += 1
            time.sleep(0.15)

        except urllib.error.HTTPError as e:
            total_failed += 1
            print(f"  HTTP {e.code} for {path}: {e.reason}")
        except urllib.error.URLError as e:
            total_failed += 1
            print(f"  Network error for {path}: {e.reason}")
        except Exception as e:
            total_failed += 1
            print(f"  Failed {path}: {e}")

    print(f"DOWNLOAD_COMPLETE: downloaded={total_downloaded} blank={total_blank} failed={total_failed}")


if __name__ == "__main__":
    main()