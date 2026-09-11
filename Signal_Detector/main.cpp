#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "SineGenerator.h"
#include "NoiseInjector.h"
#include "SignalPipeline.h"
#include "FeatureExtractor.h"
#include "DBSCAN.h"
#include "OCSVM.h"
#include "TypeClassifier.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <random>
#include <fstream>
#include <cstdlib>
#include <dlib/svm.h>
#include <cmath>
#include <limits>
#include <algorithm>

int anomalyStart = 0;
AnomalyType type = AnomalyType::None;
static bool shouldFocusView = false;

int main() {

    if (!glfwInit())
        return -1;

    const char* glsl_version = "#version 130";

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Signal Detector", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SineGenerator gen(2.0f, 1.0f, 100.0f);
    NoiseInjector noise(0.1f);
    std::vector<float> signal;
    std::vector<bool> isAnomaly;
    std::vector<float> cleanSignal;
    std::vector<FeatureVector> features;

    int selectedAnomalyOption = 0;
    const char* anomalyOptions[] = { "Random", "Spike", "Stuck", "Drift" };

    std::random_device rd;
    std::mt19937 randomGen(rd());

    float dbscanEpsilon = 1.5f;
    int dbscanMinPts = 7;
    std::vector<int> dbscanLabels;

    float ocsvmNu = 0.001f;
    float ocsvmGamma = 0.05f;
    float ocsvmThreshold = -0.1f;
    std::vector<double> ocsvmScores;

    dlib::decision_function<kernel_type> ocsvmModel;
    NormalizerStats ocsvmStats;
    bool modelLoaded = loadOCSVMModel("ocsvm_model.dat", ocsvmModel, ocsvmStats);

    TypeClassifierModel typeClassifier;
    bool typeClassifierLoaded = loadTypeClassifier("type_classifier.dat", typeClassifier);

    const float sampleRate = 100.0f;
    const float durationSeconds = 300.0f;
    const int bufferSize = (int)(sampleRate * durationSeconds);

    int spikeStart = bufferSize / 2;
    float spikeMagnitude = 3.0f;
    int spikeDuration = 10;

    int stuckStart = bufferSize / 2;
    int stuckDuration = 50;

    float driftRate = 0.05f;
    int driftStart = bufferSize / 2;
    int driftDuration = 500;

    float residualWeight = 1.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("Signal Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        if (ImGui::CollapsingHeader("Configuration")) {
            ImGui::SliderFloat("DBSCAN Epsilon", &dbscanEpsilon, 0.1f, 5.0f);
            ImGui::SliderInt("DBSCAN MinPts", &dbscanMinPts, 2, 20);

            ImGui::SliderFloat("OCSVM Nu", &ocsvmNu, 0.001f, 0.5f);
            ImGui::SliderFloat("OCSVM Gamma", &ocsvmGamma, 0.01f, 10.0f);
            ImGui::SliderFloat("OCSVM Threshold", &ocsvmThreshold, -5.0f, 5.0f);
        }

        ImGui::Separator();

        static std::string trainStatus;

        if (ImGui::Button("Train Models")) {
            try {
                std::vector<FeatureVector> trainingFeatures;
                const int trainingSignals = 75;
                const int windowSize = 50;
                const int extendedSize = bufferSize + windowSize;

                for (int s = 0; s < trainingSignals; s++) {
                    float freqT, ampT, phaseT, noiseT;
                    randomizeWave(randomGen, freqT, ampT, phaseT, noiseT);
                    gen.setFrequency(freqT);
                    gen.setAmplitude(ampT);
                    gen.setPhase(phaseT);
                    noise.setStdDev(noiseT);

                    std::vector<float> sig, clean;
                    std::vector<bool> anomFlags;
                    AnomalyType none = AnomalyType::None;

                    generateSignal(sig, clean, anomFlags, gen, noise, extendedSize, none,
                        0, 0.0f, 0,
                        0, 0,
                        0.0f, 0, 0);

                    std::vector<FeatureVector> feat = extractFeatures(sig, clean, windowSize);
                    feat.erase(feat.begin(), feat.begin() + windowSize);

                    trainingFeatures.insert(trainingFeatures.end(), feat.begin(), feat.end());
                }

                const size_t maxTrainingSamples = 3000;
                if (trainingFeatures.size() > maxTrainingSamples) {
                    std::shuffle(trainingFeatures.begin(), trainingFeatures.end(), randomGen);
                    trainingFeatures.resize(maxTrainingSamples);
                }

                if (trainingFeatures.empty()) {
                    trainStatus = "Training failed: no data collected";
                }
                else {
                    ocsvmStats = Normalizer::fit(trainingFeatures);
                    ocsvmModel = trainGlobalOCSVM(trainingFeatures, ocsvmStats, ocsvmNu, ocsvmGamma, residualWeight);
                    saveOCSVMModel("ocsvm_model.dat", ocsvmModel, ocsvmStats);
                    modelLoaded = true;

                    std::vector<LabeledSample> labeledData;
                    const int signalsPerType = 20;

                    AnomalyType typesToTrain[3] = { AnomalyType::Spike, AnomalyType::Stuck, AnomalyType::Drift };

                    for (AnomalyType t : typesToTrain) {
                        for (int s = 0; s < signalsPerType; s++) {
                            float freqT, ampT, phaseT, noiseT;
                            randomizeWave(randomGen, freqT, ampT, phaseT, noiseT);
                            gen.setFrequency(freqT);
                            gen.setAmplitude(ampT);
                            gen.setPhase(phaseT);
                            noise.setStdDev(noiseT);

                            int spikeStartT = bufferSize / 2, spikeDurT = 20, stuckStartT = bufferSize / 2, stuckDurT = 200, driftStartT = bufferSize / 2, driftDurT = 1000;
                            float spikeMagT = 3.0f, driftRateT = 0.005f;

                            std::vector<float> sig, clean;
                            std::vector<bool> anomFlags;

                            generateSignal(sig, clean, anomFlags, gen, noise, extendedSize, t,
                                spikeStartT, spikeMagT, spikeDurT,
                                stuckStartT, stuckDurT,
                                driftRateT, driftStartT, driftDurT);

                            std::vector<FeatureVector> feat = extractFeatures(sig, clean, windowSize);
                            feat.erase(feat.begin(), feat.begin() + windowSize);
                            anomFlags.erase(anomFlags.begin(), anomFlags.begin() + windowSize);

                            unsigned long label = (t == AnomalyType::Spike) ? 1 : (t == AnomalyType::Stuck) ? 2 : 3;

                            for (int i = 0; i < (int)feat.size(); i++) {
                                if (anomFlags[i]) {
                                    labeledData.push_back({ feat[i], label });
                                }
                            }
                        }
                    }

                    if (labeledData.empty()) {
                        trainStatus = "OCSVM trained, type classifier failed: no labeled data";
                    }
                    else {
                        typeClassifier = trainTypeClassifier(labeledData, ocsvmStats, ocsvmModel, ocsvmNu, ocsvmGamma, residualWeight);                        saveTypeClassifier("type_classifier.dat", typeClassifier);
                        typeClassifierLoaded = true;
                        trainStatus = "Training completed: OCSVM (" + std::to_string(trainingSignals) + " signals) + Type Classifier (" + std::to_string(labeledData.size()) + " samples)";
                    }
                }
            }
            catch (std::exception& e) {
                trainStatus = std::string("Training failed: ") + e.what();
            }
        }

        if (!trainStatus.empty()) {
            ImGui::Text("%s", trainStatus.c_str());
        }

        if (ImGui::Button("Generate Signal")) {
            float freqToUse, ampToUse, phaseToUse, noiseToUse;
            randomizeWave(randomGen, freqToUse, ampToUse, phaseToUse, noiseToUse);

            gen.setFrequency(freqToUse);
            gen.setAmplitude(ampToUse);
            gen.setPhase(phaseToUse);
            noise.setStdDev(noiseToUse);

            const int windowSize = 50;
            const int extendedSize = bufferSize + windowSize;

            int spikeStartToUse = spikeStart;
            int spikeDurationToUse = spikeDuration;
            int stuckStartToUse = stuckStart;
            int stuckDurationToUse = stuckDuration;
            int driftStartToUse = driftStart;
            int driftDurationToUse = driftDuration;
            float spikeMagnitudeToUse = spikeMagnitude;
            float driftRateToUse = driftRate;

            randomizeAnomaly(randomGen, extendedSize, type, spikeStartToUse, spikeMagnitudeToUse, spikeDurationToUse,
                stuckStartToUse, stuckDurationToUse,
                driftRateToUse, driftStartToUse, driftDurationToUse);

            if (selectedAnomalyOption == 1) type = AnomalyType::Spike;
            else if (selectedAnomalyOption == 2) type = AnomalyType::Stuck;
            else if (selectedAnomalyOption == 3) type = AnomalyType::Drift;

            generateSignal(signal, cleanSignal, isAnomaly, gen, noise, extendedSize, type,
                spikeStartToUse, spikeMagnitudeToUse, spikeDurationToUse,
                stuckStartToUse, stuckDurationToUse,
                driftRateToUse, driftStartToUse, driftDurationToUse);

            features = extractFeatures(signal, cleanSignal, windowSize);

            signal.erase(signal.begin(), signal.begin() + windowSize);
            cleanSignal.erase(cleanSignal.begin(), cleanSignal.begin() + windowSize);
            isAnomaly.erase(isAnomaly.begin(), isAnomaly.begin() + windowSize);
            features.erase(features.begin(), features.begin() + windowSize);

            int rawStart = spikeStartToUse;
            if (type == AnomalyType::Stuck) rawStart = stuckStartToUse;
            if (type == AnomalyType::Drift) rawStart = driftStartToUse;

            anomalyStart = std::max(0, rawStart - windowSize);
            shouldFocusView = true;

            dbscanLabels.clear();
            ocsvmScores.clear();
        }

        ImGui::SameLine();

        ImGui::SetNextItemWidth(100.0f);
        ImGui::Combo("Anomaly Type", &selectedAnomalyOption, anomalyOptions, IM_ARRAYSIZE(anomalyOptions));

        if (ImGui::Button("Run DBSCAN")) {
            dbscanLabels = runDBSCAN(features, dbscanEpsilon, dbscanMinPts, residualWeight);
        }

        ImGui::SameLine();

        if (ImGui::Button("Run OCSVM") && modelLoaded) {
            ocsvmScores = scoreWithModel(features, ocsvmStats, ocsvmModel, residualWeight);
        }
        if (!modelLoaded) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "No trained model loaded — click Train Models first.");
        }

        if (!dbscanLabels.empty()) {
            int maxCluster = -1;
            int noiseCount = 0;
            for (int label : dbscanLabels) {
                if (label == -1) noiseCount++;
                if (label > maxCluster) maxCluster = label;
            }
            ImGui::Text("Clusters found: %d, Noise points: %d", maxCluster + 1, noiseCount);
        }

        if (type != AnomalyType::None) {
            const char* anomalyName = "";
            switch (type) {
            case AnomalyType::Spike: anomalyName = "Spike"; break;
            case AnomalyType::Stuck: anomalyName = "Stuck"; break;
            case AnomalyType::Drift: anomalyName = "Drift"; break;
            default: anomalyName = "None"; break;
            }
            ImGui::Text("Anomaly: %s at sample %d", anomalyName, anomalyStart);
        }
        else {
            ImGui::Text("Anomaly: None");
        }

        if (!ocsvmScores.empty()) {
            int genuineOcsvmFlags = 0;
            double minScore = std::numeric_limits<double>::infinity();
            for (int i = 0; i < (int)ocsvmScores.size(); i++) {
                if (!std::isnan(ocsvmScores[i])) {
                    if (ocsvmScores[i] < ocsvmThreshold) genuineOcsvmFlags++;
                    if (ocsvmScores[i] < minScore) minScore = ocsvmScores[i];
                }
            }
            ImGui::Text("Genuine OCSVM anomalies: %d", genuineOcsvmFlags);
            ImGui::Text("Min score in signal: %.4f (threshold: %.4f)", minScore, ocsvmThreshold);
        }

        if (ImPlot::BeginPlot("Sine Wave", ImVec2(-1, 600))) {
            if (shouldFocusView) {
                double minX = std::max(0.0, (double)anomalyStart - 200.0);
                double maxX = (double)anomalyStart + 200.0;
                ImPlot::SetupAxisLimits(ImAxis_X1, minX, maxX, ImGuiCond_Always);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
                shouldFocusView = false;
            }
            else {
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_None);
            }

            if (!signal.empty()) {
                ImPlot::PlotLine("signal", signal.data(), (int)signal.size());
            }

            if (!dbscanLabels.empty() && dbscanLabels.size() == signal.size()) {
                std::vector<double> noiseX, noiseY;
                for (int i = 0; i < (int)dbscanLabels.size(); i++) {
                    if (dbscanLabels[i] == -1) {
                        noiseX.push_back((double)i);
                        noiseY.push_back((double)signal[i]);
                    }
                }
                if (!noiseX.empty()) {
                    ImPlot::PlotScatter("DBSCAN Noise (Anomaly Candidates)", noiseX.data(), noiseY.data(), (int)noiseX.size());
                }
            }

            if (!ocsvmScores.empty() && ocsvmScores.size() == signal.size()) {
                float baseOcsvmThreshold = ocsvmThreshold;
                std::vector<double> roughX, roughY;
                for (int i = 0; i < (int)ocsvmScores.size(); i++) {
                    if (!std::isnan(ocsvmScores[i]) && ocsvmScores[i] < baseOcsvmThreshold) {
                        roughX.push_back((double)i);
                        roughY.push_back((double)signal[i]);
                    }
                }

                unsigned long predictedLabel = 0;
                int labelSampleIdx = -1;

                if (typeClassifierLoaded && !roughX.empty()) {
                    std::vector<std::pair<int, int>> runs;
                    int runStart = 0;
                    for (int i = 1; i <= (int)roughX.size(); i++) {
                        bool isBreak = (i == (int)roughX.size()) || (roughX[i] - roughX[i - 1] > 1.0);
                        if (isBreak) {
                            runs.push_back({ runStart, i - runStart });
                            runStart = i;
                        }
                    }
                    int bestRun = 0;
                    for (int i = 1; i < (int)runs.size(); i++) {
                        if (runs[i].second > runs[bestRun].second) bestRun = i;
                    }

                    int startIdx = (int)roughX[runs[bestRun].first];
                    labelSampleIdx = startIdx;
                    predictedLabel = predictType(features[startIdx], ocsvmStats, typeClassifier, residualWeight);

                    double typeThreshold = thresholdForType(typeClassifier, predictedLabel, baseOcsvmThreshold);
                    ocsvmThreshold = std::min((float)typeThreshold, baseOcsvmThreshold);
                }

                std::vector<double> ocX, ocY;
                for (int i = 0; i < (int)ocsvmScores.size(); i++) {
                    if (!std::isnan(ocsvmScores[i]) && ocsvmScores[i] < ocsvmThreshold) {
                        ocX.push_back((double)i);
                        ocY.push_back((double)signal[i]);
                    }
                }

                if (!ocX.empty()) {
                    ImPlot::PlotScatter("OCSVM Anomalies", ocX.data(), ocY.data(), (int)ocX.size());
                }
                if (predictedLabel != 0 && labelSampleIdx >= 0 && labelSampleIdx < (int)signal.size()) {
                    double labelX = (double)labelSampleIdx;
                    double labelY = (double)signal[labelSampleIdx] + 1.0;
                    ImPlot::Annotation(labelX, labelY, ImVec4(1, 1, 1, 1), ImVec2(0, -10), true, "%s", typeLabelToString(predictedLabel));
                }
            }

            if (!ocsvmScores.empty() && ocsvmScores.size() == signal.size() &&
                !dbscanLabels.empty() && dbscanLabels.size() == signal.size()) {

                std::vector<double> ocOnlyX, ocOnlyY;
                for (int i = 0; i < (int)ocsvmScores.size(); i++) {
                    bool wasDbscanNoise = (dbscanLabels[i] == -1);
                    if (!std::isnan(ocsvmScores[i])) {
                        bool ocsvmFlagged = (ocsvmScores[i] < ocsvmThreshold);
                        if (ocsvmFlagged && !wasDbscanNoise) {
                            ocOnlyX.push_back((double)i);
                            ocOnlyY.push_back((double)signal[i]);
                        }
                    }
                }
            }

            if (type != AnomalyType::None) {
                double markerX = (double)std::max(0, anomalyStart);
                ImPlot::PlotInfLines("Anomaly Start", &markerX, 1);
            }

            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}