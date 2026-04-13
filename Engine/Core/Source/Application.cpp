#include "SHE/Core/Application.hpp"

#include "SHE/Core/Logger.hpp"
#include "SHE/Core/TickContext.hpp"

#include <exception>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace she
{
namespace
{
std::string BuildPlatformSummary(RuntimeServices& services)
{
    std::ostringstream stream;
    stream << "window_events_pumped=true\n";
    stream << "window_should_close=" << (services.window->ShouldClose() ? "true" : "false");
    return stream.str();
}

std::string BuildFixedUpdateSummary(RuntimeServices& services, const std::size_t fixedStepCount)
{
    std::ostringstream stream;
    stream << "fixed_steps=" << fixedStepCount << '\n';
    stream << "timer_count=" << services.gameplay->GetTimerCount() << '\n';
    stream << "pending_commands_before_update=" << services.gameplay->GetPendingCommandCount();
    return stream.str();
}

std::string BuildPresentationSummary(RuntimeServices& services)
{
    std::ostringstream stream;
    stream << "active_scene=" << services.scene->GetActiveSceneName() << '\n';
    stream << "entity_count=" << services.scene->GetEntityCount() << '\n';
    stream << "ui_frame_submitted=true";
    return stream.str();
}

std::string BuildAudioSummary(const double deltaSeconds)
{
    std::ostringstream stream;
    stream << "delta_seconds=" << deltaSeconds;
    return stream.str();
}
} // namespace

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
            m_services.diagnostics->BeginFrame(m_frameIndex);
            m_services.window->PumpEvents();
            m_services.diagnostics->RecordPhase("Platform", BuildPlatformSummary(m_services));
            const FrameSchedule schedule = m_clock.AdvanceFrame();
            m_services.gameplay->BeginFrame(m_frameIndex);

            RunFixedUpdates(schedule.fixedSteps);
            m_services.diagnostics->RecordPhase("FixedUpdate", BuildFixedUpdateSummary(m_services, schedule.fixedSteps));

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

            m_services.gameplay->AdvanceFrame(schedule.deltaSeconds);
            m_services.gameplay->FlushCommands();
            m_services.scripting->Update(schedule.deltaSeconds);
            m_services.scene->UpdateSceneGraph(schedule.deltaSeconds);
            m_services.diagnostics->RecordPhase("Gameplay", m_services.gameplay->BuildGameplayDigest());

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
            m_services.diagnostics->RecordPhase("Presentation", BuildPresentationSummary(m_services));
            m_services.audio->Update(schedule.deltaSeconds);
            m_services.diagnostics->RecordPhase("Audio", BuildAudioSummary(schedule.deltaSeconds));
            m_services.diagnostics->EndFrame();
            m_services.ai->RefreshContext(m_frameIndex);

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
    m_services.reflection->Initialize();
    m_services.data->Initialize();
    m_services.gameplay->Initialize();
    m_services.renderer->Initialize(m_config);
    m_services.physics->Initialize();
    m_services.audio->Initialize();
    m_services.ui->Initialize();
    m_services.scripting->Initialize();
    m_services.diagnostics->Initialize();
    m_services.ai->Initialize();
}

void Application::ShutdownServices()
{
    SHE_LOG_INFO("Application", "Shutting down runtime services.");

    m_services.ai->Shutdown();
    m_services.diagnostics->Shutdown();
    m_services.scripting->Shutdown();
    m_services.ui->Shutdown();
    m_services.audio->Shutdown();
    m_services.physics->Shutdown();
    m_services.renderer->Shutdown();
    m_services.gameplay->Shutdown();
    m_services.data->Shutdown();
    m_services.reflection->Shutdown();
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

        m_services.gameplay->AdvanceFixedStep(m_clock.GetFixedTimeStep());
        m_services.physics->Step(m_clock.GetFixedTimeStep());
    }
}
} // namespace she
