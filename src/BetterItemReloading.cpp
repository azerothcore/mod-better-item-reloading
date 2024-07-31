#include "Chat.h"
#include "Player.h"
#include "Opcodes.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "ScriptMgr.h"
#include "Language.h"
#include "DisableMgr.h"
#include "Tokenize.h"

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Acore::ChatCommands;

class BetterItemReloading : public CommandScript
{
public:
    BetterItemReloading() : CommandScript("BetterItemReloading") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable breloadCommandTable =
        {
            { "item",  HandleBetterReloadItemsCommand, SEC_ADMINISTRATOR, Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "breload", breloadCommandTable}
        };

        return commandTable;
    }

    static bool HandleBetterReloadItemsCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        std::vector<Item*> pItems;
        std::vector<uint8> slots;
        std::vector<uint8> bagSlots;

        for (auto const& itr : Acore::Tokenize(args, ' ', false))
        {
            uint32 entry = uint32(atoi(itr.data()));

            if (!handler->IsConsole())
            {
                // Deequip the item and cache it
                for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
                {
                    Item* pItem = handler->GetSession()->GetPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
                    if (pItem && pItem->GetEntry() == entry)
                    {
                        uint8 slot = pItem->GetSlot();
                        pItems.push_back(pItem);
                        slots.push_back(slot);
                        handler->GetSession()->GetPlayer()->DestroyItem(pItem->GetBagSlot(), slot, true);
                    }
                }
            }

            QueryResult result = WorldDatabase.Query("SELECT `entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `FlagsExtra`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `RequiredSkill`, `RequiredSkillRank`, `requiredspell`, `requiredhonorrank`, `RequiredCityRank`, `RequiredReputationFaction`, `RequiredReputationRank`, `maxcount`, `stackable`, `ContainerSlots`, `StatsCount`, `stat_type1`, `stat_value1`, `stat_type2`, `stat_value2`, `stat_type3`, `stat_value3`, `stat_type4`, `stat_value4`, `stat_type5`, `stat_value5`, `stat_type6`, `stat_value6`, `stat_type7`, `stat_value7`, `stat_type8`, `stat_value8`, `stat_type9`, `stat_value9`, `stat_type10`, `stat_value10`, `ScalingStatDistribution`, `ScalingStatValue`, `dmg_min1`, `dmg_max1`, `dmg_type1`, `dmg_min2`, `dmg_max2`, `dmg_type2`, `armor`, `holy_res`, `fire_res`, `nature_res`, `frost_res`, `shadow_res`, `arcane_res`, `delay`, `ammo_type`, `RangedModRange`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellppmRate_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`, `spellid_2`, `spelltrigger_2`, `spellcharges_2`, `spellppmRate_2`, `spellcooldown_2`, `spellcategory_2`, `spellcategorycooldown_2`, `spellid_3`, `spelltrigger_3`, `spellcharges_3`, `spellppmRate_3`, `spellcooldown_3`, `spellcategory_3`, `spellcategorycooldown_3`, `spellid_4`, `spelltrigger_4`, `spellcharges_4`, `spellppmRate_4`, `spellcooldown_4`, `spellcategory_4`, `spellcategorycooldown_4`, `spellid_5`, `spelltrigger_5`, `spellcharges_5`, `spellppmRate_5`, `spellcooldown_5`, `spellcategory_5`, `spellcategorycooldown_5`, `bonding`, `description`, `PageText`, `LanguageID`, `PageMaterial`, `startquest`, `lockid`, `Material`, `sheath`, `RandomProperty`, `RandomSuffix`, `block`, `itemset`, `MaxDurability`, `area`, `Map`, `BagFamily`, `TotemCategory`, `socketColor_1`, `socketContent_1`, `socketColor_2`, `socketContent_2`, `socketColor_3`, `socketContent_3`, `socketBonus`, `GemProperties`, `RequiredDisenchantSkill`, `ArmorDamageModifier`, `duration`, `ItemLimitCategory`, `HolidayId`, `ScriptName`, `DisenchantID`, `FoodType`, `minMoneyLoot`, `maxMoneyLoot`, `flagsCustom`, `VerifiedBuild` FROM `item_template` WHERE `entry`={}", entry);

            if (!result)
            {
                handler->PSendSysMessage("Couldn't reload item_template entry %u", entry);
                continue;
            }

            Field* fields = result->Fetch();

            ItemTemplate* itemTemplate;
            std::unordered_map<uint32, ItemTemplate>::const_iterator hasItem = sObjectMgr->GetItemTemplateStore()->find(entry);

            if (hasItem == sObjectMgr->GetItemTemplateStore()->end())
            {
                auto itStore = const_cast<ItemTemplateContainer*>(sObjectMgr->GetItemTemplateStore());
                itStore->insert(std::make_pair(entry, ItemTemplate()));
                auto itStoreFast = const_cast<std::vector<ItemTemplate*>*>(sObjectMgr->GetItemTemplateStoreFast());

                // Sadly, we have to reinsert all items here
                uint32 max = 0;
                for (ItemTemplateContainer::const_iterator itr = itStore->begin(); itr != itStore->end(); ++itr)
                    if (itr->first > max)
                        max = itr->first;
                if (max)
                {
                    itStoreFast->clear();
                    itStoreFast->resize(max + 1, NULL);
                    for (ItemTemplateContainer::iterator itr = itStore->begin(); itr != itStore->end(); ++itr)
                        (*itStoreFast)[itr->first] = &(itr->second);
                }
            }

            itemTemplate = const_cast<ItemTemplate*>(&sObjectMgr->GetItemTemplateStore()->at(entry));

            itemTemplate->ItemId = entry;
            itemTemplate->Class = uint32(fields[1].Get<uint8>());
            itemTemplate->SubClass = uint32(fields[2].Get<uint8>());
            itemTemplate->SoundOverrideSubclass = int32(fields[3].Get<int8>());
            itemTemplate->Name1 = fields[4].Get<std::string>();
            itemTemplate->DisplayInfoID = fields[5].Get<uint32>();
            itemTemplate->Quality = uint32(fields[6].Get<uint8>());
            itemTemplate->Flags = fields[7].Get<uint32>();
            itemTemplate->Flags2 = fields[8].Get<uint32>();
            itemTemplate->BuyCount = uint32(fields[9].Get<uint8>());
            itemTemplate->BuyPrice = int32(fields[10].Get<int64>());
            itemTemplate->SellPrice = fields[11].Get<uint32>();
            itemTemplate->InventoryType = uint32(fields[12].Get<uint8>());
            itemTemplate->AllowableClass = fields[13].Get<int32>();
            itemTemplate->AllowableRace = fields[14].Get<int32>();
            itemTemplate->ItemLevel = uint32(fields[15].Get<uint16>());
            itemTemplate->RequiredLevel = uint32(fields[16].Get<uint8>());
            itemTemplate->RequiredSkill = uint32(fields[17].Get<uint16>());
            itemTemplate->RequiredSkillRank = uint32(fields[18].Get<uint16>());
            itemTemplate->RequiredSpell = fields[19].Get<uint32>();
            itemTemplate->RequiredHonorRank = fields[20].Get<uint32>();
            itemTemplate->RequiredCityRank = fields[21].Get<uint32>();
            itemTemplate->RequiredReputationFaction = uint32(fields[22].Get<uint16>());
            itemTemplate->RequiredReputationRank = uint32(fields[23].Get<uint16>());
            itemTemplate->MaxCount = fields[24].Get<int32>();
            itemTemplate->Stackable = fields[25].Get<int32>();
            itemTemplate->ContainerSlots = uint32(fields[26].Get<uint8>());
            itemTemplate->StatsCount = uint32(fields[27].Get<uint8>());

            for (uint8 i = 0; i < itemTemplate->StatsCount; ++i)
            {
                itemTemplate->ItemStat[i].ItemStatType = uint32(fields[28 + i * 2].Get<uint8>());
                itemTemplate->ItemStat[i].ItemStatValue = int32(fields[29 + i * 2].Get<int16>());
            }

            itemTemplate->ScalingStatDistribution = uint32(fields[48].Get<uint16>());
            itemTemplate->ScalingStatValue = fields[49].Get<int32>();

            for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
            {
                itemTemplate->Damage[i].DamageMin = fields[50 + i * 3].Get<float>();
                itemTemplate->Damage[i].DamageMax = fields[51 + i * 3].Get<float>();
                itemTemplate->Damage[i].DamageType = uint32(fields[52 + i * 3].Get<uint8>());
            }

            itemTemplate->Armor = uint32(fields[56].Get<uint16>());
            itemTemplate->HolyRes = uint32(fields[57].Get<uint8>());
            itemTemplate->FireRes = uint32(fields[58].Get<uint8>());
            itemTemplate->NatureRes = uint32(fields[59].Get<uint8>());
            itemTemplate->FrostRes = uint32(fields[60].Get<uint8>());
            itemTemplate->ShadowRes = uint32(fields[61].Get<uint8>());
            itemTemplate->ArcaneRes = uint32(fields[62].Get<uint8>());
            itemTemplate->Delay = uint32(fields[63].Get<uint16>());
            itemTemplate->AmmoType = uint32(fields[64].Get<uint8>());
            itemTemplate->RangedModRange = fields[65].Get<float>();

            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            {
                itemTemplate->Spells[i].SpellId = fields[66 + i * 7].Get<int32>();
                itemTemplate->Spells[i].SpellTrigger = uint32(fields[67 + i * 7].Get<uint8>());
                itemTemplate->Spells[i].SpellCharges = int32(fields[68 + i * 7].Get<int16>());
                itemTemplate->Spells[i].SpellPPMRate = fields[69 + i * 7].Get<float>();
                itemTemplate->Spells[i].SpellCooldown = fields[70 + i * 7].Get<int32>();
                itemTemplate->Spells[i].SpellCategory = uint32(fields[71 + i * 7].Get<uint16>());
                itemTemplate->Spells[i].SpellCategoryCooldown = fields[72 + i * 7].Get<int32>();
            }

            itemTemplate->Bonding = uint32(fields[101].Get<uint8>());
            itemTemplate->Description = fields[102].Get<std::string>();
            itemTemplate->PageText = fields[103].Get<uint32>();
            itemTemplate->LanguageID = uint32(fields[104].Get<uint8>());
            itemTemplate->PageMaterial = uint32(fields[105].Get<uint8>());
            itemTemplate->StartQuest = fields[106].Get<uint32>();
            itemTemplate->LockID = fields[107].Get<uint32>();
            itemTemplate->Material = int32(fields[108].Get<int8>());
            itemTemplate->Sheath = uint32(fields[109].Get<uint8>());
            itemTemplate->RandomProperty = fields[110].Get<uint32>();
            itemTemplate->RandomSuffix = fields[111].Get<int32>();
            itemTemplate->Block = fields[112].Get<uint32>();
            itemTemplate->ItemSet = fields[113].Get<uint32>();
            itemTemplate->MaxDurability = uint32(fields[114].Get<uint16>());
            itemTemplate->Area = fields[115].Get<uint32>();
            itemTemplate->Map = uint32(fields[116].Get<uint16>());
            itemTemplate->BagFamily = fields[117].Get<uint32>();
            itemTemplate->TotemCategory = fields[118].Get<uint32>();

            for (uint8 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
            {
                itemTemplate->Socket[i].Color = uint32(fields[119 + i * 2].Get<uint8>());
                itemTemplate->Socket[i].Content = fields[120 + i * 2].Get<uint32>();
            }

            itemTemplate->socketBonus = fields[125].Get<uint32>();
            itemTemplate->GemProperties = fields[126].Get<uint32>();
            itemTemplate->RequiredDisenchantSkill = uint32(fields[127].Get<int16>());
            itemTemplate->ArmorDamageModifier = fields[128].Get<float>();
            itemTemplate->Duration = fields[129].Get<uint32>();
            itemTemplate->ItemLimitCategory = uint32(fields[130].Get<int16>());
            itemTemplate->HolidayId = fields[131].Get<uint32>();
            itemTemplate->ScriptId = sObjectMgr->GetScriptId(fields[132].Get<std::string>());
            itemTemplate->DisenchantID = fields[133].Get<uint32>();
            itemTemplate->FoodType = uint32(fields[134].Get<uint8>());
            itemTemplate->MinMoneyLoot = fields[135].Get<uint32>();
            itemTemplate->MaxMoneyLoot = fields[136].Get<uint32>();
            itemTemplate->FlagsCu = fields[137].Get<uint32>();

            if (itemTemplate->Class >= MAX_ITEM_CLASS)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Class value ({})", entry, itemTemplate->Class);
                itemTemplate->Class = ITEM_CLASS_MISC;
            }

            if (itemTemplate->SubClass >= MaxItemSubclassValues[itemTemplate->Class])
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Subclass value ({}) for class {}", entry, itemTemplate->SubClass, itemTemplate->Class);
                itemTemplate->SubClass = 0;// exist for all item classes
            }

            if (itemTemplate->Quality >= MAX_ITEM_QUALITY)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Quality value ({})", entry, itemTemplate->Quality);
                itemTemplate->Quality = ITEM_QUALITY_NORMAL;
            }

            if (itemTemplate->Flags2 & ITEM_FLAGS_EXTRA_HORDE_ONLY)
            {
                if (FactionEntry const* faction = sFactionStore.LookupEntry(HORDE))
                    if ((itemTemplate->AllowableRace & faction->BaseRepRaceMask[0]) == 0)
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has value ({}) in `AllowableRace` races, not compatible with ITEM_FLAGS_EXTRA_HORDE_ONLY ({}) in Flags field, item cannot be equipped or used by these races.", entry, itemTemplate->AllowableRace, ITEM_FLAGS_EXTRA_HORDE_ONLY);

                if (itemTemplate->Flags2 & ITEM_FLAGS_EXTRA_ALLIANCE_ONLY)
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has value ({}) in `Flags2` flags (ITEM_FLAGS_EXTRA_ALLIANCE_ONLY) and ITEM_FLAGS_EXTRA_HORDE_ONLY ({}) in Flags field, this is a wrong combination.", entry, ITEM_FLAGS_EXTRA_ALLIANCE_ONLY, ITEM_FLAGS_EXTRA_HORDE_ONLY);
            }
            else if (itemTemplate->Flags2 & ITEM_FLAGS_EXTRA_ALLIANCE_ONLY)
            {
                if (FactionEntry const* faction = sFactionStore.LookupEntry(ALLIANCE))
                    if ((itemTemplate->AllowableRace & faction->BaseRepRaceMask[0]) == 0)
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has value ({}) in `AllowableRace` races, not compatible with ITEM_FLAGS_EXTRA_ALLIANCE_ONLY ({}) in Flags field, item cannot be equipped or used by these races.", entry, itemTemplate->AllowableRace, ITEM_FLAGS_EXTRA_ALLIANCE_ONLY);
            }

            if (itemTemplate->BuyCount <= 0)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong BuyCount value ({}), set to default(1).", entry, itemTemplate->BuyCount);
                itemTemplate->BuyCount = 1;
            }

            if (itemTemplate->InventoryType >= MAX_INVTYPE)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong InventoryType value ({})", entry, itemTemplate->InventoryType);
                itemTemplate->InventoryType = INVTYPE_NON_EQUIP;
            }

            if (itemTemplate->RequiredSkill >= MAX_SKILL_TYPE)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong RequiredSkill value ({})", entry, itemTemplate->RequiredSkill);
                itemTemplate->RequiredSkill = 0;
            }

            {
                // can be used in equip slot, as page read use in inventory, or spell casting at use
                bool req = itemTemplate->InventoryType != INVTYPE_NON_EQUIP || itemTemplate->PageText;
                if (!req)
                    for (uint8 j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
                    {
                        if (itemTemplate->Spells[j].SpellId)
                        {
                            req = true;
                            break;
                        }
                    }

                if (req)
                {
                    if (!(itemTemplate->AllowableClass & CLASSMASK_ALL_PLAYABLE))
                        LOG_ERROR("sql.sql", "Item (Entry: {}) does not have any playable classes ({}) in `AllowableClass` and can't be equipped or used.", entry, itemTemplate->AllowableClass);

                    if (!(itemTemplate->AllowableRace & RACEMASK_ALL_PLAYABLE))
                        LOG_ERROR("sql.sql", "Item (Entry: {}) does not have any playable races ({}) in `AllowableRace` and can't be equipped or used.", entry, itemTemplate->AllowableRace);
                }
            }

            if (itemTemplate->RequiredSpell && !sSpellMgr->GetSpellInfo(itemTemplate->RequiredSpell))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has a wrong (non-existing) spell in RequiredSpell ({})", entry, itemTemplate->RequiredSpell);
                itemTemplate->RequiredSpell = 0;
            }

            if (itemTemplate->RequiredReputationRank >= MAX_REPUTATION_RANK)
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong reputation rank in RequiredReputationRank ({}), item can't be used.", entry, itemTemplate->RequiredReputationRank);

            if (itemTemplate->RequiredReputationFaction)
            {
                if (!sFactionStore.LookupEntry(itemTemplate->RequiredReputationFaction))
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (not existing) faction in RequiredReputationFaction ({})", entry, itemTemplate->RequiredReputationFaction);
                    itemTemplate->RequiredReputationFaction = 0;
                }

                if (itemTemplate->RequiredReputationRank == MIN_REPUTATION_RANK)
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has min. reputation rank in RequiredReputationRank (0) but RequiredReputationFaction > 0, faction setting is useless.", entry);
            }

            if (itemTemplate->MaxCount < -1)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has too large negative in maxcount (%i), replace by value (-1) no storing limits.", entry, itemTemplate->MaxCount);
                itemTemplate->MaxCount = -1;
            }

            if (itemTemplate->Stackable == 0)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong value in stackable (%i), replace by default 1.", entry, itemTemplate->Stackable);
                itemTemplate->Stackable = 1;
            }
            else if (itemTemplate->Stackable < -1)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has too large negative in stackable (%i), replace by value (-1) no stacking limits.", entry, itemTemplate->Stackable);
                itemTemplate->Stackable = -1;
            }

            if (itemTemplate->ContainerSlots > MAX_BAG_SIZE)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has too large value in ContainerSlots ({}), replace by hardcoded limit ({}).", entry, itemTemplate->ContainerSlots, MAX_BAG_SIZE);
                itemTemplate->ContainerSlots = MAX_BAG_SIZE;
            }

            if (itemTemplate->StatsCount > MAX_ITEM_PROTO_STATS)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has too large value in statscount ({}), replace by hardcoded limit ({}).", entry, itemTemplate->StatsCount, MAX_ITEM_PROTO_STATS);
                itemTemplate->StatsCount = MAX_ITEM_PROTO_STATS;
            }

            for (uint8 j = 0; j < itemTemplate->StatsCount; ++j)
            {
                // for ItemStatValue != 0
                if (itemTemplate->ItemStat[j].ItemStatValue && itemTemplate->ItemStat[j].ItemStatType >= MAX_ITEM_MOD)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (non-existing?) stat_type%d ({})", entry, j + 1, itemTemplate->ItemStat[j].ItemStatType);
                    itemTemplate->ItemStat[j].ItemStatType = 0;
                }

                switch (itemTemplate->ItemStat[j].ItemStatType)
                {
                case ITEM_MOD_SPELL_HEALING_DONE:
                case ITEM_MOD_SPELL_DAMAGE_DONE:
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has deprecated stat_type%d ({})", entry, j + 1, itemTemplate->ItemStat[j].ItemStatType);
                    break;
                default:
                    break;
                }
            }

            for (uint8 j = 0; j < MAX_ITEM_PROTO_DAMAGES; ++j)
            {
                if (itemTemplate->Damage[j].DamageType >= MAX_SPELL_SCHOOL)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong dmg_type%d ({})", entry, j + 1, itemTemplate->Damage[j].DamageType);
                    itemTemplate->Damage[j].DamageType = 0;
                }
            }

            // special format
            if ((itemTemplate->Spells[0].SpellId == 483) || (itemTemplate->Spells[0].SpellId == 55884))
            {
                // spell_1
                if (itemTemplate->Spells[0].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spelltrigger_%d ({}) for special learning format", entry, 0 + 1, itemTemplate->Spells[0].SpellTrigger);
                    itemTemplate->Spells[0].SpellId = 0;
                    itemTemplate->Spells[0].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                    itemTemplate->Spells[1].SpellId = 0;
                    itemTemplate->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }

                // spell_2 have learning spell
                if (itemTemplate->Spells[1].SpellTrigger != ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spelltrigger_%d ({}) for special learning format.", entry, 1 + 1, itemTemplate->Spells[1].SpellTrigger);
                    itemTemplate->Spells[0].SpellId = 0;
                    itemTemplate->Spells[1].SpellId = 0;
                    itemTemplate->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                else if (!itemTemplate->Spells[1].SpellId)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) does not have an expected spell in spellid_%d in special learning format.", entry, 1 + 1);
                    itemTemplate->Spells[0].SpellId = 0;
                    itemTemplate->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                else if (itemTemplate->Spells[1].SpellId != -1)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itemTemplate->Spells[1].SpellId);
                    if (!spellInfo && !DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, itemTemplate->Spells[1].SpellId, NULL))
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (not existing) spell in spellid_%d (%d)", entry, 1 + 1, itemTemplate->Spells[1].SpellId);
                        itemTemplate->Spells[0].SpellId = 0;
                        itemTemplate->Spells[1].SpellId = 0;
                        itemTemplate->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                    }
                    // allowed only in special format
                    else if ((itemTemplate->Spells[1].SpellId == 483) || (itemTemplate->Spells[1].SpellId == 55884))
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has broken spell in spellid_%d (%d)", entry, 1 + 1, itemTemplate->Spells[1].SpellId);
                        itemTemplate->Spells[0].SpellId = 0;
                        itemTemplate->Spells[1].SpellId = 0;
                        itemTemplate->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                    }
                }

                // spell_3*, spell_4*, spell_5* is empty
                for (uint8 j = 2; j < MAX_ITEM_PROTO_SPELLS; ++j)
                {
                    if (itemTemplate->Spells[j].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spelltrigger_%d ({})", entry, j + 1, itemTemplate->Spells[j].SpellTrigger);
                        itemTemplate->Spells[j].SpellId = 0;
                        itemTemplate->Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                    }
                    else if (itemTemplate->Spells[j].SpellId != 0)
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong spell in spellid_%d (%d) for learning special format", entry, j + 1, itemTemplate->Spells[j].SpellId);
                        itemTemplate->Spells[j].SpellId = 0;
                    }
                }
            }
            // normal spell list
            else
            {
                for (uint8 j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
                {
                    if (itemTemplate->Spells[j].SpellTrigger >= MAX_ITEM_SPELLTRIGGER || itemTemplate->Spells[j].SpellTrigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spelltrigger_%d ({})", entry, j + 1, itemTemplate->Spells[j].SpellTrigger);
                        itemTemplate->Spells[j].SpellId = 0;
                        itemTemplate->Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                    }

                    if (itemTemplate->Spells[j].SpellId && itemTemplate->Spells[j].SpellId != -1)
                    {
                        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itemTemplate->Spells[j].SpellId);
                        if (!spellInfo && !DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, itemTemplate->Spells[j].SpellId, NULL))
                        {
                            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (not existing) spell in spellid_%d (%d)", entry, j + 1, itemTemplate->Spells[j].SpellId);
                            itemTemplate->Spells[j].SpellId = 0;
                        }
                        // allowed only in special format
                        else if ((itemTemplate->Spells[j].SpellId == 483) || (itemTemplate->Spells[j].SpellId == 55884))
                        {
                            LOG_ERROR("sql.sql", "Item (Entry: {}) has broken spell in spellid_%d (%d)", entry, j + 1, itemTemplate->Spells[j].SpellId);
                            itemTemplate->Spells[j].SpellId = 0;
                        }
                    }
                }
            }

            if (itemTemplate->Bonding >= MAX_BIND_TYPE)
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Bonding value ({})", entry, itemTemplate->Bonding);

            if (itemTemplate->PageText && !sObjectMgr->GetPageText(itemTemplate->PageText))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has non existing first page (Id:{})", entry, itemTemplate->PageText);

            if (itemTemplate->LockID && !sLockStore.LookupEntry(itemTemplate->LockID))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong LockID ({})", entry, itemTemplate->LockID);

            if (itemTemplate->Sheath >= MAX_SHEATHETYPE)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Sheath ({})", entry, itemTemplate->Sheath);
                itemTemplate->Sheath = SHEATHETYPE_NONE;
            }

            if (itemTemplate->RandomProperty)
            {
                if (itemTemplate->RandomProperty == -1)
                    itemTemplate->RandomProperty = 0;

                else if (!sItemRandomPropertiesStore.LookupEntry(GetItemEnchantMod(itemTemplate->RandomProperty)))
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has unknown (wrong or not listed in `item_enchantment_template`) RandomProperty ({})", entry, itemTemplate->RandomProperty);
                    itemTemplate->RandomProperty = 0;
                }
            }

            if (itemTemplate->RandomSuffix && !sItemRandomSuffixStore.LookupEntry(GetItemEnchantMod(itemTemplate->RandomSuffix)))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong RandomSuffix ({})", entry, itemTemplate->RandomSuffix);
                itemTemplate->RandomSuffix = 0;
            }

            if (itemTemplate->ItemSet && !sItemSetStore.LookupEntry(itemTemplate->ItemSet))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) have wrong ItemSet ({})", entry, itemTemplate->ItemSet);
                itemTemplate->ItemSet = 0;
            }

            if (itemTemplate->Area && !sAreaTableStore.LookupEntry(itemTemplate->Area))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Area ({})", entry, itemTemplate->Area);

            if (itemTemplate->Map && !sMapStore.LookupEntry(itemTemplate->Map))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Map ({})", entry, itemTemplate->Map);

            if (itemTemplate->BagFamily)
            {
                // check bits
                for (uint32 j = 0; j < sizeof(itemTemplate->BagFamily) * 8; ++j)
                {
                    uint32 mask = 1 << j;
                    if ((itemTemplate->BagFamily & mask) == 0)
                        continue;

                    ItemBagFamilyEntry const* bf = sItemBagFamilyStore.LookupEntry(j + 1);
                    if (!bf)
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has bag family bit set not listed in ItemBagFamily.dbc, remove bit", entry);
                        itemTemplate->BagFamily &= ~mask;
                        continue;
                    }

                    if (BAG_FAMILY_MASK_CURRENCY_TOKENS & mask)
                    {
                        CurrencyTypesEntry const* ctEntry = sCurrencyTypesStore.LookupEntry(itemTemplate->ItemId);
                        if (!ctEntry)
                        {
                            LOG_ERROR("sql.sql", "Item (Entry: {}) has currency bag family bit set in BagFamily but not listed in CurrencyTypes.dbc, remove bit", entry);
                            itemTemplate->BagFamily &= ~mask;
                        }
                    }
                }
            }

            if (itemTemplate->TotemCategory && !sTotemCategoryStore.LookupEntry(itemTemplate->TotemCategory))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong TotemCategory ({})", entry, itemTemplate->TotemCategory);

            for (uint8 j = 0; j < MAX_ITEM_PROTO_SOCKETS; ++j)
            {
                if (itemTemplate->Socket[j].Color && (itemTemplate->Socket[j].Color & SOCKET_COLOR_ALL) != itemTemplate->Socket[j].Color)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong socketColor_%d ({})", entry, j + 1, itemTemplate->Socket[j].Color);
                    itemTemplate->Socket[j].Color = 0;
                }
            }

            if (itemTemplate->GemProperties && !sGemPropertiesStore.LookupEntry(itemTemplate->GemProperties))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong GemProperties ({})", entry, itemTemplate->GemProperties);

            if (itemTemplate->FoodType >= MAX_PET_DIET)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong FoodType value ({})", entry, itemTemplate->FoodType);
                itemTemplate->FoodType = 0;
            }

            if (itemTemplate->ItemLimitCategory && !sItemLimitCategoryStore.LookupEntry(itemTemplate->ItemLimitCategory))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong LimitCategory value ({})", entry, itemTemplate->ItemLimitCategory);
                itemTemplate->ItemLimitCategory = 0;
            }

            if (itemTemplate->HolidayId && !sHolidaysStore.LookupEntry(itemTemplate->HolidayId))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong HolidayId value ({})", entry, itemTemplate->HolidayId);
                itemTemplate->HolidayId = 0;
            }

            if (itemTemplate->FlagsCu & ITEM_FLAGS_CU_DURATION_REAL_TIME && !itemTemplate->Duration)
            {
                LOG_ERROR("sql.sql", "Item (Entry {}) has flag ITEM_FLAGS_CU_DURATION_REAL_TIME but it does not have duration limit", entry);
                itemTemplate->FlagsCu &= ~ITEM_FLAGS_CU_DURATION_REAL_TIME;
            }

            // Fill categories map
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            {
                if (itemTemplate->Spells[i].SpellId && itemTemplate->Spells[i].SpellCategory && itemTemplate->Spells[i].SpellCategoryCooldown)
                {
                    SpellCategoryStore::const_iterator ct = sSpellsByCategoryStore.find(itemTemplate->Spells[i].SpellCategory);
                    if (ct != sSpellsByCategoryStore.end())
                    {
                        const SpellCategorySet& ct_set = ct->second;
                        if (ct_set.find(std::pair(true, static_cast<unsigned int>(itemTemplate->Spells[i].SpellId))) == ct_set.end())
                            sSpellsByCategoryStore[itemTemplate->Spells[i].SpellCategory].insert(std::pair(true, static_cast<unsigned int>(itemTemplate->Spells[i].SpellId)));
                    }
                    else
                        sSpellsByCategoryStore[itemTemplate->Spells[i].SpellCategory].insert(std::pair(true, static_cast<unsigned int>(itemTemplate->Spells[i].SpellId)));
                }
            }

            if (!handler->IsConsole())
            {
                SendItemQuery(handler->GetSession()->GetPlayer(), itemTemplate);

                // safety check
                if (pItems.size() == slots.size())
                {
                    // re-equip the item to apply the new stats
                    for (size_t i = 0; i < pItems.size(); i++)
                        handler->GetSession()->GetPlayer()->EquipItem(slots.at(i), pItems.at(i), true);
                }

            }

            handler->PSendSysMessage("Reloaded item template entry %u", entry);
        }

        return true;
    }

private:
    static void SendItemQuery(Player* player, ItemTemplate* item)
    {
        if (!player || !item)
            return;

        std::string name = item->Name1;
        std::string description = item->Description;

        int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();

        if (loc_idx >= 0)
        {
            if (ItemLocale const* il = sObjectMgr->GetItemLocale(item->ItemId))
            {
                ObjectMgr::GetLocaleString(il->Name, loc_idx, name);
                ObjectMgr::GetLocaleString(il->Description, loc_idx, description);
            }
        }

        WorldPacket data(SMSG_ITEM_QUERY_SINGLE_RESPONSE, 600);
        data << item->ItemId;
        data << item->Class;
        data << item->SubClass;
        data << item->SoundOverrideSubclass;
        data << name;
        data << uint8(0x00);
        data << uint8(0x00);
        data << uint8(0x00);
        data << item->DisplayInfoID;
        data << item->Quality;
        data << item->Flags;
        data << item->Flags2;
        data << item->BuyPrice;
        data << item->SellPrice;
        data << item->InventoryType;
        data << item->AllowableClass;
        data << item->AllowableRace;
        data << item->ItemLevel;
        data << item->RequiredLevel;
        data << item->RequiredSkill;
        data << item->RequiredSkillRank;
        data << item->RequiredSpell;
        data << item->RequiredHonorRank;
        data << item->RequiredCityRank;
        data << item->RequiredReputationFaction;
        data << item->RequiredReputationRank;
        data << int32(item->MaxCount);
        data << int32(item->Stackable);
        data << item->ContainerSlots;
        data << item->StatsCount;

        for (uint32 i = 0; i < item->StatsCount; ++i)
        {
            data << item->ItemStat[i].ItemStatType;
            data << item->ItemStat[i].ItemStatValue;
        }

        data << item->ScalingStatDistribution;
        data << item->ScalingStatValue;

        for (int i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            data << item->Damage[i].DamageMin;
            data << item->Damage[i].DamageMax;
            data << item->Damage[i].DamageType;
        }

        data << item->Armor;
        data << item->HolyRes;
        data << item->FireRes;
        data << item->NatureRes;
        data << item->FrostRes;
        data << item->ShadowRes;
        data << item->ArcaneRes;
        data << item->Delay;
        data << item->AmmoType;
        data << item->RangedModRange;

        for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
        {
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(item->Spells[s].SpellId);
            if (spell)
            {
                bool db_data = item->Spells[s].SpellCooldown >= 0 || item->Spells[s].SpellCategoryCooldown >= 0;

                data << item->Spells[s].SpellId;
                data << item->Spells[s].SpellTrigger;
                data << uint32(-abs(item->Spells[s].SpellCharges));

                if (db_data)
                {
                    data << uint32(item->Spells[s].SpellCooldown);
                    data << uint32(item->Spells[s].SpellCategory);
                    data << uint32(item->Spells[s].SpellCategoryCooldown);
                }
                else
                {
                    data << uint32(spell->RecoveryTime);
                    data << uint32(spell->GetCategory());
                    data << uint32(spell->CategoryRecoveryTime);
                }
            }
            else
            {
                data << uint32(0);
                data << uint32(0);
                data << uint32(0);
                data << uint32(-1);
                data << uint32(0);
                data << uint32(-1);
            }
        }

        data << item->Bonding;
        data << description;
        data << item->PageText;
        data << item->LanguageID;
        data << item->PageMaterial;
        data << item->StartQuest;
        data << item->LockID;
        data << int32(item->Material);
        data << item->Sheath;
        data << item->RandomProperty;
        data << item->RandomSuffix;
        data << item->Block;
        data << item->ItemSet;
        data << item->MaxDurability;
        data << item->Area;
        data << item->Map;
        data << item->BagFamily;
        data << item->TotemCategory;

        for (int s = 0; s < MAX_ITEM_PROTO_SOCKETS; ++s)
        {
            data << item->Socket[s].Color;
            data << item->Socket[s].Content;
        }

        data << item->socketBonus;
        data << item->GemProperties;
        data << item->RequiredDisenchantSkill;
        data << item->ArmorDamageModifier;
        data << item->Duration;
        data << item->ItemLimitCategory;
        data << item->HolidayId;

        player->GetSession()->SendPacket(&data);
    }
};

void AddBetterItemReloadingScripts()
{
    new BetterItemReloading();
}
