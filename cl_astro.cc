#include "globals.h"


cl_astro::cl_astro()
{
	setLocation();
	colour = getColour();

	if (random() % 12)
	{
		// Star
		type = TYPE_STAR;
		diam = 2 + (int)random() % 4;
		return;
	}

	// Planet
	diam = 10 + (int)random() % 10;

	if (random() % 2)
	{
		type = TYPE_RING_PLANET;

		// Need at least 1 ring type set. Can have 2.
		if (random() % 2)
		{
			ring_type[RING_VERT] = true;
			ring_colour[RING_VERT] = getColour();
		}
		else ring_type[RING_VERT] = false;

		if (!ring_type[RING_VERT] || (random() % 2))
		{
			ring_type[RING_HORIZ] = true;
			ring_colour[RING_HORIZ] = getColour();
		}
		else ring_type[RING_HORIZ] = false;
	}
	else type = TYPE_PLANET;
}




void cl_astro::setLocation()
{
	x = random() % LANDSCAPE_WIDTH;
	y = random() % SCR_TOP;
}




int cl_astro::getColour()
{
	int col;

	do
	{
		col = (int)random() % NUM_FULL_COLOURS;
	} while(col == BLACK);

	return col;
}




void cl_astro::draw()
{
	draw_x = x - background_x_shift;
	if (draw_x < -LANDSCAPE_WIDTH/2) draw_x += LANDSCAPE_WIDTH;

	// Draw ring half behind
	if (type == TYPE_RING_PLANET)
	{
		if (ring_type[RING_VERT])
		{
			drawOrFillVertArc(
				ring_colour[RING_VERT],
				2,diam / 3,diam * 2,draw_x,y,false);
		}
		if (ring_type[RING_HORIZ])
		{
			drawOrFillHorizArc(
				ring_colour[RING_HORIZ],	
				2,diam * 2,diam / 3,draw_x,y,false);
		}
	}

	// Draw main circle
	drawOrFillCircle(colour,0,diam,diam,draw_x,y,true);

	switch(type)
	{
	case TYPE_PLANET:
		break;

	case TYPE_RING_PLANET:
		// Draw ring half in front
		if (ring_type[RING_VERT])
		{
			drawOrFillVertArc(
				ring_colour[RING_VERT],
				2,-diam / 3,diam * 2,draw_x,y,false);
		}
		if (ring_type[RING_HORIZ])
		{
			drawOrFillHorizArc(
				ring_colour[RING_HORIZ],
				2,diam * 2,-diam / 3,draw_x,y,false);
		}
		break;

	case TYPE_STAR:
		// Change star colour to make it twinkle
		if (!(random() % 100)) colour = getColour();
		break;

	default:
		assert(0);
	}
}
