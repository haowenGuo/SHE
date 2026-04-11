#pragma once

#include "SHE/Core/AppClock.hpp"
#include "SHE/Core/ApplicationConfig.hpp"
#include "SHE/Core/LayerStack.hpp"
#include "SHE/Core/RuntimeServices.hpp"

namespace she
{
// Application is the runtime conductor. It knows the order in which services
// should be started and updated, but it does not own gameplay logic itself.
class Application
{
public:
    Application(ApplicationConfig config, RuntimeServices services);

    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> overlay);
    void RequestQuit();

    [[nodiscard]] int Run();

private:
    void InitializeServices();
    void ShutdownServices();
    void AttachLayers();
    void DetachLayers();
    void RunFixedUpdates(std::size_t fixedStepCount);

    ApplicationConfig m_config;
    RuntimeServices m_services;
    LayerStack m_layers;
    AppClock m_clock;
    bool m_isRunning = false;
    bool m_layersAttached = false;
    FrameIndex m_frameIndex = 0;
};
} // namespace she
