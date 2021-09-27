#include "globals.h"

cl_reverse_g_powerup::cl_reverse_g_powerup(): 
	cl_powerup(TYPE_REVERSE_G_POWERUP)
{
	main_colour = draw_colour = BLUE;
}




void cl_reverse_g_powerup::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_SHIP)
	{
		// Override
		if (zero_g_timer) zero_g_timer = 0;

		// Set reverse G
		gravity = -start_gravity;
		reverse_g_timer = 400 + (int)random() % 100;
	}
	cl_powerup::collidedWith(obj);
}




void cl_reverse_g_powerup::run()
{
	if (++draw_colour >= RED) draw_colour = BLUE;
	cl_powerup::run();
}




void cl_reverse_g_powerup::draw()
{
	if (doDraw())
	{
		double col = draw_colour;

		// Draw a strobing arrow pointing up
		objDrawLine(draw_colour,10,0,40,-30,10);
		objDrawLine(draw_colour,10,0,40,30,10);

		for(int i=40;i > -40;i-=10)
		{
			objDrawLine(col,10,0,i,0,i-10);
			if (++col >= RED) col = BLUE;
		}
	}
}
