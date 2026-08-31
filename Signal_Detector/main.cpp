#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "SineGenerator.h"
#include "NoiseInjector.h"

#include <GLFW/glfw3.h>
#include <vector>

enum class AnomalyType {
    None,
    Spike,
    Stuck,
    Drift
};

void generateSignal(std::vector<float>& signal, SineGenerator& gen, NoiseInjector& noise,
    int bufferSize, AnomalyType selectedAnomaly,
    int spikeStart, float spikeMagnitude,
    int stuckStart, int stuckDuration,
    float driftRate, int driftStart) {

    gen.reset();
    signal.clear();
    float stuckValue = 0.0f;

    for (int i = 0; i < bufferSize; i++) {
        float clean = gen.getNextSample();
        float noisy = noise.apply(clean);

        bool spikeThisSample = (selectedAnomaly == AnomalyType::Spike) && (i == spikeStart);
        noisy = noise.applySpike(noisy, spikeThisSample, spikeMagnitude);

        bool isStuck = (selectedAnomaly == AnomalyType::Stuck) && (i >= stuckStart) && (i < stuckStart + stuckDuration);
        if (isStuck && i == stuckStart) {
            stuckValue = noisy;
        }
        noisy = noise.applyStuck(noisy, isStuck, stuckValue);

        bool isDrifting = (selectedAnomaly == AnomalyType::Drift) && (i >= driftStart);
        int samplesSinceDriftStart = isDrifting ? (i - driftStart) : 0;
        noisy = noise.applyDrift(noisy, isDrifting, driftRate, samplesSinceDriftStart);

        signal.push_back(noisy);
    }
}

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

    AnomalyType selectedAnomaly = AnomalyType::None;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("Signal Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        
        if (ImGui::Button("Reset")) {
            freqSlider = 2.0f;
            ampSlider = 1.0f;
            phaseSlider = 0.0f;
            noiseSlider = 0.1f;
            spikeMagnitude = 3.0f;
            selectedAnomaly = AnomalyType::None;
        }

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
            ImGui::SliderFloat("Drift Rate", &driftRate, 0.001f, 0.1f);
        }

        if (ImGui::Button("Generate Signal")) {
            gen.setFrequency(freqSlider);
            gen.setAmplitude(ampSlider);
            gen.setPhase(phaseSlider);
            noise.setStdDev(noiseSlider);

            generateSignal(signal, gen, noise, bufferSize, selectedAnomaly,
                spikeStart, spikeMagnitude,
                stuckStart, stuckDuration,
                driftRate, driftStart);
        }

        if (ImPlot::BeginPlot("Sine Wave")) {
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1000, ImGuiCond_Once);
            ImPlot::PlotLine("signal", signal.data(), (int)signal.size());
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