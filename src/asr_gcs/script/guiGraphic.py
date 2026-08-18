#!/usr/bin/env python3
"""
ASR GCS (AAU Space Robotics) — UI design mockup, iteration 4 (NON-FUNCTIONAL).

A pure design study: every value on screen is fake. Current layout:

* Top bar — logo badge (placeholder for the real logo asset), AUTO/MANUAL
  switch, yellow ARM button (same yellow in both themes), T+, and the
  link-status strip UAV / RADIO / WIFI / RTK / CONTROLLER. No bottom bar.
* Map — GPS/OSM-style base layer (roads + buildings, ref 57.034000 N
  9.882000 E), big red E-STOP top-left, action strip on the left edge,
  mission bar (execute / pause / restart / load) bottom-center, and
  attitude horizon + compass + altitude tape bottom-right. No info chips.
* Right column — Power & Motors (battery glyph, electrical rows, vertical
  motor bars), State · NED, Probe Info, GPS · RTK (incl. lat/lon).
* Bottom-left — Command box (pose + gimbal) next to the console.
* Nav rail — Fly / Plan / Checklist / Camera on top; day/night toggle
  directly above Settings at the bottom. Camera tab shows a mock feed
  with FPV crosshair and OSD chips.
* Brand — navy / burnt-orange / silver palette sampled from the AAU Space
  Robotics logo (``BRAND_*`` and ``PALETTES`` below); day theme included.

The interactive bits that work: nav rail pages, day/night toggle,
AUTO/MANUAL switch. Everything else is static.

Run:  python3 gui_example.py
"""

import sys

from PyQt5.QtCore import Qt, QPointF, QRectF
from PyQt5.QtGui import (
    QColor,
    QFont,
    QLinearGradient,
    QPainter,
    QPainterPath,
    QPen,
    QPolygonF,
)
from PyQt5.QtWidgets import (
    QApplication,
    QButtonGroup,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPushButton,
    QSizePolicy,
    QSlider,
    QStackedWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

# --------------------------------------------------------------------------
# Brand colors, sampled from the AAU Space Robotics logo.
# --------------------------------------------------------------------------
BRAND_NAVY = "#2a2860"
BRAND_ORANGE = "#c96a26"
BRAND_SILVER = "#d9dade"

# --------------------------------------------------------------------------
# Palettes — "dark" for night ops, "day" is high-contrast for sunlight.
# Both are built around the brand: navy-tinted surfaces, orange accent.
# Semantic amber is shifted to gold so warnings never read as "selected".
# C holds the active palette; the whole UI is rebuilt when it changes.
# --------------------------------------------------------------------------
PALETTES = {
    "dark": {
        "bg":           "#0d0d20",
        "surface":      "#15152f",
        "surface2":     "#1b1b3c",
        "inset":        "#0a0a1c",
        "border":       "#2b2b52",
        "border_soft":  "#212140",
        "text":         "#e2e3ea",
        "dim":          "#9d9fb8",
        "faint":        "#666884",
        "accent":       "#e07b2e",
        "accent_soft":  "rgba(224, 123, 46, 0.16)",
        "accent_hover": "#ec934e",
        "on_accent":    "#160b02",
        "hover":        "#222246",
        "green":        "#3fd68f",
        "green_soft":   "rgba(63, 214, 143, 0.14)",
        "amber":        "#f5c84c",
        "amber_soft":   "rgba(245, 200, 76, 0.15)",
        "red":          "#ff5c5c",
        "red_soft":     "rgba(255, 92, 92, 0.14)",
        "axis_x":       "#ff8f8f",
        "axis_y":       "#7ee2a8",
        "axis_z":       "#7cc4ff",
        "map_bg":       "#101024",
        "map_road":     "#20203f",
        "map_block":    "#171732",
        "glass":        "rgba(10, 10, 28, 0.88)",
        "bezel":        "#0a0a1c",
    },
    "day": {
        "bg":           "#d0d1d8",
        "surface":      "#eeeff3",
        "surface2":     "#dfe0e8",
        "inset":        "#ffffff",
        "border":       "#8f92a6",
        "border_soft":  "#b3b5c4",
        "text":         "#16163a",
        "dim":          "#3d3f5e",
        "faint":        "#5e6078",
        "accent":       "#b25a14",
        "accent_soft":  "rgba(178, 90, 20, 0.16)",
        "accent_hover": "#9d4f11",
        "on_accent":    "#ffffff",
        "hover":        "#cfd0da",
        "green":        "#0e7d3e",
        "green_soft":   "rgba(14, 125, 62, 0.16)",
        "amber":        "#8a6a00",
        "amber_soft":   "rgba(138, 106, 0, 0.18)",
        "red":          "#c22828",
        "red_soft":     "rgba(194, 40, 40, 0.16)",
        "axis_x":       "#c22828",
        "axis_y":       "#0e7d3e",
        "axis_z":       "#0a66c2",
        "map_bg":       "#eae6db",
        "map_road":     "#ffffff",
        "map_block":    "#d9d3c5",
        "glass":        "rgba(255, 255, 255, 0.92)",
        "bezel":        "#f4f5f8",
    },
}
C = dict(PALETTES["dark"])

SANS = '"Inter", "SF Pro Display", "Segoe UI", "Ubuntu", "DejaVu Sans"'
MONO = '"JetBrains Mono", "Cascadia Code", "Ubuntu Mono", "DejaVu Sans Mono"'


def build_qss() -> str:
    qss = """
    QMainWindow, QWidget { background: transparent; color: $text;
        font-family: $SANS; font-size: 13px; }
    QMainWindow { background: $bg; }

    /* ---- chrome bars ------------------------------------------------ */
    QFrame#TopBar   { background: $surface; border-bottom: 1px solid $border; }
    QFrame#NavRail  { background: $surface; border-right: 1px solid $border; }

    /* ---- cards ------------------------------------------------------ */
    QFrame[card="true"] { background: $surface; border: 1px solid $border;
        border-radius: 12px; }
    QLabel[cls="cardTitle"] { color: $faint; font-size: 10px;
        font-weight: 700; }

    /* ---- text roles -------------------------------------------------- */
    QLabel[cls="logo"]  { font-size: 15px; font-weight: 800; }
    QLabel[cls="dim"]   { color: $dim; }
    QLabel[cls="faint"] { color: $faint; font-size: 11px; }
    QLabel[cls="tiny"]  { color: $faint; font-size: 10px; font-weight: 600; }
    QLabel[cls="status"]{ color: $dim; font-size: 11px; font-weight: 700; }
    QLabel[cls="mono"]  { font-family: $MONO; font-size: 15px; font-weight: 600; }
    QLabel[cls="monodim"] { font-family: $MONO; font-size: 13px; color: $dim; }
    QLabel[cls="monosm"]  { font-family: $MONO; font-size: 12px; color: $dim; }
    QLabel[cls="time"]  { font-family: $MONO; font-size: 16px; font-weight: 700; }

    /* ---- pills -------------------------------------------------------- */
    QLabel[cls="pill"] { border-radius: 13px; padding: 0px 12px;
        font-size: 12px; font-weight: 600; min-height: 26px; max-height: 26px; }
    QLabel[cls="pill"][tone="green"] { background: $green_soft; color: $green; }
    QLabel[cls="pill"][tone="amber"] { background: $amber_soft; color: $amber; }
    QLabel[cls="pill"][tone="red"]   { background: $red_soft;   color: $red; }
    QLabel[cls="pill"][tone="dim"]   { background: $surface2;   color: $dim; }
    QLabel[cls="pill"][tone="accent"]{ background: $accent_soft; color: $accent; }

    /* ---- buttons ------------------------------------------------------ */
    QPushButton { border: none; border-radius: 8px; padding: 6px 14px;
        color: $text; background: $surface2; font-weight: 600; }
    QPushButton:hover { background: $hover; }

    QPushButton[cls="primary"] { background: $accent; color: $on_accent;
        font-weight: 700; padding: 8px 16px; }
    QPushButton[cls="primary"]:hover { background: $accent_hover; }

    QPushButton[cls="ghost"] { background: transparent;
        border: 1px solid $border; color: $dim; }
    QPushButton[cls="ghost"]:hover { border-color: $accent; color: $accent; }

    /* round emergency stop, overlaid top-left on the map */
    QPushButton[cls="estop"] { background: $red; color: #ffffff;
        font-weight: 800; font-size: 12px; border-radius: 31px;
        border: 4px solid rgba(255, 255, 255, 0.35);
        min-width: 62px; max-width: 62px; min-height: 62px; max-height: 62px;
        padding: 0px; }
    QPushButton[cls="estop"]:hover { background: #ff7a7a; }

    /* ARM stays the same yellow in every theme — deliberate */
    QPushButton[cls="arm"] { background: #ffd21e; color: #231a02;
        font-weight: 800; font-size: 14px; border-radius: 9px;
        padding: 9px 20px; }
    QPushButton[cls="arm"]:hover { background: #ffe45f; }

    /* segmented AUTO / MANUAL switch */
    QFrame[cls="segbox"] { background: $inset; border: 1px solid $border;
        border-radius: 9px; }
    QPushButton[cls="seg"] { background: transparent; color: $dim;
        border-radius: 6px; padding: 0px; font-weight: 700;
        font-size: 12px; }
    QPushButton[cls="seg"]:checked { background: $accent; color: $on_accent; }

    QPushButton[cls="chip"] { background: transparent; color: $dim;
        border: 1px solid $border; border-radius: 12px; padding: 3px 10px;
        font-size: 11px; font-weight: 600; }
    QPushButton[cls="chip"]:hover { color: $text; }
    QPushButton[cls="chip"]:checked { background: $accent_soft;
        border-color: $accent; color: $accent; }

    QPushButton[cls="mapbtn"] { background: transparent; color: $text;
        font-size: 16px; border-radius: 10px; padding: 0px;
        min-width: 42px; max-width: 42px; min-height: 42px; max-height: 42px; }
    QPushButton[cls="mapbtn"]:hover { background: $accent_soft; color: $accent; }
    QPushButton[cls="zoom"] { background: transparent; color: $dim;
        font-size: 15px; border-radius: 8px; padding: 0px;
        min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; }
    QPushButton[cls="zoom"]:hover { color: $text; background: $surface2; }

    QPushButton[cls="send"] { background: $accent_soft; color: $accent;
        border-radius: 8px; font-size: 14px; font-weight: 700;
        min-width: 36px; max-width: 36px; min-height: 32px; max-height: 32px;
        padding: 0px; }
    QPushButton[cls="send"]:hover { background: $accent; color: $on_accent; }

    /* flat text button inside glass overlays (e.g. plan status) */
    QPushButton[cls="flat"] { background: transparent; color: $dim;
        font-size: 12px; font-weight: 700; padding: 4px 10px;
        border-radius: 8px; }
    QPushButton[cls="flat"]:hover { color: $accent; background: $accent_soft; }

    /* ---- nav rail ------------------------------------------------------ */
    QFrame#NavRail QPushButton { background: transparent; color: $faint;
        font-size: 16px; border-radius: 10px; padding: 0px;
        min-width: 40px; max-width: 40px; min-height: 40px; max-height: 40px; }
    QFrame#NavRail QPushButton:hover { color: $text; background: $surface2; }
    QFrame#NavRail QPushButton:checked { color: $accent; background: $accent_soft; }

    /* ---- inputs -------------------------------------------------------- */
    QFrame[cls="field"] { background: $inset; border: 1px solid $border;
        border-radius: 8px; }
    QLineEdit { background: transparent; border: none; padding: 6px 8px;
        color: $text; font-family: $MONO; font-size: 13px;
        selection-background-color: $accent; selection-color: $on_accent; }

    /* ---- console -------------------------------------------------------- */
    QTextEdit#Console { background: $inset; border: 1px solid $border_soft;
        border-radius: 8px; font-family: $MONO; font-size: 12px;
        padding: 4px 6px; }

    /* ---- slider ----------------------------------------------------------- */
    QSlider::groove:horizontal { height: 6px; background: $inset;
        border: 1px solid $border_soft; border-radius: 3px; }
    QSlider::sub-page:horizontal { background: $accent; border-radius: 3px; }
    QSlider::handle:horizontal { width: 16px; height: 16px; margin: -6px 0;
        border-radius: 8px; background: $text; }
    QSlider::handle:horizontal:hover { background: $accent; }

    /* ---- flight-plan rows --------------------------------------------------- */
    QFrame[cls="wprow"] { background: transparent; border: 1px solid transparent;
        border-radius: 8px; }
    QFrame[cls="wprow"]:hover { background: $surface2; }
    QFrame[cls="wprow"][active="true"] { background: $accent_soft;
        border: 1px solid $accent; }
    QLabel[cls="wpbadge"] { background: $surface2; border: 1px solid $border;
        border-radius: 11px; min-width: 22px; max-width: 22px;
        min-height: 22px; max-height: 22px; color: $dim;
        font-size: 11px; font-weight: 700; }
    QLabel[cls="wpbadge"][state="done"]   { color: $green; border-color: $green; }
    QLabel[cls="wpbadge"][state="active"] { background: $accent;
        color: $on_accent; border-color: $accent; }
    QLabel[cls="wpchip"] { background: $surface2; color: $dim;
        border-radius: 9px; padding: 1px 8px; font-family: $MONO;
        font-size: 11px; }

    /* ---- glass overlays on the map -------------------------------------------- */
    QFrame[cls="glass"] { background: $glass;
        border: 1px solid $border; border-radius: 12px; }

    /* ---- scrollbars --------------------------------------------------- */
    QScrollBar:vertical { background: transparent; width: 8px; margin: 2px; }
    QScrollBar::handle:vertical { background: $border; border-radius: 4px;
        min-height: 24px; }
    QScrollBar::handle:vertical:hover { background: $faint; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0px; }
    """
    qss = qss.replace("$SANS", SANS).replace("$MONO", MONO)
    # longest keys first so e.g. $accent never eats into $accent_soft
    for key in sorted(C, key=len, reverse=True):
        qss = qss.replace("$" + key, C[key])
    return qss


# --------------------------------------------------------------------------
# Small construction helpers
# --------------------------------------------------------------------------
def label(text, cls=None, tone=None, align=None, rich=False):
    lab = QLabel(text)
    if rich:
        lab.setTextFormat(Qt.RichText)
    if cls:
        lab.setProperty("cls", cls)
    if tone:
        lab.setProperty("tone", tone)
    if align is not None:
        lab.setAlignment(align)
    return lab


def button(text, cls=None, checkable=False, checked=False):
    btn = QPushButton(text)
    btn.setCursor(Qt.PointingHandCursor)
    if cls:
        btn.setProperty("cls", cls)
    if checkable:
        btn.setCheckable(True)
        btn.setChecked(checked)
    return btn


def status_dot(name, tone):
    """Flat link-status item for the top bar: colored dot + label."""
    return label(f"<span style='color:{C[tone]}'>●</span>&nbsp;{name}",
                 cls="status", rich=True)


def vsep(height=24):
    sep = QFrame()
    sep.setFixedSize(1, height)
    sep.setStyleSheet(f"background: {C['border']};")
    return sep


def hsep():
    sep = QFrame()
    sep.setFixedHeight(1)
    sep.setStyleSheet(f"background: {C['border_soft']};")
    return sep


def card(title, right_widget=None):
    """Rounded card with an uppercase mini-title. Returns (frame, body_layout)."""
    frame = QFrame()
    frame.setProperty("card", True)
    body = QVBoxLayout(frame)
    body.setContentsMargins(14, 10, 14, 10)
    body.setSpacing(6)
    head = QHBoxLayout()
    head.setSpacing(8)
    head.addWidget(label(title.upper(), cls="cardTitle"))
    head.addStretch(1)
    if right_widget is not None:
        head.addWidget(right_widget)
    body.addLayout(head)
    return frame, body


def field(placeholder, unit=None):
    """Input with an optional trailing unit, drawn as one rounded control."""
    frame = QFrame()
    frame.setProperty("cls", "field")
    lay = QHBoxLayout(frame)
    lay.setContentsMargins(4, 0, 10, 0)
    lay.setSpacing(4)
    edit = QLineEdit()
    edit.setPlaceholderText(placeholder)
    lay.addWidget(edit, 1)
    if unit:
        lay.addWidget(label(unit, cls="faint"))
    frame.setFixedHeight(32)
    return frame


# --------------------------------------------------------------------------
# Custom-painted widgets: map, instruments, motor bars, battery
# --------------------------------------------------------------------------
class MapCanvas(QWidget):
    """Fake 2D tactical map: grid, geofence, planned path, trail, drone."""

    SCALE = 36  # px per metre

    TRAIL = [(0.0, 0.0), (0.3, 0.6), (0.8, 1.1), (1.4, 1.45),
             (2.0, 1.5), (2.55, 1.9), (3.0, 2.25)]
    WAYPOINTS = [(2.0, 1.5), (4.0, 3.0), (6.0, 1.2), (4.5, -1.8)]
    ACTIVE_WP = 1
    DRONE = (3.0, 2.25)
    HEADING = 53.0

    # fake OSM-style base layer around home (ref 57.034000 N, 9.882000 E):
    # roads as ((x1, y1), (x2, y2), width) and building footprints as
    # (center_x, center_y, width, height), all in metres
    ROADS = (((-6.2, -14.0), (-6.2, 14.0), 1.8),
             ((10.2, -14.0), (10.2, 14.0), 1.8),
             ((-18.0, 5.0), (28.0, 5.0), 1.8))
    BUILDINGS = ((-10.5, 2.2, 5.0, 3.0), (-10.0, -4.0, 4.0, 4.5),
                 (-2.5, 7.6, 5.5, 2.6), (3.5, 7.8, 5.0, 2.4),
                 (13.5, 7.8, 4.5, 2.6), (14.5, -0.5, 5.0, 5.0),
                 (13.0, -6.5, 6.0, 3.0), (4.5, -8.2, 6.5, 2.8),
                 (-3.5, -8.0, 4.0, 2.6), (18.5, 2.0, 3.5, 3.5))

    def __init__(self, on_plan=None):
        super().__init__()
        self._on_plan = on_plan
        self.setMinimumSize(480, 360)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._build_overlays()

    # ---- overlay widgets (children, repositioned on resize) ----
    def _build_overlays(self):
        # big red emergency stop, always visible top-left on the map
        self.estop = button("STOP", cls="estop")
        self.estop.setParent(self)
        self.estop.setToolTip("Emergency stop — kills motors immediately")

        # left floating action strip, QGC fly-view style
        self.strip = QFrame(self)
        self.strip.setProperty("cls", "glass")
        col = QVBoxLayout(self.strip)
        col.setContentsMargins(8, 10, 8, 10)
        col.setSpacing(2)
        for glyph, name in (("▲", "Takeoff"), ("▼", "Land"), ("⌂", "Return"),
                            ("⏸", "Hold"), ("⌖", "Origin")):
            btn = button(glyph, cls="mapbtn")
            btn.setToolTip(name)
            col.addWidget(btn, 0, Qt.AlignHCenter)
            col.addWidget(label(name, cls="tiny", align=Qt.AlignHCenter))
            if name != "Origin":
                col.addSpacing(6)

        # top-right zoom cluster
        self.zoom = QFrame(self)
        self.zoom.setProperty("cls", "glass")
        zoom_lay = QVBoxLayout(self.zoom)
        zoom_lay.setContentsMargins(5, 5, 5, 5)
        zoom_lay.setSpacing(2)
        for glyph in ("+", "−", "⛶"):
            zoom_lay.addWidget(button(glyph, cls="zoom"))

        # bottom-right instruments, altitude tape at the outer edge
        self.instruments = QWidget(self)
        inst_lay = QHBoxLayout(self.instruments)
        inst_lay.setContentsMargins(0, 0, 0, 0)
        inst_lay.setSpacing(10)
        inst_lay.addWidget(AttitudeIndicator(roll=8.0, pitch=5.0))
        inst_lay.addWidget(CompassGauge(heading=self.HEADING))
        inst_lay.addWidget(AltitudeTape(alt=12.4))

        # bottom-center mission control bar (Fly page only): plan status
        # shortcut + execute / pause / restart / load
        self.missionbar = None
        if self._on_plan is not None:
            self.missionbar = QFrame(self)
            self.missionbar.setProperty("cls", "glass")
            mbar = QHBoxLayout(self.missionbar)
            mbar.setContentsMargins(10, 6, 10, 6)
            mbar.setSpacing(6)
            plan = button("◈  #3 Goto active", cls="flat")
            plan.setToolTip("Open flight plan")
            plan.clicked.connect(self._on_plan)
            mbar.addWidget(plan)
            mbar.addWidget(vsep(20))
            execute = button("▶  Execute", cls="primary")
            execute.setFixedHeight(30)
            execute.setToolTip("Execute mission")
            mbar.addWidget(execute)
            for glyph, tip in (("⏸", "Pause mission"),
                               ("↺", "Restart mission")):
                btn = button(glyph, cls="zoom")
                btn.setToolTip(tip)
                mbar.addWidget(btn)
            mbar.addWidget(vsep(20))
            load = button("↥  Load", cls="ghost")
            load.setFixedHeight(28)
            load.setToolTip("Load mission from file")
            mbar.addWidget(load)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        w, h = self.width(), self.height()
        self.estop.move(14, 14)
        self.strip.adjustSize()
        self.strip.move(12, (h - self.strip.height()) // 2)
        self.zoom.adjustSize()
        self.zoom.move(w - self.zoom.width() - 12, 12)
        self.instruments.adjustSize()
        self.instruments.move(w - self.instruments.width() - 14,
                              h - self.instruments.height() - 14)
        if self.missionbar is not None:
            self.missionbar.adjustSize()
            self.missionbar.move((w - self.missionbar.width()) // 2,
                                 h - self.missionbar.height() - 14)

    # ---- painting ----
    def _to_px(self, x, y):
        cx = self.width() * 0.42
        cy = self.height() * 0.56
        return QPointF(cx + x * self.SCALE, cy - y * self.SCALE)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        rect = QRectF(self.rect()).adjusted(0.5, 0.5, -0.5, -0.5)
        clip = QPainterPath()
        clip.addRoundedRect(rect, 14, 14)
        p.setClipPath(clip)

        p.fillRect(self.rect(), QColor(C["map_bg"]))
        self._paint_streets(p)
        self._paint_geofence(p)
        self._paint_plan(p)
        self._paint_trail(p)
        self._paint_home(p)
        self._paint_waypoints(p)
        self._paint_drone(p)
        self._paint_scalebar(p)

        p.setClipping(False)
        p.setPen(QPen(QColor(C["border"]), 1))
        p.setBrush(Qt.NoBrush)
        p.drawRoundedRect(rect, 14, 14)

    def _paint_streets(self, p):
        """OSM-style base layer: roads and building footprints."""
        for (x1, y1), (x2, y2), width_m in self.ROADS:
            pen = QPen(QColor(C["map_road"]), width_m * self.SCALE,
                       Qt.SolidLine, Qt.FlatCap)
            p.setPen(pen)
            p.drawLine(self._to_px(x1, y1), self._to_px(x2, y2))
        edge = QColor(C["text"])
        edge.setAlpha(18)
        p.setPen(QPen(edge, 1))
        p.setBrush(QColor(C["map_block"]))
        for bx, by, bw, bh in self.BUILDINGS:
            top_left = self._to_px(bx - bw / 2, by + bh / 2)
            p.drawRect(QRectF(top_left.x(), top_left.y(),
                              bw * self.SCALE, bh * self.SCALE))

    def _paint_geofence(self, p):
        color = QColor(C["amber"])
        color.setAlpha(130)
        pen = QPen(color, 1.4)
        pen.setDashPattern([5, 6])
        p.setPen(pen)
        p.setBrush(Qt.NoBrush)
        radius = 9.0 * self.SCALE
        p.drawEllipse(self._to_px(0, 0), radius, radius)

    def _paint_plan(self, p):
        color = QColor(C["accent"])
        color.setAlpha(160)
        pen = QPen(color, 1.6)
        pen.setDashPattern([6, 5])
        p.setPen(pen)
        pts = [self._to_px(0, 0)] + [self._to_px(x, y) for x, y in self.WAYPOINTS]
        for a, b in zip(pts, pts[1:]):
            p.drawLine(a, b)

    def _paint_trail(self, p):
        color = QColor(C["accent"])
        color.setAlpha(230)
        p.setPen(QPen(color, 2.4, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
        path = QPainterPath(self._to_px(*self.TRAIL[0]))
        for x, y in self.TRAIL[1:]:
            path.lineTo(self._to_px(x, y))
        p.drawPath(path)

    def _paint_home(self, p):
        pt = self._to_px(0, 0)
        p.setBrush(QColor(C["surface2"]))
        p.setPen(QPen(QColor(C["green"]), 1.6))
        p.drawEllipse(pt, 10, 10)
        p.setPen(QColor(C["green"]))
        p.setFont(QFont("DejaVu Sans", 8, QFont.Bold))
        p.drawText(QRectF(pt.x() - 10, pt.y() - 10, 20, 20), Qt.AlignCenter, "H")

    def _paint_waypoints(self, p):
        p.setFont(QFont("DejaVu Sans", 8, QFont.Bold))
        for i, (x, y) in enumerate(self.WAYPOINTS):
            pt = self._to_px(x, y)
            active = i == self.ACTIVE_WP
            if active:
                halo = QColor(C["accent"])
                halo.setAlpha(50)
                p.setBrush(halo)
                p.setPen(Qt.NoPen)
                p.drawEllipse(pt, 17, 17)
                p.setBrush(QColor(C["accent"]))
                p.setPen(QPen(QColor(C["on_accent"]), 1))
            else:
                p.setBrush(QColor(C["surface2"]))
                p.setPen(QPen(QColor(C["accent"]), 1.4))
            p.drawEllipse(pt, 10, 10)
            p.setPen(QColor(C["on_accent"]) if active else QColor(C["dim"]))
            p.drawText(QRectF(pt.x() - 10, pt.y() - 10, 20, 20),
                       Qt.AlignCenter, str(i + 1))

    def _paint_drone(self, p):
        pt = self._to_px(*self.DRONE)
        p.save()
        p.translate(pt)
        p.rotate(self.HEADING)
        arm_pen = QPen(QColor(C["text"]), 2)
        p.setPen(arm_pen)
        for dx, dy in ((-11, -11), (11, -11), (-11, 11), (11, 11)):
            p.drawLine(QPointF(0, 0), QPointF(dx, dy))
            rotor = QColor(C["text"])
            rotor.setAlpha(60)
            p.setBrush(rotor)
            p.setPen(QPen(QColor(C["text"]), 1.4))
            p.drawEllipse(QPointF(dx, dy), 6.5, 6.5)
            p.setPen(arm_pen)
        p.setBrush(QColor(C["accent"]))
        p.setPen(QPen(QColor(C["on_accent"]), 1.4))
        p.drawEllipse(QPointF(0, 0), 6.5, 6.5)
        # nose marker
        p.setBrush(QColor(C["accent"]))
        p.setPen(Qt.NoPen)
        p.drawPolygon(QPolygonF([QPointF(0, -21), QPointF(-4.5, -13),
                                 QPointF(4.5, -13)]))
        p.restore()

    def _paint_scalebar(self, p):
        length = 2 * self.SCALE
        x0, y0 = 18.0, self.height() - 20.0
        p.setPen(QPen(QColor(C["dim"]), 1.4))
        p.drawLine(QPointF(x0, y0), QPointF(x0 + length, y0))
        p.drawLine(QPointF(x0, y0 - 4), QPointF(x0, y0 + 4))
        p.drawLine(QPointF(x0 + length, y0 - 4), QPointF(x0 + length, y0 + 4))
        p.setFont(QFont("DejaVu Sans", 8))
        p.drawText(QPointF(x0 + length + 8, y0 + 4), "2 m")
        # north hint, to the right of the e-stop
        p.setFont(QFont("DejaVu Sans", 9, QFont.Bold))
        p.drawText(QPointF(90, 34), "N ↑")


class AttitudeIndicator(QWidget):
    """Round artificial horizon, QGC style."""

    def __init__(self, roll=0.0, pitch=0.0, size=116):
        super().__init__()
        self.roll, self.pitch = roll, pitch
        self.setFixedSize(size, size)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        cx, cy = self.width() / 2, self.height() / 2
        radius = min(cx, cy) - 3

        p.translate(cx, cy)
        bezel = QColor(C["bezel"])
        bezel.setAlpha(238)
        p.setBrush(bezel)
        p.setPen(QPen(QColor(C["border"]), 1.4))
        p.drawEllipse(QPointF(0, 0), radius + 2, radius + 2)

        inner = radius - 7
        clip = QPainterPath()
        clip.addEllipse(QPointF(0, 0), inner, inner)
        p.save()
        p.setClipPath(clip)
        p.rotate(-self.roll)
        offset = self.pitch * 2.2  # px per degree of pitch
        big = inner * 3
        p.fillRect(QRectF(-big, -big, 2 * big, big + offset), QColor("#2b5876"))
        p.fillRect(QRectF(-big, offset, 2 * big, big), QColor("#5d4037"))
        p.setPen(QPen(QColor("#e8edf4"), 1.6))
        p.drawLine(QPointF(-inner, offset), QPointF(inner, offset))
        # pitch ladder
        p.setPen(QPen(QColor(255, 255, 255, 170), 1.2))
        for deg in (-20, -10, 10, 20):
            y = offset - deg * 2.2
            half = 18 if deg % 20 == 0 else 13
            p.drawLine(QPointF(-half, y), QPointF(half, y))
        p.restore()

        # roll tick marks (fixed) + pointer (rotates with roll)
        p.setPen(QPen(QColor(C["dim"]), 1.4))
        for ang in (-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60):
            p.save()
            p.rotate(ang)
            tick = 7 if ang in (-60, -30, 0, 30, 60) else 4
            p.drawLine(QPointF(0, -inner), QPointF(0, -inner + tick))
            p.restore()
        p.save()
        p.rotate(-self.roll)
        p.setBrush(QColor(C["accent"]))
        p.setPen(Qt.NoPen)
        p.drawPolygon(QPolygonF([QPointF(0, -inner + 2), QPointF(-5, -inner + 11),
                                 QPointF(5, -inner + 11)]))
        p.restore()

        # fixed aircraft symbol
        p.setPen(QPen(QColor("#ffd75e"), 2.4, Qt.SolidLine, Qt.RoundCap))
        p.drawLine(QPointF(-20, 0), QPointF(-7, 0))
        p.drawLine(QPointF(7, 0), QPointF(20, 0))
        p.drawLine(QPointF(-7, 0), QPointF(0, 5))
        p.drawLine(QPointF(0, 5), QPointF(7, 0))


class CompassGauge(QWidget):
    """Rotating compass rose with fixed lubber line and heading readout."""

    def __init__(self, heading=0.0, size=116):
        super().__init__()
        self.heading = heading
        self.setFixedSize(size, size)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        cx, cy = self.width() / 2, self.height() / 2
        radius = min(cx, cy) - 3
        p.translate(cx, cy)

        bezel = QColor(C["bezel"])
        bezel.setAlpha(238)
        p.setBrush(bezel)
        p.setPen(QPen(QColor(C["border"]), 1.4))
        p.drawEllipse(QPointF(0, 0), radius + 2, radius + 2)

        inner = radius - 7
        # rotating rose
        for ang in range(0, 360, 15):
            p.save()
            p.rotate(ang - self.heading)
            is_cardinal = ang % 90 == 0
            length = 8 if is_cardinal else (6 if ang % 45 == 0 else 3)
            p.setPen(QPen(QColor(C["text"] if is_cardinal else C["faint"]),
                          1.6 if is_cardinal else 1.1))
            p.drawLine(QPointF(0, -inner), QPointF(0, -inner + length))
            if is_cardinal:
                letter = {0: "N", 90: "E", 180: "S", 270: "W"}[ang]
                p.setPen(QColor(C["accent"] if letter == "N" else C["dim"]))
                p.setFont(QFont("DejaVu Sans", 9, QFont.Bold))
                p.drawText(QRectF(-9, -inner + 9, 18, 14), Qt.AlignCenter, letter)
            p.restore()

        # fixed lubber line
        p.setBrush(QColor(C["accent"]))
        p.setPen(Qt.NoPen)
        p.drawPolygon(QPolygonF([QPointF(0, -inner - 1), QPointF(-5, -inner + 9),
                                 QPointF(5, -inner + 9)]))

        # aircraft glyph + heading readout on a small backdrop
        p.setBrush(QColor(C["text"]))
        p.drawPolygon(QPolygonF([QPointF(0, -10), QPointF(-7, 8),
                                 QPointF(0, 3), QPointF(7, 8)]))
        backdrop = QColor(C["bezel"])
        backdrop.setAlpha(240)
        p.setBrush(backdrop)
        p.setPen(Qt.NoPen)
        p.drawRoundedRect(QRectF(-21, 15, 42, 17), 5, 5)
        p.setPen(QColor(C["dim"]))
        p.setFont(QFont("DejaVu Sans Mono", 9, QFont.Bold))
        p.drawText(QRectF(-21, 15, 42, 17), Qt.AlignCenter,
                   f"{int(self.heading):03d}°")


class LogoBadge(QWidget):
    """Simplified AAU Space Robotics mark (crescent, star, rover) in the
    exact brand colors — swap for a QLabel with the real pixmap asset.
    Deliberately not theme-aware: a logo keeps its colors."""

    def __init__(self, size=38):
        super().__init__()
        self.setFixedSize(size, size)
        self.setToolTip("AAU Space Robotics")

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        center = QPointF(self.width() / 2, self.height() / 2)
        radius = self.width() / 2 - 2
        p.setBrush(QColor(BRAND_NAVY))
        p.setPen(QPen(QColor(C["border"]), 1.2))
        p.drawEllipse(center, radius, radius)

        clip = QPainterPath()
        clip.addEllipse(center, radius - 1, radius - 1)
        p.setClipPath(clip)
        # planet arc: orange disc cut by an offset navy disc
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(BRAND_ORANGE))
        p.drawEllipse(QPointF(center.x() + 3, center.y() - 4),
                      radius - 4, radius - 4)
        p.setBrush(QColor(BRAND_NAVY))
        p.drawEllipse(QPointF(center.x() + 0.5, center.y() - 1.5),
                      radius - 6, radius - 6)
        # four-point star, top-right
        sx, sy = center.x() + 8.5, center.y() - 10.5
        star = QPolygonF([
            QPointF(sx, sy - 4), QPointF(sx + 1.2, sy - 1.2),
            QPointF(sx + 4, sy), QPointF(sx + 1.2, sy + 1.2),
            QPointF(sx, sy + 4), QPointF(sx - 1.2, sy + 1.2),
            QPointF(sx - 4, sy), QPointF(sx - 1.2, sy - 1.2),
        ])
        p.setBrush(QColor(BRAND_SILVER))
        p.drawPolygon(star)
        # rover silhouette: body, two wheels, mast with arm
        p.setBrush(QColor(BRAND_SILVER))
        p.drawRoundedRect(QRectF(center.x() - 8, center.y() + 1, 12, 6), 1.5, 1.5)
        p.drawEllipse(QPointF(center.x() - 5.5, center.y() + 8.5), 2.2, 2.2)
        p.drawEllipse(QPointF(center.x() + 1.5, center.y() + 8.5), 2.2, 2.2)
        arm = QPen(QColor(BRAND_SILVER), 1.4, Qt.SolidLine, Qt.RoundCap)
        p.setPen(arm)
        p.drawLine(QPointF(center.x() - 6, center.y() + 1),
                   QPointF(center.x() - 6, center.y() - 4))
        p.drawLine(QPointF(center.x() - 6, center.y() - 4),
                   QPointF(center.x() - 1, center.y() - 6.5))
        p.setPen(Qt.NoPen)
        p.drawEllipse(QPointF(center.x() - 1, center.y() - 6.5), 1.5, 1.5)


class AltitudeTape(QWidget):
    """Vertical altitude gauge for the map instrument cluster (0–20 m)."""

    def __init__(self, alt=12.4, top=20.0, size=116):
        super().__init__()
        self.alt, self.top = alt, top
        self.setFixedSize(46, size)
        self.setToolTip(f"Altitude {alt:.1f} m")

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        h = self.height()
        rect = QRectF(self.rect()).adjusted(0.5, 0.5, -0.5, -0.5)
        bezel = QColor(C["bezel"])
        bezel.setAlpha(238)
        p.setBrush(bezel)
        p.setPen(QPen(QColor(C["border"]), 1.4))
        p.drawRoundedRect(rect, 10, 10)

        margin = 10.0
        span = h - 2 * margin

        def y_at(alt):
            return h - margin - alt / self.top * span

        p.setFont(QFont("DejaVu Sans", 7))
        for alt in range(0, int(self.top) + 1):
            major = alt % 5 == 0
            p.setPen(QPen(QColor(C["dim"] if major else C["faint"]),
                          1.2 if major else 1.0))
            p.drawLine(QPointF(6, y_at(alt)),
                       QPointF(13 if major else 10, y_at(alt)))
            if major:
                p.setPen(QColor(C["faint"]))
                p.drawText(QRectF(16, y_at(alt) - 6, 26, 12),
                           Qt.AlignLeft | Qt.AlignVCenter, str(alt))

        # current-altitude pointer chip
        y0 = y_at(self.alt)
        p.setBrush(QColor(C["accent"]))
        p.setPen(Qt.NoPen)
        p.drawRoundedRect(QRectF(6, y0 - 9, 34, 18), 5, 5)
        p.setPen(QColor(C["on_accent"]))
        p.setFont(QFont("DejaVu Sans Mono", 8, QFont.Bold))
        p.drawText(QRectF(6, y0 - 9, 34, 18), Qt.AlignCenter,
                   f"{self.alt:.1f}")


class CameraFeed(QWidget):
    """Mock live camera view: fake scene, FPV crosshair and OSD chips.
    In the real app this becomes a video sink (GStreamer/QMediaPlayer)."""

    def __init__(self):
        super().__init__()
        self.setMinimumSize(480, 360)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

    def _chip(self, p, text, x, y, right=False, dot=None):
        p.setFont(QFont("DejaVu Sans Mono", 9, QFont.Bold))
        metrics = p.fontMetrics()
        text_w = metrics.horizontalAdvance(text)
        dot_w = 14 if dot else 0
        w, h = text_w + dot_w + 20, 24
        if right:
            x -= w
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(8, 10, 18, 150))
        p.drawRoundedRect(QRectF(x, y, w, h), 6, 6)
        if dot:
            p.setBrush(QColor(dot))
            p.drawEllipse(QPointF(x + 13, y + h / 2), 3.5, 3.5)
        p.setPen(QColor("#eef1f6"))
        p.drawText(QRectF(x + 10 + dot_w, y, text_w + 2, h),
                   Qt.AlignVCenter | Qt.AlignLeft, text)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        w, h = self.width(), self.height()
        rect = QRectF(self.rect()).adjusted(0.5, 0.5, -0.5, -0.5)
        clip = QPainterPath()
        clip.addRoundedRect(rect, 14, 14)
        p.setClipPath(clip)

        # fake scene: sky, horizon, terrain, building silhouettes
        horizon = h * 0.55
        sky = QLinearGradient(0, 0, 0, horizon)
        sky.setColorAt(0.0, QColor("#28506f"))
        sky.setColorAt(1.0, QColor("#8fb4cd"))
        p.fillRect(QRectF(0, 0, w, horizon), sky)
        ground = QLinearGradient(0, horizon, 0, h)
        ground.setColorAt(0.0, QColor("#6d5f45"))
        ground.setColorAt(1.0, QColor("#3a3226"))
        p.fillRect(QRectF(0, horizon, w, h - horizon), ground)
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(24, 30, 44, 190))
        for bx, bw, bh in ((0.08, 0.10, 46), (0.20, 0.06, 24), (0.62, 0.09, 34),
                           (0.74, 0.05, 52), (0.88, 0.08, 28)):
            p.drawRect(QRectF(w * bx, horizon - bh, w * bw, bh))
        p.setPen(QPen(QColor(255, 255, 255, 90), 1.2))
        p.drawLine(QPointF(0, horizon), QPointF(w, horizon))

        # FPV crosshair
        cx, cy = w / 2, h / 2
        p.setPen(QPen(QColor(255, 255, 255, 200), 1.6))
        p.setBrush(Qt.NoBrush)
        p.drawEllipse(QPointF(cx, cy), 26, 26)
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            p.drawLine(QPointF(cx + dx * 34, cy + dy * 34),
                       QPointF(cx + dx * 58, cy + dy * 58))
        p.drawEllipse(QPointF(cx, cy), 2, 2)

        # OSD corners
        self._chip(p, "REC 00:32", 14, 14, dot=C["red"])
        self._chip(p, "MAIN · 1080p30 · 45 ms", w - 14, 14, right=True)
        self._chip(p, "ALT 12.4 m   SPD 3.2 m/s", 14, h - 38)
        self._chip(p, "GIMBAL 32°", w - 14, h - 38, right=True)

        p.setClipping(False)
        p.setPen(QPen(QColor(C["border"]), 1))
        p.setBrush(Qt.NoBrush)
        p.drawRoundedRect(rect, 14, 14)


class MotorBar(QWidget):
    """Horizontal output bar for one motor: label, bar, value in one row."""

    def __init__(self, name, pct):
        super().__init__()
        self.name, self.pct = name, pct
        self.setFixedHeight(14)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        w, h = self.width(), self.height()
        p.setPen(QColor(C["faint"]))
        p.setFont(QFont("DejaVu Sans", 8, QFont.Bold))
        p.drawText(QRectF(0, 0, 26, h), Qt.AlignVCenter | Qt.AlignLeft,
                   self.name)
        track = QRectF(30, h / 2 - 4.5, w - 30 - 44, 9)
        p.setPen(QPen(QColor(C["border"]), 1))
        p.setBrush(QColor(C["inset"]))
        p.drawRoundedRect(track, 4.5, 4.5)
        fill = QRectF(track.x() + 1.5, track.y() + 1.5,
                      (track.width() - 3) * self.pct / 100.0,
                      track.height() - 3)
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(C["accent"]))
        p.drawRoundedRect(fill, 3, 3)
        p.setPen(QColor(C["text"]))
        p.setFont(QFont("DejaVu Sans Mono", 8, QFont.Bold))
        p.drawText(QRectF(w - 40, 0, 40, h), Qt.AlignVCenter | Qt.AlignRight,
                   f"{self.pct:.0f}%")


class BatteryIcon(QWidget):
    """Upright battery glyph with a charge-level fill."""

    def __init__(self, pct):
        super().__init__()
        self.pct = pct
        self.setFixedSize(34, 62)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        w = self.width()
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(C["border"]))
        p.drawRoundedRect(QRectF(w / 2 - 6, 0, 12, 5), 2, 2)
        body = QRectF(4.5, 4.5, w - 9, self.height() - 5)
        p.setPen(QPen(QColor(C["border"]), 1.6))
        p.setBrush(QColor(C["inset"]))
        p.drawRoundedRect(body, 4, 4)
        tone = C["green"] if self.pct > 40 else (
            C["amber"] if self.pct > 20 else C["red"])
        inner_h = (body.height() - 6) * self.pct / 100.0
        fill = QRectF(body.x() + 3, body.bottom() - 3 - inner_h,
                      body.width() - 6, inner_h)
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(tone))
        p.drawRoundedRect(fill, 2.5, 2.5)


class BatteryBar(QWidget):
    """Slim horizontal level bar; colored by charge unless a tone is given."""

    def __init__(self, pct, tone=None):
        super().__init__()
        self.pct, self.tone = pct, tone
        self.setFixedHeight(14)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        rect = QRectF(self.rect()).adjusted(0.5, 0.5, -0.5, -0.5)
        p.setPen(QPen(QColor(C["border"]), 1))
        p.setBrush(QColor(C["inset"]))
        p.drawRoundedRect(rect, 7, 7)
        tone = C[self.tone] if self.tone else (
            C["green"] if self.pct > 40 else
            C["amber"] if self.pct > 20 else C["red"])
        fill = QRectF(rect.x() + 2, rect.y() + 2,
                      (rect.width() - 4) * self.pct / 100.0, rect.height() - 4)
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(tone))
        p.drawRoundedRect(fill, 5, 5)


# --------------------------------------------------------------------------
# Chrome: top bar, nav rail, status bar
# --------------------------------------------------------------------------
def build_mode_switch():
    """AUTO / MANUAL — the only two modes; manual needs a controller.
    Equal-width segments so the switch reads as one balanced control."""
    seg = QFrame()
    seg.setProperty("cls", "segbox")
    # fixed height, otherwise the frame stretches to fill the top bar
    seg.setFixedHeight(32)
    seg_lay = QHBoxLayout(seg)
    seg_lay.setContentsMargins(3, 3, 3, 3)
    seg_lay.setSpacing(2)
    group = QButtonGroup(seg)
    group.setExclusive(True)
    for name, checked in (("AUTO", True), ("MANUAL", False)):
        btn = button(name, cls="seg", checkable=True, checked=checked)
        btn.setFixedSize(84, 24)
        group.addButton(btn)
        seg_lay.addWidget(btn)
    return seg


def build_topbar():
    """App-wide top bar. The e-stop lives on the map, not here."""
    bar = QFrame()
    bar.setObjectName("TopBar")
    bar.setFixedHeight(58)
    lay = QHBoxLayout(bar)
    lay.setContentsMargins(16, 0, 16, 0)
    lay.setSpacing(12)

    lay.addWidget(LogoBadge())
    lay.addWidget(vsep())
    lay.addWidget(build_mode_switch())
    lay.addWidget(vsep())
    arm = button("ARMED", cls="arm")
    arm.setToolTip("Hold to disarm")
    lay.addWidget(arm)
    lay.addWidget(label("T+ 04:12", cls="time"))
    lay.addStretch(1)

    # flat link-status strip: every link, every state, no bubbles
    for name, tone in (("UAV", "green"), ("RADIO", "green"),
                       ("WIFI · OFF", "faint"), ("RTK · SURVEY-IN", "amber"),
                       ("CONTROLLER", "green")):
        lay.addWidget(status_dot(name, tone))
    return bar


NAV_ITEMS = (("✈", "Fly"), ("◈", "Plan"), ("☑", "Checklist"),
             ("◉", "Camera"))


def build_navrail(win):
    rail = QFrame()
    rail.setObjectName("NavRail")
    rail.setFixedWidth(56)
    lay = QVBoxLayout(rail)
    lay.setContentsMargins(8, 12, 8, 12)
    lay.setSpacing(6)
    group = QButtonGroup(rail)
    group.setExclusive(True)

    def nav_button(glyph, tip, idx):
        btn = button(glyph, checkable=True, checked=idx == win.page)
        btn.setToolTip(tip)
        btn.clicked.connect(lambda _, i=idx: win.set_page(i))
        group.addButton(btn, idx)
        return btn

    for idx, (glyph, tip) in enumerate(NAV_ITEMS):
        lay.addWidget(nav_button(glyph, tip, idx), 0, Qt.AlignHCenter)
    lay.addStretch(1)
    # bottom cluster: day/night toggle directly above settings
    theme = button("☀" if win.theme == "dark" else "☾")
    theme.setToolTip("Day / night theme")
    theme.clicked.connect(win.toggle_theme)
    lay.addWidget(theme, 0, Qt.AlignHCenter)
    lay.addWidget(nav_button("⚙", "Settings", 4), 0, Qt.AlignHCenter)
    return rail, group


# --------------------------------------------------------------------------
# Left panel: telemetry cards
# --------------------------------------------------------------------------
AXIS_KEYS = {"X": "axis_x", "Y": "axis_y", "Z": "axis_z"}


def axis_label(axis, align=None):
    return label(f"<span style='color:{C[AXIS_KEYS[axis]]}'>{axis}</span>",
                 cls="dim", align=align, rich=True)


def build_state_card():
    """Instrument-cluster style: axes as columns, POS / TGT / VEL as rows,
    with speed, height and attitude numbers folded into the same card
    (the artificial horizon lives on the map only)."""
    frame, body = card("State · NED")
    grid = QGridLayout()
    grid.setHorizontalSpacing(10)
    grid.setVerticalSpacing(5)
    for col, axis in enumerate(("X", "Y", "Z"), start=1):
        grid.addWidget(axis_label(axis, align=Qt.AlignRight), 0, col)
    rows = (("POS m", "mono", ("2.63", "1.94", "-12.42"), ""),
            ("TGT m", "monodim", ("3.00", "1.50", "-12.00"), "→ "),
            ("VEL m/s", "monosm", ("1.24", "0.86", "-0.12"), ""))
    for r, (name, cls, vals, prefix) in enumerate(rows, start=1):
        grid.addWidget(label(name, cls="tiny"), r, 0)
        for col, val in enumerate(vals, start=1):
            grid.addWidget(label(prefix + val, cls=cls, align=Qt.AlignRight),
                           r, col)
    for col in range(1, 4):
        grid.setColumnStretch(col, 1)
    body.addLayout(grid)
    body.addWidget(hsep())
    foot = QHBoxLayout()
    stats = (("GND SPD", "3.2 m/s"), ("HEIGHT", "12.4 m"), ("ROLL", "8.2°"),
             ("PITCH", "5.1°"), ("YAW", "53.2°"))
    for name, val in stats:
        cell = QVBoxLayout()
        cell.setSpacing(1)
        cell.addWidget(label(val, cls="monodim"))
        cell.addWidget(label(name, cls="tiny"))
        foot.addLayout(cell)
    body.addLayout(foot)
    return frame


def build_probe_card():
    found = label(f"<span style='color:{C['accent']}'>3 found</span>",
                  cls="faint", rich=True)
    frame, body = card("Probe Info", found)
    grid = QGridLayout()
    grid.setHorizontalSpacing(12)
    grid.setVerticalSpacing(3)
    for col, name in enumerate(("ID", "X", "Y", "Z", "C")):
        grid.addWidget(label(name, cls="tiny",
                             align=Qt.AlignLeft if col == 0 else Qt.AlignRight),
                       0, col)
    rows = (("01", "2.10", "0.40", "-0.32", "7.8"),
            ("02", "3.55", "1.85", "-0.30", "6.1"),
            ("03", "5.02", "0.75", "-0.35", "8.4"))
    for r, row in enumerate(rows, start=1):
        for col, val in enumerate(row):
            grid.addWidget(label(val, cls="monosm",
                                 align=Qt.AlignLeft if col == 0 else Qt.AlignRight),
                           r, col)
    for col in range(1, 5):
        grid.setColumnStretch(col, 1)
    body.addLayout(grid)
    return frame


def build_gps_card():
    rtk_state = label(f"<span style='color:{C['amber']}'>●</span> SURVEY-IN",
                      cls="status", rich=True)
    frame, body = card("GPS · RTK", rtk_state)
    row = QHBoxLayout()
    for name, val in (("FIX", "3D"), ("SATS", "12"), ("ACC", "0.8 m")):
        cell = QVBoxLayout()
        cell.setSpacing(1)
        cell.addWidget(label(val, cls="mono"))
        cell.addWidget(label(name, cls="tiny"))
        row.addLayout(cell)
    body.addLayout(row)
    survey = QHBoxLayout()
    survey.setSpacing(10)
    survey.addWidget(BatteryBar(62, tone="amber"), 1)
    survey.addWidget(label("62%", cls="monosm"))
    body.addLayout(survey)
    body.addWidget(label("Lat 57.034000°   Lon 9.882000°", cls="monosm"))
    return frame


def build_info_panel():
    """Right-hand information column, top to bottom by priority."""
    panel = QWidget()
    panel.setFixedWidth(300)
    lay = QVBoxLayout(panel)
    lay.setContentsMargins(0, 0, 0, 0)
    lay.setSpacing(10)
    lay.addWidget(build_power_card())
    lay.addWidget(build_state_card())
    lay.addWidget(build_probe_card())
    lay.addWidget(build_gps_card())
    lay.addStretch(1)
    return panel


# --------------------------------------------------------------------------
# Center: map + console
# --------------------------------------------------------------------------
LOG_LINES = (
    ("12:03:41.208", "INFO",  "Heartbeat OK — link latency 23 ms"),
    ("12:03:42.011", "INFO",  "Home position set (0.00, 0.00, 0.00)"),
    ("12:03:43.542", "WARN",  "GPS: satellites low (7) — waiting for fix"),
    ("12:03:45.104", "INFO",  "EKF2 aligned, ready for takeoff"),
    ("12:03:47.881", "ERROR", "Offboard setpoint timeout — reverting to Hold"),
    ("12:03:49.310", "INFO",  "Mission uploaded: 5 items"),
    ("12:03:52.766", "INFO",  "Takeoff complete — holding at 2.0 m"),
)
LOG_TONES = {"INFO": "green", "WARN": "amber", "ERROR": "red", "DEBUG": "faint"}


def build_console_card():
    frame, body = card("Console")
    head = body.itemAt(0).layout()
    # severity filter chips + actions live in the card header
    for i, (name, on) in enumerate((("INFO", True), ("WARN", True),
                                    ("ERROR", True), ("DEBUG", False))):
        head.insertWidget(1 + i, button(name, cls="chip", checkable=True,
                                        checked=on))
    save = button("⤓ Save", cls="ghost")
    save.setFixedHeight(24)
    clear = button("Clear", cls="ghost")
    clear.setFixedHeight(24)
    head.addWidget(save)
    head.addWidget(clear)

    console = QTextEdit()
    console.setObjectName("Console")
    console.setReadOnly(True)
    html = []
    for ts, level, msg in LOG_LINES:
        html.append(
            f"<div><span style='color:{C['faint']}'>[{ts}]</span> "
            f"<span style='color:{C[LOG_TONES[level]]}; font-weight:600'>"
            f"{level}</span> "
            f"<span style='color:{C['text']}'>{msg}</span></div>")
    console.setHtml("".join(html))
    body.addWidget(console, 1)
    frame.setFixedHeight(172)
    return frame


def build_center(on_plan=None):
    panel = QWidget()
    lay = QVBoxLayout(panel)
    lay.setContentsMargins(0, 0, 0, 0)
    lay.setSpacing(12)
    lay.addWidget(MapCanvas(on_plan=on_plan), 1)
    bottom = QHBoxLayout()
    bottom.setSpacing(12)
    bottom.addWidget(build_command_card())
    bottom.addWidget(build_console_card(), 1)
    lay.addLayout(bottom)
    return panel


# --------------------------------------------------------------------------
# Command and power cards (left panel, below the state card)
# --------------------------------------------------------------------------
def build_command_card():
    """Pose command box, bottom-left (gimbal lives on the Camera tab)."""
    frame, body = card("Command")
    body.addStretch(1)
    pose = QHBoxLayout()
    pose.setSpacing(8)
    pose_lab = label("Pose", cls="dim")
    pose_lab.setFixedWidth(48)
    pose.addWidget(pose_lab)
    pose.addWidget(field("x  y  -z  yaw"), 1)
    pose.addWidget(button("➤", cls="send"))
    body.addLayout(pose)
    body.addStretch(1)
    frame.setFixedWidth(330)
    return frame


def build_power_card():
    """Battery glyph + electrical rows on top, motor bars lying below."""
    frame, body = card("Power & Motors")
    top = QHBoxLayout()
    top.setSpacing(16)
    batt = QVBoxLayout()
    batt.setSpacing(3)
    batt.addWidget(BatteryIcon(76), 0, Qt.AlignHCenter)
    batt.addWidget(label("76%", cls="mono", align=Qt.AlignHCenter))
    top.addLayout(batt)
    rows = QGridLayout()
    rows.setHorizontalSpacing(10)
    rows.setVerticalSpacing(6)
    stats = (("Voltage", "15.8 V"), ("Current", "12.4 A"),
             ("Discharge", "1 240 mAh"), ("Avg curr", "11.9 A"))
    for r, (name, val) in enumerate(stats):
        rows.addWidget(label(name, cls="dim"), r, 0)
        rows.addWidget(label(val, cls="mono", align=Qt.AlignRight), r, 1)
    rows.setColumnStretch(1, 1)
    top.addLayout(rows, 1)
    body.addLayout(top)
    body.addWidget(hsep())
    for name, pct in (("M1", 52), ("M2", 55), ("M3", 54), ("M4", 51)):
        body.addWidget(MotorBar(name, pct))
    return frame


# --------------------------------------------------------------------------
# Plan page: the flight plan lives here now (second priority on the Fly page)
# --------------------------------------------------------------------------
MISSION = (
    ("1", "Takeoff", "climb to 2.0 m", "2.0 m", "done"),
    ("2", "Goto", "x 2.0 · y 1.5 · z -2.0", "3.2 m/s", "done"),
    ("3", "Goto", "x 4.0 · y 3.0 · z -2.0", "3.2 m/s", "active"),
    ("4", "Spin", "yaw 180° at 30°/s", "180°", "pending"),
    ("5", "Land", "at current position", "—", "pending"),
)


def mission_row(num, title, sub, chip, state):
    row = QFrame()
    row.setProperty("cls", "wprow")
    row.setProperty("active", state == "active")
    lay = QHBoxLayout(row)
    lay.setContentsMargins(8, 6, 8, 6)
    lay.setSpacing(10)
    badge = label("✓" if state == "done" else num, cls="wpbadge",
                  align=Qt.AlignCenter)
    badge.setProperty("state", state)
    lay.addWidget(badge)
    text = QVBoxLayout()
    text.setSpacing(0)
    text.addWidget(label(title))
    text.addWidget(label(sub, cls="faint"))
    lay.addLayout(text, 1)
    lay.addWidget(label(chip, cls="wpchip"))
    return row


def build_mission_card():
    frame, body = card("Flight Plan", label("5 items", cls="pill", tone="dim"))
    for item in MISSION:
        body.addWidget(mission_row(*item))
    chips = QHBoxLayout()
    chips.setSpacing(5)
    for name in ("Manual", "Goto", "Takeoff", "Land", "Spin"):
        chips.addWidget(button(name, cls="chip"))
    chips.addStretch(1)
    body.addSpacing(2)
    body.addLayout(chips)
    body.addSpacing(4)
    actions = QHBoxLayout()
    actions.setSpacing(8)
    actions.addWidget(button("▶  Execute", cls="primary"), 1)
    actions.addWidget(button("↥  Load", cls="ghost"))
    actions.addWidget(button("Clear", cls="ghost"))
    body.addLayout(actions)
    return frame


def build_plan_page():
    page = QWidget()
    lay = QHBoxLayout(page)
    lay.setContentsMargins(12, 12, 12, 12)
    lay.setSpacing(12)
    side = QWidget()
    side.setFixedWidth(380)
    side_lay = QVBoxLayout(side)
    side_lay.setContentsMargins(0, 0, 0, 0)
    side_lay.setSpacing(12)
    side_lay.addWidget(build_mission_card())
    side_lay.addWidget(label("Click the map to append a Goto — mockup only",
                             cls="faint"))
    side_lay.addStretch(1)
    lay.addWidget(side)
    lay.addWidget(MapCanvas(), 1)
    return page


# --------------------------------------------------------------------------
# Checklist & Settings pages (placeholders to make the rail navigable)
# --------------------------------------------------------------------------
CHECKLIST = (
    ("Battery secured and charged", True),
    ("Propellers inspected and tightened", True),
    ("RTK base survey-in complete", False),
    ("Geofence configured", True),
    ("Telemetry link verified", True),
    ("Controller paired (manual mode)", False),
)


def build_checklist_page():
    page = QWidget()
    lay = QVBoxLayout(page)
    lay.setContentsMargins(24, 24, 24, 24)
    frame, body = card("Pre-flight Checklist",
                       label("4 / 6", cls="pill", tone="amber"))
    for text, done in CHECKLIST:
        mark = (f"<span style='color:{C['green']}'>✓</span>" if done
                else f"<span style='color:{C['faint']}'>○</span>")
        body.addWidget(label(f"{mark}&nbsp;&nbsp;{text}", rich=True))
    body.addSpacing(4)
    body.addWidget(button("Mark all complete", cls="ghost"))
    frame.setFixedWidth(440)
    lay.addWidget(frame, 0, Qt.AlignTop | Qt.AlignLeft)
    lay.addStretch(1)
    return page


def build_camera_page():
    """Camera tab: the feed view plus source and capture controls."""
    page = QWidget()
    lay = QVBoxLayout(page)
    lay.setContentsMargins(12, 12, 12, 12)
    lay.setSpacing(12)
    lay.addWidget(CameraFeed(), 1)
    row = QHBoxLayout()
    row.setSpacing(6)
    sources = QButtonGroup(page)
    sources.setExclusive(True)
    for name, checked in (("Main", True), ("IR", False)):
        btn = button(name, cls="chip", checkable=True, checked=checked)
        sources.addButton(btn)
        row.addWidget(btn)
    row.addSpacing(6)
    row.addWidget(label("rtsp://uav:8554/cam", cls="monosm"))
    row.addStretch(1)
    # gimbal control sits with the camera it points
    row.addWidget(label("Gimbal", cls="dim"))
    row.addWidget(label("0°", cls="tiny"))
    slider = QSlider(Qt.Horizontal)
    slider.setRange(0, 90)
    slider.setValue(32)
    slider.setFixedWidth(180)
    row.addWidget(slider)
    row.addWidget(label("90°", cls="tiny"))
    row.addWidget(label("32°", cls="mono"))
    stop = button("⏹", cls="ghost")
    stop.setFixedHeight(28)
    stop.setToolTip("Stop gimbal")
    row.addWidget(stop)
    row.addSpacing(10)
    row.addWidget(button("⏺  Record", cls="ghost"))
    row.addWidget(button("⤓  Snapshot", cls="ghost"))
    lay.addLayout(row)
    return page


SETTINGS = (
    ("Connection", (("Device", "/dev/ttyUSB0"), ("Baud rate", "57600"),
                    ("WiFi telemetry", "udp://:14550"),
                    ("RTK base", "survey-in · 0.8 m"))),
    ("General", (("Units", "metric"), ("Theme", "night / day toggle"),
                 ("Video feed", "rtsp://uav:8554/cam"))),
)


def build_settings_page():
    page = QWidget()
    lay = QVBoxLayout(page)
    lay.setContentsMargins(24, 24, 24, 24)
    lay.setSpacing(12)
    for title, rows in SETTINGS:
        frame, body = card(title)
        grid = QGridLayout()
        grid.setHorizontalSpacing(16)
        grid.setVerticalSpacing(8)
        for r, (name, val) in enumerate(rows):
            grid.addWidget(label(name, cls="dim"), r, 0)
            grid.addWidget(label(val, cls="monosm", align=Qt.AlignRight), r, 1)
        grid.setColumnStretch(1, 1)
        body.addLayout(grid)
        frame.setFixedWidth(440)
        lay.addWidget(frame, 0, Qt.AlignLeft)
    lay.addWidget(label("Placeholder page — settings are part of the design "
                        "study, not wired up.", cls="faint"))
    lay.addStretch(1)
    return page


def build_fly_page(win):
    page = QWidget()
    lay = QHBoxLayout(page)
    lay.setContentsMargins(12, 12, 12, 12)
    lay.setSpacing(12)
    lay.addWidget(build_center(on_plan=lambda: win.set_page(1)), 1)
    lay.addWidget(build_info_panel())
    return page


# --------------------------------------------------------------------------
# Main window — rebuilds the whole UI when the theme changes
# --------------------------------------------------------------------------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.theme = "dark"
        self.page = 0
        self.setWindowTitle("ASR GCS — design mockup")
        self._rebuild()

    def _rebuild(self):
        C.clear()
        C.update(PALETTES[self.theme])
        self.setStyleSheet(build_qss())

        root = QWidget()
        root_lay = QVBoxLayout(root)
        root_lay.setContentsMargins(0, 0, 0, 0)
        root_lay.setSpacing(0)
        root_lay.addWidget(build_topbar())

        middle = QHBoxLayout()
        middle.setContentsMargins(0, 0, 0, 0)
        middle.setSpacing(0)
        rail, self.nav_group = build_navrail(self)
        middle.addWidget(rail)

        self.stack = QStackedWidget()
        self.stack.addWidget(build_fly_page(self))
        self.stack.addWidget(build_plan_page())
        self.stack.addWidget(build_checklist_page())
        self.stack.addWidget(build_camera_page())
        self.stack.addWidget(build_settings_page())
        self.stack.setCurrentIndex(self.page)
        middle.addWidget(self.stack, 1)

        root_lay.addLayout(middle, 1)
        self.setCentralWidget(root)

    def set_page(self, idx):
        self.page = idx
        self.stack.setCurrentIndex(idx)
        self.nav_group.button(idx).setChecked(True)

    def toggle_theme(self):
        self.theme = "day" if self.theme == "dark" else "dark"
        self._rebuild()


def main():
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    app = QApplication(sys.argv)
    win = MainWindow()
    win.resize(1680, 980)
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()