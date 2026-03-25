#include "IrisQualityAPI.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>
#include <cmath>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace fs = std::filesystem;

static int qualityFolder(double q) {
    int bucket = (int) (std::round(q / 10.0) * 10);
    if (bucket < 0) bucket = 0;
    if (bucket > 100) bucket = 100;
    return bucket;
}

static void writeTxt(const std::string& txtPath, const IrisResult& r) {
    std::ofstream f(txtPath);

    f << "---overall---\n";
    f << "iso_overall_quality: " << r.getMetric("iso_overall_quality") << "\n";
    f << "biqt_quality: " << r.getMetric("quality") << "\n";

    f << "\n---ISO---\n";
    for (const auto& key : {
        "iso_sharpness", "iso_usable_iris_area", "iso_iris_pupil_contrast",
        "iso_iris_sclera_contrast", "iso_iris_pupil_ratio", "iso_iris_pupil_concentricity",
        "iso_margin_adequacy", "iso_greyscale_utilization", "iso_pupil_boundary_circularity"
    }) {
        f << key << ": " << r.getMetric(key) << "\n";
    }

    f << "\n---raw---\n";
    for (const auto& key : {
        "contrast", "sharpness", "iris_pupil_gs", "iris_sclera_gs", "pupil_circularity_avg_deviation"
    }) {
        f << key << ": " << r.getMetric(key) << "\n";
    }

    f << "\n---normalized---\n";
    for (const auto& key : {
        "normalized_contrast", "normalized_sharpness", "normalized_iris_diameter",
        "normalized_iris_sclera_gs", "normalized_iris_pupil_gs",
        "normalized_iso_usable_iris_area", "normalized_iso_sharpness",
        "normalized_iso_iris_pupil_ratio", "normalized_iso_iris_pupil_contrast",
        "normalized_iso_iris_sclera_contrast", "normalized_iso_margin_adequacy",
        "normalized_iso_greyscale_utilization", "normalized_iso_iris_pupil_concentricity",
        "normalized_iso_iris_diameter"
    }) {
        f << key << ": " << r.getMetric(key) << "\n";
    }

    f << "\n---geometry---\n";
    for (const auto& key : {
        "image_width", "image_height",
        "iris_center_x", "iris_center_y", "iris_diameter",
        "pupil_center_x", "pupil_center_y", "pupil_diameter", "pupil_radius"
    }) {
        f << key << ": " << r.getFeature(key) << " px\n";
    }
}

// draw iris and pupil
static void saveAnnotated(const std::string& srcPath, const std::string& destPath,
                           const IrisResult& r) {
    cv::Mat img = cv::imread(srcPath, cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        // fallback: copy
        fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing);
        return;
    }

    // convert to BGR (to draw coloured circles)
    cv::Mat bgr;
    cv::cvtColor(img, bgr, cv::COLOR_GRAY2BGR);

    int irisCx = (int) r.getFeature("iris_center_x");
    int irisCy = (int) r.getFeature("iris_center_y");
    int irisR = (int) (r.getFeature("iris_diameter") / 2.0);

    int pupilCx = (int) r.getFeature("pupil_center_x");
    int pupilCy = (int) r.getFeature("pupil_center_y");
    int pupilR = (int) r.getFeature("pupil_radius");

    if (irisR > 0) cv::circle(bgr, {irisCx,  irisCy}, irisR, {0, 255, 0}, 2);
    if (pupilR > 0) cv::circle(bgr, {pupilCx, pupilCy}, pupilR, {0, 0, 255}, 2);

    cv::imwrite(destPath, bgr);
}

static void printHelp(const char* prog) {
    std::cout
        << "Usage: " << prog << " --input <dir> --output <dir> [--recursive]\n\n"
        << "--input  <dir>   Source folder with iris images\n"
        << "--output <dir>   Destination folder (subfolders 0,10,...,100 created automatically)\n"
        << "--recursive      Scan input subdirectories recursively\n"
        << "--help           Show this help\n\n"
        << "Supported formats: PNG, JPG/JPEG, BMP, TIFF\n";
}

int main(int argc, char** argv) {
    std::string inputDir, outputDir;
    bool recursive = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" && i+1 < argc) inputDir = argv[++i];
        else if (arg == "--output" && i+1 < argc) outputDir = argv[++i];
        else if (arg == "--recursive") recursive = true;
        else {printHelp(argv[0]); return 0;}
    }

    if (inputDir.empty() || outputDir.empty()) {
        std::cerr << "Error: --input and --output are required.\n";
        printHelp(argv[0]);
        return 1;
    }

    // collect images
    std::vector<std::string> images;
    try {
        images = PathUtils::collectImages(inputDir, recursive);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if(images.empty()) {
        std::cerr << "No supported images found in: " << inputDir << "\n";
        return 1;
    }

    // Create output folders 0, 10, 20, ..., 100
    for(int b = 0; b <= 100; b += 10)
        fs::create_directories(fs::path(outputDir) / std::to_string(b));

    std::cout << "Found " << images.size() << " image(s). Processing...\n\n";

    volatile int ok = 0, failed = 0;

    for(const auto& srcPath : images) {
        std::cout << fs::path(srcPath).filename().string() << " ... ";

        IrisResult r;
                
        try {
            r = QualityAPI::analyze(srcPath);
        } catch (std::out_of_range* e) {
            std::cerr << "FAILED: iris radius out of range\n";
            delete e;
            ++failed;
            continue;
        } catch (const std::exception& e) {
            std::cerr << "FAILED: " << e.what() << "\n";
            ++failed;
            continue;
        }

        if (r.hasError()) {
            std::cerr << "FAILED: " << r.error << "\n";
            ++failed;
            continue;
        }

        double q = QualityAPI::overallQuality(r);
        int bucket = qualityFolder(q);

        fs::path stem = fs::path(srcPath).stem();
        fs::path ext = fs::path(srcPath).extension();
        fs::path destDir = fs::path(outputDir) / std::to_string(bucket);

        std::string imgDest = (destDir / (stem.string() + ext.string())).string();
        std::string txtDest = (destDir / (stem.string() + ".txt")).string();

        saveAnnotated(srcPath, imgDest, r);
        writeTxt(txtDest, r);

        std::cout << "quality=" << q << " -> folder " << bucket << "\n";
        ++ok;
    }

    std::cout << "\nDone: " << ok << " OK, " << failed << " failed\n";
    return (failed == 0) ? 0 : 2;
}