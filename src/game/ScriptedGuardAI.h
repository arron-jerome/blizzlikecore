/*
 * BlizzLikeCore Copyright (C) 2013  BlizzLikeGroup
 * Integrated Files: CREDITS.md and LICENSE.md
 */

#ifndef SC_GUARDAI_H
#define SC_GUARDAI_H

#define GENERIC_CREATURE_COOLDOWN 5000

struct guardAI : public ScriptedAI
{
    guardAI(Creature* c) : ScriptedAI(c) {}

    uint32 GlobalCooldown;                                  //This variable acts like the global cooldown that players have (1.5 seconds)
    uint32 BuffTimer;                                       //This variable keeps track of buffs

    void Reset();

    void EnterCombat(Unit* /*who*/);

    void JustDied(Unit* Killer);

    void UpdateAI(const uint32 diff);

    //common used for guards in main cities
    void DoReplyToTextEmote(uint32 em);
};

struct guardAI_orgrimmar : public guardAI
{
    guardAI_orgrimmar(Creature* c) : guardAI(c) {}

    void ReceiveEmote(Player* player, uint32 text_emote);
};

struct guardAI_stormwind : public guardAI
{
    guardAI_stormwind(Creature* c) : guardAI(c) {}

    void ReceiveEmote(Player* player, uint32 text_emote);
};
#endif

