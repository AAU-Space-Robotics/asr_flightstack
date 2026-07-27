#!/bin/bash

# Installs desktop launchers for the ASR Ground Control Station:
#
#   ASR GCS            - app only: no terminal window; everything shuts down
#                        when the GUI window is closed. Output goes to
#                        ~/.local/state/asr_gcs/gcs.log.
#   ASR GCS (Terminal) - same, but with the launch output in a terminal.
#
# Both run setup/run_gcs.sh, which sources ROS + the workspace and activates
# the venv. Launchers are placed on the desktop and in the app grid.

SCRIPT_DIR=$(dirname "$(realpath "$0")")
WORKSPACE_DIR=$(dirname "$SCRIPT_DIR")
RUN_SCRIPT="$SCRIPT_DIR/run_gcs.sh"
ICON="$WORKSPACE_DIR/src/asr_gcs/images/LOGO.png"
APP_DIR="$HOME/.local/share/applications"
DESKTOP_DIR=$(xdg-user-dir DESKTOP 2>/dev/null || echo "$HOME/Desktop")

if [ ! -f "$RUN_SCRIPT" ]; then
    echo "Error: $RUN_SCRIPT not found."
    exit 1
fi
chmod +x "$RUN_SCRIPT"

[ -f "$ICON" ] || echo "Warning: icon not found at $ICON - launchers will have no icon."
[ -f "$WORKSPACE_DIR/install/setup.bash" ] || echo "Warning: workspace not built yet - build it before using the launchers."

TERMINAL_EXEC=""
for term in gnome-terminal konsole xfce4-terminal x-terminal-emulator; do
    if command -v "$term" >/dev/null 2>&1; then
        TERMINAL_EXEC="$term"
        break
    fi
done

if [ "$TERMINAL_EXEC" = "gnome-terminal" ]; then
    TERM_EXEC_LINE="gnome-terminal -- $RUN_SCRIPT"
    TERM_FLAG=false
elif [ -n "$TERMINAL_EXEC" ]; then
    TERM_EXEC_LINE="$TERMINAL_EXEC -e $RUN_SCRIPT"
    TERM_FLAG=false
else
    TERM_EXEC_LINE="$RUN_SCRIPT"
    TERM_FLAG=true
fi

mkdir -p "$APP_DIR" "$DESKTOP_DIR"

cat > "$APP_DIR/asr-gcs.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=ASR GCS
Comment=AAU Space Robotics Ground Control Station
Exec=$RUN_SCRIPT
Icon=$ICON
Terminal=false
Categories=Utility;
EOF

cat > "$APP_DIR/asr-gcs-terminal.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=ASR GCS (Terminal)
Comment=AAU Space Robotics Ground Control Station - with terminal output
Exec=$TERM_EXEC_LINE
Icon=$ICON
Terminal=$TERM_FLAG
Categories=Utility;
EOF

for f in asr-gcs.desktop asr-gcs-terminal.desktop; do
    cp "$APP_DIR/$f" "$DESKTOP_DIR/$f"
    chmod +x "$DESKTOP_DIR/$f"
    if ! env -u GIO_USE_VFS -u GIO_MODULE_DIR gio set "$DESKTOP_DIR/$f" metadata::trusted true 2>/dev/null; then
        echo "Note: could not mark $f as trusted - right-click the icon and choose 'Allow Launching'."
    fi
done

command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APP_DIR" 2>/dev/null

echo "Installed launchers:"
echo "  ASR GCS            (app only, logs to ~/.local/state/asr_gcs/gcs.log)"
echo "  ASR GCS (Terminal) (output in a terminal window)"
echo "They are on the desktop ($DESKTOP_DIR) and in the application grid."
echo ""
echo "If a desktop icon shows a red cross / raw filename, right-click it and"
echo "choose 'Allow Launching' (one-time GNOME safety check)."
