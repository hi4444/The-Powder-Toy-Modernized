#include "simulation/ElementCommon.h"
#include "SOAP.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void changeType(ELEMENT_CHANGETYPE_FUNC_ARGS);

void Element::Element_SOAP()
{
	Identifier = "DEFAULT_PT_SOAP";
	Name = "SOAP";
	Colour = 0xF5F5DC_rgb;
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
	Hardness = 19;

	Weight = 35;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	HeatConduct = 29;
	Description = "Soap. Creates bubbles, washes off deco color, and cures virus.";

	Properties = TYPE_LIQUID|PROP_NEUTPENETRATE|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.tmp = -1;
	DefaultProperties.tmp2 = -1;

	Update = &update;
	Graphics = &graphics;
	ChangeType = &changeType;
}

static bool validIndex(int i)
{
	return i >= 0 && i < NPART;
}

void Element_SOAP_detach(Simulation * sim, int i)
{
	if ((sim->parts[i].ctype&2) == 2 && validIndex(sim->parts[i].tmp) && sim->parts[sim->parts[i].tmp].type == PT_SOAP)
	{
		if ((sim->parts[sim->parts[i].tmp].ctype&4) == 4)
			sim->parts[sim->parts[i].tmp].ctype ^= 4;
	}

	if ((sim->parts[i].ctype&4) == 4 && validIndex(sim->parts[i].tmp2) && sim->parts[sim->parts[i].tmp2].type == PT_SOAP)
	{
		if ((sim->parts[sim->parts[i].tmp2].ctype&2) == 2)
			sim->parts[sim->parts[i].tmp2].ctype ^= 2;
	}

	sim->parts[i].ctype = 0;
}

static void attach(Particle * parts, int i1, int i2)
{
	if (!(parts[i2].ctype&4))
	{
		parts[i1].ctype |= 2;
		parts[i1].tmp = i2;

		parts[i2].ctype |= 4;
		parts[i2].tmp2 = i1;
	}
	else if (!(parts[i2].ctype&2))
	{
		parts[i1].ctype |= 4;
		parts[i1].tmp2= i2;

		parts[i2].ctype |= 2;
		parts[i2].tmp = i1;
	}
}

constexpr float FREEZING = 248.15f;
constexpr float BLEND = 0.85f;

static int update(UPDATE_FUNC_ARGS)
{
	//0x01 - bubble on/off
	//0x02 - first mate yes/no
	//0x04 - "back" mate yes/no

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	auto &part = parts[i];
	if (part.ctype & 1)
	{
		// reset invalid SOAP links
		if (!validIndex(part.tmp) || !validIndex(part.tmp2))
		{
			part.tmp = part.tmp2 = part.ctype = 0;
			return 0;
		}
		if (part.temp > FREEZING)
		{
			if (part.life <= 0)
			{
				//if only connected on one side
				if ((part.ctype & 6) != 6 && (part.ctype & 6))
				{
					int target = i;
					//break entire bubble in a loop
					while ((parts[target].ctype & 6) != 6 && (parts[target].ctype & 6) && parts[target].type == PT_SOAP)
					{
						if (parts[target].ctype & 2)
						{
							target = parts[target].tmp;
							if (!validIndex(target))
							{
								break;
							}
							Element_SOAP_detach(sim, target);
						}
						if (parts[target].ctype&4)
						{
							target = parts[target].tmp2;
							if (!validIndex(target))
							{
								break;
							}
							Element_SOAP_detach(sim, target);
						}
					}
				}
				if ((part.ctype & 6) != 6)
					part.ctype = 0;
				if (validIndex(part.tmp) && (part.ctype & 6) == 6 && (parts[part.tmp].ctype & 6) == 6 && parts[part.tmp].tmp == i)
					Element_SOAP_detach(sim, i);
			}
			part.vy = (part.vy - 0.1f) * 0.5f;
			part.vx *= 0.5f;
		}
		if (!(part.ctype & 2))
		{
			for (auto rx = -2; rx <= 2; rx++)
			{
				for (auto ry = -2; ry <= 2; ry++)
				{
					if (rx || ry)
					{
						auto r = pmap[y+ry][x+rx];
						if (!r)
							continue;
						auto rid = ID(r);
						if ((parts[rid].type == PT_SOAP) && (parts[rid].ctype & 1) && !(parts[rid].ctype & 4))
							attach(parts, i, rid);
					}
				}
			}
		}
		else
		{
			if (part.life <= 0)
			{
				for (auto rx = -2; rx <= 2; rx++)
				{
					for (auto ry = -2; ry <= 2; ry++)
					{
						if (rx || ry)
						{
							auto r = pmap[y+ry][x+rx];
							if (!r && !sim->bmap[(y+ry)/CELL][(x+rx)/CELL])
								continue;
							if (part.temp > FREEZING)
							{
								if (sim->bmap[(y+ry)/CELL][(x+rx)/CELL]
									|| (r && !(elements[TYP(r)].Properties&TYPE_GAS)
								    && TYP(r) != PT_SOAP && TYP(r) != PT_GLAS))
								{
									Element_SOAP_detach(sim, i);
									continue;
								}
							}
							if (TYP(r) == PT_SOAP)
							{
								auto rid = ID(r);
								if (parts[rid].ctype == 1)
								{
									int buf = part.tmp;

									part.tmp = rid;
									if (validIndex(buf) && parts[buf].type == PT_SOAP)
										parts[buf].tmp2 = rid;
									parts[rid].tmp2 = i;
									parts[rid].tmp = buf;
									parts[rid].ctype = 7;
								}
								else if (parts[rid].ctype == 7 && part.tmp != rid && part.tmp2 != rid)
								{
									if (validIndex(part.tmp) && parts[part.tmp].type == PT_SOAP)
										parts[part.tmp].tmp2 = parts[rid].tmp2;
									if (validIndex(parts[rid].tmp2) && parts[parts[rid].tmp2].type == PT_SOAP)
										parts[parts[rid].tmp2].tmp = part.tmp;
									parts[rid].tmp2 = i;
									part.tmp = rid;
								}
							}
						}
					}
				}
			}
		}
		if ((part.ctype & 2) && validIndex(part.tmp))
		{
			float d, dx, dy;
			dx = part.x - parts[part.tmp].x;
			dy = part.y - parts[part.tmp].y;
			d = 9/(pow(dx, 2)+pow(dy, 2)+9)-0.5;
			parts[part.tmp].vx -= dx*d;
			parts[part.tmp].vy -= dy*d;
			part.vx += dx*d;
			part.vy += dy*d;
			if ((parts[part.tmp].ctype&2) && (parts[part.tmp].ctype&1)
					&& validIndex(parts[part.tmp].tmp)
					&& (parts[parts[part.tmp].tmp].ctype&2) && (parts[parts[part.tmp].tmp].ctype&1))
			{
				int ii = parts[parts[part.tmp].tmp].tmp;
				if (validIndex(ii))
				{
					dx = parts[ii].x - parts[part.tmp].x;
					dy = parts[ii].y - parts[part.tmp].y;
					d = 81/(pow(dx, 2)+pow(dy, 2)+81)-0.5;
					parts[part.tmp].vx -= dx*d*0.5f;
					parts[part.tmp].vy -= dy*d*0.5f;
					parts[ii].vx += dx*d*0.5f;
					parts[ii].vy += dy*d*0.5f;
				}
			}
		}
	}
	else
	{
		if (sim->pv[y/CELL][x/CELL] > 0.5f || sim->pv[y/CELL][x/CELL] < (-0.5f))
		{
			part.ctype = 1;
			part.life = 10;
		}

		//SOAP+OIL foam effect
		for (auto rx=-2; rx<3; rx++)
			for (auto ry=-2; ry<3; ry++)
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (TYP(r) == PT_OIL)
					{
						float ax, ay, gx, gy;

						sim->GetGravityField(x, y, elements[PT_SOAP].Gravity, 1.0f, gx, gy);

						auto rid = ID(r);
						ax = ((part.vx - gx) * 0.5f + parts[rid].vx) / 2;
						ay = ((part.vy - gy) * 0.5f + parts[rid].vy) / 2;
						part.vx = parts[rid].vx = ax;
						part.vy = parts[rid].vy = ay;
					}
				}
	}
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)!=PT_SOAP)
				{
					auto rid = ID(r);
					auto tr = float((parts[rid].dcolour>>16)&0xFF);
					auto tg = float((parts[rid].dcolour>>8)&0xFF);
					auto tb = float((parts[rid].dcolour)&0xFF);
					auto ta = float((parts[rid].dcolour>>24)&0xFF);
					auto nr = int(tr*BLEND);
					auto ng = int(tg*BLEND);
					auto nb = int(tb*BLEND);
					auto na = int(ta*BLEND);
					parts[rid].dcolour = nr<<16 | ng<<8 | nb | na<<24;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= EFFECT_LINES|PMODE_BLUR;
	return 1;
}

static void changeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (from == PT_SOAP && to != PT_SOAP)
	{
		Element_SOAP_detach(sim, i);
	}
}
