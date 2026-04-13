#include "FallingBlocksGame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <sstream>

namespace she
{
namespace
{
struct PieceRotation
{
    std::array<FallingBlocksCellPosition, 4> cells{};
};

constexpr std::array<FallingBlocksPieceKind, 7> kAllPieceKinds = {
    FallingBlocksPieceKind::I,
    FallingBlocksPieceKind::O,
    FallingBlocksPieceKind::T,
    FallingBlocksPieceKind::S,
    FallingBlocksPieceKind::Z,
    FallingBlocksPieceKind::J,
    FallingBlocksPieceKind::L};

constexpr std::array<PieceRotation, 4> kIPieceRotations = {{
    {{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}}},
    {{{{2, 0}, {2, 1}, {2, 2}, {2, 3}}}},
    {{{{0, 2}, {1, 2}, {2, 2}, {3, 2}}}},
    {{{{1, 0}, {1, 1}, {1, 2}, {1, 3}}}},
}};

constexpr std::array<PieceRotation, 4> kOPieceRotations = {{
    {{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}},
    {{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}},
    {{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}},
    {{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}},
}};

constexpr std::array<PieceRotation, 4> kTPieceRotations = {{
    {{{{1, 0}, {0, 1}, {1, 1}, {2, 1}}}},
    {{{{1, 0}, {1, 1}, {2, 1}, {1, 2}}}},
    {{{{0, 1}, {1, 1}, {2, 1}, {1, 2}}}},
    {{{{1, 0}, {0, 1}, {1, 1}, {1, 2}}}},
}};

constexpr std::array<PieceRotation, 4> kSPieceRotations = {{
    {{{{1, 0}, {2, 0}, {0, 1}, {1, 1}}}},
    {{{{1, 0}, {1, 1}, {2, 1}, {2, 2}}}},
    {{{{1, 1}, {2, 1}, {0, 2}, {1, 2}}}},
    {{{{0, 0}, {0, 1}, {1, 1}, {1, 2}}}},
}};

constexpr std::array<PieceRotation, 4> kZPieceRotations = {{
    {{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}},
    {{{{2, 0}, {1, 1}, {2, 1}, {1, 2}}}},
    {{{{0, 1}, {1, 1}, {1, 2}, {2, 2}}}},
    {{{{1, 0}, {0, 1}, {1, 1}, {0, 2}}}},
}};

constexpr std::array<PieceRotation, 4> kJPieceRotations = {{
    {{{{0, 0}, {0, 1}, {1, 1}, {2, 1}}}},
    {{{{1, 0}, {2, 0}, {1, 1}, {1, 2}}}},
    {{{{0, 1}, {1, 1}, {2, 1}, {2, 2}}}},
    {{{{1, 0}, {1, 1}, {0, 2}, {1, 2}}}},
}};

constexpr std::array<PieceRotation, 4> kLPieceRotations = {{
    {{{{2, 0}, {0, 1}, {1, 1}, {2, 1}}}},
    {{{{1, 0}, {1, 1}, {1, 2}, {2, 2}}}},
    {{{{0, 1}, {1, 1}, {2, 1}, {0, 2}}}},
    {{{{0, 0}, {1, 0}, {1, 1}, {1, 2}}}},
}};

constexpr std::array<FallingBlocksCellPosition, 7> kRotationKickOffsets = {
    FallingBlocksCellPosition{0, 0},
    FallingBlocksCellPosition{-1, 0},
    FallingBlocksCellPosition{1, 0},
    FallingBlocksCellPosition{-2, 0},
    FallingBlocksCellPosition{2, 0},
    FallingBlocksCellPosition{0, -1},
    FallingBlocksCellPosition{0, -2}};

[[nodiscard]] const std::array<PieceRotation, 4>& GetPieceRotations(const FallingBlocksPieceKind kind)
{
    switch (kind)
    {
    case FallingBlocksPieceKind::I:
        return kIPieceRotations;
    case FallingBlocksPieceKind::O:
        return kOPieceRotations;
    case FallingBlocksPieceKind::T:
        return kTPieceRotations;
    case FallingBlocksPieceKind::S:
        return kSPieceRotations;
    case FallingBlocksPieceKind::Z:
        return kZPieceRotations;
    case FallingBlocksPieceKind::J:
        return kJPieceRotations;
    case FallingBlocksPieceKind::L:
        return kLPieceRotations;
    case FallingBlocksPieceKind::None:
    default:
        return kOPieceRotations;
    }
}

[[nodiscard]] int WrapRotation(const int rotation)
{
    const int wrapped = rotation % 4;
    return wrapped < 0 ? wrapped + 4 : wrapped;
}

[[nodiscard]] int BaseScoreForLineClear(const int clearedLines, const int level)
{
    switch (clearedLines)
    {
    case 1:
        return 100 * level;
    case 2:
        return 300 * level;
    case 3:
        return 500 * level;
    case 4:
        return 800 * level;
    default:
        return 0;
    }
}
} // namespace

const char* GetFallingBlocksPieceName(const FallingBlocksPieceKind kind)
{
    switch (kind)
    {
    case FallingBlocksPieceKind::I:
        return "I";
    case FallingBlocksPieceKind::O:
        return "O";
    case FallingBlocksPieceKind::T:
        return "T";
    case FallingBlocksPieceKind::S:
        return "S";
    case FallingBlocksPieceKind::Z:
        return "Z";
    case FallingBlocksPieceKind::J:
        return "J";
    case FallingBlocksPieceKind::L:
        return "L";
    case FallingBlocksPieceKind::None:
    default:
        return "None";
    }
}

const char* GetFallingBlocksPhaseName(const FallingBlocksGamePhase phase)
{
    switch (phase)
    {
    case FallingBlocksGamePhase::Playing:
        return "Playing";
    case FallingBlocksGamePhase::GameOver:
        return "GameOver";
    default:
        return "Unknown";
    }
}

FallingBlocksGame::FallingBlocksGame()
{
    Reset();
}

void FallingBlocksGame::Reset(const std::uint32_t seed)
{
    m_seed = seed;
    m_lockedCells.fill(FallingBlocksPieceKind::None);
    m_bag.clear();
    m_phase = FallingBlocksGamePhase::Playing;
    m_score = 0;
    m_linesCleared = 0;
    m_level = 1;
    m_latestStatus = "Stack steady. Clear lines and keep the ceiling open.";
    m_activePiece = {};
    m_nextPiece = FallingBlocksPieceKind::None;

    const FallingBlocksPieceKind openingPiece = DrawNextPiece();
    m_nextPiece = DrawNextPiece();
    if (!SpawnNextActivePiece(openingPiece))
    {
        m_phase = FallingBlocksGamePhase::GameOver;
        m_latestStatus = "Game over. Press R to restart.";
        return;
    }

    std::ostringstream stream;
    stream << "Spawned " << GetFallingBlocksPieceName(m_activePiece.kind) << ".";
    m_latestStatus = stream.str();
}

FallingBlocksActionResult FallingBlocksGame::ApplyAction(const FallingBlocksAction action)
{
    if (action == FallingBlocksAction::Restart)
    {
        Reset(m_seed);
        return FallingBlocksActionResult{
            true,
            false,
            false,
            false,
            true,
            false,
            0,
            0,
            0};
    }

    if (m_phase == FallingBlocksGamePhase::GameOver || m_activePiece.kind == FallingBlocksPieceKind::None)
    {
        return {};
    }

    switch (action)
    {
    case FallingBlocksAction::MoveLeft:
        return TryMovePiece(-1, 0, false);
    case FallingBlocksAction::MoveRight:
        return TryMovePiece(1, 0, false);
    case FallingBlocksAction::RotateClockwise:
        return TryRotatePieceClockwise();
    case FallingBlocksAction::SoftDrop:
        return TryMovePiece(0, 1, true);
    case FallingBlocksAction::HardDrop:
        return HardDropPiece();
    case FallingBlocksAction::StepGravity:
        return TryMovePiece(0, 1, false);
    case FallingBlocksAction::Restart:
    default:
        return {};
    }
}

FallingBlocksSnapshot FallingBlocksGame::BuildSnapshot() const
{
    FallingBlocksSnapshot snapshot;
    snapshot.phase = m_phase;
    snapshot.score = m_score;
    snapshot.linesCleared = m_linesCleared;
    snapshot.level = m_level;
    snapshot.lockedCells = m_lockedCells;
    snapshot.activePiece = m_activePiece;
    snapshot.nextPiece = m_nextPiece;
    snapshot.activeCells = BuildVisibleCells(m_activePiece);
    snapshot.latestStatus = m_latestStatus;

    const FallingBlocksActivePiece nextPreviewPiece{
        m_nextPiece,
        0,
        0,
        0};
    snapshot.nextPreviewCells = BuildVisibleCells(nextPreviewPiece);
    return snapshot;
}

double FallingBlocksGame::GetGravityIntervalSeconds() const
{
    const double interval = 0.78 - static_cast<double>(m_level - 1) * 0.055;
    return std::max(0.08, interval);
}

void FallingBlocksGame::DebugClearBoard()
{
    m_lockedCells.fill(FallingBlocksPieceKind::None);
    m_phase = FallingBlocksGamePhase::Playing;
    m_latestStatus = "Debug board cleared.";
}

bool FallingBlocksGame::DebugSetCell(const int x, const int y, const FallingBlocksPieceKind kind)
{
    if (!IsInsideBoard(x, y))
    {
        return false;
    }

    m_lockedCells[BoardIndex(x, y)] = kind;
    return true;
}

bool FallingBlocksGame::DebugSetActivePiece(
    const FallingBlocksPieceKind kind,
    const int rotation,
    const int originX,
    const int originY)
{
    const FallingBlocksActivePiece candidate{
        kind,
        WrapRotation(rotation),
        originX,
        originY};
    if (!CanPlacePiece(candidate))
    {
        return false;
    }

    m_activePiece = candidate;
    m_phase = FallingBlocksGamePhase::Playing;
    return true;
}

bool FallingBlocksGame::DebugSpawnPiece(const FallingBlocksPieceKind kind)
{
    return SpawnNextActivePiece(kind);
}

void FallingBlocksGame::DebugSetNextQueue(const std::vector<FallingBlocksPieceKind>& queue)
{
    m_bag = queue;
    std::reverse(m_bag.begin(), m_bag.end());
}

bool FallingBlocksGame::IsInsideBoard(const int x, const int y) const
{
    return x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight;
}

bool FallingBlocksGame::IsCellOccupied(const int x, const int y) const
{
    if (!IsInsideBoard(x, y))
    {
        return false;
    }

    return m_lockedCells[BoardIndex(x, y)] != FallingBlocksPieceKind::None;
}

bool FallingBlocksGame::CanPlacePiece(const FallingBlocksActivePiece& piece) const
{
    if (piece.kind == FallingBlocksPieceKind::None)
    {
        return false;
    }

    const auto& rotation = GetPieceRotations(piece.kind)[WrapRotation(piece.rotation)];
    for (const FallingBlocksCellPosition& cell : rotation.cells)
    {
        const int x = piece.originX + cell.x;
        const int y = piece.originY + cell.y;
        if (x < 0 || x >= kBoardWidth || y >= kBoardHeight)
        {
            return false;
        }

        if (y >= 0 && IsCellOccupied(x, y))
        {
            return false;
        }
    }

    return true;
}

std::array<FallingBlocksVisibleCell, 4> FallingBlocksGame::BuildVisibleCells(const FallingBlocksActivePiece& piece) const
{
    std::array<FallingBlocksVisibleCell, 4> visibleCells{};
    if (piece.kind == FallingBlocksPieceKind::None)
    {
        return visibleCells;
    }

    const auto& rotation = GetPieceRotations(piece.kind)[WrapRotation(piece.rotation)];
    for (std::size_t index = 0; index < rotation.cells.size(); ++index)
    {
        visibleCells[index] = FallingBlocksVisibleCell{
            piece.originX + rotation.cells[index].x,
            piece.originY + rotation.cells[index].y,
            piece.kind};
    }

    return visibleCells;
}

FallingBlocksPieceKind FallingBlocksGame::DrawNextPiece()
{
    RefillBagIfNeeded();
    const FallingBlocksPieceKind nextPiece = m_bag.back();
    m_bag.pop_back();
    return nextPiece;
}

void FallingBlocksGame::RefillBagIfNeeded()
{
    if (!m_bag.empty())
    {
        return;
    }

    std::array<FallingBlocksPieceKind, 7> shuffledPieces = kAllPieceKinds;
    std::mt19937 random(m_seed++);
    std::shuffle(shuffledPieces.begin(), shuffledPieces.end(), random);
    m_bag.assign(shuffledPieces.begin(), shuffledPieces.end());
}

FallingBlocksActionResult FallingBlocksGame::TryMovePiece(
    const int deltaX,
    const int deltaY,
    const bool rewardSoftDrop)
{
    FallingBlocksActivePiece candidate = m_activePiece;
    candidate.originX += deltaX;
    candidate.originY += deltaY;
    if (CanPlacePiece(candidate))
    {
        m_activePiece = candidate;
        FallingBlocksActionResult result;
        result.stateChanged = true;
        result.pieceMoved = true;
        if (rewardSoftDrop && deltaY > 0)
        {
            result.droppedRows = deltaY;
            result.scoreDelta = deltaY;
            m_score += deltaY;
            std::ostringstream stream;
            stream << "Soft dropped " << deltaY << " row.";
            if (deltaY != 1)
            {
                stream << 's';
            }
            m_latestStatus = stream.str();
        }
        return result;
    }

    if (deltaY > 0)
    {
        return LockActivePiece();
    }

    return {};
}

FallingBlocksActionResult FallingBlocksGame::TryRotatePieceClockwise()
{
    FallingBlocksActivePiece candidate = m_activePiece;
    candidate.rotation = WrapRotation(candidate.rotation + 1);

    for (const FallingBlocksCellPosition& kick : kRotationKickOffsets)
    {
        candidate.originX = m_activePiece.originX + kick.x;
        candidate.originY = m_activePiece.originY + kick.y;
        if (CanPlacePiece(candidate))
        {
            m_activePiece = candidate;
            m_latestStatus = "Rotation accepted.";
            return FallingBlocksActionResult{
                true,
                false,
                true,
                false,
                false,
                false,
                0,
                0,
                0};
        }
    }

    return {};
}

FallingBlocksActionResult FallingBlocksGame::HardDropPiece()
{
    FallingBlocksActionResult result;
    while (true)
    {
        FallingBlocksActivePiece candidate = m_activePiece;
        candidate.originY += 1;
        if (!CanPlacePiece(candidate))
        {
            break;
        }

        m_activePiece = candidate;
        ++result.droppedRows;
    }

    if (result.droppedRows > 0)
    {
        result.stateChanged = true;
        result.pieceMoved = true;
        result.scoreDelta += result.droppedRows * 2;
        m_score += result.droppedRows * 2;
    }

    FallingBlocksActionResult lockResult = LockActivePiece();
    result.stateChanged = result.stateChanged || lockResult.stateChanged;
    result.pieceLocked = lockResult.pieceLocked;
    result.spawnedNextPiece = lockResult.spawnedNextPiece;
    result.triggeredGameOver = lockResult.triggeredGameOver;
    result.clearedLineCount = lockResult.clearedLineCount;
    result.scoreDelta += lockResult.scoreDelta;
    return result;
}

FallingBlocksActionResult FallingBlocksGame::LockActivePiece()
{
    FallingBlocksActionResult result;
    if (m_activePiece.kind == FallingBlocksPieceKind::None)
    {
        return result;
    }

    const auto visibleCells = BuildVisibleCells(m_activePiece);
    for (const FallingBlocksVisibleCell& cell : visibleCells)
    {
        if (cell.y < 0)
        {
            m_phase = FallingBlocksGamePhase::GameOver;
            m_activePiece = {};
            m_latestStatus = "Game over. Press R to restart.";
            result.stateChanged = true;
            result.pieceLocked = true;
            result.triggeredGameOver = true;
            return result;
        }

        m_lockedCells[BoardIndex(cell.x, cell.y)] = cell.kind;
    }

    result.stateChanged = true;
    result.pieceLocked = true;

    const int clearedLines = ClearCompletedLines();
    result.clearedLineCount = clearedLines;
    if (clearedLines > 0)
    {
        m_linesCleared += clearedLines;
        m_level = 1 + (m_linesCleared / 10);
        const int scoreGain = BaseScoreForLineClear(clearedLines, m_level);
        m_score += scoreGain;
        result.scoreDelta += scoreGain;
        std::ostringstream stream;
        stream << "Cleared " << clearedLines << " line";
        if (clearedLines != 1)
        {
            stream << 's';
        }
        stream << '.';
        m_latestStatus = stream.str();
    }

    const FallingBlocksPieceKind nextActivePiece = m_nextPiece;
    m_nextPiece = DrawNextPiece();
    result.spawnedNextPiece = SpawnNextActivePiece(nextActivePiece);
    result.triggeredGameOver = m_phase == FallingBlocksGamePhase::GameOver;

    if (!result.triggeredGameOver && clearedLines == 0)
    {
        std::ostringstream stream;
        stream << "Locked " << GetFallingBlocksPieceName(nextActivePiece) << " stack. Next "
               << GetFallingBlocksPieceName(m_nextPiece) << '.';
        m_latestStatus = stream.str();
    }

    return result;
}

int FallingBlocksGame::ClearCompletedLines()
{
    int clearedLineCount = 0;

    for (int row = kBoardHeight - 1; row >= 0; --row)
    {
        bool fullRow = true;
        for (int column = 0; column < kBoardWidth; ++column)
        {
            if (m_lockedCells[BoardIndex(column, row)] == FallingBlocksPieceKind::None)
            {
                fullRow = false;
                break;
            }
        }

        if (!fullRow)
        {
            continue;
        }

        ++clearedLineCount;
        for (int shiftRow = row; shiftRow > 0; --shiftRow)
        {
            for (int column = 0; column < kBoardWidth; ++column)
            {
                m_lockedCells[BoardIndex(column, shiftRow)] = m_lockedCells[BoardIndex(column, shiftRow - 1)];
            }
        }

        for (int column = 0; column < kBoardWidth; ++column)
        {
            m_lockedCells[BoardIndex(column, 0)] = FallingBlocksPieceKind::None;
        }

        ++row;
    }

    return clearedLineCount;
}

bool FallingBlocksGame::SpawnNextActivePiece(const FallingBlocksPieceKind kind)
{
    const FallingBlocksActivePiece candidate = BuildSpawnPiece(kind);
    if (!CanPlacePiece(candidate))
    {
        m_phase = FallingBlocksGamePhase::GameOver;
        m_activePiece = {};
        m_latestStatus = "Game over. Press R to restart.";
        return false;
    }

    m_phase = FallingBlocksGamePhase::Playing;
    m_activePiece = candidate;
    return true;
}

FallingBlocksActivePiece FallingBlocksGame::BuildSpawnPiece(const FallingBlocksPieceKind kind) const
{
    return FallingBlocksActivePiece{
        kind,
        0,
        3,
        -1};
}

int FallingBlocksGame::BoardIndex(const int x, const int y) const
{
    return y * kBoardWidth + x;
}
} // namespace she
