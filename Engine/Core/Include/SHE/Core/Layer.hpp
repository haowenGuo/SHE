#pragma once

#include "SHE/Core/TickContext.hpp"

#include <string>

namespace she
{
// A Layer is the main extension seam between engine runtime code and concrete
// gameplay or tooling code. The application loop knows how to drive layers in a
// stable order, while each layer decides what behavior to add to the frame.
class Layer
{
public:
    explicit Layer(std::string name);
    virtual ~Layer() = default;

    [[nodiscard]] const std::string& GetName() const;

    virtual void OnAttach(RuntimeServices& services);
    virtual void OnDetach(RuntimeServices& services);
    virtual void OnFixedUpdate(const TickContext& context);
    virtual void OnUpdate(const TickContext& context);
    virtual void OnRender(const TickContext& context);
    virtual void OnUi(const TickContext& context);

private:
    std::string m_name;
};
} // namespace she

