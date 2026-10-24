import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter
from PySide6.QtCore import Qt
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtWidgets import (
    QApplication, QWidget, QLabel, QPushButton, QFileDialog, QHBoxLayout,
    QVBoxLayout, QGridLayout, QGroupBox, QComboBox, QSlider, QSpinBox,
    QMessageBox
)


def clamp255(arr):
    return np.clip(arr, 0, 255).astype(np.uint8)


def pil_to_array(image):
    return np.asarray(image.convert("RGB"), dtype=np.uint8)


def array_to_pil(arr):
    return Image.fromarray(clamp255(arr), mode="RGB")


def rgb_to_gray(arr):
    r = arr[:, :, 0].astype(np.float32)
    g = arr[:, :, 1].astype(np.float32)
    b = arr[:, :, 2].astype(np.float32)
    return 0.299 * r + 0.587 * g + 0.114 * b


def histogram(gray):
    values = np.clip(np.rint(gray), 0, 255).astype(np.uint8)
    return np.bincount(values.ravel(), minlength=256)


def linear_contrast(arr):
    src = arr.astype(np.float32)
    out = np.empty_like(src)
    for channel in range(3):
        plane = src[:, :, channel]
        lo = float(plane.min())
        hi = float(plane.max())
        if hi - lo < 1e-9:
            out[:, :, channel] = plane
        else:
            out[:, :, channel] = (plane - lo) * 255.0 / (hi - lo)
    return clamp255(out)


def equalize_gray(gray):
    values = np.clip(np.rint(gray), 0, 255).astype(np.uint8)
    hist = histogram(values)
    cdf = hist.cumsum()
    nonzero = cdf[cdf > 0]
    if len(nonzero) == 0:
        return values
    cdf_min = nonzero[0]
    denom = values.size - cdf_min
    if denom <= 0:
        return values
    lut = np.round((cdf - cdf_min) * 255.0 / denom)
    lut = np.clip(lut, 0, 255).astype(np.uint8)
    return lut[values]


def histogram_equalization(arr):
    src = arr.astype(np.float32)
    gray = rgb_to_gray(src)
    eq = equalize_gray(gray).astype(np.float32)
    ratio = eq / np.maximum(gray, 1.0)
    out = src * ratio[:, :, None]
    black = gray < 1.0
    if np.any(black):
        out[black] = eq[black, None]
    return clamp255(out)


def convolve_gray(gray, kernel):
    kh, kw = kernel.shape
    ph, pw = kh // 2, kw // 2
    padded = np.pad(gray, ((ph, ph), (pw, pw)), mode="edge")
    out = np.zeros_like(gray, dtype=np.float32)
    for y in range(kh):
        for x in range(kw):
            out += kernel[y, x] * padded[y:y + gray.shape[0], x:x + gray.shape[1]]
    return out


def laplacian_sharpen(arr, strength=1.0):
    src = arr.astype(np.float32)
    kernel = np.array([[0, -1, 0], [-1, 4, -1], [0, -1, 0]], dtype=np.float32)
    out = np.empty_like(src)
    for channel in range(3):
        lap = convolve_gray(src[:, :, channel], kernel)
        out[:, :, channel] = src[:, :, channel] + strength * lap
    return clamp255(out)


def unsharp_mask(arr, radius=2.0, amount=1.0):
    image = array_to_pil(arr)
    blurred = np.asarray(image.filter(ImageFilter.GaussianBlur(radius=radius)), dtype=np.float32)
    src = arr.astype(np.float32)
    result = src + amount * (src - blurred)
    return clamp255(result)


def histogram_image(arr, width=512, height=220):
    gray = rgb_to_gray(arr)
    hist = histogram(gray).astype(np.float64)
    if hist.max() > 0:
        hist /= hist.max()
    canvas = np.full((height, width, 3), 255, dtype=np.uint8)
    xs = np.linspace(0, 255, width).astype(int)
    for x, idx in enumerate(xs):
        h = int(hist[idx] * (height - 20))
        canvas[height - 1 - h:height - 1, x] = (40, 40, 40)
    canvas[height - 1, :] = (0, 0, 0)
    return canvas


def array_to_pixmap(arr):
    arr = np.ascontiguousarray(arr)
    h, w, _ = arr.shape
    qimg = QImage(arr.data, w, h, 3 * w, QImage.Format_RGB888).copy()
    return QPixmap.fromImage(qimg)


class ImageView(QLabel):
    def __init__(self, text):
        super().__init__(text)
        self.setAlignment(Qt.AlignCenter)
        self.setMinimumSize(340, 260)
        self.setStyleSheet("QLabel { border: 1px solid #999; background: #f4f4f4; }")
        self._pixmap = None

    def set_array(self, arr):
        self._pixmap = array_to_pixmap(arr)
        self._refresh()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._refresh()

    def _refresh(self):
        if self._pixmap is not None:
            self.setPixmap(self._pixmap.scaled(
                self.size(), Qt.KeepAspectRatio, Qt.SmoothTransformation
            ))


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Лабораторная работа 2 - обработка изображений")
        self.original = None
        self.result = None
        self.current_path = None

        outer = QVBoxLayout(self)

        buttons = QHBoxLayout()
        load_btn = QPushButton("Открыть изображение...")
        load_btn.clicked.connect(self.open_image)
        save_btn = QPushButton("Сохранить результат...")
        save_btn.clicked.connect(self.save_result)
        reset_btn = QPushButton("Сбросить")
        reset_btn.clicked.connect(self.reset_image)
        buttons.addWidget(load_btn)
        buttons.addWidget(save_btn)
        buttons.addWidget(reset_btn)
        buttons.addStretch(1)
        outer.addLayout(buttons)

        controls = QGroupBox("Метод обработки")
        grid = QGridLayout(controls)
        self.method = QComboBox()
        self.method.addItems([
            "Линейное контрастирование",
            "Эквализация гистограммы",
            "Высокочастотный фильтр: Лапласиан",
            "Высокочастотный фильтр: нерезкая маска",
        ])
        self.method.currentIndexChanged.connect(self.update_controls)

        self.strength = QSlider(Qt.Horizontal)
        self.strength.setRange(1, 30)
        self.strength.setValue(10)
        self.strength.valueChanged.connect(self.apply_method)

        self.strength_value = QSpinBox()
        self.strength_value.setRange(1, 30)
        self.strength_value.setValue(10)
        self.strength_value.valueChanged.connect(self.strength.setValue)
        self.strength.valueChanged.connect(self.strength_value.setValue)

        apply_btn = QPushButton("Применить")
        apply_btn.clicked.connect(self.apply_method)

        grid.addWidget(QLabel("Алгоритм:"), 0, 0)
        grid.addWidget(self.method, 0, 1, 1, 2)
        grid.addWidget(QLabel("Коэффициент, x0.1:"), 1, 0)
        grid.addWidget(self.strength, 1, 1)
        grid.addWidget(self.strength_value, 1, 2)
        grid.addWidget(apply_btn, 2, 0, 1, 3)
        outer.addWidget(controls)

        views = QHBoxLayout()
        self.original_view = ImageView("Исходное изображение")
        self.result_view = ImageView("Результат")
        views.addWidget(self.original_view)
        views.addWidget(self.result_view)
        outer.addLayout(views, 1)

        hist_group = QGroupBox("Гистограммы яркости")
        hist_layout = QHBoxLayout(hist_group)
        self.hist_before = ImageView("До обработки")
        self.hist_after = ImageView("После обработки")
        self.hist_before.setMinimumHeight(170)
        self.hist_after.setMinimumHeight(170)
        hist_layout.addWidget(self.hist_before)
        hist_layout.addWidget(self.hist_after)
        outer.addWidget(hist_group)

        self.info = QLabel("Откройте изображение для начала работы.")
        outer.addWidget(self.info)

        self.resize(980, 820)
        self.update_controls()

    def update_controls(self):
        sharpening = self.method.currentIndex() >= 2
        self.strength.setEnabled(sharpening)
        self.strength_value.setEnabled(sharpening)
        if self.original is not None:
            self.apply_method()

    def open_image(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Открыть изображение", "",
            "Изображения (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)"
        )
        if not path:
            return
        try:
            image = Image.open(path).convert("RGB")
            self.original = pil_to_array(image)
            self.result = self.original.copy()
            self.current_path = Path(path)
            self.original_view.set_array(self.original)
            self.hist_before.set_array(histogram_image(self.original))
            self.apply_method()
        except Exception as exc:
            QMessageBox.critical(self, "Ошибка", f"Не удалось открыть изображение:\n{exc}")

    def apply_method(self):
        if self.original is None:
            return
        idx = self.method.currentIndex()
        k = self.strength.value() / 10.0
        if idx == 0:
            self.result = linear_contrast(self.original)
            name = "линейное контрастирование"
        elif idx == 1:
            self.result = histogram_equalization(self.original)
            name = "эквализация гистограммы"
        elif idx == 2:
            self.result = laplacian_sharpen(self.original, strength=k)
            name = f"Лапласиан, коэффициент {k:.1f}"
        else:
            self.result = unsharp_mask(self.original, radius=2.0, amount=k)
            name = f"нерезкая маска, коэффициент {k:.1f}"

        self.result_view.set_array(self.result)
        self.hist_after.set_array(histogram_image(self.result))
        self.info.setText(
            f"Метод: {name}. Размер: {self.original.shape[1]} x {self.original.shape[0]} пикселей."
        )

    def reset_image(self):
        if self.original is None:
            return
        self.result = self.original.copy()
        self.result_view.set_array(self.result)
        self.hist_after.set_array(histogram_image(self.result))
        self.info.setText("Результат сброшен к исходному изображению.")

    def save_result(self):
        if self.result is None:
            QMessageBox.information(self, "Сохранение", "Сначала откройте изображение.")
            return
        default = "result.png"
        if self.current_path is not None:
            default = str(self.current_path.with_name(self.current_path.stem + "_result.png"))
        path, _ = QFileDialog.getSaveFileName(
            self, "Сохранить результат", default, "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)"
        )
        if path:
            array_to_pil(self.result).save(path)


def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
