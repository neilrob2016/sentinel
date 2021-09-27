#include "globals.h"

st_vertex cl_stalag::breech_vtx[STALAG_BREECH_VTX] =
{
	{ -15,  10 },
	{ -15,   0 },
	{  -8, -15 },
	{   8, -15 },
	{  15,   0 },
	{  15,  10 }
};


st_vertex cl_stalag::barrel_vtx[STALAG_BARREL_VTX] =
{
	{ -8,-15 },
	{ -5,-35 },
	{  5,-35 },
	{  8,-15 }
};


// Strobe squares
st_vertex cl_stalag::square_vtx[4][STALAG_SQUARE_VTX] =
{
	{
		{ -6,10 },
		{  6,10 },
		{  5, 0 },
		{ -5, 0 }
	},

	{
		{ -5,  0 },
		{  5,  0 },
		{  4,-10 },
		{ -4,-10 }
	},

	{
		{ -4,-10 },
		{  4,-10 },
		{  3,-20 },
		{ -3,-20 }
	},

	{
		{ -3,-20 },
		{  3,-20 },
		{  2,-35 },
		{ -2,-35 }
	}
};

//////////////////////////////////// INIT /////////////////////////////////////


cl_stalag::cl_stalag(): cl_object(TYPE_STALAG,50)
{
	explode_bits_cnt = 50;
}




void cl_stalag::activate(cl_object *activator)
{
	cl_object *obj;
	int hl_left;
	int hl_right;
	int o;

	setStage(STAGE_RUN);
	setExplodeBits();

	health = 8;
	main_colour = GREY; 
	explode_colour = YELLOW;
	strobe_colour = GREEN;
	fireball_diam = 5;
	fireball_colour = RED;
	col_add = 1;
	flash_timer = 0;
	do_apc_check = true;
	shoot_every = level < 10 ? 15 - level : 5;

	if (game_stage == GAME_STAGE_SHOW_CAST) 
	{
		angle = 45;
		return;
	}
	angle = 0;

	if (level < 5)
		angle_add = 1;
	else if (level < 13)
		angle_add = 2 + level - 5;
	else
		angle_add = 10;

	// Find place to camp
	do
	{
		o = 0;
		hl = (int)random() % num_hills[HILL_TOP];
		y = hill[HILL_TOP][hl].y;
		x = hill[HILL_TOP][hl].x;

		// Don't start within half a screens width of the moonbase
		if (fabs(getXDistance(x,moonbase->x)) < SCR_X_MID) continue;

		// Make sure stalag is on a peak , not in a dip
		hl_left = hl ? hl - 1 : num_hills[HILL_TOP]-1;
		hl_right = (hl + 1) % num_hills[HILL_TOP];

		// Check y to make sure we don't start in a dip
		if (hill[HILL_TOP][hl_left].y > y || 
		    hill[HILL_TOP][hl_right].y > y)
		{
			// Don't appear within 2 peaks of another stalag or
			// a striker
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
		}
	} while(o < NUM_OBJECTS);

	// Stick out a bit
	y -= 20;
}




void cl_stalag::reset()
{
	if (stage == STAGE_RUN) angle = 0;
}



////////////////////////////////// RUNTIME ///////////////////////////////////

/*** No damage to us or ship if we collide , it bounces off. Only lasers ***/
void cl_stalag::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_SHIP_LASER || obj->type == TYPE_BOMB)
	{
		if (subDamage(obj)) incScore(250);
		flash_timer = 3;
	}
}




/*** Rotate and shoot ***/
void cl_stalag::run()
{
	double ship_ang;
	double angle_diff;

	if (flash_timer) --flash_timer;
	strobe_colour = (strobe_colour + 2) % RED;

	if (all_pods_captured && do_apc_check)
	{
		angle_add *= 2;
		shoot_every /= 2;
		do_apc_check = false;
	}

	// Get angle to ship and rotate appropriately. 
	ship_ang = getAngle(x,y,ship->x,ship->y);
	if (ship_ang < 100 || ship_ang > 260) return;

	// Angle = 0 is stalag pointing down so add 180
	incAngle(ship_ang,180);
	angle_diff = angleDiff(angle,ship_ang);

	// Rotate ...
	if (fabs(angle_diff) >= angle_add)
	{
		if (angle_diff > 0) 
			incAngle(angle,angle_add);
		else
			incAngle(angle,-angle_add);
	}

	// Shoot ...
	if (!(stage_timer % shoot_every) && 
	    fabs(angle_diff) <= 45 && 
	    fabs(getXDistance(x,ship->x)) < SCR_WIDTH)
		activateObjects(TYPE_STALAG_LASER,1,this);
}




void cl_stalag::runExplode()
{
	runExplodeBits();

	if (stage_timer == 1) playSound(SND_STALAG_EXPLODE);

	if (stage_timer < 20) fireball_diam += 20;
	else if (stage_timer < 100) fireball_diam -= 5;
	else
	{
		setStage(STAGE_INACTIVE);
		return;
	}

	explode_colour -= 0.5;
	if (explode_colour < RED) explode_colour = RED2;
	fireball_colour += col_add;
	if (fireball_colour == ORANGE) col_add = -0.3;
}


//////////////////////////////////// DRAW ////////////////////////////////////

void cl_stalag::draw()
{
	int i;

	// Draw base. Never rotates so don't use objDraw...
	drawOrFillRectangle(STEEL_BLUE,0,20,40,draw_x - 10,y,true);

	// Draw breech and barrel
	if (flash_timer)
	{
		objDrawOrFillPolygon(YELLOW,STALAG_BREECH_VTX,breech_vtx,true);
		objDrawOrFillPolygon(YELLOW,STALAG_BARREL_VTX,barrel_vtx,true);
		return;
	}
	objDrawOrFillPolygon(main_colour,STALAG_BREECH_VTX,breech_vtx,true);
	objDrawOrFillPolygon(PURPLE,STALAG_BARREL_VTX,barrel_vtx,true);

	// Draw strobe squares
	for(i=0;i < 4;++i)
	{
		objDrawOrFillPolygon(
			strobe_colour+i*2,
			STALAG_SQUARE_VTX,square_vtx[3-i],true);
	}
}




void cl_stalag::drawExplode()
{
	objDrawOrFillCircle(fireball_colour,0,fireball_diam,0,0,true);
	drawExplodeBits();
}
