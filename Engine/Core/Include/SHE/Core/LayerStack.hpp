#pragma once

#include "SHE/Core/Layer.hpp"

#include <memory>
#include <vector>

namespace she
{
// LayerStack keeps gameplay layers and overlays in a predictable order.
// The insert index makes it possible to place overlays above regular layers
// without losing the simple "vector in execution order" mental model.
class LayerStack
{
public:
    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> overlay);

    [[nodiscard]] std::vector<std::unique_ptr<Layer>>& GetLayers();
    [[nodiscard]] const std::vector<std::unique_ptr<Layer>>& GetLayers() const;

private:
    std::vector<std::unique_ptr<Layer>> m_layers;
    std::size_t m_overlayInsertIndex = 0;
};
} // namespace she

