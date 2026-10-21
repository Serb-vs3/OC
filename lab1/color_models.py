import re
import sys

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QColor, QPalette, QRegularExpressionValidator
from PySide6.QtWidgets import (QApplication, QWidget, QFrame, QLabel, QHBoxLayout, QVBoxLayout,
                               QGridLayout, QGroupBox, QLineEdit, QPushButton, QColorDialog,
                               QSlider, QSpinBox, QDoubleSpinBox)

EPS = 1e-9


def clamp01(x):
    return 0.0 if x < 0 else 1.0 if x > 1 else x


def to8(x):
    return max(0, min(255, int(round(x * 255))))


def rgb_to_cmyk(r, g, b):
    k = 1.0 - max(r, g, b)
    if k >= 1.0 - EPS:
        return 0.0, 0.0, 0.0, 1.0
    d = 1.0 - k
    return (1 - r - k) / d, (1 - g - k) / d, (1 - b - k) / d, k


def cmyk_to_rgb(c, m, y, k):
    return (1 - c) * (1 - k), (1 - m) * (1 - k), (1 - y) * (1 - k)


def rgb_to_hls(r, g, b):
    mx, mn = max(r, g, b), min(r, g, b)
    l = (mx + mn) / 2.0
    d = mx - mn
    if d < EPS:
        return 0.0, l, 0.0
    s = d / (1.0 - abs(2.0 * l - 1.0))
    if mx == r:
        h = ((g - b) / d) % 6.0
    elif mx == g:
        h = (b - r) / d + 2.0
    else:
        h = (r - g) / d + 4.0
    return h * 60.0, l, clamp01(s)


def hls_to_rgb(h, l, s):
    c = (1.0 - abs(2.0 * l - 1.0)) * s
    hp = (h % 360.0) / 60.0
    x = c * (1.0 - abs(hp % 2.0 - 1.0))
    if hp < 1:
        r, g, b = c, x, 0
    elif hp < 2:
        r, g, b = x, c, 0
    elif hp < 3:
        r, g, b = 0, c, x
    elif hp < 4:
        r, g, b = 0, x, c
    elif hp < 5:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x
    m = l - c / 2.0
    return clamp01(r + m), clamp01(g + m), clamp01(b + m)


class ColorState:
    def __init__(self):
        self.rgb = (0.0, 0.0, 0.0)
        self.hls = (0.0, 0.0, 0.0)
        self.cmyk = (0.0, 0.0, 0.0, 1.0)
        self.set_rgb(0.20, 0.55, 0.95)

    def _derive_hls(self):
        h, l, s = rgb_to_hls(*self.rgb)
        ph, _, ps = self.hls
        if l < 1e-6 or l > 1 - 1e-6:
            h, s = ph, ps
        elif s < 1e-6:
            h = ph
        self.hls = (h, l, s)

    def _derive_cmyk(self):
        c, m, y, k = rgb_to_cmyk(*self.rgb)
        if k >= 1.0 - 1e-6:
            c, m, y = self.cmyk[:3]
        self.cmyk = (c, m, y, k)

    def set_rgb(self, r, g, b):
        self.rgb = (clamp01(r), clamp01(g), clamp01(b))
        self._derive_hls()
        self._derive_cmyk()

    def set_cmyk(self, c, m, y, k):
        self.cmyk = (clamp01(c), clamp01(m), clamp01(y), clamp01(k))
        self.rgb = cmyk_to_rgb(*self.cmyk)
        self._derive_hls()

    def set_hls(self, h, l, s):
        self.hls = (min(max(h, 0.0), 360.0), clamp01(l), clamp01(s))
        self.rgb = hls_to_rgb(*self.hls)
        self._derive_cmyk()

    def hex(self):
        r, g, b = (to8(v) for v in self.rgb)
        return f"#{r:02X}{g:02X}{b:02X}"


class Channel:
    """Одна строка: подпись + ползунок + поле ввода."""

    def __init__(self, label, vmax, dec, on_change):
        self.vmax = vmax
        self.label = QLabel(label)
        if dec == 0:
            self.spin = QSpinBox()
            self.scale = 1
        else:
            self.spin = QDoubleSpinBox()
            self.spin.setDecimals(dec)
            self.spin.setSingleStep(0.1)
            self.scale = 10
        self.spin.setRange(0, vmax)
        self.spin.setFixedWidth(80)
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, int(vmax * self.scale))
        self.slider.valueChanged.connect(lambda v: on_change(v / self.scale / vmax))
        self.spin.valueChanged.connect(lambda v: on_change(v / vmax))

    def set_value(self, v):
        """v — значение 0..1."""
        val = v * self.vmax
        if not self.spin.hasFocus() and not self.spin.lineEdit().hasFocus():
            self.spin.blockSignals(True)
            self.spin.setValue(val)
            self.spin.blockSignals(False)
        if not self.slider.isSliderDown():
            self.slider.blockSignals(True)
            self.slider.setValue(int(round(val * self.scale)))
            self.slider.blockSignals(False)


class ModelGroup(QGroupBox):
    edited = Signal(list)

    def __init__(self, title, channels):
        super().__init__(title)
        self.vals = [0.0] * len(channels)
        self.rows = []
        grid = QGridLayout(self)
        for i, (label, vmax, dec) in enumerate(channels):
            row = Channel(label, vmax, dec, lambda v, i=i: self._changed(i, v))
            grid.addWidget(row.label, i, 0)
            grid.addWidget(row.slider, i, 1)
            grid.addWidget(row.spin, i, 2)
            self.rows.append(row)
        grid.setColumnStretch(1, 1)

    def _changed(self, i, v):
        self.vals[i] = clamp01(v)
        self.edited.emit(list(self.vals))

    def set_values(self, vals):
        self.vals = list(vals)
        for row, v in zip(self.rows, vals):
            row.set_value(v)


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Цветовые модели — CMYK, RGB, HLS")
        self.state = ColorState()

        outer = QVBoxLayout(self)

        top = QHBoxLayout()
        self.swatch = QFrame()
        self.swatch.setFrameShape(QFrame.StyledPanel)
        self.swatch.setAutoFillBackground(True)
        self.swatch.setMinimumSize(120, 60)
        top.addWidget(self.swatch, 1)

        self.hex = QLineEdit()
        self.hex.setFixedWidth(90)
        self.hex.setMaxLength(7)
        self.hex.setValidator(QRegularExpressionValidator(r"#?[0-9A-Fa-f]{0,6}"))
        self.hex.textEdited.connect(self._set_hex)
        pick = QPushButton("Палитра…")
        pick.clicked.connect(self._palette)
        top.addWidget(self.hex)
        top.addWidget(pick)
        outer.addLayout(top)

        self.cmyk = ModelGroup("CMYK", [("C", 100, 1), ("M", 100, 1), ("Y", 100, 1), ("K", 100, 1)])
        self.rgb = ModelGroup("RGB", [("R", 255, 0), ("G", 255, 0), ("B", 255, 0)])
        self.hls = ModelGroup("HLS", [("H", 360, 1), ("L", 100, 1), ("S", 100, 1)])
        for g in (self.cmyk, self.rgb, self.hls):
            outer.addWidget(g)

        self.rgb.edited.connect(lambda v: self._apply(lambda s: s.set_rgb(*v)))
        self.cmyk.edited.connect(lambda v: self._apply(lambda s: s.set_cmyk(*v)))
        self.hls.edited.connect(lambda v: self._apply(lambda s: s.set_hls(v[0] * 360, v[1], v[2])))

        self.resize(520, self.sizeHint().height())
        self.refresh()

    def _apply(self, fn):
        fn(self.state)
        self.refresh()

    def _set_hex(self, text):
        m = re.fullmatch(r"#?([0-9a-fA-F]{6})", text.strip())
        if m:
            v = int(m.group(1), 16)
            self.state.set_rgb(((v >> 16) & 255) / 255, ((v >> 8) & 255) / 255, (v & 255) / 255)
            self.refresh()

    def _palette(self):
        r, g, b = (to8(v) for v in self.state.rgb)
        col = QColorDialog.getColor(QColor(r, g, b), self, "Палитра")
        if col.isValid():
            self.state.set_rgb(col.red() / 255, col.green() / 255, col.blue() / 255)
            self.refresh()

    def refresh(self):
        s = self.state
        self.rgb.set_values(list(s.rgb))
        self.cmyk.set_values(list(s.cmyk))
        h, l, sat = s.hls
        self.hls.set_values([h / 360.0, l, sat])
        pal = self.swatch.palette()
        pal.setColor(QPalette.Window, QColor(s.hex()))
        self.swatch.setPalette(pal)
        if not self.hex.hasFocus():
            self.hex.setText(s.hex())


def main():
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
