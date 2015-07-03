#include "monstergenerator.h"
#include "color.h"
#include "translations.h"
#include "rng.h"
#include "debug.h"
#include "item_group.h"
#include "catacharset.h"
#include "item.h"
#include "output.h"
#include "monattack.h"
#include "mondefense.h"
#include "mondeath.h"
#include "monfaction.h"
#include "mongroup.h"

MonsterGenerator::MonsterGenerator()
{
    mon_templates["mon_null"] = new mtype();
    mon_species["spec_null"] = new species_type();
    //ctor
    init_phases();
    init_attack();
    init_defense();
    init_death();
    init_flags();
    init_trigger();
    init_sizes();
}

MonsterGenerator::~MonsterGenerator()
{
    reset();
}

void MonsterGenerator::reset()
{
    for( auto &elem : mon_templates ) {
        delete elem.second;
    }
    mon_templates.clear();
    for( auto &elem : mon_species ) {
        delete elem.second;
    }
    mon_species.clear();
    mon_templates["mon_null"] = new mtype();
    mon_species["spec_null"] = new species_type();
}

void MonsterGenerator::finalize_mtypes()
{
    for( auto &elem : mon_templates ) {
        mtype *mon = elem.second;
        apply_species_attributes(mon);
        set_mtype_flags(mon);
        set_species_ids( mon );
    }
}

void MonsterGenerator::apply_species_attributes(mtype *mon)
{
    for (std::set<std::string>::iterator spec = mon->species.begin(); spec != mon->species.end();
         ++spec) {
        if (mon_species.find(*spec) != mon_species.end()) {
            species_type *mspec = mon_species[*spec];

            // apply species flags/triggers
            apply_set_to_set(mspec->flags, mon->flags);
            apply_set_to_set(mspec->anger_trig, mon->anger);
            apply_set_to_set(mspec->fear_trig, mon->fear);
            apply_set_to_set(mspec->placate_trig, mon->placate);
        }
    }
}
void MonsterGenerator::set_mtype_flags(mtype *mon)
{
    // The flag vectors are slow, given how often has_flags() is called,
    // so instead we'll use bitsets and initialize them here.
    m_flag nflag;
    for (std::set<m_flag>::iterator flag = mon->flags.begin(); flag != mon->flags.end(); ++flag) {
        nflag = m_flag(*flag);
        mon->bitflags[nflag] = true;
    }
    monster_trigger ntrig;
    for (std::set<monster_trigger>::iterator trig = mon->anger.begin(); trig != mon->anger.end();
         ++trig) {
        ntrig = monster_trigger(*trig);
        mon->bitanger[ntrig] = true;
    }
    for (std::set<monster_trigger>::iterator trig = mon->fear.begin(); trig != mon->fear.end();
         ++trig) {
        ntrig = monster_trigger(*trig);
        mon->bitfear[ntrig] = true;
    }
    for (std::set<monster_trigger>::iterator trig = mon->placate.begin(); trig != mon->placate.end();
         ++trig) {
        ntrig = monster_trigger(*trig);
        mon->bitplacate[ntrig] = true;
    }
}

template <typename T>
void MonsterGenerator::apply_set_to_set(std::set<T> from, std::set<T> &to)
{
    for( const auto &elem : from ) {
        to.insert( elem );
    }
}

void MonsterGenerator::init_phases()
{
    phase_map["NULL"] = PNULL;
    phase_map["SOLID"] = SOLID;
    phase_map["LIQUID"] = LIQUID;
    phase_map["GAS"] = GAS;
    phase_map["PLASMA"] = PLASMA;
}

void MonsterGenerator::init_sizes()
{
    size_map["TINY"] = MS_TINY; // Rodent
    size_map["SMALL"] = MS_SMALL; // Half human
    size_map["MEDIUM"] = MS_MEDIUM; // Human
    size_map["LARGE"] = MS_LARGE; // Cow
    size_map["HUGE"] = MS_HUGE; // TAAAANK
}

void MonsterGenerator::init_death()
{
    death_map["NORMAL"] = &mdeath::normal;// Drop a body
    death_map["ACID"] = &mdeath::acid;// Acid instead of a body
    death_map["BOOMER"] = &mdeath::boomer;// Explodes in vomit :3
    death_map["BOOMER_GLOW"] = &mdeath::boomer_glow;// Explodes in glowing vomit :3
    death_map["KILL_VINES"] = &mdeath::kill_vines;// Kill all nearby vines
    death_map["VINE_CUT"] = &mdeath::vine_cut;// Kill adjacent vine if it's cut
    death_map["TRIFFID_HEART"] = &mdeath::triffid_heart;// Destroy all roots
    death_map["FUNGUS"] = &mdeath::fungus;// Explodes in spores D:
    death_map["DISINTEGRATE"] = &mdeath::disintegrate;// Falls apart
    death_map["WORM"] = &mdeath::worm;// Spawns 2 half-worms
    death_map["DISAPPEAR"] = &mdeath::disappear;// Hallucination disappears
    death_map["GUILT"] = &mdeath::guilt;// Morale penalty
    death_map["BRAINBLOB"] = &mdeath::brainblob;// Frees blobs, redirects to brainblob()
    death_map["BLOBSPLIT"] = &mdeath::blobsplit;// Creates more blobs
    death_map["JACKSON"] = &mdeath::jackson;// Reverts dancers
    death_map["MELT"] = &mdeath::melt;// Normal death, but melts
    death_map["AMIGARA"] = &mdeath::amigara;// Removes hypnosis if last one
    death_map["THING"] = &mdeath::thing;// Turn into a full thing
    death_map["EXPLODE"] = &mdeath::explode;// Damaging explosion
    death_map["FOCUSEDBEAM"] = &mdeath::focused_beam;// Blinding ray
    death_map["BROKEN"] = &mdeath::broken;// Spawns a broken robot.
    death_map["RATKING"] = &mdeath::ratking;// Cure verminitis
    death_map["DARKMAN"] = &mdeath::darkman;// sight returns to normal
    death_map["GAS"] = &mdeath::gas;// Explodes in toxic gas
    death_map["KILL_BREATHERS"] = &mdeath::kill_breathers;// All breathers die
    death_map["BROKEN_AMMO"] = &mdeath::broken_ammo;// Gives a message about destroying ammo and then calls "BROKEN"
    death_map["SMOKEBURST"] = &mdeath::smokeburst;// Explode like a huge smoke bomb.
    death_map["JABBERWOCKY"] = &mdeath::jabberwock; // Snicker-snack!
    death_map["DETONATE"] = &mdeath::detonate; // Take them with you
    death_map["GAMEOVER"] = &mdeath::gameover;// Game over!  Defense mode

    /* Currently Unimplemented */
    //death_map["SHRIEK"] = &mdeath::shriek;// Screams loudly
    //death_map["HOWL"] = &mdeath::howl;// Wolf's howling
    //death_map["RATTLE"] = &mdeath::rattle;// Rattles like a rattlesnake
}

void MonsterGenerator::init_attack()
{
    attack_map["NONE"] = &mattack::none;
    attack_map["ANTQUEEN"] = &mattack::antqueen;
    attack_map["SHRIEK"] = &mattack::shriek;
    attack_map["RATTLE"] = &mattack::rattle;
    attack_map["HOWL"] = &mattack::howl;
    attack_map["ACID"] = &mattack::acid;
    attack_map["ACID_BARF"] = &mattack::acid_barf;
    attack_map["ACID_ACCURATE"] = &mattack::acid_accurate;
    attack_map["SHOCKSTORM"] = &mattack::shockstorm;
    attack_map["PULL_METAL_WEAPON"] = &mattack::pull_metal_weapon;
    attack_map["SMOKECLOUD"] = &mattack::smokecloud;
    attack_map["BOOMER"] = &mattack::boomer;
    attack_map["BOOMER_GLOW"] = &mattack::boomer_glow;
    attack_map["RESURRECT"] = &mattack::resurrect;
    attack_map["SMASH"] = &mattack::smash;
    attack_map["SCIENCE"] = &mattack::science;
    attack_map["GROWPLANTS"] = &mattack::growplants;
    attack_map["GROW_VINE"] = &mattack::grow_vine;
    attack_map["VINE"] = &mattack::vine;
    attack_map["SPIT_SAP"] = &mattack::spit_sap;
    attack_map["TRIFFID_HEARTBEAT"] = &mattack::triffid_heartbeat;
    attack_map["FUNGUS"] = &mattack::fungus;
    attack_map["FUNGUS_HAZE"] = &mattack::fungus_haze;
    attack_map["FUNGUS_BIG_BLOSSOM"] = &mattack::fungus_big_blossom;
    attack_map["FUNGUS_INJECT"] = &mattack::fungus_inject;
    attack_map["FUNGUS_BRISTLE"] = &mattack::fungus_bristle;
    attack_map["FUNGUS_GROWTH"] = &mattack::fungus_growth;
    attack_map["FUNGUS_SPROUT"] = &mattack::fungus_sprout;
    attack_map["FUNGUS_FORTIFY"] = &mattack::fungus_fortify;
    attack_map["LEAP"] = &mattack::leap;
    attack_map["DERMATIK"] = &mattack::dermatik;
    attack_map["DERMATIK_GROWTH"] = &mattack::dermatik_growth;
    attack_map["PLANT"] = &mattack::plant;
    attack_map["DISAPPEAR"] = &mattack::disappear;
    attack_map["FORMBLOB"] = &mattack::formblob;
    attack_map["CALLBLOBS"] = &mattack::callblobs;
    attack_map["JACKSON"] = &mattack::jackson;
    attack_map["DANCE"] = &mattack::dance;
    attack_map["DOGTHING"] = &mattack::dogthing;
    attack_map["TENTACLE"] = &mattack::tentacle;
    attack_map["GENE_STING"] = &mattack::gene_sting;
    attack_map["PARA_STING"] = &mattack::para_sting;
    attack_map["TRIFFID_GROWTH"] = &mattack::triffid_growth;
    attack_map["STARE"] = &mattack::stare;
    attack_map["FEAR_PARALYZE"] = &mattack::fear_paralyze;
    attack_map["PHOTOGRAPH"] = &mattack::photograph;
    attack_map["TAZER"] = &mattack::tazer;
    attack_map["SMG"] = &mattack::smg;
    attack_map["LASER"] = &mattack::laser;
    attack_map["RIFLE_TUR"] = &mattack::rifle_tur;
    attack_map["BMG_TUR"] = &mattack::bmg_tur;
    attack_map["SEARCHLIGHT"] = &mattack::searchlight;
    attack_map["FLAMETHROWER"] = &mattack::flamethrower;
    attack_map["COPBOT"] = &mattack::copbot;
    attack_map["CHICKENBOT"] = &mattack::chickenbot;
    attack_map["MULTI_ROBOT"] = &mattack::multi_robot;
    attack_map["RATKING"] = &mattack::ratking;
    attack_map["GENERATOR"] = &mattack::generator;
    attack_map["UPGRADE"] = &mattack::upgrade;
    attack_map["BREATHE"] = &mattack::breathe;
    attack_map["BITE"] = &mattack::bite;
    attack_map["IMPALE"] = &mattack::impale;
    attack_map["BRANDISH"] = &mattack::brandish;
    attack_map["FLESH_GOLEM"] = &mattack::flesh_golem;
    attack_map["LUNGE"] = &mattack::lunge;
    attack_map["LONGSWIPE"] = &mattack::longswipe;
    attack_map["PARROT"] = &mattack::parrot;
    attack_map["DARKMAN"] = &mattack::darkman;
    attack_map["SLIMESPRING"] = &mattack::slimespring;
    attack_map["BIO_OP_TAKEDOWN"] = &mattack::bio_op_takedown;
    attack_map["SUICIDE"] = &mattack::suicide;
    attack_map["KAMIKAZE"] = &mattack::kamikaze;
    attack_map["GRENADIER"] = &mattack::grenadier;
    attack_map["GRENADIER_ELITE"] = &mattack::grenadier_elite;
    attack_map["RIOTBOT"] = &mattack::riotbot;
    attack_map["STRETCH_ATTACK"] = &mattack::stretch_attack;
    attack_map["STRETCH_BITE"] = &mattack::stretch_bite;
    attack_map["RANGED_PULL"] = &mattack::ranged_pull;
    attack_map["GRAB"] = &mattack::grab;
    attack_map["GRAB_DRAG"] = &mattack::grab_drag;
}

void MonsterGenerator::init_defense()
{
    defense_map["NONE"] = &mdefense::none; //No special attack-back
    defense_map["ZAPBACK"] = &mdefense::zapback; //Shock attacker on hit
    defense_map["ACIDSPLASH"] = &mdefense::acidsplash; //Shock attacker on hit
}

void MonsterGenerator::init_trigger()
{
    trigger_map["NULL"] = MTRIG_NULL;// = 0,
    trigger_map["STALK"] = MTRIG_STALK;//  // Increases when following the player
    trigger_map["MEAT"] = MTRIG_MEAT;//  // Meat or a corpse nearby
    trigger_map["PLAYER_WEAK"] = MTRIG_HOSTILE_WEAK;// // Hurt hostile player/npc/monster seen
    trigger_map["PLAYER_CLOSE"] = MTRIG_HOSTILE_CLOSE;// // Hostile creature within a few tiles
    trigger_map["HURT"] = MTRIG_HURT;//  // We are hurt
    trigger_map["FIRE"] = MTRIG_FIRE;//  // Fire nearby
    trigger_map["FRIEND_DIED"] = MTRIG_FRIEND_DIED;// // A monster of the same type died
    trigger_map["FRIEND_ATTACKED"] = MTRIG_FRIEND_ATTACKED;// // A monster of the same type attacked
    trigger_map["SOUND"] = MTRIG_SOUND;//  // Heard a sound
}

void MonsterGenerator::init_flags()
{
    // see mtype.h for commentary
    flag_map["NULL"] = MF_NULL;
    flag_map["SEES"] = MF_SEES;
    flag_map["VIS50"] = MF_VIS50;
    flag_map["VIS40"] = MF_VIS40;
    flag_map["VIS30"] = MF_VIS30;
    flag_map["VIS20"] = MF_VIS20;
    flag_map["VIS10"] = MF_VIS10;
    flag_map["HEARS"] = MF_HEARS;
    flag_map["GOODHEARING"] = MF_GOODHEARING;
    flag_map["SMELLS"] = MF_SMELLS;
    flag_map["KEENNOSE"] = MF_KEENNOSE;
    flag_map["STUMBLES"] = MF_STUMBLES;
    flag_map["WARM"] = MF_WARM;
    flag_map["NOHEAD"] = MF_NOHEAD;
    flag_map["HARDTOSHOOT"] = MF_HARDTOSHOOT;
    flag_map["GRABS"] = MF_GRABS;
    flag_map["BASHES"] = MF_BASHES;
    flag_map["GROUP_BASH"] = MF_GROUP_BASH;
    flag_map["DESTROYS"] = MF_DESTROYS;
    flag_map["BORES"] = MF_BORES;
    flag_map["POISON"] = MF_POISON;
    flag_map["VENOM"] = MF_VENOM;
    flag_map["BADVENOM"] = MF_BADVENOM;
    flag_map["PARALYZEVENOM"] = MF_PARALYZE;
    flag_map["BLEED"] = MF_BLEED;
    flag_map["WEBWALK"] = MF_WEBWALK;
    flag_map["DIGS"] = MF_DIGS;
    flag_map["CAN_DIG"] = MF_CAN_DIG;
    flag_map["FLIES"] = MF_FLIES;
    flag_map["AQUATIC"] = MF_AQUATIC;
    flag_map["SWIMS"] = MF_SWIMS;
    flag_map["FISHABLE"] = MF_FISHABLE;
    flag_map["ATTACKMON"] = MF_ATTACKMON;
    flag_map["ANIMAL"] = MF_ANIMAL;
    flag_map["PLASTIC"] = MF_PLASTIC;
    flag_map["SUNDEATH"] = MF_SUNDEATH;
    flag_map["ELECTRIC"] = MF_ELECTRIC;
    flag_map["ACIDPROOF"] = MF_ACIDPROOF;
    flag_map["ACIDTRAIL"] = MF_ACIDTRAIL;
    flag_map["FIREPROOF"] = MF_FIREPROOF;
    flag_map["LEAKSGAS"] = MF_LEAKSGAS;
    flag_map["SLUDGEPROOF"] = MF_SLUDGEPROOF;
    flag_map["SLUDGETRAIL"] = MF_SLUDGETRAIL;
    flag_map["FIREY"] = MF_FIREY;
    flag_map["QUEEN"] = MF_QUEEN;
    flag_map["ELECTRONIC"] = MF_ELECTRONIC;
    flag_map["FUR"] = MF_FUR;
    flag_map["LEATHER"] = MF_LEATHER;
    flag_map["FEATHER"] = MF_FEATHER;
    flag_map["CBM_CIV"] = MF_CBM_CIV;
    flag_map["BONES"] = MF_BONES;
    flag_map["FAT"] = MF_FAT;
    flag_map["IMMOBILE"] = MF_IMMOBILE;
    flag_map["HIT_AND_RUN"] = MF_HIT_AND_RUN;
    flag_map["GUILT"] = MF_GUILT;
    flag_map["HUMAN"] = MF_HUMAN;
    flag_map["NO_BREATHE"] = MF_NO_BREATHE;
    flag_map["REGENERATES_50"] = MF_REGENERATES_50;
    flag_map["REGENERATES_10"] = MF_REGENERATES_10;
    flag_map["FLAMMABLE"] = MF_FLAMMABLE;
    flag_map["REVIVES"] = MF_REVIVES;
    flag_map["CHITIN"] = MF_CHITIN;
    flag_map["VERMIN"] = MF_VERMIN;
    flag_map["NOGIB"] = MF_NOGIB;
    flag_map["HUNTS_VERMIN"] = MF_HUNTS_VERMIN;
    flag_map["SMALL_BITER"] = MF_SMALL_BITER;
    flag_map["ABSORBS"] = MF_ABSORBS;
    flag_map["LARVA"] = MF_LARVA;
    flag_map["ARTHROPOD_BLOOD"] = MF_ARTHROPOD_BLOOD;
    flag_map["ACID_BLOOD"] = MF_ACID_BLOOD;
    flag_map["BILE_BLOOD"] = MF_BILE_BLOOD;
    flag_map["REGEN_MORALE"] = MF_REGENMORALE;
    flag_map["CBM_POWER"] = MF_CBM_POWER;
    flag_map["CBM_SCI"] = MF_CBM_SCI;
    flag_map["CBM_OP"] = MF_CBM_OP;
    flag_map["CBM_TECH"] = MF_CBM_TECH;
    flag_map["CBM_SUBS"] = MF_CBM_SUBS;
    flag_map["SWARMS"] = MF_SWARMS;
    flag_map["CLIMBS"] = MF_CLIMBS;
    flag_map["GROUP_MORALE"] = MF_GROUP_MORALE;
    flag_map["INTERIOR_AMMO"] = MF_INTERIOR_AMMO;
    flag_map["PUSH_MON"] = MF_PUSH_MON;
}

void MonsterGenerator::set_species_ids( mtype *mon )
{
    const std::set< std::string > &specs = mon->species;
    std::set< int > ret;
    for( const auto &s : specs ) {
        auto iter = mon_species.find( s );
        if( iter != mon_species.end() ) {
            mon->species_id.insert( iter->second->short_id );
        } else {
            debugmsg( "Tried to assign species %s to monster %s, but no entry for the species exists", s.c_str(), mon->id.c_str() );
        }
    }
}

void MonsterGenerator::load_monster(JsonObject &jo)
{
    // id
    std::string mid;
    if (jo.has_member("id")) {
        mid = jo.get_string("id");
        if (mon_templates.count(mid) > 0) {
            delete mon_templates[mid];
        }

        mtype *newmon = new mtype;

        newmon->id = mid;
        newmon->name = jo.get_string("name").c_str();
        if(jo.has_member("name_plural")) {
            newmon->name_plural = jo.get_string("name_plural");
        } else {
            // default behaviour: Assume the regular plural form (appending an “s”)
            newmon->name_plural = newmon->name + "s";
        }
        newmon->description = _(jo.get_string("description").c_str());

        // Have to overwrite the default { "hflesh" } here
        newmon->mat = { jo.get_string("material") };

        newmon->species = jo.get_tags("species");
        newmon->categories = jo.get_tags("categories");

        // See monfaction.cpp
        newmon->default_faction =
            monfactions::get_or_add_faction( mfaction_str_id( jo.get_string("default_faction") ) );

        newmon->sym = jo.get_string("symbol");
        if( utf8_wrapper( newmon->sym ).display_width() != 1 ) {
            jo.throw_error( "monster symbol should be exactly one console cell width", "symbol" );
        }
        newmon->color = color_from_string(jo.get_string("color"));
        newmon->size = get_from_string(jo.get_string("size", "MEDIUM"), size_map, MS_MEDIUM);
        newmon->phase = get_from_string(jo.get_string("phase", "SOLID"), phase_map, SOLID);

        newmon->difficulty = jo.get_int("diff", 0);
        newmon->agro = jo.get_int("aggression", 0);
        newmon->morale = jo.get_int("morale", 0);
        newmon->speed = jo.get_int("speed", 0);
        newmon->melee_skill = jo.get_int("melee_skill", 0);
        newmon->melee_dice = jo.get_int("melee_dice", 0);
        newmon->melee_sides = jo.get_int("melee_dice_sides", 0);
        newmon->melee_cut = jo.get_int("melee_cut", 0);
        newmon->sk_dodge = jo.get_int("dodge", 0);
        newmon->armor_bash = jo.get_int("armor_bash", 0);
        newmon->armor_cut = jo.get_int("armor_cut", 0);
        newmon->hp = jo.get_int("hp", 0);
        jo.read("starting_ammo", newmon->starting_ammo);
        newmon->luminance = jo.get_float("luminance", 0);
        newmon->revert_to_itype = jo.get_string( "revert_to_itype", "" );
        newmon->vision_day = jo.get_int("vision_day", 40);
        newmon->vision_night = jo.get_int("vision_night", 1);

        if (jo.has_array("attack_effs")) {
            JsonArray jsarr = jo.get_array("attack_effs");
            while (jsarr.has_more()) {
                JsonObject e = jsarr.next_object();
                mon_effect_data new_eff(e.get_string("id", "null"), e.get_int("duration", 0),
                                    get_body_part_token( e.get_string("bp", "NUM_BP") ), e.get_bool("permanent", false),
                                    e.get_int("chance", 100));
                newmon->atk_effs.push_back(new_eff);
            }
        }

        if (jo.has_string("death_drops")) {
            newmon->death_drops = jo.get_string("death_drops");
        } else if (jo.has_object("death_drops")) {
            JsonObject death_frop_json = jo.get_object("death_drops");
            // Make up a group name, should be unique (include the monster id),
            newmon->death_drops = newmon->id + "_death_drops_auto";
            const std::string subtype = death_frop_json.get_string("subtype", "distribution");
            // and load the entry as a standard item group using the made up name.
            item_group::load_item_group(death_frop_json, newmon->death_drops, subtype);
        } else if (jo.has_member("death_drops")) {
            jo.throw_error("invalid type, must be string or object", "death_drops");
        }

        newmon->dies = get_death_functions(jo, "death_function");
        load_special_defense(newmon, jo, "special_when_hit");
        load_special_attacks(newmon, jo, "special_attacks");

        if (jo->has_member("upgrades")) {
            JsonObject &jou = jo->get_object("upgrades");
            newmon->half_life = jou.get_int("half_life", -1);
            newmon->upgrade_group = mongroup_id( jou.get_string("upgrade_group", "GROUP_NULL") );
            newmon->upgrade_into = jou.get_string("upgrade_into", "NULL");
        }

        std::set<std::string> flags, anger_trig, placate_trig, fear_trig;
        flags = jo.get_tags("flags");
        anger_trig = jo.get_tags("anger_triggers");
        placate_trig = jo.get_tags("placate_triggers");
        fear_trig = jo.get_tags("fear_triggers");

        newmon->flags = get_set_from_tags(flags, flag_map, MF_NULL);
        newmon->anger = get_set_from_tags(anger_trig, trigger_map, MTRIG_NULL);
        newmon->fear = get_set_from_tags(fear_trig, trigger_map, MTRIG_NULL);
        newmon->placate = get_set_from_tags(placate_trig, trigger_map, MTRIG_NULL);

        mon_templates[mid] = newmon;
    }
}
void MonsterGenerator::load_species(JsonObject &jo)
{
    // id, flags, triggers (anger, placate, fear)
    std::string sid;
    if (jo.has_member("id")) {
        sid = jo.get_string("id");
        int species_num = mon_species.size();
        if (mon_species.count(sid) > 0) {
            species_num = mon_species[sid]->short_id; // Keep it or weird things may happen
            delete mon_species[sid];
        }

        std::set<std::string> sflags, sanger, sfear, splacate;
        sflags = jo.get_tags("flags");
        sanger = jo.get_tags("anger_triggers");
        sfear  = jo.get_tags("fear_triggers");
        splacate = jo.get_tags("placate_triggers");

        std::set<m_flag> flags = get_set_from_tags(sflags, flag_map, MF_NULL);
        std::set<monster_trigger> anger, fear, placate;
        anger = get_set_from_tags(sanger, trigger_map, MTRIG_NULL);
        fear = get_set_from_tags(sfear, trigger_map, MTRIG_NULL);
        placate = get_set_from_tags(splacate, trigger_map, MTRIG_NULL);

        species_type *new_species = new species_type(species_num, sid, flags, anger, fear, placate);

        mon_species[sid] = new_species;
    }
}

mtype *MonsterGenerator::get_mtype(std::string mon)
{
    mtype *default_montype = mon_templates["mon_null"];

    if (mon == "mon_zombie_fast") {
        mon = "mon_zombie_dog";
    }
    if (mon == "mon_fungaloid_dormant") {
        mon = "mon_fungaloid";
    }

    if (mon_templates.find(mon) != mon_templates.end()) {
        return mon_templates[mon];
    }
    debugmsg("Could not find monster with type %s", mon.c_str());
    return default_montype;
}
bool MonsterGenerator::has_mtype(const std::string &mon) const
{
    return mon_templates.count(mon) > 0;
}
bool MonsterGenerator::has_species(const std::string &species) const
{
    return mon_species.count(species) > 0;
}
mtype *MonsterGenerator::get_mtype(int mon)
{
    int count = 0;
    for( auto &elem : mon_templates ) {
        if (count == mon) {
            return elem.second;
        }
        ++count;
    }
    return mon_templates["mon_null"];
}

std::map<std::string, mtype *> MonsterGenerator::get_all_mtypes() const
{
    return mon_templates;
}
std::vector<std::string> MonsterGenerator::get_all_mtype_ids() const
{
    std::vector<std::string> hold;
    for( const auto &elem : mon_templates ) {
        hold.push_back( elem.first );
    }
    return hold;
}

mtype *MonsterGenerator::get_valid_hallucination()
{
    std::vector<mtype *> potentials;
    for( auto &elem : mon_templates ) {
        if( elem.first != "mon_null" && elem.first != "mon_generator" ) {
            potentials.push_back( elem.second );
        }
    }

    return potentials[rng(0, potentials.size() - 1)];
}

m_flag MonsterGenerator::m_flag_from_string( std::string flag ) const
{
    return flag_map.find( flag )->second;
}

std::vector<mon_action_death> MonsterGenerator::get_death_functions(JsonObject &jo,
        std::string member)
{
    std::vector<mon_action_death> deaths;

    std::set<std::string> death_flags = jo.get_tags(member);

    std::set<std::string>::iterator it = death_flags.begin();
    for (; it != death_flags.end(); ++it) {
        if ( death_map.find(*it) != death_map.end() ) {
            deaths.push_back(death_map[*it]);
        } else {
            jo.throw_error("Invalid death_function");
        }
    }

    if (deaths.empty()) {
        deaths.push_back(death_map["NORMAL"]);
    }
    return deaths;
}

void MonsterGenerator::load_special_attacks(mtype *m, JsonObject &jo, std::string member) {
    m->sp_attack.clear(); // make sure we're running with
    m->sp_freq.clear();   // everything cleared

    if (jo.has_array(member)) {
        JsonArray outer = jo.get_array(member);
        while (outer.has_more()) {
            JsonArray inner = outer.next_array();
            if ( attack_map.find(inner.get_string(0)) != attack_map.end() ) {
                m->sp_attack.push_back(attack_map[inner.get_string(0)]);
                m->sp_freq.push_back(inner.get_int(1));
            } else {
                inner.throw_error("Invalid special_attacks");
            }
        }
    }

    if (m->sp_attack.empty()) {
        m->sp_attack.push_back(attack_map["NONE"]);
        m->sp_freq.push_back(0);
    }
}

void MonsterGenerator::load_special_defense(mtype *m, JsonObject &jo, std::string member) {
    if (jo.has_array(member)) {
        JsonArray jsarr = jo.get_array(member);
        if ( defense_map.find(jsarr.get_string(0)) != defense_map.end() ) {
            m->sp_defense = defense_map[jsarr.get_string(0)];
            m->def_chance = jsarr.get_int(1);
        } else {
            jsarr.throw_error("Invalid special_when_hit");
        }
    }

    if (m->sp_defense == NULL) {
        m->sp_defense = defense_map["NONE"];
    }
}

template <typename T>
std::set<T> MonsterGenerator::get_set_from_tags(std::set<std::string> tags,
        std::map<std::string, T> conversion_map, T fallback)
{
    std::set<T> ret;

    if (!tags.empty()) {
        for( const auto &tag : tags ) {
            if( conversion_map.find( tag ) != conversion_map.end() ) {
                ret.insert( conversion_map[tag] );
            }
        }
    }
    if (ret.empty()) {
        ret.insert(fallback);
    }

    return ret;
}

template <typename T>
T MonsterGenerator::get_from_string(std::string tag, std::map<std::string, T> conversion_map,
                                    T fallback)
{
    T ret = fallback;
    if (conversion_map.find(tag) != conversion_map.end()) {
        ret = conversion_map[tag];
    }
    return ret;
}

void MonsterGenerator::check_monster_definitions() const
{
    for( const auto &elem : mon_templates ) {
        const mtype *mon = elem.second;
        for(std::set<std::string>::iterator spec = mon->species.begin(); spec != mon->species.end();
            ++spec) {
            if(!has_species(*spec)) {
                debugmsg("monster %s has invalid species %s", mon->id.c_str(), spec->c_str());
            }
        }
        if (!mon->death_drops.empty() && !item_group::group_is_defined(mon->death_drops)) {
            debugmsg("monster %s has unknown death drop item group: %s", mon->id.c_str(),
                     mon->death_drops.c_str());
        }
        if( !mon->revert_to_itype.empty() && !item::type_is_defined( mon->revert_to_itype ) ) {
            debugmsg("monster %s has unknown revert_to_itype: %s", mon->id.c_str(),
                     mon->revert_to_itype.c_str());
        }
        for( auto & s : mon->starting_ammo ) {
            if( !item::type_is_defined( s.first ) ) {
                debugmsg( "starting ammo %s of monster %s is unknown", s.first.c_str(), mon->id.c_str() );
            }
        }
        if( !mon->upgrade_group.is_valid() ) {
            debugmsg( "upgrade_group %s of monster %s is not a valid monster group",
                      mon->upgrade_group.c_str(), mon->id.c_str() );
        }
        if( mon->upgrades_into != "NULL" && !has_mtype( mon->upgrades_into ) ) {
            debugmsg( "upgrades_into %s of monster %s is not a valid monster id",
                      mon->upgrades_into.c_str(), mon->id.c_str() );
        }
    }
}
