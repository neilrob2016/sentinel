#include "globals.h"

cl_laser_powerup::cl_laser_powerup(): cl_powerup(TYPE_LASER_POWERUP)
{
	main_colour = draw_colour = YELLOW;
}




void cl_laser_powerup::activate(cl_object *activator)
{
	angle = 0;
	cl_powerup::activate(NULL);
}




void cl_laser_powerup::run()
{
	incAngle(angle,5);
	rotatePoint(-25,-25,angle,&x1,&y1);
	rotatePoint(-25,25,angle,&x2,&y2);
	rotatePoint(25,-25,angle,&x3,&y3);
	rotatePoint(25,25,angle,&x4,&y4);

	draw_colour -= 0.5;
	if (draw_colour < ORANGE) draw_colour = YELLOW;

	cl_powerup::run();
}




void cl_laser_powerup::draw()
{
	if (doDraw())
	{
		objDrawLine(draw_colour,5,x1,y1,x2,y2);
		objDrawLine(draw_colour,5,x3,y3,x4,y4);
	}
}
