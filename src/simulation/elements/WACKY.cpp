#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

// ==========================
// EDIT THESE LISTS
// ==========================

// If this list has elements, ONLY these will spawn.
static const int whitelist[] = {
    PT_POLO,
    PT_GRVT,
    PT_PROT,
    PT_BVBR,
    PT_VIBR,
    PT_EXOT,
    PT_ELEC,
    PT_ISZS,
    PT_ISOZ,
    //PT_WARP,
    PT_DEUT,
    //PT_AMTR,
    PT_URAN,
    PT_PHOT,
    PT_PLUT,
    PT_NEUT
};

// These will NEVER spawn (used only if whitelist is empty)
static const int blacklist[] = {
    PT_SING,
    PT_AMTR,
	PT_WARP,
    PT_WACKY,
};

// ==========================

static bool isInList(int type, const int* list, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (list[i] == type)
            return true;
    }
    return false;
}

void Element::Element_WACKY()
{
    Identifier = "DEFAULT_PT_WACKY";
    Name = "WACKY";
    Colour = 0xFFD010_rgb;
    MenuVisible = 1;
    MenuSection = SC_SPECIAL;
    Enabled = 1;

    Advection = 0.0f;
    AirDrag = 0.00f * CFDS;
    AirLoss = 0.90f;
    Loss = 0.00f;
    Collision = 0.0f;
    Gravity = 0.0f;
    Diffusion = 0.00f;
    HotAir = 0.000f * CFDS;
    Falldown = 0;

    Flammable = 0;
    Explosive = 0;
    Meltable = 0;
    Hardness = 1;

    Weight = 100;

    HeatConduct = 251;
    Description = "Wacky, A special element that spawns nuclear elements randomly.";

    Properties = TYPE_SOLID | PROP_NOCTYPEDRAW;

    LowPressure = IPL;
    LowPressureTransition = NT;
    HighPressure = IPH;
    HighPressureTransition = NT;
    LowTemperature = ITL;
    LowTemperatureTransition = NT;
    HighTemperature = ITH;
    HighTemperatureTransition = NT;

    Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
    if (sim->rng.chance(1, 30))
    {
        int rtype;

        int whitelistCount = sizeof(whitelist) / sizeof(whitelist[0]);

        if (whitelistCount > 0)
        {
            // Whitelist mode
            rtype = whitelist[sim->rng.between(0, whitelistCount - 1)];
        }
        else
        {
            // Blacklist mode
            int blacklistCount = sizeof(blacklist) / sizeof(blacklist[0]);
            int maxElement = 176; // Adjust if your build changes

            do {
                rtype = sim->rng.between(0, maxElement);
            } while (isInList(rtype, blacklist, blacklistCount));
        }

        parts[i].tmp = rtype;

        sim->create_part(-1,
            x + sim->rng.between(-1, 1),
            y + sim->rng.between(-1, 1),
            rtype);
    }

    return 0;
}