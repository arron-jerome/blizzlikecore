/*
 * BlizzLikeCore Copyright (C) 2013  BlizzLikeGroup
 * Integrated Files: CREDITS.md and LICENSE.md
 */

#include "IdleMovementGenerator.h"
#include "CreatureAI.h"
#include "Creature.h"

IdleMovementGenerator si_idleMovement;

// StopMoving is needed to make unit stop if its last movement generator expires
// But it should not be sent otherwise there are many redundent packets
void IdleMovementGenerator::Initialize(Unit &owner)
{
    if (owner.hasUnitState(UNIT_STAT_MOVE))
        owner.StopMoving();
}

void
IdleMovementGenerator::Reset(Unit& owner)
{
    if (owner.hasUnitState(UNIT_STAT_MOVE))
        owner.StopMoving();
}

void RotateMovementGenerator::Initialize(Unit& owner)
{
    if (owner.hasUnitState(UNIT_STAT_MOVE))
        owner.StopMoving();

    if (owner.getVictim())
        owner.SetInFront(owner.getVictim());

    owner.addUnitState(UNIT_STAT_ROTATING);

    owner.AttackStop();
}

bool RotateMovementGenerator::Update(Unit& owner, const uint32& diff)
{
    float angle = owner.GetOrientation();
    if (m_direction == ROTATE_DIRECTION_LEFT)
    {
        angle += (float)diff * M_PI * 2 / m_maxDuration;
        while (angle >= M_PI * 2) angle -= M_PI * 2;
    }
    else
    {
        angle -= (float)diff * M_PI * 2 / m_maxDuration;
        while (angle < 0) angle += M_PI * 2;
    }
    owner.SetOrientation(angle);
    owner.SendMovementFlagUpdate(); // this is a hack. we do not have anything correct to send in the beginning

    if (m_duration > diff)
        m_duration -= diff;
    else
        return false;

    return true;
}

void RotateMovementGenerator::Finalize(Unit &unit)
{
    unit.clearUnitState(UNIT_STAT_ROTATING);
    if (unit.GetTypeId() == TYPEID_UNIT)
        unit.ToCreature()->AI()->MovementInform(ROTATE_MOTION_TYPE, 0);
}

void
DistractMovementGenerator::Initialize(Unit& owner)
{
    owner.addUnitState(UNIT_STAT_DISTRACTED);
}

void
DistractMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_DISTRACTED);
}

bool
DistractMovementGenerator::Update(Unit& /*owner*/, const uint32& time_diff)
{
    if (time_diff > m_timer)
        return false;

    m_timer -= time_diff;
    return true;
}

void
AssistanceDistractMovementGenerator::Finalize(Unit &unit)
{
    unit.clearUnitState(UNIT_STAT_DISTRACTED);
    unit.ToCreature()->SetReactState(REACT_AGGRESSIVE);
}

