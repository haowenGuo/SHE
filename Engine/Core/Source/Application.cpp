#include "SHE/Core/Application.hpp"

#include "SHE/Core/Logger.hpp"
#include "SHE/Core/TickContext.hpp"

#include <exception>
#include <stdexcept>
#include <utility>

namespace she
{
Application::Application(ApplicationConfig config, RuntimeServices services)
    : m_config(std::move(config))
    , m_services(std::move(services))
    , m_clock(
          m_config.fixedTimeStep,
          m_config.maxFrameDelta,
          m_config.targetFrameRate,
          m_config.useDeterministicFrameTiming)
{
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    if (m_layersAttached && layer)
    {
        layer->OnAttach(m_services);
    }

    m_layers.PushLayer(std::move(layer));
}

void Application::PushOverlay(std::unique_ptr<Layer> overlay)
{
    if (m_layersAttached && overlay)
    {
        overlay->OnAttach(m_services);
    }

    m_layers.PushOverlay(std::move(overlay));
}

void Application::RequestQuit()
{
    m_isRunning = false;
    if (m_services.window)
    {
        m_services.window->RequestClose();
    }
}

int Application::Run()
{
    try
    {
        if (!m_services.IsComplete())
        {
            throw std::runtime_error("RuntimeServices is incomplete. Every service must be assigned.");
        }

        InitializeServices();
        AttachLayers();

        m_clock.Reset();
        m_isRunning = true;
        m_frameIndex = 0;

        while (m_isRunning && !m_services.window->ShouldClose())
        {
            m_services.window->PumpEvents();
            const FrameSchedule schedule = m_clock.AdvanceFrame();

            RunFixedUpdates(schedule.fixedSteps);

            TickContext updateContext{
                schedule.deltaSeconds,
                m_frameIndex,
                false,
                m_services,
            };

            for (const auto& layer : m_layers.GetLayers())
            {
                layer->OnUpdate(updateContext);
            }

            m_services.scene->UpdateSceneGraph(schedule.deltaSeconds);

            m_services.renderer->BeginFrame(m_frameIndex);
            for (const auto& layer : m_layers.GetLayers())
            {
                layer->OnRender(updateContext);
            }

            m_services.renderer->SubmitSceneSnapshot(
                m_services.scene->GetActiveSceneName(),
                m_services.scene->GetEntityCount());

            m_services.ui->BeginFrame(m_frameIndex);
            for (const auto& layer : m_layers.GetLayers())
            {
                layer->OnUi(updateContext);
            }
            m_services.ui->EndFrame();

            m_services.renderer->EndFrame();
            m_services.audio->Update(schedule.deltaSeconds);

            ++m_frameIndex;
            if (m_config.maxFrames > 0 && m_frameIndex >= m_config.maxFrames)
            {
                RequestQuit();
            }
        }

        DetachLayers();
        ShutdownServices();
        return 0;
    }
    catch (const std::exception& exception)
    {
        SHE_LOG_ERROR("Application", exception.what());

        if (m_layersAttached)
        {
            DetachLayers();
        }

        if (m_services.IsComplete())
        {
            ShutdownServices();
        }

        return 1;
    }
}

void Application::InitializeServices()
{
    SHE_LOG_INFO("Application", "Initializing runtime services.");

    m_services.window->Initialize(m_config);
    m_services.assets->Initialize();
    m_services.scene->Initialize();
    m_services.renderer->Initialize(m_config);
    m_services.physics->Initialize();
    m_services.audio->Initialize();
    m_services.ui->Initialize();
}

void Application::ShutdownServices()
{
    SHE_LOG_INFO("Application", "Shutting down runtime services.");

    m_services.ui->Shutdown();
    m_services.audio->Shutdown();
    m_services.physics->Shutdown();
    m_services.renderer->Shutdown();
    m_services.scene->Shutdown();
    m_services.assets->Shutdown();
    m_services.window->Shutdown();
}

void Application::AttachLayers()
{
    for (const auto& layer : m_layers.GetLayers())
    {
        layer->OnAttach(m_services);
    }

    m_layersAttached = true;
}

void Application::DetachLayers()
{
    auto& layers = m_layers.GetLayers();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it)
    {
        (*it)->OnDetach(m_services);
    }

    m_layersAttached = false;
}

void Application::RunFixedUpdates(const std::size_t fixedStepCount)
{
    for (std::size_t step = 0; step < fixedStepCount; ++step)
    {
        TickContext fixedContext{
            m_clock.GetFixedTimeStep(),
            m_frameIndex,
            true,
            m_services,
        };

        for (const auto& layer : m_layers.GetLayers())
        {
            layer->OnFixedUpdate(fixedContext);
        }

        m_services.physics->Step(m_clock.GetFixedTimeStep());
    }
}
} // namespace she
