#include "globals.h"

cl_ship_laser::cl_ship_laser(): cl_laser(TYPE_SHIP_LASER)
{
	damage = 1;
}




void cl_ship_laser::activate(cl_object *activator)
{
	main_colour = YELLOW;
	angle = ship->angle;

	if (ship->dual_laser)
	{
		// activator pointer just used as a flag to indicate side of 
		// ship. Set = left side, null = right side
		y = 0;
		x = activator ? -25 : 25;
		rotatePoint(x,y,angle,&x,&y);
		x += ship->x;
		y += ship->y;
	}
	else
	{
		x = ship->x;
		y = ship->y;
	}

	xspd = SIN(angle);
	yspd = COS(angle);
	x += xspd * 15;
	y += yspd * 15;

	cl_laser::activate(ship);

	playSound(SND_SHIP_LASER);
}




void cl_ship_laser::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_SHIP:
	case TYPE_SHIP_LASER:
	case TYPE_STALAG_LASER:
	case_POWERUPS:
		break;

	default:
		setStage(STAGE_EXPLODE);
	}
}




void cl_ship_laser::run()
{
	cl_laser::run();

	// Stop if we're out of time or offscreen
	if (stage_timer == 40 || 
	    (y1 > SCR_TOP && y2 > SCR_TOP) ||
	    fabs(getXDistance(x,ship->x)) > SCR_WIDTH)
		setStage(STAGE_INACTIVE);
}
