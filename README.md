# biqtchecker

A one-shot CLI tool for validating BIQT-Iris metric quality on an iris dataset.
Built on top of [BIQTIris](https://github.com/mitre/biqt-iris) by mitre.

Sorts images into quality buckets and visualizes detector output, making it easy
to compare BIQT-Iris metrics against other iris quality analyzers.

---

## Requirements

### Windows
- Visual Studio 2019+ with C++ Desktop workload
- [vcpkg](https://github.com/microsoft/vcpkg) with `jsoncpp` and `opencv`

### Linux (Ubuntu/Debian)
- g++ with C++17 support, cmake
- libjsoncpp-dev, libopencv-dev

---

## Build

### Windows (MSVC + vcpkg)
```cmd
vcpkg install jsoncpp:x64-windows opencv:x64-windows
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake -A x64
cmake --build . --config Release
```

### Linux
```bash
sudo apt install -y build-essential cmake libjsoncpp-dev libopencv-dev
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

---

## Usage
```bash
biqtchecker --input ./dataset --output ./results
biqtchecker --input ./dataset --output ./results --recursive
```

Each image is evaluated by BIQT-Iris. The `iso_overall_quality` score is rounded
to the nearest multiple of 10 and the image is placed into the corresponding
subfolder (`0`, `10`, ..., `100`) inside `--output`.

For each image two files are written into the bucket folder:
- `<name>.<ext>` - image with iris (green) and pupil (red) circles drawn by OpenCV
- `<name>.txt` - all 29 metrics and 9 geometry features from BIQT-Iris

The visualized circles let you immediately spot detector misses - if the circles
land outside the iris, the metrics for that image are unreliable.

---

## Supported formats

PNG, JPG/JPEG, BMP, TIFF - 8 bpp grayscale, 256×256 to 1000×680 px.