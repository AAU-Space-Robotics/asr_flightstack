import os, urllib.request, time, math

TILE_DIR = "/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/tiles"

regions = [
    (54.5, 57.8, 8.0, 15.2, range(14, 14)),           # All Denmark zoom 4-13
    (56.868, 57.228, 9.621, 10.221, range(19, 19)),   # Aalborg 20km zoom 14-18
    (56.98, 57.088, 9.831, 10.011, range(19, 20)),    # Aalborg 6km zoom 19
]

def lat_lon_to_tile(lat, lon, zoom):
    n = 2 ** zoom
    x = int((lon + 180.0) / 360.0 * n)
    lat_rad = math.radians(lat)
    y = int((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * n)
    return x, y

total_downloaded = 0
total_skipped = 0

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
                url = f"https://tile.openstreetmap.org/{z}/{x}/{y}.png"

                try:
                    req = urllib.request.Request(url, headers={"User-Agent": "ASR-GCS/1.0"})
                    with urllib.request.urlopen(req) as response:
                        with open(path, "wb") as f:
                            f.write(response.read())
                    total_downloaded += 1
                    time.sleep(0.15)
                except Exception as e:
                    print(f"  Failed {path}: {e}")

        print(f"  Zoom {z} done")

print(f"\nFinished. Downloaded: {total_downloaded}, Skipped (already existed): {total_skipped}")