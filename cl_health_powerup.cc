#include "globals.h"

#define STEPS 20

cl_health_powerup::cl_health_powerup(): cl_powerup(TYPE_HEALTH_POWERUP)
{
	main_colour = draw_colour = RED;
	y_add = diam / STEPS;
	col_add = (double)(GREEN2 - RED) / STEPS;
	ang_add = 180.0 / STEPS;
}




void cl_health_powerup::run()
{
	if ((draw_colour += 2) > GREEN2) draw_colour = RED;
	cl_powerup::run();
}




void cl_health_powerup::draw()
{
	double y1;
	double col;
	double width;
	int i;

	if (!doDraw()) return;

	y1 = y - radius;
	col = draw_colour;
	angle = ang_add / 2;

	for(i=0;i < STEPS;++i)
	{
		width = diam * SIN(angle);
		drawOrFillRectangle(col,0,width,10,draw_x-width/2,y1,true);

		col += col_add;
		if (col > GREEN2) col = RED;

		y1 += y_add;
		angle += ang_add;
	}
}
