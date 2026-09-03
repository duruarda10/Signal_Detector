#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "SineGenerator.h"
#include "NoiseInjector.h"
#include "SignalPipeline.h"
#include "FeatureExtractor.h"
#include "DBSCAN.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <random>
#include <fstream>
#include <cstdlib>
#include <dlib/svm.h>

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
    std::vector<FeatureVector> features;

    std::random_device rd;
    std::mt19937 randomGen(rd());

    float dbscanEpsilon = 0.7f;
    int dbscanMinPts = 5;
    std::vector<int> dbscanLabels;

    const float sampleRate = 100.0f;
    const float durationSeconds = 300.0f;
    const int bufferSize = (int)(sampleRate * durationSeconds);

    int spikeStart = bufferSize / 2;
    float spikeMagnitude = 3.0f;

    int stuckStart = bufferSize / 2;
    int stuckDuration = 50;

    float driftRate = 0.01f;
    int driftStart = bufferSize / 2;
    int driftDuration = 500;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("Signal Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::SliderFloat("DBSCAN Epsilon", &dbscanEpsilon, 0.1f, 5.0f);
        ImGui::SliderInt("DBSCAN MinPts", &dbscanMinPts, 2, 20);

        ImGui::Separator();

        if (ImGui::Button("Generate Signal")) {
            float freqToUse, ampToUse, phaseToUse, noiseToUse;
            randomizeWave(randomGen, freqToUse, ampToUse, phaseToUse, noiseToUse);

            gen.setFrequency(freqToUse);
            gen.setAmplitude(ampToUse);
            gen.setPhase(phaseToUse);
            noise.setStdDev(noiseToUse);

            int spikeStartToUse = spikeStart;
            int stuckStartToUse = stuckStart;
            int stuckDurationToUse = stuckDuration;
            int driftStartToUse = driftStart;
            int driftDurationToUse = driftDuration;
            float spikeMagnitudeToUse = spikeMagnitude;
            float driftRateToUse = driftRate;

            randomizeAnomaly(randomGen, bufferSize, type, spikeStartToUse, spikeMagnitudeToUse,
                stuckStartToUse, stuckDurationToUse,
                driftRateToUse, driftStartToUse, driftDurationToUse);
            anomalyStart = spikeStartToUse;
            shouldFocusView = true;

            generateSignal(signal, isAnomaly, gen, noise, bufferSize, type,
                spikeStartToUse, spikeMagnitudeToUse,
                stuckStartToUse, stuckDurationToUse,
                driftRateToUse, driftStartToUse, driftDurationToUse);

            features = extractFeatures(signal, 50);

            dbscanLabels.clear();
        }

        if (ImGui::Button("Export Features to CSV")) {
            std::ofstream file("features.csv");
            if (!file.is_open()) {
                ImGui::Text("Failed to open file!");
            }
            else {
                file << "index,rawValue,rollingMean,rollingStdDev,rateOfChange,zScore,isAnomaly\n";
                for (int i = 0; i < (int)features.size(); i++) {
                    file << i << ","
                        << features[i].rawValue << ","
                        << features[i].rollingAverage << ","
                        << features[i].rollingStdDev << ","
                        << features[i].rateOfChange << ","
                        << features[i].zScore << ","
                        << (isAnomaly[i] ? 1 : 0) << "\n";
                }
                file.close();
                system("start features.csv");
            }
        }

        if (ImGui::Button("Run DBSCAN")) {
            dbscanLabels = runDBSCAN(features, dbscanEpsilon, dbscanMinPts);
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

        if (ImPlot::BeginPlot("Sine Wave", ImVec2(-1, 600))) {
            if (shouldFocusView) {
                ImPlot::SetupAxisLimits(ImAxis_X1, anomalyStart - 200, anomalyStart + 200, ImGuiCond_Always);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
                shouldFocusView = false;
            }
            else {
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_None);
            }

            ImPlot::PlotLine("signal", signal.data(), (int)signal.size());

            if (!dbscanLabels.empty()) {
                std::vector<double> noiseX, noiseY;
                for (int i = 0; i < (int)dbscanLabels.size(); i++) {
                    if (dbscanLabels[i] == -1) {
                        noiseX.push_back(i);
                        noiseY.push_back(signal[i]);
                    }
                }
                if (!noiseX.empty()) {
                    ImPlot::PlotScatter("DBSCAN Noise (Anomaly Candidates)", noiseX.data(), noiseY.data(), (int)noiseX.size());
                }
            }

            if (type != AnomalyType::None) {
                double markerX = (double)anomalyStart;
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