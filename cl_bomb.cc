#include "globals.h"

cl_bomb::cl_bomb(): cl_sphere(TYPE_BOMB,POD_DIAM)
{
}




void cl_bomb::activate(cl_object *activator)
{
	cl_sphere::activate(NULL);
	reset();
}




void cl_bomb::reset()
{
	cl_sphere::reset();
	setMass(1);

	explode_timer = 0;
	shockwave_diam = (int)diam;
	strobe_colour = 0;
}




/*** Set the mass. damage and the colour ***/
void cl_bomb::setMass(int m)
{
	mass = m;
	sprintf(text,"%d",mass);

	// Do no damage until we explode
	damage = 0;
	blast_radius = 200 + 100 * mass;
	explode_bits_cnt = 20 * mass;
	setExplodeBits();

	fireball_diam = 10;
	explode_colour = TURQUOISE;

	main_colour = YELLOW - mass * 3;

	// Increase mass and damage every 15 secs + up to 5 secs
	inc_at = 600 + (int)random() % 200;

	flash_timer = 0;
}




/*** Bounce off something or explode ***/
void cl_bomb::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_SHIP_LASER:
	case_POWERUPS:
		break;

	case TYPE_BULLET: 
	case TYPE_PLASMA:
	case TYPE_MISSILE:
	case TYPE_STALAG_LASER:
		// Only damaged if been grabbed
		if (grabber && subDamage(obj)) flash_timer = 2;
		break;

	default:
		bounceOffObject(obj);
		spin();
	}
}




/*** Call parent run to bounce etc then see if its time to increase mass ***/
void cl_bomb::run()
{
	cl_sphere::run();

	if (flash_timer) --flash_timer;

	// We explode after a given number of bounces if we've been dropped
	// regardless of hitting anything.
	if (released)
	{
		// 80 * 25000 (mainloop delay) = 2,000,000 = 2 seconds delay
		// before we explode
		if (++explode_timer == 80)
		{
			setStage(STAGE_EXPLODE);
			return;
		}
		strobe_colour = (strobe_colour + 2) % (YELLOW - MAUVE);
		return;
	}

	// Up to max mass of 5 which gives damage of 50
	if (mass < 5 && stage_timer == inc_at) 
	{
		setMass(mass + 1);
		stage_timer = 0;
	}
}




/*** This is where we cause the damage ***/
void cl_bomb::runExplode()
{
	cl_object *obj;
	int o;

	if (stage_timer == 1)
	{
		// Do sound. Small = 1->2, Medium = 3->4, Large = 5+
		if (mass < 3) 
			playSound(SND_SMALL_BOMB_EXPLODE);
		else if (mass < 5) 
			playSound(SND_MEDIUM_BOMB_EXPLODE);
		else
			playSound(SND_LARGE_BOMB_EXPLODE);

		damage = mass * 10;

		// Check for objects within blast radius. Only check once 
		// else we'd keep calling collidedWith() on objects inside
		FOR_ALL_OBJECTS(o)
		{
			obj = object[o];
			if (obj != this &&
			    obj->stage == STAGE_RUN &&
			    distToObject(obj) <= blast_radius)
			{
				switch(obj->type)
				{
				case TYPE_LAIR:
				case TYPE_SILO:
				case TYPE_PLASMA:
				case TYPE_MISSILE:
				case TYPE_GRYPPER:
				case TYPE_ZOMBIE:
				case TYPE_SKYTH:
				case TYPE_STALAG:
				case TYPE_DISRUPTOR:
				case TYPE_STRIKER:
					obj->collidedWith(this);
					break;

				default:
					break;
				}
			}
		}
	}
	else if (stage_timer < 20) fireball_diam += 20;
	else if (fireball_diam > 0) fireball_diam -= 10;
	else setStage(STAGE_INACTIVE);

	if (explode_colour < RED) explode_colour += 0.5;

	runExplodeBits();

	shockwave_diam += 40;
}




void cl_bomb::draw()
{
	double dm;
	int col;
	int i;

	if (flash_timer) objDrawOrFillCircle(WHITE,0,diam,0,0,true);
	else if (released)
	{
		// Draw concentric rings
		dm = diam / 4;
		for(i=4;i;--i)
		{
			col = (strobe_colour + i * 2) % (YELLOW - MAUVE) + MAUVE;
			objDrawOrFillCircle(col,0,dm*i,0,0,true);
		}
	}
	else objDrawOrFillCircle(main_colour,0,diam,0,0,true);

	drawText(text,BLACK,2,angle,0,1,1,draw_x,y);
}




void cl_bomb::drawExplode()
{
	if (canDrawShockwave())
		objDrawOrFillCircle(YELLOW,5,shockwave_diam,0,0,false);

	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
	drawExplodeBits();
}




bool cl_bomb::canDrawShockwave()
{
	return shockwave_diam < blast_radius * 2;
}
