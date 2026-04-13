#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace she
{
enum class FallingBlocksPieceKind
{
    None = 0,
    I,
    O,
    T,
    S,
    Z,
    J,
    L
};

enum class FallingBlocksGamePhase
{
    Playing,
    GameOver
};

enum class FallingBlocksAction
{
    MoveLeft,
    MoveRight,
    RotateClockwise,
    SoftDrop,
    HardDrop,
    StepGravity,
    Restart
};

struct FallingBlocksCellPosition
{
    int x = 0;
    int y = 0;
};

struct FallingBlocksVisibleCell
{
    int x = 0;
    int y = 0;
    FallingBlocksPieceKind kind = FallingBlocksPieceKind::None;
};

struct FallingBlocksActivePiece
{
    FallingBlocksPieceKind kind = FallingBlocksPieceKind::None;
    int rotation = 0;
    int originX = 0;
    int originY = 0;
};

struct FallingBlocksActionResult
{
    bool stateChanged = false;
    bool pieceMoved = false;
    bool pieceRotated = false;
    bool pieceLocked = false;
    bool spawnedNextPiece = false;
    bool triggeredGameOver = false;
    int droppedRows = 0;
    int clearedLineCount = 0;
    int scoreDelta = 0;
};

struct FallingBlocksSnapshot
{
    static constexpr int kBoardWidth = 10;
    static constexpr int kBoardHeight = 20;
    static constexpr int kBoardCellCount = kBoardWidth * kBoardHeight;

    FallingBlocksGamePhase phase = FallingBlocksGamePhase::Playing;
    int score = 0;
    int linesCleared = 0;
    int level = 1;
    std::array<FallingBlocksPieceKind, kBoardCellCount> lockedCells{};
    FallingBlocksActivePiece activePiece{};
    FallingBlocksPieceKind nextPiece = FallingBlocksPieceKind::None;
    std::array<FallingBlocksVisibleCell, 4> activeCells{};
    std::array<FallingBlocksVisibleCell, 4> nextPreviewCells{};
    std::string latestStatus;
};

[[nodiscard]] const char* GetFallingBlocksPieceName(FallingBlocksPieceKind kind);
[[nodiscard]] const char* GetFallingBlocksPhaseName(FallingBlocksGamePhase phase);

class FallingBlocksGame final
{
public:
    static constexpr int kBoardWidth = FallingBlocksSnapshot::kBoardWidth;
    static constexpr int kBoardHeight = FallingBlocksSnapshot::kBoardHeight;
    static constexpr int kBoardCellCount = FallingBlocksSnapshot::kBoardCellCount;

    FallingBlocksGame();

    void Reset(std::uint32_t seed = 0x17U);
    [[nodiscard]] FallingBlocksActionResult ApplyAction(FallingBlocksAction action);
    [[nodiscard]] FallingBlocksSnapshot BuildSnapshot() const;
    [[nodiscard]] double GetGravityIntervalSeconds() const;

    void DebugClearBoard();
    bool DebugSetCell(int x, int y, FallingBlocksPieceKind kind);
    bool DebugSetActivePiece(FallingBlocksPieceKind kind, int rotation, int originX, int originY);
    bool DebugSpawnPiece(FallingBlocksPieceKind kind);
    void DebugSetNextQueue(const std::vector<FallingBlocksPieceKind>& queue);

private:
    [[nodiscard]] bool IsInsideBoard(int x, int y) const;
    [[nodiscard]] bool IsCellOccupied(int x, int y) const;
    [[nodiscard]] bool CanPlacePiece(const FallingBlocksActivePiece& piece) const;
    [[nodiscard]] std::array<FallingBlocksVisibleCell, 4> BuildVisibleCells(const FallingBlocksActivePiece& piece) const;
    [[nodiscard]] FallingBlocksPieceKind DrawNextPiece();
    void RefillBagIfNeeded();
    [[nodiscard]] FallingBlocksActionResult TryMovePiece(int deltaX, int deltaY, bool rewardSoftDrop);
    [[nodiscard]] FallingBlocksActionResult TryRotatePieceClockwise();
    [[nodiscard]] FallingBlocksActionResult HardDropPiece();
    [[nodiscard]] FallingBlocksActionResult LockActivePiece();
    [[nodiscard]] int ClearCompletedLines();
    [[nodiscard]] bool SpawnNextActivePiece(FallingBlocksPieceKind kind);
    [[nodiscard]] FallingBlocksActivePiece BuildSpawnPiece(FallingBlocksPieceKind kind) const;
    [[nodiscard]] int BoardIndex(int x, int y) const;

    std::array<FallingBlocksPieceKind, kBoardCellCount> m_lockedCells{};
    FallingBlocksActivePiece m_activePiece{};
    FallingBlocksPieceKind m_nextPiece = FallingBlocksPieceKind::None;
    FallingBlocksGamePhase m_phase = FallingBlocksGamePhase::Playing;
    int m_score = 0;
    int m_linesCleared = 0;
    int m_level = 1;
    std::uint32_t m_seed = 0x17U;
    std::vector<FallingBlocksPieceKind> m_bag;
    std::string m_latestStatus = "Stack steady.";
};
} // namespace she
