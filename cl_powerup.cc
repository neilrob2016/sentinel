#include "globals.h"

// Hang around for 30 seconds
#define TTL 1200


/*** Constructor ***/
cl_powerup::cl_powerup(en_object_type ty): cl_object(ty,SHIP_DIAM*2)
{
}




/*** Find a nice spot to hang around in ***/
void cl_powerup::activate(cl_object *activator)
{
	cl_object *obj;
	int o;

	min_y = (double)max_hill_height + radius;
	max_y = (double)SCR_TOP - diam;
	if (level >= MIN_LEVEL_TOP_HILLS) max_y -= max_hill_height;

	setStage(STAGE_RUN);

	do
	{
		x = random() % LANDSCAPE_WIDTH;
		y = min_y + (random() % (int)(max_y - min_y));

		// Make sure we haven't appear in the middle of a static 
		// object
		FOR_ALL_OBJECTS(o)
		{
			obj = object[o];
			if (obj == this) continue;

			switch(obj->type)
			{
			case TYPE_LAIR:
			case TYPE_MOONBASE:
			case TYPE_SILO:
			case TYPE_SHIP:
			case TYPE_POD:
			case TYPE_BOMB:
			case TYPE_SHIELD_POWERUP:
			case TYPE_LASER_POWERUP:
			case TYPE_HYPERSPACE_POWERUP:
				if (distToObject(obj) > 0) continue;
				break;

			default:
				continue;
			}
			break;
		}
	} while(o < NUM_OBJECTS);

	setSpeed();
	playSound(SND_POWERUP_APPEAR);
}




void cl_powerup::setSpeed()
{
	xspd = random() % 2 ? 0.5 : -0.5;
	yspd = random() % 2 ? 0.5 : -0.5;
}




void cl_powerup::reset()
{
	setStage(STAGE_INACTIVE);
}




void cl_powerup::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_SHIP:
		textobj[TEXT_POWERUP]->activatePowerup(this);
		playSound(SND_POWERUP_ACTIVATE);
		setStage(STAGE_INACTIVE);
		break;

	case TYPE_MOONBASE:
	case TYPE_LAIR:
	case TYPE_SILO:
		xspd = -xspd;
		yspd = -yspd;
		break;

	default:
		break;
	}
}




/*** Is called by draw functions to see if powerup should flash when its 5 
     seconds from disappearing ***/
bool cl_powerup::doDraw()
{
	return (stage_timer < TTL - 200 || (stage_timer % 10) > 5);
}




void cl_powerup::run()
{
	if (stage_timer == TTL)
	{
		playSound(SND_POWERUP_DISAPPEAR);
		setStage(STAGE_INACTIVE);
		return;
	}

	// Change main_colour so it flashes in the radar
	main_colour = (stage_timer % 20 < 10) ? draw_colour : BLACK;
	updateXY(x,y);

	if (y > max_y)
	{
		y = max_y;
		yspd = -fabs(yspd);
	}
	else if (y < max_hill_height)
	{
		y = max_hill_height;
		yspd = fabs(yspd);
	}

	if (!(stage_timer % 200)) setSpeed();
}

