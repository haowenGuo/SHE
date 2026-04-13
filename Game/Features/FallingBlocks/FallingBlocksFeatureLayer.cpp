#include "FallingBlocksFeatureLayer.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace she
{
namespace
{
constexpr std::string_view kSceneName = "FallingBlocksBoard";
constexpr std::string_view kMoveLeftCommand = "FallingBlocksMoveLeft";
constexpr std::string_view kMoveRightCommand = "FallingBlocksMoveRight";
constexpr std::string_view kRotateCommand = "FallingBlocksRotateClockwise";
constexpr std::string_view kSoftDropCommand = "FallingBlocksSoftDrop";
constexpr std::string_view kHardDropCommand = "FallingBlocksHardDrop";
constexpr std::string_view kGravityCommand = "FallingBlocksAdvanceGravity";
constexpr std::string_view kRestartCommand = "RestartFallingBlocksGame";

constexpr Vector2 kCameraCenter{2.5F, 0.0F};
constexpr float kCameraZoom = 0.5F;
constexpr float kHudPanelCenterX = 9.0F;
constexpr float kGridLineThickness = 0.05F;
constexpr float kHorizontalInitialRepeatDelay = 0.18F;
constexpr float kHorizontalRepeatInterval = 0.07F;
constexpr float kSoftDropRepeatInterval = 0.045F;

[[nodiscard]] bool IsPressed(const RuntimeServices& services, const KeyCode primary, const KeyCode secondary)
{
    return services.window->GetKeyState(primary).pressed || services.window->GetKeyState(secondary).pressed;
}

[[nodiscard]] bool IsDown(const RuntimeServices& services, const KeyCode primary, const KeyCode secondary)
{
    return services.window->GetKeyState(primary).isDown || services.window->GetKeyState(secondary).isDown;
}

[[nodiscard]] Vector2 BuildBoardCellCenter(const int column, const int row)
{
    return Vector2{
        -4.5F + static_cast<float>(column),
        9.5F - static_cast<float>(row)};
}

[[nodiscard]] Color BuildPieceTint(const FallingBlocksPieceKind kind)
{
    switch (kind)
    {
    case FallingBlocksPieceKind::I:
        return Color{0.44F, 0.91F, 0.97F, 1.0F};
    case FallingBlocksPieceKind::O:
        return Color{0.98F, 0.86F, 0.32F, 1.0F};
    case FallingBlocksPieceKind::T:
        return Color{0.73F, 0.49F, 0.94F, 1.0F};
    case FallingBlocksPieceKind::S:
        return Color{0.47F, 0.89F, 0.42F, 1.0F};
    case FallingBlocksPieceKind::Z:
        return Color{0.95F, 0.42F, 0.40F, 1.0F};
    case FallingBlocksPieceKind::J:
        return Color{0.40F, 0.56F, 0.97F, 1.0F};
    case FallingBlocksPieceKind::L:
        return Color{0.96F, 0.60F, 0.27F, 1.0F};
    case FallingBlocksPieceKind::None:
    default:
        return Color{0.70F, 0.72F, 0.78F, 1.0F};
    }
}

[[nodiscard]] Color BuildBackdropColor(const FallingBlocksGamePhase phase)
{
    if (phase == FallingBlocksGamePhase::GameOver)
    {
        return Color{0.15F, 0.04F, 0.07F, 1.0F};
    }

    return Color{0.05F, 0.08F, 0.12F, 1.0F};
}

[[nodiscard]] std::string_view GetPieceMaterialName(const FallingBlocksPieceKind kind)
{
    switch (kind)
    {
    case FallingBlocksPieceKind::I:
        return "materials/falling_blocks.piece_i";
    case FallingBlocksPieceKind::O:
        return "materials/falling_blocks.piece_o";
    case FallingBlocksPieceKind::T:
        return "materials/falling_blocks.piece_t";
    case FallingBlocksPieceKind::S:
        return "materials/falling_blocks.piece_s";
    case FallingBlocksPieceKind::Z:
        return "materials/falling_blocks.piece_z";
    case FallingBlocksPieceKind::J:
        return "materials/falling_blocks.piece_j";
    case FallingBlocksPieceKind::L:
        return "materials/falling_blocks.piece_l";
    case FallingBlocksPieceKind::None:
    default:
        return "materials/falling_blocks.hud";
    }
}

[[nodiscard]] std::array<bool, 7> BuildDigitSegments(const int digit)
{
    switch (digit)
    {
    case 0:
        return {true, true, true, true, true, true, false};
    case 1:
        return {false, true, true, false, false, false, false};
    case 2:
        return {true, true, false, true, true, false, true};
    case 3:
        return {true, true, true, true, false, false, true};
    case 4:
        return {false, true, true, false, false, true, true};
    case 5:
        return {true, false, true, true, false, true, true};
    case 6:
        return {true, false, true, true, true, true, true};
    case 7:
        return {true, true, true, false, false, false, false};
    case 8:
        return {true, true, true, true, true, true, true};
    case 9:
        return {true, true, true, true, false, true, true};
    default:
        return {false, false, false, false, false, false, false};
    }
}

using DotGlyph = std::array<std::string_view, 5>;

[[nodiscard]] DotGlyph GetGlyph(const char value)
{
    switch (value)
    {
    case 'A':
        return {".###.", "#...#", "#####", "#...#", "#...#"};
    case 'C':
        return {".####", "#....", "#....", "#....", ".####"};
    case 'E':
        return {"#####", "#....", "####.", "#....", "#####"};
    case 'G':
        return {".####", "#....", "#.###", "#...#", ".###."};
    case 'I':
        return {"#####", "..#..", "..#..", "..#..", "#####"};
    case 'L':
        return {"#....", "#....", "#....", "#....", "#####"};
    case 'M':
        return {"#...#", "##.##", "#.#.#", "#...#", "#...#"};
    case 'N':
        return {"#...#", "##..#", "#.#.#", "#..##", "#...#"};
    case 'O':
        return {".###.", "#...#", "#...#", "#...#", ".###."};
    case 'R':
        return {"####.", "#...#", "####.", "#.#..", "#..##"};
    case 'S':
        return {".####", "#....", ".###.", "....#", "####."};
    case 'T':
        return {"#####", "..#..", "..#..", "..#..", "..#.."};
    case 'V':
        return {"#...#", "#...#", "#...#", ".#.#.", "..#.."};
    case 'X':
        return {"#...#", ".#.#.", "..#..", ".#.#.", "#...#"};
    case ' ':
    default:
        return {".....", ".....", ".....", ".....", "....."};
    }
}

[[nodiscard]] std::string BuildOutcomePayload(
    const FallingBlocksSnapshot& snapshot,
    const FallingBlocksAction action,
    const FallingBlocksActionResult& result,
    std::string_view source)
{
    std::ostringstream stream;
    stream << "source=" << source << "; action=";
    switch (action)
    {
    case FallingBlocksAction::MoveLeft:
        stream << "move_left";
        break;
    case FallingBlocksAction::MoveRight:
        stream << "move_right";
        break;
    case FallingBlocksAction::RotateClockwise:
        stream << "rotate_clockwise";
        break;
    case FallingBlocksAction::SoftDrop:
        stream << "soft_drop";
        break;
    case FallingBlocksAction::HardDrop:
        stream << "hard_drop";
        break;
    case FallingBlocksAction::StepGravity:
        stream << "gravity";
        break;
    case FallingBlocksAction::Restart:
        stream << "restart";
        break;
    }
    stream << "; changed=" << (result.stateChanged ? "true" : "false");
    stream << "; cleared_lines=" << result.clearedLineCount;
    stream << "; score=" << snapshot.score;
    stream << "; lines=" << snapshot.linesCleared;
    stream << "; level=" << snapshot.level;
    stream << "; phase=" << GetFallingBlocksPhaseName(snapshot.phase);
    stream << "; active_piece=" << GetFallingBlocksPieceName(snapshot.activePiece.kind);
    stream << "; next_piece=" << GetFallingBlocksPieceName(snapshot.nextPiece);
    return stream.str();
}
} // namespace

FallingBlocksFeatureLayer::FallingBlocksFeatureLayer() : Layer("FallingBlocksFeatureLayer")
{
}

void FallingBlocksFeatureLayer::OnAttach(RuntimeServices& services)
{
    m_services = &services;
    m_loggedControls = false;
    m_latestDebugReport.clear();
    m_latestAuthoringContext.clear();
    m_latestGameplayDigest.clear();
    m_lastCompletedFrame.reset();
    m_gravityAccumulator = 0.0;
    ResetInputRepeatState();

    services.ai->SetAuthoringIntent(
        "Ship a falling-block puzzle in Game/Features/FallingBlocks with gameplay-command routing, stable diagnostics, "
        "and a renderable 2D board/HUD surface.");

    RegisterFeatureContracts(services);
    RegisterVisualAssets(services);
    RegisterGameplayContracts(services);
    SubscribeToGameplay(services);

    services.scene->SetActiveScene(std::string(kSceneName));
    m_cameraEntityId = services.scene->CreateEntity("FallingBlocksCamera");
    services.scene->TrySetEntityTransform(
        m_cameraEntityId,
        SceneTransform{kCameraCenter, 0.0F, Vector2{1.0F, 1.0F}});

    m_game.Reset();
    const FallingBlocksSnapshot snapshot = m_game.BuildSnapshot();
    services.gameplay->QueueEvent(
        "falling_blocks",
        "GameStarted",
        "phase=playing; active_piece=" + std::string(GetFallingBlocksPieceName(snapshot.activePiece.kind)) +
            "; next_piece=" + std::string(GetFallingBlocksPieceName(snapshot.nextPiece)));
}

void FallingBlocksFeatureLayer::OnUpdate(const TickContext& context)
{
    if (!m_loggedControls)
    {
        m_loggedControls = true;
        SHE_LOG_INFO(
            "Game",
            "Falling blocks controls: A/D or Left/Right move, W/Up/X rotate, S/Down soft drop, Space hard drop, R restart, Esc quit.");
    }

    if (context.services.window->GetKeyState(KeyCode::Escape).pressed)
    {
        context.services.window->RequestClose();
        return;
    }

    QueueInputCommands(context);
}

void FallingBlocksFeatureLayer::OnRender(const TickContext& context)
{
    const FallingBlocksSnapshot snapshot = m_game.BuildSnapshot();
    const WindowState windowState = context.services.window->GetWindowState();

    context.services.renderer->SubmitCamera(
        Camera2DSubmission{
            m_cameraEntityId,
            "FallingBlocksCamera",
            kCameraCenter,
            kCameraZoom,
            windowState.width,
            windowState.height,
            BuildBackdropColor(snapshot.phase)});

    SubmitBoard(context, snapshot);
    SubmitHud(context, snapshot);
}

void FallingBlocksFeatureLayer::OnUi(const TickContext& context)
{
    m_latestDebugReport = context.services.ui->BuildLatestDebugReport();
    m_latestAuthoringContext = context.services.ai->ExportAuthoringContext();
    m_latestGameplayDigest = context.services.gameplay->BuildGameplayDigest();
    const auto lastFrame = context.services.renderer->GetLastCompletedFrame();
    if (lastFrame.has_value())
    {
        m_lastCompletedFrame = lastFrame;
    }
}

void FallingBlocksFeatureLayer::OnDetach(RuntimeServices& services)
{
    if (m_actionSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_actionSubscriptionId);
        m_actionSubscriptionId = 0;
    }

    m_latestDebugReport = services.ui->BuildLatestDebugReport();
    m_latestAuthoringContext = services.ai->ExportAuthoringContext();
    m_latestGameplayDigest = services.gameplay->BuildGameplayDigest();
    m_lastCompletedFrame = services.renderer->GetLastCompletedFrame();
    m_services = nullptr;
}

FallingBlocksSnapshot FallingBlocksFeatureLayer::BuildSnapshot() const
{
    return m_game.BuildSnapshot();
}

const std::string& FallingBlocksFeatureLayer::GetLatestDebugReport() const
{
    return m_latestDebugReport;
}

const std::string& FallingBlocksFeatureLayer::GetLatestAuthoringContext() const
{
    return m_latestAuthoringContext;
}

const std::string& FallingBlocksFeatureLayer::GetLatestGameplayDigest() const
{
    return m_latestGameplayDigest;
}

const std::optional<RenderFrameSnapshot>& FallingBlocksFeatureLayer::GetLastCompletedFrame() const
{
    return m_lastCompletedFrame;
}

void FallingBlocksFeatureLayer::RegisterFeatureContracts(RuntimeServices& services)
{
    services.reflection->RegisterType(
        "feature_component",
        "FallingBlocksBoardCell",
        "Single logical board cell in the W17 falling-block feature.");
    services.reflection->RegisterType(
        "feature_component",
        "FallingBlocksPiece",
        "Active or preview tetromino state used by the W17 falling-block feature.");
    services.reflection->RegisterFeature(
        "FallingBlocksFeature",
        "Game/Features/FallingBlocks",
        "AI-native falling-block game with command-routed input, structured diagnostics, and a fully rendered 10x20 board.");
}

void FallingBlocksFeatureLayer::RegisterGameplayContracts(RuntimeServices& services)
{
    services.gameplay->RegisterCommand(
        {std::string(kMoveLeftCommand), "Move the active piece one column left.", "falling_blocks", "MoveLeftRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kMoveRightCommand), "Move the active piece one column right.", "falling_blocks", "MoveRightRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kRotateCommand), "Rotate the active piece clockwise.", "falling_blocks", "RotateClockwiseRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kSoftDropCommand), "Soft drop the active piece one row.", "falling_blocks", "SoftDropRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kHardDropCommand), "Hard drop and lock the active piece.", "falling_blocks", "HardDropRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kGravityCommand), "Advance one gravity step.", "falling_blocks", "GravityStepRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kRestartCommand), "Restart the falling-block run.", "falling_blocks", "RestartRequested"});
}

void FallingBlocksFeatureLayer::SubscribeToGameplay(RuntimeServices& services)
{
    m_actionSubscriptionId = services.gameplay->SubscribeToEvent(
        "falling_blocks",
        "",
        [this](const GameplayEvent& event)
        {
            HandleGameplayAction(event);
        });
}

void FallingBlocksFeatureLayer::ResetInputRepeatState()
{
    m_horizontalDirection = 0;
    m_horizontalHoldSeconds = 0.0;
    m_horizontalRepeatArmed = false;
    m_softDropHoldSeconds = 0.0;
    m_softDropRepeatArmed = false;
}

void FallingBlocksFeatureLayer::QueueInputCommands(const TickContext& context)
{
    const FallingBlocksSnapshot snapshot = m_game.BuildSnapshot();
    const bool restartPressed = IsPressed(context.services, KeyCode::R, KeyCode::Enter);
    if (snapshot.phase == FallingBlocksGamePhase::GameOver)
    {
        m_gravityAccumulator = 0.0;
        ResetInputRepeatState();
        if (restartPressed)
        {
            QueueGameplayCommand(kRestartCommand, "source=input");
        }
        return;
    }

    const bool rotatePressed = context.services.window->GetKeyState(KeyCode::Up).pressed ||
                               context.services.window->GetKeyState(KeyCode::W).pressed ||
                               context.services.window->GetKeyState(KeyCode::X).pressed;
    const bool hardDropPressed = context.services.window->GetKeyState(KeyCode::Space).pressed;

    if (rotatePressed)
    {
        QueueGameplayCommand(kRotateCommand, "source=input");
    }

    if (hardDropPressed)
    {
        QueueGameplayCommand(kHardDropCommand, "source=input");
        m_gravityAccumulator = 0.0;
        ResetInputRepeatState();
        return;
    }

    const bool leftPressed = IsPressed(context.services, KeyCode::Left, KeyCode::A);
    const bool rightPressed = IsPressed(context.services, KeyCode::Right, KeyCode::D);
    const bool leftDown = IsDown(context.services, KeyCode::Left, KeyCode::A);
    const bool rightDown = IsDown(context.services, KeyCode::Right, KeyCode::D);

    int desiredDirection = 0;
    if (leftDown != rightDown)
    {
        desiredDirection = leftDown ? -1 : 1;
    }

    if (desiredDirection != m_horizontalDirection)
    {
        m_horizontalDirection = desiredDirection;
        m_horizontalHoldSeconds = 0.0;
        m_horizontalRepeatArmed = false;

        if (desiredDirection < 0 || (desiredDirection == 0 && leftPressed && !rightPressed))
        {
            QueueGameplayCommand(kMoveLeftCommand, "source=input");
        }
        else if (desiredDirection > 0 || (desiredDirection == 0 && rightPressed && !leftPressed))
        {
            QueueGameplayCommand(kMoveRightCommand, "source=input");
        }
    }
    else if (m_horizontalDirection != 0)
    {
        m_horizontalHoldSeconds += context.deltaSeconds;
        while (m_horizontalHoldSeconds >= (m_horizontalRepeatArmed ? kHorizontalRepeatInterval : kHorizontalInitialRepeatDelay))
        {
            m_horizontalHoldSeconds -= m_horizontalRepeatArmed ? kHorizontalRepeatInterval : kHorizontalInitialRepeatDelay;
            m_horizontalRepeatArmed = true;
            QueueGameplayCommand(
                m_horizontalDirection < 0 ? kMoveLeftCommand : kMoveRightCommand,
                "source=input_hold");
        }
    }

    const bool softDropPressed = IsPressed(context.services, KeyCode::Down, KeyCode::S);
    const bool softDropDown = IsDown(context.services, KeyCode::Down, KeyCode::S);
    if (softDropPressed)
    {
        QueueGameplayCommand(kSoftDropCommand, "source=input");
    }

    if (softDropDown)
    {
        m_softDropHoldSeconds += context.deltaSeconds;
        while (m_softDropHoldSeconds >= kSoftDropRepeatInterval)
        {
            m_softDropHoldSeconds -= kSoftDropRepeatInterval;
            m_softDropRepeatArmed = true;
            QueueGameplayCommand(kSoftDropCommand, "source=input_hold");
        }
    }
    else
    {
        m_softDropHoldSeconds = 0.0;
        m_softDropRepeatArmed = false;
    }

    m_gravityAccumulator += context.deltaSeconds;
    const double gravityInterval = m_game.GetGravityIntervalSeconds();
    while (m_gravityAccumulator >= gravityInterval)
    {
        m_gravityAccumulator -= gravityInterval;
        QueueGameplayCommand(kGravityCommand, "source=gravity");
    }

    if (restartPressed)
    {
        QueueGameplayCommand(kRestartCommand, "source=input");
    }
}

void FallingBlocksFeatureLayer::QueueGameplayCommand(std::string_view commandName, std::string payload)
{
    if (m_services == nullptr)
    {
        return;
    }

    m_services->gameplay->QueueCommand(std::string(commandName), std::move(payload));
}

void FallingBlocksFeatureLayer::HandleGameplayAction(const GameplayEvent& event)
{
    FallingBlocksAction action;
    if (event.name == "MoveLeftRequested")
    {
        action = FallingBlocksAction::MoveLeft;
    }
    else if (event.name == "MoveRightRequested")
    {
        action = FallingBlocksAction::MoveRight;
    }
    else if (event.name == "RotateClockwiseRequested")
    {
        action = FallingBlocksAction::RotateClockwise;
    }
    else if (event.name == "SoftDropRequested")
    {
        action = FallingBlocksAction::SoftDrop;
    }
    else if (event.name == "HardDropRequested")
    {
        action = FallingBlocksAction::HardDrop;
    }
    else if (event.name == "GravityStepRequested")
    {
        action = FallingBlocksAction::StepGravity;
    }
    else if (event.name == "RestartRequested")
    {
        action = FallingBlocksAction::Restart;
        m_gravityAccumulator = 0.0;
        ResetInputRepeatState();
    }
    else
    {
        return;
    }

    const FallingBlocksActionResult result = m_game.ApplyAction(action);
    PublishActionOutcome(action, result, event.source.empty() ? event.payload : event.source);
}

void FallingBlocksFeatureLayer::PublishActionOutcome(
    const FallingBlocksAction action,
    const FallingBlocksActionResult& result,
    std::string_view source)
{
    if (m_services == nullptr || !result.stateChanged)
    {
        return;
    }

    const FallingBlocksSnapshot snapshot = m_game.BuildSnapshot();
    m_services->gameplay->QueueEvent(
        "falling_blocks",
        "StateChanged",
        BuildOutcomePayload(snapshot, action, result, source));

    if (result.clearedLineCount > 0)
    {
        m_services->gameplay->QueueEvent(
            "falling_blocks",
            "LinesCleared",
            "count=" + std::to_string(result.clearedLineCount) + "; total_lines=" + std::to_string(snapshot.linesCleared) +
                "; score=" + std::to_string(snapshot.score));
    }

    if (result.pieceLocked)
    {
        m_services->gameplay->QueueEvent(
            "falling_blocks",
            "PieceLocked",
            "next_piece=" + std::string(GetFallingBlocksPieceName(snapshot.nextPiece)) + "; score=" +
                std::to_string(snapshot.score));
    }

    if (result.triggeredGameOver)
    {
        m_services->gameplay->QueueEvent(
            "falling_blocks",
            "GameOver",
            "score=" + std::to_string(snapshot.score) + "; lines=" + std::to_string(snapshot.linesCleared));
    }

    if (action == FallingBlocksAction::Restart)
    {
        m_services->gameplay->QueueEvent(
            "falling_blocks",
            "GameRestarted",
            "active_piece=" + std::string(GetFallingBlocksPieceName(snapshot.activePiece.kind)) + "; next_piece=" +
                std::string(GetFallingBlocksPieceName(snapshot.nextPiece)));
    }
}

void FallingBlocksFeatureLayer::RegisterVisualAssets(RuntimeServices& services)
{
    if (!services.assets->HasLoader("texture_placeholder"))
    {
        services.assets->RegisterLoader(
            AssetLoaderDescriptor{
                "texture_placeholder",
                "texture",
                {".placeholder", ".png"},
                "Placeholder texture loader for feature-owned 2D materials."});
    }

    const auto registerTexture = [&services](std::string logicalName, std::string sourcePath, std::string recordId)
    {
        services.assets->RegisterAsset(
            AssetMetadata{
                kInvalidAssetId,
                std::move(logicalName),
                std::move(sourcePath),
                "texture",
                "",
                "Game/Features/FallingBlocks",
                std::move(recordId),
                {"falling_blocks", "texture"},
                {
                    {"usage", "runtime_render"},
                }});
    };

    registerTexture(
        "textures/falling_blocks/board",
        "Game/Features/FallingBlocks/Data/board.placeholder",
        "feature.falling_blocks.texture.board");
    registerTexture(
        "textures/falling_blocks/frame",
        "Game/Features/FallingBlocks/Data/frame.placeholder",
        "feature.falling_blocks.texture.frame");
    registerTexture(
        "textures/falling_blocks/grid",
        "Game/Features/FallingBlocks/Data/grid.placeholder",
        "feature.falling_blocks.texture.grid");
    registerTexture(
        "textures/falling_blocks/hud",
        "Game/Features/FallingBlocks/Data/hud.placeholder",
        "feature.falling_blocks.texture.hud");
    registerTexture(
        "textures/falling_blocks/piece_i",
        "Game/Features/FallingBlocks/Data/piece_i.placeholder",
        "feature.falling_blocks.texture.piece_i");
    registerTexture(
        "textures/falling_blocks/piece_o",
        "Game/Features/FallingBlocks/Data/piece_o.placeholder",
        "feature.falling_blocks.texture.piece_o");
    registerTexture(
        "textures/falling_blocks/piece_t",
        "Game/Features/FallingBlocks/Data/piece_t.placeholder",
        "feature.falling_blocks.texture.piece_t");
    registerTexture(
        "textures/falling_blocks/piece_s",
        "Game/Features/FallingBlocks/Data/piece_s.placeholder",
        "feature.falling_blocks.texture.piece_s");
    registerTexture(
        "textures/falling_blocks/piece_z",
        "Game/Features/FallingBlocks/Data/piece_z.placeholder",
        "feature.falling_blocks.texture.piece_z");
    registerTexture(
        "textures/falling_blocks/piece_j",
        "Game/Features/FallingBlocks/Data/piece_j.placeholder",
        "feature.falling_blocks.texture.piece_j");
    registerTexture(
        "textures/falling_blocks/piece_l",
        "Game/Features/FallingBlocks/Data/piece_l.placeholder",
        "feature.falling_blocks.texture.piece_l");

    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.board", "textures/falling_blocks/board", Color{0.12F, 0.14F, 0.18F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.frame", "textures/falling_blocks/frame", Color{0.72F, 0.78F, 0.88F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.grid", "textures/falling_blocks/grid", Color{0.18F, 0.22F, 0.28F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.hud", "textures/falling_blocks/hud", Color{0.84F, 0.88F, 0.94F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_i", "textures/falling_blocks/piece_i", BuildPieceTint(FallingBlocksPieceKind::I)});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_o", "textures/falling_blocks/piece_o", BuildPieceTint(FallingBlocksPieceKind::O)});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_t", "textures/falling_blocks/piece_t", BuildPieceTint(FallingBlocksPieceKind::T)});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_s", "textures/falling_blocks/piece_s", BuildPieceTint(FallingBlocksPieceKind::S)});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_z", "textures/falling_blocks/piece_z", BuildPieceTint(FallingBlocksPieceKind::Z)});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_j", "textures/falling_blocks/piece_j", BuildPieceTint(FallingBlocksPieceKind::J)});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{"materials/falling_blocks.piece_l", "textures/falling_blocks/piece_l", BuildPieceTint(FallingBlocksPieceKind::L)});
}

void FallingBlocksFeatureLayer::SubmitSprite(
    const TickContext& context,
    std::string_view entityName,
    std::string_view materialName,
    const Vector2& position,
    const Vector2& size,
    const Color& tint,
    const int sortKey) const
{
    context.services.renderer->SubmitSprite(
        Sprite2DSubmission{
            kInvalidEntityId,
            std::string(entityName),
            std::string(materialName),
            position,
            0.0F,
            size,
            tint,
            sortKey});
}

void FallingBlocksFeatureLayer::SubmitBoard(const TickContext& context, const FallingBlocksSnapshot& snapshot) const
{
    SubmitSprite(
        context,
        "BoardBackdrop",
        "materials/falling_blocks.board",
        Vector2{0.0F, 0.0F},
        Vector2{10.8F, 20.8F},
        Color{0.12F, 0.14F, 0.18F, 1.0F},
        -100);

    SubmitSprite(
        context,
        "BoardFrameTop",
        "materials/falling_blocks.frame",
        Vector2{0.0F, 10.48F},
        Vector2{10.95F, 0.22F},
        Color{0.82F, 0.86F, 0.92F, 1.0F},
        -90);
    SubmitSprite(
        context,
        "BoardFrameBottom",
        "materials/falling_blocks.frame",
        Vector2{0.0F, -10.48F},
        Vector2{10.95F, 0.22F},
        Color{0.82F, 0.86F, 0.92F, 1.0F},
        -90);
    SubmitSprite(
        context,
        "BoardFrameLeft",
        "materials/falling_blocks.frame",
        Vector2{-5.48F, 0.0F},
        Vector2{0.22F, 20.95F},
        Color{0.82F, 0.86F, 0.92F, 1.0F},
        -90);
    SubmitSprite(
        context,
        "BoardFrameRight",
        "materials/falling_blocks.frame",
        Vector2{5.48F, 0.0F},
        Vector2{0.22F, 20.95F},
        Color{0.82F, 0.86F, 0.92F, 1.0F},
        -90);

    for (int column = 0; column <= FallingBlocksGame::kBoardWidth; ++column)
    {
        SubmitSprite(
            context,
            "BoardGridVertical",
            "materials/falling_blocks.grid",
            Vector2{-5.0F + static_cast<float>(column), 0.0F},
            Vector2{kGridLineThickness, 20.0F},
            Color{0.18F, 0.22F, 0.28F, 0.80F},
            -80);
    }

    for (int row = 0; row <= FallingBlocksGame::kBoardHeight; ++row)
    {
        SubmitSprite(
            context,
            "BoardGridHorizontal",
            "materials/falling_blocks.grid",
            Vector2{0.0F, 10.0F - static_cast<float>(row)},
            Vector2{10.0F, kGridLineThickness},
            Color{0.18F, 0.22F, 0.28F, 0.80F},
            -80);
    }

    for (int row = 0; row < FallingBlocksGame::kBoardHeight; ++row)
    {
        for (int column = 0; column < FallingBlocksGame::kBoardWidth; ++column)
        {
            const FallingBlocksPieceKind kind = snapshot.lockedCells[row * FallingBlocksGame::kBoardWidth + column];
            if (kind == FallingBlocksPieceKind::None)
            {
                continue;
            }

            SubmitSprite(
                context,
                "LockedCell",
                GetPieceMaterialName(kind),
                BuildBoardCellCenter(column, row),
                Vector2{0.92F, 0.92F},
                BuildPieceTint(kind),
                10);
        }
    }

    for (const FallingBlocksVisibleCell& cell : snapshot.activeCells)
    {
        if (cell.kind == FallingBlocksPieceKind::None || cell.y < 0)
        {
            continue;
        }

        const Color tint = BuildPieceTint(cell.kind);
        SubmitSprite(
            context,
            "ActiveCell",
            GetPieceMaterialName(cell.kind),
            BuildBoardCellCenter(cell.x, cell.y),
            Vector2{0.94F, 0.94F},
            Color{
                std::min(1.0F, tint.r + 0.10F),
                std::min(1.0F, tint.g + 0.10F),
                std::min(1.0F, tint.b + 0.10F),
                1.0F},
            20);
    }
}

void FallingBlocksFeatureLayer::SubmitHud(const TickContext& context, const FallingBlocksSnapshot& snapshot) const
{
    SubmitSprite(
        context,
        "HudPanelMain",
        "materials/falling_blocks.hud",
        Vector2{kHudPanelCenterX, 4.3F},
        Vector2{7.2F, 11.8F},
        Color{0.11F, 0.13F, 0.18F, 0.95F},
        -70);
    SubmitSprite(
        context,
        "HudPanelNext",
        "materials/falling_blocks.hud",
        Vector2{kHudPanelCenterX, -6.6F},
        Vector2{7.2F, 5.8F},
        Color{0.11F, 0.13F, 0.18F, 0.95F},
        -70);

    const Color bannerTint = snapshot.phase == FallingBlocksGamePhase::GameOver
                                 ? Color{0.78F, 0.16F, 0.20F, 1.0F}
                                 : Color{0.20F, 0.56F, 0.90F, 1.0F};
    SubmitSprite(
        context,
        "HudStatusBanner",
        "materials/falling_blocks.frame",
        Vector2{kHudPanelCenterX, 9.25F},
        Vector2{6.6F, 1.2F},
        bannerTint,
        -60);

    SubmitDotText(context, "SCORE", Vector2{6.2F, 7.9F}, 0.17F, Color{0.86F, 0.90F, 0.97F, 1.0F}, -40);
    SubmitDotText(context, "LINES", Vector2{6.2F, 4.55F}, 0.17F, Color{0.59F, 0.89F, 0.96F, 1.0F}, -40);
    SubmitDotText(context, "LEVEL", Vector2{6.2F, 1.45F}, 0.17F, Color{0.99F, 0.81F, 0.45F, 1.0F}, -40);
    SubmitDotText(context, "NEXT", Vector2{7.3F, -4.65F}, 0.17F, Color{0.86F, 0.90F, 0.97F, 1.0F}, -40);

    SubmitSevenSegmentNumber(
        context,
        std::clamp(snapshot.score, 0, 999999),
        6,
        Vector2{6.55F, 6.45F},
        0.72F,
        Color{0.94F, 0.96F, 1.0F, 1.0F},
        Color{0.20F, 0.22F, 0.27F, 0.90F},
        -20);
    SubmitSevenSegmentNumber(
        context,
        std::clamp(snapshot.linesCleared, 0, 999),
        3,
        Vector2{8.0F, 3.05F},
        0.72F,
        Color{0.62F, 0.92F, 0.98F, 1.0F},
        Color{0.18F, 0.22F, 0.27F, 0.90F},
        -20);
    SubmitSevenSegmentNumber(
        context,
        std::clamp(snapshot.level, 0, 99),
        2,
        Vector2{8.75F, -0.05F},
        0.72F,
        Color{1.0F, 0.82F, 0.48F, 1.0F},
        Color{0.18F, 0.22F, 0.27F, 0.90F},
        -20);

    int minPreviewX = 999;
    int minPreviewY = 999;
    for (const FallingBlocksVisibleCell& cell : snapshot.nextPreviewCells)
    {
        if (cell.kind == FallingBlocksPieceKind::None)
        {
            continue;
        }
        minPreviewX = std::min(minPreviewX, cell.x);
        minPreviewY = std::min(minPreviewY, cell.y);
    }

    for (const FallingBlocksVisibleCell& cell : snapshot.nextPreviewCells)
    {
        if (cell.kind == FallingBlocksPieceKind::None)
        {
            continue;
        }

        const float previewX = 7.25F + static_cast<float>(cell.x - minPreviewX);
        const float previewY = -7.55F - static_cast<float>(cell.y - minPreviewY);
        SubmitSprite(
            context,
            "NextPieceCell",
            GetPieceMaterialName(cell.kind),
            Vector2{previewX, previewY},
            Vector2{0.82F, 0.82F},
            BuildPieceTint(cell.kind),
            10);
    }

    if (snapshot.phase == FallingBlocksGamePhase::GameOver)
    {
        SubmitDotText(context, "GAME OVER", Vector2{6.25F, 9.55F}, 0.15F, Color{1.0F, 0.95F, 0.95F, 1.0F}, -10);
    }
}

void FallingBlocksFeatureLayer::SubmitSevenSegmentNumber(
    const TickContext& context,
    const int value,
    const int digitCount,
    const Vector2& anchor,
    const float scale,
    const Color& litTint,
    const Color& unlitTint,
    const int sortKey) const
{
    std::ostringstream stream;
    stream << std::setw(digitCount) << std::setfill('0') << value;
    const std::string digits = stream.str();

    for (int index = 0; index < digitCount; ++index)
    {
        const float x = anchor.x + static_cast<float>(index) * (0.9F * scale);
        SubmitSevenSegmentDigit(
            context,
            digits[static_cast<std::size_t>(index)] - '0',
            Vector2{x, anchor.y},
            scale,
            litTint,
            unlitTint,
            sortKey);
    }
}

void FallingBlocksFeatureLayer::SubmitSevenSegmentDigit(
    const TickContext& context,
    const int digit,
    const Vector2& center,
    const float scale,
    const Color& litTint,
    const Color& unlitTint,
    const int sortKey) const
{
    const std::array<bool, 7> segments = BuildDigitSegments(digit);
    const float horizontalWidth = 0.56F * scale;
    const float horizontalHeight = 0.12F * scale;
    const float verticalWidth = 0.12F * scale;
    const float verticalHeight = 0.48F * scale;
    const std::array<Vector2, 7> offsets = {
        Vector2{0.0F, 0.62F * scale},
        Vector2{0.29F * scale, 0.31F * scale},
        Vector2{0.29F * scale, -0.31F * scale},
        Vector2{0.0F, -0.62F * scale},
        Vector2{-0.29F * scale, -0.31F * scale},
        Vector2{-0.29F * scale, 0.31F * scale},
        Vector2{0.0F, 0.0F}};

    for (int index = 0; index < 7; ++index)
    {
        const bool horizontal = index == 0 || index == 3 || index == 6;
        SubmitSprite(
            context,
            "HudDigitSegment",
            "materials/falling_blocks.hud",
            Vector2{
                center.x + offsets[static_cast<std::size_t>(index)].x,
                center.y + offsets[static_cast<std::size_t>(index)].y},
            horizontal ? Vector2{horizontalWidth, horizontalHeight} : Vector2{verticalWidth, verticalHeight},
            segments[static_cast<std::size_t>(index)] ? litTint : unlitTint,
            sortKey);
    }
}

void FallingBlocksFeatureLayer::SubmitDotText(
    const TickContext& context,
    const std::string_view text,
    const Vector2& topLeft,
    const float cellSize,
    const Color& tint,
    const int sortKey) const
{
    float cursorX = topLeft.x;
    for (const char rawCharacter : text)
    {
        const char character = rawCharacter >= 'a' && rawCharacter <= 'z'
                                   ? static_cast<char>(rawCharacter - ('a' - 'A'))
                                   : rawCharacter;
        const DotGlyph glyph = GetGlyph(character);
        for (std::size_t row = 0; row < glyph.size(); ++row)
        {
            for (std::size_t column = 0; column < glyph[row].size(); ++column)
            {
                if (glyph[row][column] != '#')
                {
                    continue;
                }

                SubmitSprite(
                    context,
                    "HudDotGlyph",
                    "materials/falling_blocks.hud",
                    Vector2{
                        cursorX + static_cast<float>(column) * cellSize,
                        topLeft.y - static_cast<float>(row) * cellSize},
                    Vector2{cellSize * 0.82F, cellSize * 0.82F},
                    tint,
                    sortKey);
            }
        }
        cursorX += cellSize * 6.0F;
    }
}
} // namespace she
