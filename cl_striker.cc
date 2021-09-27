#include "globals.h"

// Just a simple triangle
st_vertex cl_striker::body_vtx[STRIKER_VTX] =
{
	{  25,25  },
	{  15,10  },
	{   5,-25 },
	{   3,-25 },
	{   3,10  },
	{  -3,10  },
	{  -3,-25 },
	{  -5,-25 },
	{ -15,10  },
	{ -25,25  }
};


//////////////////////////////////// INIT /////////////////////////////////////


cl_striker::cl_striker(): cl_object(TYPE_STRIKER,50)
{
	explode_bits_cnt = 200;
	damage = 1; // Beam damage only
}




void cl_striker::activate(cl_object *activator)
{
	cl_object *obj;
	int hl_left;
	int hl_right;
	int o;

	setStage(STAGE_RUN);
	reset();

	if (game_stage == GAME_STAGE_SHOW_CAST)
	{
		max_bolt_length = 200;
		init_bolt_add = 10;
		bolt_timer = 100;
		return;
	}

	setExplodeBits();
	fireball_diam = 0;
	explode_colour = YELLOW;

	// Find place to camp
	do
	{
		o = 0;
		hl = (int)random() % num_hills[HILL_TOP];
		y = hill[HILL_TOP][hl].y;
		x = hill[HILL_TOP][hl].x;

		// Don't start within half a screens width of the moonbase
		if (fabs(getXDistance(x,moonbase->x)) < SCR_X_MID) continue;

		hl_left = hl ? hl - 1 : num_hills[HILL_TOP]-1;
		hl_right = (hl + 1) % num_hills[HILL_TOP];

		// Don't appear within 2 peaks of a stalag or other striker
		FOR_ALL_OBJECTS(o)
		{
			obj = object[o];
			if (obj != this &&
			    obj->stage == STAGE_RUN &&
			    (obj->type == TYPE_STALAG ||
			     obj->type == TYPE_STRIKER) &&
			    (obj->hl == hl ||
			     obj->hl == hl_left ||
			     obj->hl == hl_right)) break;
		}
	} while(o < NUM_OBJECTS);

	// Get maximum length of bolt to strike hill below
	max_bolt_length = y - getHillHeight(HILL_BOT,x)->y - radius;
	init_bolt_add = max_bolt_length / 20;
	
	health = 20;
	y -= 20;
	do_apc_check = true;

	// Max of 1 second
	bolt_timer = level < 12 ? 160 - level * 10 : 40;
}




void cl_striker::reset()
{
	main_colour = PURPLE;
	col_add = 0;
	bolt_length = 0;
	bolt_add = 0;
}




bool cl_striker::haveCollided(cl_object *obj)
{
	// Hit striker itself
	if (distToObject(obj) <= 0) return true;

	// See if we've hit bolt
	return (obj->type == TYPE_SHIP &&
	        bolt_length &&
	        fabs(getXDistance(x,obj->x)) < radius + obj->radius &&
	        obj->y + obj->radius > y - radius - bolt_length);
}



////////////////////////////////// RUNTIME ///////////////////////////////////

/*** No damage to us or ship if we collide , it bounces off. Only lasers ***/
void cl_striker::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_SHIP_LASER || obj->type == TYPE_BOMB)
	{
		if (subDamage(obj)) incScore(500);
		flash_timer = 3;
	}
}




/*** Rotate and shoot ***/
void cl_striker::run()
{
	if (all_pods_captured && do_apc_check)
	{
		bolt_timer /= 2;
		do_apc_check = false;
	}

	// Update bolt length
	if (bolt_length)
	{
		// Add to bolt
		bolt_length += bolt_add;

		// Check for max length
		if (bolt_length >= max_bolt_length)
		{
			bolt_length = max_bolt_length;
			bolt_add = 0;
		}
		// Check for finished
		else if (bolt_length <= 0)
		{
			bolt_length = 0;
			bolt_add = 0;
			main_colour = PURPLE;
		}

		// Check for bolt shortening start
		if (stage_timer == bolt_return_timer) bolt_add = -init_bolt_add;

		// Strobe colour
		main_colour += col_add;
		if (main_colour >= PURPLE || main_colour <= TURQUOISE) 
			col_add = -col_add;

		// Continuous sound
		if (IN_HEARING_DISTANCE()) playSound(SND_STRIKER);
	}
	// Else check for time to start a new bolt
	else if (!(stage_timer % bolt_timer))
	{
		bolt_add = init_bolt_add;
		bolt_length += bolt_add;
		main_colour = TURQUOISE;
		col_add = 2;
		bolt_return_timer = stage_timer + 100;
	}

	if (flash_timer) --flash_timer;
}




void cl_striker::runExplode()
{
	runExplodeBits();

	if (stage_timer == 1) playSound(SND_STRIKER_EXPLODE);
	else if (stage_timer < 15) fireball_diam += 30;
	else if (stage_timer < 85) fireball_diam -= 5;
	else
	{
		setStage(STAGE_INACTIVE);
		return;
	}

	if ((explode_colour -= 0.25) <= RED) explode_colour = RED2;
}


//////////////////////////////////// DRAW ////////////////////////////////////

void cl_striker::draw()
{
	int i;
	int col;

	objDrawOrFillHorizArc(BLUE,0,diam,20,0,25,true);
	objDrawOrFillPolygon(
		flash_timer ? YELLOW : main_colour,STRIKER_VTX,body_vtx,true);
	if (bolt_length)
	{
		for(i=0;i < 3;++i)
		{
			col = TURQUOISE + (int)random() % (PURPLE - TURQUOISE);
			drawLightning(col,bolt_length,180,draw_x,y - radius);
		}
	}
}




void cl_striker::drawExplode()
{
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
	drawExplodeBits();
}
