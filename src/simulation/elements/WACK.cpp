#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_WACK()
{
	Identifier = "DEFAULT_PT_WACK";
	Name = "WACK";
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
	Description = "Wack, A special element that spawns element's randomly.";

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
		int rtype = 0;

		do {
			rtype = sim->rng.between(0, 176); // PT_SEED = last element id
			parts[i].tmp = rtype;
		} 
		while (rtype == PT_VIRS || rtype == PT_LIFE || rtype == PT_VRSS || rtype == PT_VRSG);

		sim->create_part(-1,
			x + sim->rng.between(-1, 1),
			y + sim->rng.between(-1, 1),
			rtype);
	}

	return 0;
}