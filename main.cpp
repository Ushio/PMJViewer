#include "pr.hpp"
#include <iostream>
#include <functional>
#include <memory>

// simpler vesion of 
// https://github.com/Andrew-Helmer/stochastic-generation
void GetPMJ02Samples(
    const int num_samples,
    float* samples,
    std::function<float(void)> uniformFloat
) {
    const int nd = 2;
    static constexpr uint32_t pmj02_xors[2][32] = {
        {0x0, 0x0, 0x2, 0x6, 0x6, 0xe, 0x36, 0x4e, 0x16, 0x2e, 0x276, 0x6ce, 0x716, 0xc2e, 0x3076, 0x40ce, 0x116, 0x22e, 0x20676, 0x60ece, 0x61716, 0xe2c2e, 0x367076, 0x4ec0ce, 0x170116, 0x2c022e, 0x2700676, 0x6c00ece, 0x7001716, 0xc002c2e, 0x30007076, 0x4000c0ce},
        {0x0, 0x1, 0x3, 0x3, 0x7, 0x1b, 0x27, 0xb, 0x17, 0x13b, 0x367, 0x38b, 0x617, 0x183b, 0x2067, 0x8b, 0x117, 0x1033b, 0x30767, 0x30b8b, 0x71617, 0x1b383b, 0x276067, 0xb808b, 0x160117, 0x138033b, 0x3600767, 0x3800b8b, 0x6001617, 0x1800383b, 0x20006067, 0x808b}
    };
    auto GetPMJ02Point = [](
        int x_stratum, int y_stratum,
        float i_strata,
        float xi0, float xi1,
        float* sample) {
            sample[0] = (xi0 + x_stratum) * i_strata;
            sample[1] = (xi1 + y_stratum) * i_strata;
    };

    // Generate first sample randomly.
    for (int d = 0; d < 2; d++)
    {
        samples[d] = uniformFloat();
    }

    for (int log_n = 0; (1 << log_n) < num_samples; log_n++) {
        int prev_len = 1 << log_n;
        int n_strata = prev_len * 2;
        float i_strata = 1.0f / n_strata;
        for (int i = 0; i < prev_len && (prev_len + i) < num_samples; i++) {
            const int prev_x_idx = i ^ pmj02_xors[0][log_n];
            const int prev_x_stratum = samples[prev_x_idx * 2] * n_strata;
            const int x_stratum = prev_x_stratum ^ 1;

            const int prev_y_idx = i ^ pmj02_xors[1][log_n];
            const int prev_y_stratum = samples[prev_y_idx * 2 + 1] * n_strata;
            const int y_stratum = prev_y_stratum ^ 1;

            float* sample = &(samples[(prev_len + i) * 2]);
            GetPMJ02Point(x_stratum, y_stratum, i_strata, uniformFloat(), uniformFloat(), sample);
        }
    }
}

int main() {
    using namespace pr;

    Config config;
    config.ScreenWidth = 1920;
    config.ScreenHeight = 1080;
    config.SwapInterval = 1;
    Initialize(config);

    Camera3D camera;
    camera.origin = { 0, 0, 4 };
    camera.lookat = { 0, 0, 0 };
    camera.zUp = false;

    double e = GetElapsedTime();

    int N = 1024;
    std::vector<glm::vec2> pmjSamples(N);
    Xoshiro128StarStar random;
    GetPMJ02Samples(N, glm::value_ptr(pmjSamples[0]), [&]() { return random.uniformf(); });

    while (pr::NextFrame() == false) {
        if (IsImGuiUsingMouse() == false) {
            UpdateCameraBlenderLike(&camera);
        }

        ClearBackground(0.1f, 0.1f, 0.1f, 1);

        BeginCamera(camera);

        PushGraphicState();

        DrawGrid(GridAxis::XY, 1.0f, 10, { 128, 128, 128 });
        DrawXYZAxis(1.0f, 0.001f);

        static int showN = 16;
        for (int i = 0; i < std::min( showN, N ); ++i)
        {
            glm::vec2 p = pmjSamples[i];
            float pix = i + 1 == showN;
            DrawPoint({ p.x, p.y, 0}, { 255 ,0 ,255 }, pix ? 5 : 2 );
            
            char label[128];
            sprintf(label, "%d", i);
            DrawText({ p.x + 0.005f, p.y + 0.005f, 0 }, label, 8);
        }

        // strutum viewer
        static int visualmode = 0;
        static int k = 0;
        int nxstrutum = 0;
        int nystrutum = 0;

        int nGlidSplit = std::ceil(std::log2(showN) / 2.0f);
        if( visualmode == 0 )
        {
            int nstrutum = 1 << nGlidSplit;
            float wstrutum = 1.0f / nstrutum;
            for( int i = 1; i < nstrutum; ++i )
            {
                float x = i * wstrutum;
                DrawLine({x, 0, 0}, { x, 1, 0 }, { 128,128 ,128 });
            }
            for (int i = 1; i < nstrutum; ++i)
            {
                float y = i * wstrutum;
                DrawLine({ 0, y, 0 }, { 1, y, 0 }, { 128,128 ,128 });
            }
        }
        else
        {
            nxstrutum = ( 1 << nGlidSplit ) * (1 << nGlidSplit);
            nystrutum = 1;

            // apply constraint
            int maxS = std::log2( nxstrutum );
            k = std::min( std::max(k, 0), maxS);
            
            nxstrutum = nxstrutum >> k;
            nystrutum = nystrutum << k;

            for (int i = 1; i < nxstrutum; ++i)
            {
                float wstrutum = 1.0f / nxstrutum;
                float x = i * wstrutum;
                DrawLine({ x, 0, 0 }, { x, 1, 0 }, { 128,128 ,128 });
            }
            for (int i = 1; i < nystrutum; ++i)
            {
                float wstrutum = 1.0f / nystrutum;
                float y = i * wstrutum;
                DrawLine({ 0, y, 0 }, { 1, y, 0 }, { 128,128 ,128 });
            }
        }

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());
        ImGui::InputInt( "show N", &showN );

        ImGui::RadioButton("visualmode grid", &visualmode, 0);
        ImGui::RadioButton("visualmode custom strutum", &visualmode, 1);
        if( visualmode )
        {
            ImGui::Text("%dx%d\n", nxstrutum, nystrutum);
            ImGui::InputInt("K", &k);
        }

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
