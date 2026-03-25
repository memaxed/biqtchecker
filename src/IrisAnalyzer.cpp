#include "IrisAnalyzer.h"
#include "BIQTIris.h"

IrisAnalyzer::IrisAnalyzer() = default;
IrisAnalyzer::~IrisAnalyzer() = default;

void IrisAnalyzer::initialize() {
    initialized_ = true;
}

IrisResult IrisAnalyzer::analyze(const std::string& imagePath) {
    if (!initialized_) {
        throw std::runtime_error("IrisAnalyzer not initialized. Call initialize() first.");
    }

    IrisResult result;
    result.imagePath = imagePath;

    BIQTIris engine;
    Provider::EvaluationResult evalResult = engine.evaluate(imagePath);

    if (evalResult.errorCode != 0) {
        result.error = "BIQTIris error " + std::to_string(evalResult.errorCode) +
                       ": " + evalResult.message;
        return result;
    }

    if (evalResult.qualityResult.empty()) {
        result.error = "No iris detected in image";
        return result;
    }

    const auto& qr = evalResult.qualityResult[0];
    for (const auto& kv : qr.metrics)  result.metrics[kv.first]  = kv.second;
    for (const auto& kv : qr.features) result.features[kv.first] = kv.second;

    return result;
}