#include "globals.h"

cl_pod::cl_pod(): cl_sphere(TYPE_POD,POD_DIAM)
{
	explode_bits_cnt = 10;
}




void cl_pod::activate(cl_object *activator)
{
	cl_sphere::activate(NULL);

	// If not NULL then we've been spat out by lair when it explodes
	// or been hit by a bomb
	if (activator)
	{
		x = activator->x + (random() % 11) - 5;
		y = activator->y + activator->radius + radius;
		updateXY(x,y);

		xspd = 5 + random() % 5;
		if (random() % 2) xspd = -xspd;
		yspd = 5 + random() % 5;
	}
	explode_colour = MAUVE;
	main_colour = MAUVE;
	wave_arms_timer = 0;
	flash_timer = 0;
}




/*** Bounce off something or explode ***/
void cl_pod::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_SHIP_LASER:
	case_POWERUPS:
		return;

	case TYPE_GRYPPER:
		// Don't bounce if we've been grabbed by a grypper
		if (grabber && grabber->type == TYPE_GRYPPER) return;
		break;

	case TYPE_BULLET:
	case TYPE_PLASMA:
	case TYPE_MISSILE:
	case TYPE_STALAG_LASER:
		// Only suffer damage if we've been grabbed by ship
		if (grabber == ship && !subDamage(obj)) flash_timer = 2;
		return;

	case TYPE_MOONBASE:
		if (grabber == moonbase)
		{
			incScore(250);
			ship->incHealth(5);
			setStage(STAGE_INACTIVE);
			return;
		}
		break;

	case TYPE_LAIR:
		// Only counts if we're dropped and hit it on the top
		if (!grabber && hitTop(obj))
		{
			setStage(STAGE_INACTIVE);
			return;
		}
		break;

	default:
		break;
	}
	if (grabber != moonbase)
	{
		bounceOffObject(obj);
		spin();
	}
}




/*** Returns true if we've hit lair on moonbase on the top ***/
bool cl_pod::hitTop(cl_object *obj)
{
	return (x >= obj->x - obj->radius && x <= obj->x + obj->radius &&
	        y >= obj->y + obj->radius);
}


//////////////////////////////////// RUN //////////////////////////////////////

/*** If we're not grabbed and we're within the grab radius of the moonbase
     then do stuff ***/
void cl_pod::run()
{
	if (flash_timer) --flash_timer;

	if (wave_arms_timer) 
		--wave_arms_timer;
	else
		wave_arms_timer = 50 + (int)random() % 100;

	// collidedWith() function terminates this
	if (grabber == moonbase)
	{
		updateXY(x,y);
		return;
	}
	cl_sphere::run();

	// Allow spinning (hits where hill was in previous game) but don't 
	// want to see moonbase grab beam
	if (game_stage == GAME_STAGE_SHOW_CAST) return;

	// If we're not grabbed and within grabbing distance of the moonbase
	// then set speeds
	if (!grabber && distToObject(moonbase) <= moonbase->grab_dist)
	{
		grabber = moonbase;

		headToObject(moonbase);
		xspd *= 15;
		yspd *= 15;

		stage_timer = 0;
		playSound(SND_POD_SAVED);
	}
}




void cl_pod::runExplode()
{
	if (stage_timer == 1) playSound(SND_POD_EXPLODE);
	if (stage_timer < 30)

	{
		if ((explode_colour -= 0.5) <= BLUE) explode_colour = BLUE2;
		runExplodeBits();
	}
	else setStage(STAGE_INACTIVE);
}



//////////////////////////////////// DRAW /////////////////////////////////////

void cl_pod::draw()
{
	// We actually drop the grab beam, not the moonbase. This saves the
	// moonbase having to store a list of things it might be grabbing.
	if (grabber == moonbase && stage_timer % 2)
	{
		drawLine(beam_col,20,draw_x,y,moonbase->draw_x,moonbase->y);
		beam_col = (beam_col == BLUE ? LIGHT_BLUE : BLUE);
	}

	drawMan(WHITE,wave_arms_timer > 50 && ((wave_arms_timer / 10) % 2));

	// Pod circle
	objDrawOrFillCircle(flash_timer ? WHITE : main_colour,3,diam,0,0,false);
}




void cl_pod::drawExplode()
{
	if (stage_timer < 30) drawExplodeBits();
}
