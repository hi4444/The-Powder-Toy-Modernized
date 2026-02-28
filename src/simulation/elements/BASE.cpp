#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_BASE()
{
	Identifier = "DEFAULT_PT_BASE";
	Name = "BASE";
	Colour = 0x90D5FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.5f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.96f;
	Collision = 0.0f;
	Gravity = 0.08f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 16;

	HeatConduct = 31;
	HeatCapacity = 1.5f;
	Description = "Corrosive liquid. Rusts conductive solids, neutralizes acid.";

	Properties = TYPE_LIQUID|PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.life = 76;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	auto &part = parts[i];

	//Reset spark effect
	part.tmp = 0;

	if (part.life < 1)
		part.life = 1;

	if (part.life > 100)
		part.life = 100;

	float pres = sim->pv[y/CELL][x/CELL];

	//Base evaporates into BOYL or increases concentration
	if (part.life < 100 && pres < 10.0f && part.temp > (120.0f + 273.15f))
	{
		//Slow down boiling
		if (sim->rng.chance(1, 20))
		{
			//This way we preserve the total amount of concentrated BASE in the solution
			if (sim->rng.chance(1, part.life+1))
			{
				auto temp = part.temp;
				sim->create_part(i, x, y, PT_BOYL);
				part.temp = temp;
				return 1;
			}
			else
			{
				part.life++;
				//Enthalpy of vaporization
				part.temp -= 20.0f / ((float)part.life);
			}
		}
	}

	//Base's freezing point lowers with its concentration
	if (part.temp < (273.15f - ((float)part.life)/4.0f))
	{
		//We don't save base's concentration, so ICEI(BASE) will unfreeze into life = 0
		sim->part_change_type(i, x, y, PT_ICEI);
		part.ctype = PT_BASE;
		part.life = 0;
		return 1;
	}

	//Reactions
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				int rid = ID(r);
				auto &rpart = parts[rid];

				//Don't react with some elements
				if (rt != PT_BASE && rt != PT_SALT && rt != PT_SLTW && rt != PT_BOYL && rt != PT_MERC &&
					       	rt != PT_BMTL && rt != PT_BRMT && rt != PT_SOAP && rt != PT_CLNE && rt != PT_PCLN &&
						!(rt == PT_ICEI && rpart.ctype == PT_BASE) && !(rt == PT_SNOW && rpart.ctype == PT_BASE) &&
						!(rt == PT_SPRK && rpart.ctype == PT_BMTL) && !(rt == PT_SPRK && rpart.ctype == PT_BRMT))
				{
					//Base is diluted by water
					if (part.life > 1 && (rt == PT_WATR || rt == PT_DSTW || rt == PT_CBNW))
					{
					       	if (sim->rng.chance(1, 20))
						{
							int saturh = part.life/2;

							sim->part_change_type(rid, x+rx, y+ry, PT_BASE);
							rpart.life = saturh;
							rpart.temp += ((float)saturh)/10.0f;
							part.life -= saturh;
						}
					} //Base neutralizes acid
					else if (rt == PT_ACID && part.life >= rpart.life)
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						sim->part_change_type(rid, x+rx, y+ry, PT_SLTW);
						return 1;
					} //Base neutralizes CAUS
					else if (rt == PT_CAUS && part.life >= rpart.life)
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						sim->kill_part(rid);
						return 1;
					} //BASE + OIL = SOAP
					else if (part.life >= 70 && rt == PT_OIL)
					{
						sim->part_change_type(i, x, y, PT_SOAP);
						part.tmp = part.tmp2 = part.ctype = 0;
						sim->kill_part(rid);
						return 1;
					} //BASE + GOO = GEL
					else if (part.life > 1 && rt == PT_GOO)
					{
						sim->create_part(rid, x+rx, y+ry, PT_GEL);
						part.life--;
					} //BASE + BCOL = GUNP
					else if (part.life > 1 && rt == PT_BCOL)
					{
						sim->create_part(rid, x+rx, y+ry, PT_GUNP);
						part.life--;
					} //BASE + Molten ROCK = MERC
				        else if (rt == PT_LAVA && rpart.ctype == PT_ROCK && pres >= 10.0f && sim->rng.chance(1, 1000))
					{
						sim->part_change_type(i, x, y, PT_MERC);
						part.life = 0;
						part.tmp = 10;

						sim->kill_part(rid);
						return 1;
					} //Base rusts conductive solids
					else if (part.life >= 10 &&
						       	(elements[rt].Properties & (TYPE_SOLID|PROP_CONDUCTS)) == (TYPE_SOLID|PROP_CONDUCTS) && sim->rng.chance(1, 10))
					{
						sim->part_change_type(rid, x+rx, y+ry, PT_BMTL);
						rpart.tmp = sim->rng.between(20, 29);
						part.life--;
						//Draw a spark effect
						part.tmp = 1;
					} //Base destroys a substance slower if acid destroys it faster
					else if (elements[rt].Hardness > 0 && elements[rt].Hardness < 50 &&
							part.life >= (2*elements[rt].Hardness) && sim->rng.chance(50-elements[rt].Hardness, 1000))
					{
						sim->kill_part(rid);
						part.life -= 2;
						//Draw a spark
						part.tmp = 1;
					}
				}
			}
		}
	}

	//Diffusion
	for (auto trade = 0; trade<2; trade++)
	{
		auto rx = sim->rng.between(-1, 1);
		auto ry = sim->rng.between(-1, 1);
		if (rx || ry)
		{
			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			int rid = ID(r);
			if (TYP(r) == PT_BASE && (part.life > parts[rid].life) && part.life > 1)
			{
				int temp = part.life - parts[rid].life;
				if (temp == 1)
				{
					parts[rid].life++;
					part.life--;
				}
				else if (temp>0)
				{
					parts[rid].life += temp/2;
					part.life -= temp/2;
				}
			}
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int s = cpart->life;

	if (s <= 25)
	{
		*colr = 0x33;
		*colg = 0x4C;
		*colb = 0xD8;
	}
	else if (s <= 50)
	{
		*colr = 0x58;
		*colg = 0x83;
		*colb = 0xE8;
	}
	else if (s <= 75)
	{
		*colr = 0x7D;
		*colg = 0xBA;
		*colb = 0xF7;
	}

	*pixel_mode |= PMODE_BLUR;

	if (cpart->tmp == 1)
		*pixel_mode |= PMODE_SPARK;

	return 0;
}
