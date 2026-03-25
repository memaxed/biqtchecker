#pragma once

#include "IrisAnalyzer.h"
#include "PathUtils.h"

#include <string>

namespace QualityAPI {

inline IrisResult analyze(const std::string& imagePath) {
    IrisAnalyzer analyzer;
    analyzer.initialize();
    return analyzer.analyze(imagePath);
}

inline double overallQuality(const IrisResult& r) {
    return r.getMetric("iso_overall_quality");
}

}