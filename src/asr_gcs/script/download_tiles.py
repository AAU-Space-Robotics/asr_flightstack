import os, urllib.request, urllib.error, time, math

TILE_DIR = "/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/tiles"

# --- pick your tile source here ---
# "esri"    - free, no key, but real-world max zoom is often 19 outside the US
# "google"  - unofficial endpoint, no key, decent zoom-20 coverage in Europe
# "mapbox"  - official API, needs MAPBOX_TOKEN env var, reliable up to zoom 22
SOURCE = "google"

MAPBOX_TOKEN = os.environ.get("MAPBOX_TOKEN", "")

regions = [
    (57.059175, 57.068159, 10.023710, 10.040234, range(13, 21)),  # 500m radius, zoom 13-20
]


def lat_lon_to_tile(lat, lon, zoom):
    n = 2 ** zoom
    x = int((lon + 180.0) / 360.0 * n)
    lat_rad = math.radians(lat)
    y = int((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def tile_url(source, z, x, y):
    if source == "esri":
        # Esri URL order is z/y/x
        return f"https://services.arcgisonline.com/arcgis/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"
    if source == "google":
        return f"https://mt0.google.com/vt/lyrs=s&x={x}&y={y}&z={z}"
    if source == "mapbox":
        if not MAPBOX_TOKEN:
            raise RuntimeError("Set MAPBOX_TOKEN env var to use the mapbox source")
        return f"https://api.mapbox.com/v4/mapbox.satellite/{z}/{x}/{y}.png?access_token={MAPBOX_TOKEN}"
    raise ValueError(f"Unknown source: {source}")


def is_valid_image(data, min_bytes=800):
    # Google's mt0 endpoint actually serves JPEG bytes despite the "png" in the URL/path.
    # Esri and Mapbox (with .png requested) serve real PNGs. Check both signatures.
    if len(data) < min_bytes:
        return False
    is_png = data[:8] == b"\x89PNG\r\n\x1a\n"
    is_jpeg = data[:3] == b"\xff\xd8\xff"
    return is_png or is_jpeg


total_downloaded = 0
total_skipped = 0
total_blank = 0
total_failed = 0

for (lat_min, lat_max, lon_min, lon_max, zoom_levels) in regions:
    for z in zoom_levels:
        x_min, y_max = lat_lon_to_tile(lat_min, lon_min, z)
        x_max, y_min = lat_lon_to_tile(lat_max, lon_max, z)
        tile_count = (x_max - x_min + 1) * (y_max - y_min + 1)
        print(f"Zoom {z}: {tile_count} tiles ({x_max-x_min+1} x {y_max-y_min+1})")

        for x in range(x_min, x_max + 1):
            for y in range(y_min, y_max + 1):
                path = f"{TILE_DIR}/{z}/{x}/{y}.png"

                if os.path.exists(path):
                    total_skipped += 1
                    continue

                os.makedirs(f"{TILE_DIR}/{z}/{x}", exist_ok=True)
                url = tile_url(SOURCE, z, x, y)

                try:
                    req = urllib.request.Request(url, headers={"User-Agent": "ASR-GCS/1.0"})
                    with urllib.request.urlopen(req, timeout=10) as response:
                        data = response.read()

                    if not is_valid_image(data):
                        total_blank += 1
                        print(f"  Blank/invalid: {path}")
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

        print(f"  Zoom {z} done")

print(f"\nFinished. Downloaded: {total_downloaded}, Skipped: {total_skipped}, "
      f"Blank/invalid: {total_blank}, Failed: {total_failed}")