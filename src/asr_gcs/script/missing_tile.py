import os, urllib.request, math

TILE_DIR = "/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/tiles"

def lat_lon_to_tile(lat, lon, zoom):
    n = 2 ** zoom
    x = int((lon + 180.0) / 360.0 * n)
    lat_rad = math.radians(lat)
    y = int((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * n)
    return x, y

# same bbox/zoom range you're currently using
lat_min, lat_max, lon_min, lon_max = 57.059175, 57.068159, 10.023710, 10.040234
zoom_levels = range(13, 21)

def is_missing_or_bad(path, min_bytes=500):
    return (not os.path.exists(path)) or os.path.getsize(path) < min_bytes

fetched, still_bad = 0, []

for z in zoom_levels:
    x_min, y_max = lat_lon_to_tile(lat_min, lon_min, z)
    x_max, y_min = lat_lon_to_tile(lat_max, lon_max, z)

    for x in range(x_min, x_max + 1):
        for y in range(y_min, y_max + 1):
            path = f"{TILE_DIR}/{z}/{x}/{y}.png"
            if not is_missing_or_bad(path):
                continue

            os.makedirs(f"{TILE_DIR}/{z}/{x}", exist_ok=True)
            # Esri version — swap for your Google source if that's what you're using
            url = f"https://services.arcgisonline.com/arcgis/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"

            try:
                req = urllib.request.Request(url, headers={"User-Agent": "ASR-GCS/1.0"})
                with urllib.request.urlopen(req) as response:
                    data = response.read()
                with open(path, "wb") as f:
                    f.write(data)
                fetched += 1
                print(f"Fetched: {path}")
            except Exception as e:
                still_bad.append(path)
                print(f"Failed: {path} ({e})")

print(f"\nDone. Fetched {fetched} tiles, {len(still_bad)} still failing.")