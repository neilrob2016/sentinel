#include "globals.h"

cl_stalag_laser::cl_stalag_laser(): cl_laser(TYPE_STALAG_LASER)
{
	damage = 3;
}




void cl_stalag_laser::activate(cl_object *activator)
{
	main_colour = RED;
	angle = activator->angle;
	incAngle(angle,180);

	xspd = SIN(angle);
	yspd = COS(angle);

	y = activator->y + yspd * 35;
	x = activator->x + xspd * 35;

	cl_laser::activate(activator);

	playSound(SND_STALAG_LASER);
}




void cl_stalag_laser::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_SHIP || 
	    obj->type == TYPE_POD || 
	    obj->type == TYPE_BOMB || 
	    obj->type == TYPE_MOONBASE) setStage(STAGE_EXPLODE);
}




void cl_stalag_laser::run()
{
	cl_laser::run();
	if (stage_timer == 20) setStage(STAGE_INACTIVE);
}
