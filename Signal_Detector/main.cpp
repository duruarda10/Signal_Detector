#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "SineGenerator.h"
#include "NoiseInjector.h"

#include <GLFW/glfw3.h>
#include <vector>

int main() {
    if (!glfwInit())
        return -1;

    const char* glsl_version = "#version 130";
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

    float freqSlider = 2.0f;
    float ampSlider = 1.0f;
    float phaseSlider = 0.0f;
	float noiseSlider = 0.1f;
    bool triggerSpike = false;
    float spikeMagnitude = 3.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Signal Viewer");

        if (ImGui::Button("Reset")) {
            freqSlider = 2.0f;
            ampSlider = 1.0f;
            phaseSlider = 0.0f;
            noiseSlider = 0.1f;
            triggerSpike = false;
            spikeMagnitude = 3.0f;
        }

        ImGui::SliderFloat("Frequency (Hz)", &freqSlider, 0.1f, 10.0f);
        ImGui::SliderFloat("Amplitude", &ampSlider, 0.1f, 5.0f);
        ImGui::SliderFloat("Phase", &phaseSlider, 0.0f, 6.2832f);
        ImGui::SliderFloat("Noise", &noiseSlider, 0.001f, 1.0f);
        ImGui::SliderFloat("Spike Magnitude", &spikeMagnitude, 0.5f, 10.0f);
        ImGui::Checkbox("Trigger Spike", &triggerSpike);

        gen.setFrequency(freqSlider);
        gen.setAmplitude(ampSlider);
        gen.setPhase(phaseSlider);
        noise.setStdDev(noiseSlider);
        gen.reset();

        signal.clear();
        for (int i = 0; i < 500; i++) {
            float clean = gen.getNextSample();
            float noisy = noise.apply(clean);

            bool spikeSample = (triggerSpike && (i == 250));
            noisy = noise.applySpike(noisy, spikeSample, spikeMagnitude);

            signal.push_back(noisy);
        }

        if (ImPlot::BeginPlot("Sine Wave")) {
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