/*
 * BlizzLikeCore Copyright (C) 2013  BlizzLikeGroup
 * Integrated Files: CREDITS.md and LICENSE.md
 */

#ifndef BLIZZLIKE_CREATUREAISELECTOR_H
#define BLIZZLIKE_CREATUREAISELECTOR_H

class CreatureAI;
class Creature;
class MovementGenerator;

namespace FactorySelector
{
    CreatureAI* selectAI(Creature* );
    MovementGenerator* selectMovementGenerator(Creature* );
}
#endif

