#include "globals.h"

cl_hyperspace_powerup::cl_hyperspace_powerup(): 
	cl_powerup(TYPE_HYPERSPACE_POWERUP)
{
}




void cl_hyperspace_powerup::activate(cl_object *activator)
{
	width = diam;
	mult = 0.8;
	cl_powerup::activate(NULL);
}




void cl_hyperspace_powerup::run()
{
	draw_colour = hypcols[(stage_timer % 15) / 5];

	width *= mult;
	if (width <= diam / 4) mult = 1/0.8;
	else if (width >= diam) mult = 0.8;
	
	cl_powerup::run();
}




void cl_hyperspace_powerup::draw()
{
	if (doDraw()) objDrawOrFillCircle(draw_colour,10,width,0,0,false);
}
