/*
 * Integral parts of this file: CREDITS.md and LICENSE.md
 */

/* ScriptData
Name: Boss_Selin_Fireheart
Complete(%): 90
Comment: Heroic and Normal Support. Needs further testing.
Category: Magister's Terrace
EndScriptData */

#include "ScriptPCH.h"
#include "magisters_terrace.h"

#define SAY_AGGRO                       -1585000
#define SAY_ENERGY                      -1585001
#define SAY_EMPOWERED                   -1585002
#define SAY_KILL_1                      -1585003
#define SAY_KILL_2                      -1585004
#define SAY_DEATH                       -1585005
#define EMOTE_CRYSTAL                   -1585006

//Crystal effect spells
#define SPELL_FEL_CRYSTAL_COSMETIC      44374
#define SPELL_FEL_CRYSTAL_DUMMY         44329
#define SPELL_FEL_CRYSTAL_VISUAL        44355
#define SPELL_MANA_RAGE                 44320               // This spell triggers 44321, which changes scale and regens mana Requires an entry in spell_script_target

//Selin's spells
#define SPELL_DRAIN_LIFE                44294
#define SPELL_FEL_EXPLOSION             44314

#define SPELL_DRAIN_MANA                46153               // Heroic only

#define CRYSTALS_NUMBER                 5
#define DATA_CRYSTALS                   6

#define CREATURE_FEL_CRYSTAL            24722

struct boss_selin_fireheartAI : public ScriptedAI
{
    boss_selin_fireheartAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = c->GetInstanceData();

        Crystals.clear();
        // GUIDs per instance is static, so we only need to load them once.
        if (pInstance)
        {
            uint32 size = pInstance->GetData(DATA_FEL_CRYSTAL_SIZE);
            for (uint8 i = 0; i < size; ++i)
            {
                uint64 guid = pInstance->GetData64(DATA_FEL_CRYSTAL);
                debug_log("BSCR: Selin: Adding Fel Crystal %llu to list", guid);
                Crystals.push_back(guid);
            }
        }
        Heroic = c->GetMap()->IsHeroic();
    }

    ScriptedInstance* pInstance;

    std::list<uint64> Crystals;

    uint32 DrainLifeTimer;
    uint32 DrainManaTimer;
    uint32 FelExplosionTimer;
    uint32 DrainCrystalTimer;
    uint32 EmpowerTimer;

    bool IsDraining;
    bool DrainingCrystal;
    bool Heroic;
    uint64 CrystalGUID;                                     // This will help us create a pointer to the crystal we are draining. We store GUIDs, never units in case unit is deleted/offline (offline if player of course).

    void Reset()
    {
        if (pInstance)
        {
            //for (uint8 i = 0; i < CRYSTALS_NUMBER; ++i)
            for (std::list<uint64>::const_iterator itr = Crystals.begin(); itr != Crystals.end(); ++itr)
            {
                //Unit* pUnit = Unit::GetUnit(*me, FelCrystals[i]);
                Unit* pUnit = Unit::GetUnit(*me, *itr);
                if (pUnit)
                {
                    if (!pUnit->isAlive())
                        CAST_CRE(pUnit)->Respawn();      // Let the core handle setting death state, etc.

                    // Only need to set unselectable flag. You can't attack unselectable units so non_attackable flag is not necessary here.
                    pUnit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
            }

            pInstance->HandleGameObject(pInstance->GetData64(DATA_SELIN_ENCOUNTER_DOOR), true);
            // Open the big encounter door. Close it in Aggro and open it only in JustDied(and here)
                                                            // Small door opened after event are expected to be closed by default
            // Set Inst data for encounter
            pInstance->SetData(DATA_SELIN_EVENT, NOT_STARTED);
        } else error_log(ERROR_INST_DATA);

        DrainLifeTimer = 3000 + rand()%4000;
        DrainManaTimer = DrainLifeTimer + 5000;
        FelExplosionTimer = 2100;
        DrainCrystalTimer = 10000 + rand()%5000;
        DrainCrystalTimer = 20000 + rand()%5000;
        EmpowerTimer = 10000;

        IsDraining = false;
        DrainingCrystal = false;
        CrystalGUID = 0;
    }

    void SelectNearestCrystal()
    {
        if (Crystals.empty())
            return;

        //float ShortestDistance = 0;
        CrystalGUID = 0;
        Unit* pCrystal = NULL;
        Unit* CrystalChosen = NULL;
        //for (uint8 i =  0; i < CRYSTALS_NUMBER; ++i)
        for (std::list<uint64>::const_iterator itr = Crystals.begin(); itr != Crystals.end(); ++itr)
        {
            pCrystal = NULL;
            //pCrystal = Unit::GetUnit(*me, FelCrystals[i]);
            pCrystal = Unit::GetUnit(*me, *itr);
            if (pCrystal && pCrystal->isAlive())
            {
                // select nearest
                if (!CrystalChosen || me->GetDistanceOrder(pCrystal, CrystalChosen, false))
                {
                    CrystalGUID = pCrystal->GetGUID();
                    CrystalChosen = pCrystal;               // Store a copy of pCrystal so we don't need to recreate a pointer to closest crystal for the movement and yell.
                }
            }
        }
        if (CrystalChosen)
        {
            DoScriptText(SAY_ENERGY, me);
            DoScriptText(EMOTE_CRYSTAL, me);

            CrystalChosen->CastSpell(CrystalChosen, SPELL_FEL_CRYSTAL_COSMETIC, true);

            float x, y, z;                                  // coords that we move to, close to the crystal.
            CrystalChosen->GetClosePoint(x, y, z, me->GetObjectSize(), CONTACT_DISTANCE);

            me->RemoveUnitMovementFlag(MOVEFLAG_WALK_MODE);
            me->GetMotionMaster()->MovePoint(1, x, y, z);
            DrainingCrystal = true;
        }
    }

    void ShatterRemainingCrystals()
    {
        if (Crystals.empty())
            return;

        //for (uint8 i = 0; i < CRYSTALS_NUMBER; ++i)
        for (std::list<uint64>::const_iterator itr = Crystals.begin(); itr != Crystals.end(); ++itr)
        {
            //Creature* pCrystal = (Unit::GetCreature(*me, FelCrystals[i]));
            Creature* pCrystal = Unit::GetCreature(*me, *itr);
            if (pCrystal && pCrystal->isAlive())
                pCrystal->Kill(pCrystal);
        }
    }

    void EnterCombat(Unit* /*who*/)
    {
        DoScriptText(SAY_AGGRO, me);

        if (pInstance)
            pInstance->HandleGameObject(pInstance->GetData64(DATA_SELIN_ENCOUNTER_DOOR), false);
            //Close the encounter door, open it in JustDied/Reset
     }

    void KilledUnit(Unit* /*victim*/)
    {
        DoScriptText(RAND(SAY_KILL_1,SAY_KILL_2), me);
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if (type == POINT_MOTION_TYPE && id == 1)
        {
            Unit* CrystalChosen = Unit::GetUnit(*me, CrystalGUID);
            if (CrystalChosen && CrystalChosen->isAlive())
            {
                // Make the crystal attackable
                // We also remove NON_ATTACKABLE in case the database has it set.
                CrystalChosen->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE + UNIT_FLAG_NOT_SELECTABLE);
                CrystalChosen->CastSpell(me, SPELL_MANA_RAGE, true);
                IsDraining = true;
            }
            else
            {
                // Make an error message in case something weird happened here
                error_log("BSCR: Selin Fireheart unable to drain crystal as the crystal is either dead or despawned");
                DrainingCrystal = false;
            }
        }
    }

    void JustDied(Unit* /*killer*/)
    {
        DoScriptText(SAY_DEATH, me);

        if (!pInstance)
            return;

        pInstance->SetData(DATA_SELIN_EVENT, DONE);         // Encounter complete!
        pInstance->HandleGameObject(pInstance->GetData64(DATA_SELIN_ENCOUNTER_DOOR), true);                  // Open the encounter door
        pInstance->HandleGameObject(pInstance->GetData64(DATA_SELIN_DOOR), true);                 // Open the door leading further in
        ShatterRemainingCrystals();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if (!DrainingCrystal)
        {
            uint32 maxPowerMana = me->GetMaxPower(POWER_MANA);
            if (maxPowerMana && ((me->GetPower(POWER_MANA)*100 / maxPowerMana) < 10))
            {
                if (DrainLifeTimer <= diff)
                {
                    DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_DRAIN_LIFE);
                    DrainLifeTimer = 10000;
                } else DrainLifeTimer -= diff;

                // Heroic only
                if (Heroic)
                {
                    if (DrainManaTimer <= diff)
                    {
                        DoCast(SelectUnit(SELECT_TARGET_RANDOM, 1), SPELL_DRAIN_MANA);
                        DrainManaTimer = 10000;
                    } else DrainManaTimer -= diff;
                }
            }

            if (FelExplosionTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    DoCast(me, SPELL_FEL_EXPLOSION);
                    FelExplosionTimer = 2000;
                }
            } else FelExplosionTimer -= diff;

            // If below 10% mana, start recharging
            maxPowerMana = me->GetMaxPower(POWER_MANA);
            if (maxPowerMana && ((me->GetPower(POWER_MANA)*100 / maxPowerMana) < 10))
            {
                if (DrainCrystalTimer <= diff)
                {
                    SelectNearestCrystal();
                    if (Heroic)
                        DrainCrystalTimer = 10000 + rand()%5000;
                    else
                        DrainCrystalTimer = 20000 + rand()%5000;
                } else DrainCrystalTimer -= diff;
            }

        } else
        {
            if (IsDraining)
            {
                if (EmpowerTimer <= diff)
                {
                    IsDraining = false;
                    DrainingCrystal = false;

                    DoScriptText(SAY_EMPOWERED, me);

                    Unit* CrystalChosen = Unit::GetUnit(*me, CrystalGUID);
                    if (CrystalChosen && CrystalChosen->isAlive())
                        // Use Deal Damage to kill it, not setDeathState.
                        CrystalChosen->Kill(CrystalChosen);

                    CrystalGUID = 0;

                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                } else EmpowerTimer -= diff;
            }
        }

        DoMeleeAttackIfReady();                             // No need to check if we are draining crystal here, as the spell has a stun.
    }
};

CreatureAI* GetAI_boss_selin_fireheart(Creature* pCreature)
{
    return new boss_selin_fireheartAI (pCreature);
};

struct mob_fel_crystalAI : public ScriptedAI
{
    mob_fel_crystalAI(Creature* c) : ScriptedAI(c) {}

    void Reset() {}
    void EnterCombat(Unit* /*who*/) {}
    void AttackStart(Unit* /*who*/) {}
    void MoveInLineOfSight(Unit* /*who*/) {}
    void UpdateAI(const uint32 /*diff*/) {}

    void JustDied(Unit* /*killer*/)
    {
        if (ScriptedInstance* pInstance = me->GetInstanceData())
        {
            Creature* Selin = (Unit::GetCreature(*me, pInstance->GetData64(DATA_SELIN)));
            if (Selin && Selin->isAlive())
            {
                if (CAST_AI(boss_selin_fireheartAI, Selin->AI())->CrystalGUID == me->GetGUID())
                {
                    // Set this to false if we are the Creature that Selin is draining so his AI flows properly
                    CAST_AI(boss_selin_fireheartAI, Selin->AI())->DrainingCrystal = false;
                    CAST_AI(boss_selin_fireheartAI, Selin->AI())->IsDraining = false;
                    CAST_AI(boss_selin_fireheartAI, Selin->AI())->EmpowerTimer = 10000;
                    if (Selin->getVictim())
                    {
                        Selin->AI()->AttackStart(Selin->getVictim());
                        Selin->GetMotionMaster()->MoveChase(Selin->getVictim());
                    }
                }
            }
        } else error_log(ERROR_INST_DATA);
    }
};

CreatureAI* GetAI_mob_fel_crystal(Creature* pCreature)
{
    return new mob_fel_crystalAI (pCreature);
};

void AddSC_boss_selin_fireheart()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_selin_fireheart";
    newscript->GetAI = &GetAI_boss_selin_fireheart;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_fel_crystal";
    newscript->GetAI = &GetAI_mob_fel_crystal;
    newscript->RegisterSelf();
}

