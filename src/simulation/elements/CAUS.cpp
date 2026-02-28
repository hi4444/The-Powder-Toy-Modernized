#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_CAUS()
{
	Identifier = "DEFAULT_PT_CAUS";
	Name = "CAUS";
	Colour = 0x80FFA0_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 1.50f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	HeatConduct = 70;
	Description = "Caustic Gas, acts like ACID.";

	Properties = TYPE_GAS|PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.life = 75;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	auto &part = parts[i];
	bool converted = false;
	for (int rx = -2; rx <= 2; rx++)
	{
		for (int ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rid = ID(r);
				int rt = TYP(r);
				if (rt == PT_GAS)
				{
					if (sim->pv[(y+ry)/CELL][(x+rx)/CELL] > 3)
					{
						sim->part_change_type(rid, x+rx, y+ry, PT_RFRG);
						sim->part_change_type(i, x, y, PT_RFRG);
						converted = true;
					}
				}
				else if (rt != PT_ACID && rt != PT_CAUS && rt != PT_RFRG && rt != PT_RFGL)
				{
					if ((rt != PT_CLNE && rt != PT_PCLN && ((rt != PT_FOG && rt != PT_RIME) || parts[rid].tmp <= 5) && sim->rng.chance(elements[rt].Hardness, 1000)) && part.life > 50)
					{
						// GLAS protects stuff from acid
						if (sim->parts_avg(i, rid, PT_GLAS) != PT_GLAS)
						{
							float newtemp = ((60.0f - (float)elements[rt].Hardness)) * 7.0f;
							if (newtemp < 0)
								newtemp = 0;
							part.temp += newtemp;
							part.life--;
							sim->kill_part(rid);
						}
					}
					else if (part.life <= 50)
					{
						sim->kill_part(i);
						return 1;
					}
				}
			}
		}
	}
	return converted;
}
