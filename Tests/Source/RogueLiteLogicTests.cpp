#include <SDL3/SDL_main.h>

#include "RogueLite/RogueLiteGame.hpp"

#include <iostream>
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

bool TestRoomClearOpensPortalAndSpawnsReward()
{
    she::RogueLiteGame game;
    game.Reset(0x61U);
    game.DebugForceRoomCleared();

    const she::RogueLiteSnapshot snapshot = game.BuildSnapshot();
    bool passed = true;
    passed &= Expect(
        snapshot.phase == she::RogueLiteGamePhase::RoomClear,
        "Forced room clear should move the run into the room-clear phase.");
    passed &= Expect(snapshot.portalOpen, "Forced room clear should open the portal.");
    passed &= Expect(!snapshot.pickups.empty(), "Forced room clear should spawn reward pickups.");
    return passed;
}

bool TestProjectileFireCanDefeatEnemyAndAwardScrap()
{
    she::RogueLiteGame game;
    game.Reset(0x62U);
    game.DebugClearEnemies();
    game.DebugSpawnEnemy(she::RogueLiteEnemyKind::Grunt, she::Vector2{4.5F, -4.4F});

    bool enemyDefeated = false;
    for (int step = 0; step < 40; ++step)
    {
        if (step % 6 == 0)
        {
            static_cast<void>(game.ApplyAction(she::RogueLiteAction::FireRight));
        }

        const she::RogueLiteUpdateResult result = game.Update(0.05, she::Vector2{0.0F, 0.0F});
        const she::RogueLiteSnapshot snapshot = game.BuildSnapshot();
        if (result.defeatedEnemies > 0)
        {
            enemyDefeated = true;
            if (!Expect(snapshot.scrap > 0, "Defeating an enemy should award scrap."))
            {
                return false;
            }
            break;
        }
    }

    return Expect(enemyDefeated, "Player projectiles should be able to defeat a spawned grunt.");
}

bool TestGameOverAndRestartRecover()
{
    she::RogueLiteGame game;
    game.Reset(0x63U);
    game.DebugDamagePlayer(99);

    const she::RogueLiteSnapshot gameOverSnapshot = game.BuildSnapshot();
    bool passed = true;
    passed &= Expect(
        gameOverSnapshot.phase == she::RogueLiteGamePhase::GameOver,
        "Fatal player damage should trigger game over.");

    const she::RogueLiteActionResult restartResult = game.ApplyAction(she::RogueLiteAction::Restart);
    const she::RogueLiteSnapshot restartedSnapshot = game.BuildSnapshot();
    passed &= Expect(restartResult.stateChanged, "Restart should report a state change.");
    passed &= Expect(
        restartedSnapshot.phase == she::RogueLiteGamePhase::Combat,
        "Restart should return the run to combat.");
    passed &= Expect(restartedSnapshot.playerHitPoints == restartedSnapshot.playerMaxHitPoints, "Restart should refill health.");
    passed &= Expect(restartedSnapshot.roomIndex == 0, "Restart should return to room one.");
    passed &= Expect(restartedSnapshot.scrap == 0, "Restart should clear scrap.");
    return passed;
}
} // namespace

int main(int, char**)
{
    return TestRoomClearOpensPortalAndSpawnsReward() && TestProjectileFireCanDefeatEnemyAndAwardScrap() &&
                   TestGameOverAndRestartRecover()
               ? 0
               : 1;
}
