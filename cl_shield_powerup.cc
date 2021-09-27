#include "globals.h"

cl_shield_powerup::cl_shield_powerup(): cl_powerup(TYPE_SHIELD_POWERUP)
{
}




void cl_shield_powerup::run()
{
	draw_colour = (stage_timer*4) % GREEN2;
	cl_powerup::run();
}




void cl_shield_powerup::draw()
{
	if (doDraw()) objDrawOrFillCircle(draw_colour,5,diam,0,0,false);
}
