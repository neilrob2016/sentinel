#include "globals.h"

cl_sphere::cl_sphere(en_object_type ty, double dm): cl_object(ty,dm)
{
}




void cl_sphere::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setExplodeBits();
	setRandomGroundLocation(0,radius);

	angle_add = 0;
	angle = 0;
	xspd = yspd = 0;
	health = level < 10 ? 50 - level * 2: 30; // pod & bomb

	reset();
}




void cl_sphere::reset()
{
	if (stage == STAGE_RUN) stage_timer = 0;

	grabber = NULL;
	last_grabber = NULL;
	released = false;
}




bool cl_sphere::grab(cl_object *g)
{
	// Can't grab it if its exploding, is already grabbed or is a bomb
	// about to explode
	if (stage != STAGE_RUN || grabber || (type == TYPE_BOMB && released)) 
		return false;

	// If ship grabs pod before it hits the ground then extra bonus
	if (g->type == TYPE_SHIP && 
	    last_grabber && last_grabber->type == TYPE_GRYPPER)
		activateBonusText(100,this);
		
	last_grabber = grabber = g;
	xspd = yspd = 0;
	released = false;
	return true;
}




void cl_sphere::release()
{
	if (stage == STAGE_RUN)
	{
		grabber = NULL;
		released = true;
	}
}




/*** Bounce off hills and spin ***/
void cl_sphere::run()
{
	st_hill_inside *hi;
	double hill_height;

	if (grabber)
	{
		// If grabber has died then we're free
		if (grabber->stage != STAGE_RUN) grabber = NULL;
		else
		// Do nothing if grabbed by grypper.
		if (grabber->type == TYPE_GRYPPER) return;
	}

	yspd -= gravity;
	updatePrevious();
	updateXY(x,y);

	if (angle_add)
	{
		incAngle(angle,angle_add);
		if (angle_add < 0)
			angle_add += 0.01;
		else
			angle_add -= 0.01;
		if (fabs(angle_add) < 0.01) angle_add = 0;
	}

	if ((hi = insideHill(radius,x,y))) 
	{
		bounceOffHill(hi,grabber ? 0.4 : 0.6);
		spin();
		if (fabs(xspd) + fabs(yspd) > 1 && 
		    IN_HEARING_DISTANCE()) playSound(SND_BOUNCE);

		// Reset when bounced since player can't get bonus now
		if (last_grabber) last_grabber = NULL;
	}
	hill_height = getHillHeight(HILL_BOT,x)->y;

	// If we're at a very slow speed then stop moving so we don't slowly
	// keep drifting down hills
	if (!grabber && 
	    fabs(xspd) + fabs(yspd) < 1 && y - hill_height <= radius + 1)
	{
		// Don't set yspd to zero so gravity can act
		xspd = 0;
		angle_add = 0;

		// A released bomb stays released as it will explode
		if (type != TYPE_BOMB) released = false;
	}
}




/*** We've hit something so spin a bit. A very rough algorithm but it
     seems to work ok ***/
void cl_sphere::spin()
{
	if (fabs(angle_add) < 20)
	{
		if (SGN(xspd) != SGN(angle_add)) 
			angle_add += hypot(xspd,yspd) * SGN(xspd) * 2;
	}
	else angle_add = 20 * SGN(angle_add);
}
