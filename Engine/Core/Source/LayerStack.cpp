#include "SHE/Core/LayerStack.hpp"

#include <utility>

namespace she
{
Layer::Layer(std::string name) : m_name(std::move(name))
{
}

const std::string& Layer::GetName() const
{
    return m_name;
}

void Layer::OnAttach(RuntimeServices&)
{
}

void Layer::OnDetach(RuntimeServices&)
{
}

void Layer::OnFixedUpdate(const TickContext&)
{
}

void Layer::OnUpdate(const TickContext&)
{
}

void Layer::OnRender(const TickContext&)
{
}

void Layer::OnUi(const TickContext&)
{
}

void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
{
    m_layers.emplace(m_layers.begin() + static_cast<std::ptrdiff_t>(m_overlayInsertIndex), std::move(layer));
    ++m_overlayInsertIndex;
}

void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
{
    m_layers.emplace_back(std::move(overlay));
}

std::vector<std::unique_ptr<Layer>>& LayerStack::GetLayers()
{
    return m_layers;
}

const std::vector<std::unique_ptr<Layer>>& LayerStack::GetLayers() const
{
    return m_layers;
}
} // namespace she

