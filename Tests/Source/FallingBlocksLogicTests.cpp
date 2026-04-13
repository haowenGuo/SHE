#include <SDL3/SDL_main.h>

#include "FallingBlocks/FallingBlocksGame.hpp"

#include <iostream>
#include <set>
#include <string_view>

namespace
{
bool Expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }

    return true;
}

bool TestSevenBagCoversEveryPieceBeforeRepeat()
{
    she::FallingBlocksGame game;
    game.Reset(0x33U);

    std::set<she::FallingBlocksPieceKind> seenKinds;
    for (int index = 0; index < 7; ++index)
    {
        const she::FallingBlocksSnapshot snapshot = game.BuildSnapshot();
        seenKinds.insert(snapshot.activePiece.kind);
        static_cast<void>(game.ApplyAction(she::FallingBlocksAction::HardDrop));
    }

    return Expect(seenKinds.size() == 7, "Seven-bag sequence should cover all seven pieces before repeating.");
}

bool TestLineClearUpdatesScoreAndLineCount()
{
    she::FallingBlocksGame game;
    game.Reset(0x44U);
    game.DebugClearBoard();

    for (int column = 0; column < 6; ++column)
    {
        if (!game.DebugSetCell(column, 19, she::FallingBlocksPieceKind::J))
        {
            return Expect(false, "Failed to seed the bottom row for the line-clear test.");
        }
    }

    if (!game.DebugSetActivePiece(she::FallingBlocksPieceKind::I, 0, 6, 0))
    {
        return Expect(false, "Failed to position the I piece for the line-clear test.");
    }

    const she::FallingBlocksActionResult result = game.ApplyAction(she::FallingBlocksAction::HardDrop);
    const she::FallingBlocksSnapshot snapshot = game.BuildSnapshot();

    bool passed = true;
    passed &= Expect(result.clearedLineCount == 1, "Hard drop should clear exactly one line in the prepared setup.");
    passed &= Expect(snapshot.linesCleared == 1, "Game state should record the cleared line.");
    passed &= Expect(snapshot.score >= 100, "Single-line clear should award score.");
    passed &= Expect(snapshot.phase == she::FallingBlocksGamePhase::Playing, "Line clear should keep the run alive.");
    return passed;
}

bool TestGameOverAndRestartRecover()
{
    she::FallingBlocksGame game;
    game.Reset(0x55U);
    game.DebugClearBoard();

    static_cast<void>(game.DebugSetCell(4, 0, she::FallingBlocksPieceKind::Z));
    static_cast<void>(game.DebugSetCell(5, 0, she::FallingBlocksPieceKind::S));

    const bool spawned = game.DebugSpawnPiece(she::FallingBlocksPieceKind::O);
    const she::FallingBlocksSnapshot blockedSnapshot = game.BuildSnapshot();
    bool passed = true;
    passed &= Expect(!spawned, "Spawn should fail when the top entry cells are blocked.");
    passed &= Expect(
        blockedSnapshot.phase == she::FallingBlocksGamePhase::GameOver,
        "Blocked spawn should transition the game into game-over.");

    const she::FallingBlocksActionResult restartResult = game.ApplyAction(she::FallingBlocksAction::Restart);
    const she::FallingBlocksSnapshot restartedSnapshot = game.BuildSnapshot();
    passed &= Expect(restartResult.stateChanged, "Restart action should report a state change.");
    passed &= Expect(
        restartedSnapshot.phase == she::FallingBlocksGamePhase::Playing,
        "Restart should return the game to the playing state.");
    passed &= Expect(restartedSnapshot.score == 0, "Restart should clear score.");
    passed &= Expect(restartedSnapshot.linesCleared == 0, "Restart should clear line count.");
    passed &= Expect(
        restartedSnapshot.activePiece.kind != she::FallingBlocksPieceKind::None,
        "Restart should spawn a fresh active piece.");
    return passed;
}
} // namespace

int main(int, char**)
{
    return TestSevenBagCoversEveryPieceBeforeRepeat() && TestLineClearUpdatesScoreAndLineCount() &&
                   TestGameOverAndRestartRecover()
               ? 0
               : 1;
}
