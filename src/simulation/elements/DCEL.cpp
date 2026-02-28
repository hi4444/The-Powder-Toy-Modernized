#include "simulation/ElementCommon.h"
#include <algorithm>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_DCEL()
{
	Identifier = "DEFAULT_PT_DCEL";
	Name = "DCEL";
	Colour = 0x99CC00_rgb;
	MenuVisible = 1;
	MenuSection = SC_FORCE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	HeatConduct = 251;
	Description = "Decelerator, slows down nearby elements.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	auto &part = parts[i];
	float multiplier = 1.0f/1.1f;
	if (part.life != 0)
	{
		multiplier = 1.0f - (std::clamp(part.life, 0, 100) / 100.0f);
	}
	part.tmp = 0;
	constexpr int offsets[4][2] = {
		{ -1,  0 },
		{  1,  0 },
		{  0, -1 },
		{  0,  1 },
	};
	for (auto const &offset : offsets)
	{
		auto nx = x + offset[0];
		auto ny = y + offset[1];
		auto r = pmap[ny][nx];
		if (!r)
			r = sim->photons[ny][nx];
		if (!r)
			continue;
		auto rt = TYP(r);
		if (elements[rt].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY))
		{
			auto rid = ID(r);
			parts[rid].vx *= multiplier;
			parts[rid].vy *= multiplier;
			part.tmp = 1;
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->tmp)
		*pixel_mode |= PMODE_GLOW;
	return 0;
}
