#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "SineGenerator.h"
#include "NoiseInjector.h"
#include "SignalPipeline.h"
#include "FaultDetector.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <random>

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

    std::random_device rd;
    std::mt19937 randomGen(rd());

    FaultDetector detector;
    float thresholdMin = -1.5f;
    float thresholdMax = 1.5f;
    int movingAvgWindow = 50;
    float movingAvgThreshold = 1.5f;
    std::vector<bool> thresholdFlags;
    std::vector<bool> movingAvgFlags;

    const float sampleRate = 100.0f;
    const float durationSeconds = 300.0f;
    const int bufferSize = (int)(sampleRate * durationSeconds);

    float freqSlider = 2.0f;
    float ampSlider = 1.0f;
    float phaseSlider = 0.0f;
    float noiseSlider = 0.1f;

    int spikeStart = bufferSize / 2;
    float spikeMagnitude = 3.0f;

    int stuckStart = bufferSize / 2;
    int stuckDuration = 50;

    float driftRate = 0.01f;
    int driftStart = bufferSize / 2;
    int driftDuration = 500;

    AnomalyType selectedAnomaly = AnomalyType::None;
    SignalMode selectedMode = SignalMode::Manual;
	DetectorMethod selectedDetector = DetectorMethod::Threshold;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("Signal Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        
		int modeChoice = (int)selectedMode;
        ImGui::RadioButton("Manual", &modeChoice, (int)SignalMode::Manual);
        ImGui::SameLine();
        ImGui::RadioButton("Randomized", &modeChoice, (int)SignalMode::Randomized);
        selectedMode = (SignalMode)modeChoice;

        ImGui::Separator();

        if (ImGui::Button("Reset")) {
            freqSlider = 2.0f;
            ampSlider = 1.0f;
            phaseSlider = 0.0f;
            noiseSlider = 0.1f;
            spikeMagnitude = 3.0f;
            selectedAnomaly = AnomalyType::None;
        }

        if (selectedMode == SignalMode::Manual) {
            ImGui::SliderFloat("Frequency (Hz)", &freqSlider, 0.1f, 10.0f);
            ImGui::SliderFloat("Amplitude", &ampSlider, 0.1f, 5.0f);
            ImGui::SliderFloat("Phase", &phaseSlider, 0.0f, 6.2832f);
            ImGui::SliderFloat("Noise", &noiseSlider, 0.001f, 1.0f);

            int anomalyChoice = (int)selectedAnomaly;
            ImGui::RadioButton("None", &anomalyChoice, (int)AnomalyType::None);
            ImGui::SameLine();
            ImGui::RadioButton("Spike", &anomalyChoice, (int)AnomalyType::Spike);
            ImGui::SameLine();
            ImGui::RadioButton("Stuck", &anomalyChoice, (int)AnomalyType::Stuck);
            ImGui::SameLine();
            ImGui::RadioButton("Drift", &anomalyChoice, (int)AnomalyType::Drift);
            selectedAnomaly = (AnomalyType)anomalyChoice;

            ImGui::SliderFloat("Threshold Min", &thresholdMin, -5.0f, 0.0f);
            ImGui::SliderFloat("Threshold Max", &thresholdMax, 0.0f, 5.0f);
            ImGui::SliderInt("Moving Avg Window", &movingAvgWindow, 5, 500);
            ImGui::SliderFloat("Moving Avg Threshold", &movingAvgThreshold, 0.1f, 3.0f);

            if (selectedAnomaly == AnomalyType::Spike) {
                ImGui::SliderInt("Spike Start", &spikeStart, 0, bufferSize - 1);
                ImGui::SliderFloat("Spike Magnitude", &spikeMagnitude, 0.5f, 10.0f);
            }

            if (selectedAnomaly == AnomalyType::Stuck) {
                ImGui::SliderInt("Stuck Start", &stuckStart, 0, bufferSize - 1);
                ImGui::SliderInt("Stuck Duration", &stuckDuration, 1, 2000);
            }

            if (selectedAnomaly == AnomalyType::Drift) {
                ImGui::SliderInt("Drift Start", &driftStart, 0, bufferSize - 1);
                ImGui::SliderFloat("Drift Rate", &driftRate, 0.0001f, 0.01f);
                ImGui::SliderInt("Drift Duration", &driftDuration, 50, 2000);
            }
        }

        int detectorChoice = (int)selectedDetector;
        ImGui::RadioButton("Threshold", &detectorChoice, (int)DetectorMethod::Threshold);
        ImGui::SameLine();
        ImGui::RadioButton("Moving Average", &detectorChoice, (int)DetectorMethod::MovingAverage);
        selectedDetector = (DetectorMethod)detectorChoice;

        if (selectedDetector == DetectorMethod::Threshold) {
            ImGui::SliderFloat("Threshold Min", &thresholdMin, -5.0f, 0.0f);
            ImGui::SliderFloat("Threshold Max", &thresholdMax, 0.0f, 5.0f);
        }

        if (selectedDetector == DetectorMethod::MovingAverage) {
            ImGui::SliderInt("Moving Avg Window", &movingAvgWindow, 5, 500);
            ImGui::SliderFloat("Moving Avg Threshold", &movingAvgThreshold, 0.1f, 3.0f);
        }

        if (ImGui::Button("Generate Signal")) {
            gen.setFrequency(freqSlider);
            gen.setAmplitude(ampSlider);
            gen.setPhase(phaseSlider);
            noise.setStdDev(noiseSlider);

            int spikeStartToUse = spikeStart;
            int stuckStartToUse = stuckStart;
            int stuckDurationToUse = stuckDuration;
            int driftStartToUse = driftStart;
            float spikeMagnitudeToUse = spikeMagnitude;
            float driftRateToUse = driftRate;
			int driftDurationToUse = driftDuration;

            if (selectedMode == SignalMode::Randomized) {
                randomizeAnomaly(randomGen, bufferSize, type, spikeStartToUse, spikeMagnitudeToUse,
                    stuckStartToUse, stuckDurationToUse,
                    driftRateToUse, driftStartToUse, driftDurationToUse);
                anomalyStart = spikeStartToUse;
                shouldFocusView = true;
            }

            generateSignal(signal, isAnomaly, gen, noise, bufferSize, type,
                spikeStartToUse, spikeMagnitudeToUse,
                stuckStartToUse, stuckDurationToUse,
                driftRateToUse, driftStartToUse, driftDurationToUse);

            thresholdFlags.clear();
            movingAvgFlags.clear();
            for (int i = 0; i < signal.size(); i++) {
                if (selectedDetector == DetectorMethod::Threshold) {
                    bool tFlag = detector.checkThreshold(signal[i], thresholdMin, thresholdMax);
                    thresholdFlags.push_back(tFlag);
                }
                if (selectedDetector == DetectorMethod::MovingAverage) {
                    bool mFlag = detector.checkMovingAverage(signal, i, movingAvgWindow, movingAvgThreshold);
                    movingAvgFlags.push_back(mFlag);
                }
            }
        }

        if (selectedMode == SignalMode::Randomized) {
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
        }

        if (ImPlot::BeginPlot("Sine Wave", ImVec2(-1, 600))) {
            if (shouldFocusView) {
                ImPlot::SetupAxisLimits(ImAxis_X1, anomalyStart - 200, anomalyStart + 200, ImGuiCond_Always);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
                shouldFocusView = false;
            }
            else {
                if (selectedMode != SignalMode::Randomized) {
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1000, ImGuiCond_Once);
                }
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_None);
            }

            ImPlot::PlotLine("signal", signal.data(), (int)signal.size());

            if (selectedDetector == DetectorMethod::Threshold) {
                std::vector<double> tFlagX, tFlagY;
                for (int i = 0; i < (int)thresholdFlags.size(); i++) {
                    if (thresholdFlags[i]) {
                        tFlagX.push_back(i);
                        tFlagY.push_back(signal[i]);
                    }
                }
                if (!tFlagX.empty()) {
                    ImPlot::PlotScatter("Threshold Detected", tFlagX.data(), tFlagY.data(), (int)tFlagX.size());
                }
            }
            if (selectedDetector == DetectorMethod::MovingAverage) {
                std::vector<double> mFlagX, mFlagY;
                for (int i = 0; i < (int)movingAvgFlags.size(); i++) {
                    if (movingAvgFlags[i]) {
                        mFlagX.push_back(i);
                        mFlagY.push_back(signal[i]);
                    }
                }
                if (!mFlagX.empty()) {
                    ImPlot::PlotScatter("Moving Avg Detected", mFlagX.data(), mFlagY.data(), (int)mFlagX.size());
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