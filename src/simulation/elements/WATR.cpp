#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_WATR()
{
	Identifier = "DEFAULT_PT_WATR";
	Name = "WATR";
	Colour = 0x2030D0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 30;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	HeatConduct = 29;
	Description = "Water. Conducts electricity, freezes, and extinguishes fires.";

	Properties = TYPE_LIQUID | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_NEUTPASS | PROP_PHOTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &part = parts[i];
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int typ = TYP(r);
				int rid = ID(r);
				if (typ == PT_SALT && sim->rng.chance(1, 50))
				{
					sim->part_change_type(i,x,y,PT_SLTW);
					// on average, convert 3 WATR to SLTW before SALT turns into SLTW
					if (sim->rng.chance(1, 3))
						sim->part_change_type(rid,x+rx,y+ry,PT_SLTW);
				}
				else if ((typ == PT_RBDM || typ == PT_LRBD) && (sim->legacy_enable || part.temp > (273.15f + 12.0f)) && sim->rng.chance(1, 100))
				{
					sim->part_change_type(i,x,y,PT_FIRE);
					part.life = 4;
					part.ctype = PT_WATR;
				}
				else if (typ == PT_FIRE && parts[rid].ctype != PT_WATR)
				{
					sim->kill_part(rid);
					if (sim->rng.chance(1, 30))
					{
						sim->kill_part(i);
						return 1;
					}
				}
				else if (typ == PT_SLTW && sim->rng.chance(1, 2000))
				{
					sim->part_change_type(i,x,y,PT_SLTW);
				}
				else if (typ == PT_ROCK && fabs(part.vx) + fabs(part.vy) >= 0.5f && sim->rng.chance(1, 1000)) // ROCK erosion
				{
					if (sim->rng.chance(1,3))
						sim->part_change_type(rid,x+rx,y+ry,PT_SAND);
					else
						sim->part_change_type(rid,x+rx,y+ry,PT_STNE);
				}
			}
		}
	}
	return 0;
}
