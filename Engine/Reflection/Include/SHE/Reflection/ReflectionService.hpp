#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>
#include <vector>

namespace she
{
struct TypeDescriptor
{
    std::string category;
    std::string typeName;
    std::string description;
};

struct FeatureDescriptor
{
    std::string featureName;
    std::string ownerModule;
    std::string description;
};

// ReflectionService is intentionally lightweight. Its purpose is not runtime
// metaprogramming wizardry; it is to make the engine self-describing enough for
// tooling, schema docs, and AI-assisted contributors to navigate safely.
class ReflectionService final : public IReflectionService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void RegisterType(std::string category, std::string typeName, std::string description) override;
    void RegisterFeature(std::string featureName, std::string ownerModule, std::string description) override;
    [[nodiscard]] std::size_t GetRegisteredTypeCount() const override;
    [[nodiscard]] std::size_t GetRegisteredFeatureCount() const override;
    [[nodiscard]] std::string BuildTypeCatalog() const override;

private:
    std::vector<TypeDescriptor> m_types;
    std::vector<FeatureDescriptor> m_features;
};
} // namespace she

