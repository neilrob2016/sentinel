#include "globals.h"

cl_tractorbeam_powerup::cl_tractorbeam_powerup(): 
	cl_powerup(TYPE_TRACTORBEAM_POWERUP)
{
	main_colour = draw_colour = TURQUOISE2;
	spiral_radius = radius / 4;
	start_angle = 0;
}




void cl_tractorbeam_powerup::activate(cl_object *activator)
{
	start_angle = 0;
	cl_powerup::activate(NULL);
}




void cl_tractorbeam_powerup::run()
{
	if (--draw_colour < BLACK6) draw_colour = TURQUOISE2;
	incAngle(start_angle,-10);
	cl_powerup::run();
}




/*** Draw a rotating spiral ***/
void cl_tractorbeam_powerup::draw()
{
	double col = draw_colour;
	double x1;
	double y1;

	if (!doDraw()) return;

	angle = start_angle;

	for(double sp=spiral_radius;sp <= radius;sp+=0.3)
	{
		x1 = draw_x + sp * SIN(angle);
		y1 = y + sp * COS(angle);
		drawOrFillCircle(col,0,5,5,x1,y1,true);

		col -= 0.5;
		if (col < BLACK6) col = TURQUOISE2;
		incAngle(angle,-10);
	}
}
