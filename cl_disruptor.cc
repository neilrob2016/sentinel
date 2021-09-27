#include "globals.h"

#define NUM_TRIANGLES 4

st_vertex cl_disruptor::tri_vtx[NUM_TRIANGLES][DIS_TRI_VTX] =
{
	{
		{ -10,10 },
		{  10,10 },
		{   0,40 }
	},
	{
		{ 10, 10 },
		{ 10,-10 },
		{ 40, 0 }
	},
	{
		{ -10,-10 },
		{  10,-10 },
		{   0,-40 }
	},
	{
		{ -10, 10 },
		{ -10,-10 },
		{ -40, 0 }
	}
};


// Need a square since objDrawOrFillRectangle doesn't rotate
st_vertex cl_disruptor::sq_vtx[4] = 
{
	{ -10,10 },
	{  10,10 },
	{  10,-10 },
	{ -10,-10 }
};


/////////////////////////////////// INIT ////////////////////////////////////

cl_disruptor::cl_disruptor(): cl_object(TYPE_DISRUPTOR,80)
{
	damage = 20;
	explode_bits_cnt = 100;
}




void cl_disruptor::activate(cl_object *activator)
{
	cl_object *obj;
	int o;

	setStage(STAGE_MATERIALISE);
	setExplodeBits();

	main_colour = TURQUOISE2;
	main_col_add = -2;

	fireball_colour = PURPLE;
	fireball_col_add = 1;
	fireball_diam = 0;

	disrupt_colour = BLUE;
	disrupt_col_add = 0.5;
	disrupt_radius = (int)radius;
	disrupt_radius_add = 5;
	max_disrupt_radius = level < 20 ? 100 + 5 * level : 200;

	health = 5;
	angle = 0;
	y_angle = 0;
	xspd = level < 10 ? 10 + 2 * level : 30;
	if (random() % 2) xspd = -xspd;
	yspd = 0;

	x_scale = y_scale = 0;

	do_apc_check = true;

	// Find somewhere not on top of another disruptor
	do
	{
		y = SCR_Y_MID;
		x = random() % LANDSCAPE_WIDTH;

		FOR_ALL_OBJECTS(o)
		{
			obj = object[o];
			if (obj != this && 
			    obj->type == TYPE_DISRUPTOR &&
			    obj->stage != STAGE_INACTIVE &&
			    distToObject(obj) < SCR_WIDTH) break;
		}
	} while(o < NUM_OBJECTS);

	if (game_stage == GAME_STAGE_PLAY) playSound(SND_ENEMY_MATERIALISE);
}




void cl_disruptor::reset()
{
	// If we were running then re-materialise somewhere
	if (stage == STAGE_RUN) activate(NULL);
}



/////////////////////////////////// RUN ////////////////////////////////////

void cl_disruptor::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_BOMB:	
		if (!obj->damage) return;
		// Fall through

	case TYPE_SHIP_LASER:
		if (subDamage(obj)) 
			incScore(300);	
		else
			flash_timer = 2;
		break;

	case TYPE_SHIP:
		incScore(300);
		setStage(STAGE_EXPLODE);
		break;

	default:
		break;
	}
}




void cl_disruptor::run()
{
	if (all_pods_captured && do_apc_check)
	{
		xspd *= 2;
		do_apc_check = false;
	}

	// Disruptor
	disrupt_radius += disrupt_radius_add;
	if (disrupt_radius <= radius || disrupt_radius >= max_disrupt_radius)
		disrupt_radius_add = -disrupt_radius_add;

	// Movement - sinusoidal vertical
	yspd = SIN(y_angle) * 10;
	y_angle = (y_angle + 10) % 360;
	updateXY(x,y);
	incAngle(angle,2);

	// Chedk for disruptor collision with ship
	if (ship->stage == STAGE_RUN &&
	    distToObject(ship) <= disrupt_radius) ship->collidedWith(this);

	// Colours
	main_colour += main_col_add;
	if (main_colour <= BLACK6)
	{
		main_colour = BLACK6;
		main_col_add = -main_col_add;
	}
	else if (main_colour >= TURQUOISE2)
	{
		main_colour = TURQUOISE2;
		main_col_add = -main_col_add;
	}

	disrupt_colour += disrupt_col_add;
	if (disrupt_colour <= BLUE || disrupt_colour >= RED) 
		disrupt_col_add = -disrupt_col_add;

	if (IN_HEARING_DISTANCE()) playSound(SND_DISRUPTOR);
}





void cl_disruptor::runMaterialise()
{
	x_scale += 0.05;
	y_scale += 0.05;
	angle += 18;

	if (x_scale >= 1) 
	{
		setStage(STAGE_RUN);
		return;
	}
}




void cl_disruptor::runExplode()
{
	if (x_scale > 0)
	{
		x_scale -= 0.1;
		y_scale -= 0.1;
		stage_timer = 0;
		return;
	}

	if (stage_timer == 1)
	{
		// Override
		if (reverse_g_timer) reverse_g_timer = 0;

		// Set zero gravity
		gravity = 0;
		zero_g_timer = 400 + (int)random() % 100;

		textobj[TEXT_POWERUP]->activatePowerup(this);
		playSound(SND_DISRUPTOR_EXPLODE);
	}

	if (stage_timer < 10) fireball_diam += 20;
	else if (stage_timer < 50) fireball_diam -= 5;
	else
	{
		setStage(STAGE_INACTIVE);
		return;
	}
	explode_colour -= 0.5;
	if (explode_colour < RED) explode_colour = RED2;

	fireball_colour += fireball_col_add;
	if (fireball_colour == RED)
	{
		fireball_colour = RED2;
		fireball_col_add = -0.5;
	}

	runExplodeBits();
}


//////////////////////////////////// DRAW ////////////////////////////////////

void cl_disruptor::draw()
{
	if (!(stage_timer % 3))
		objDrawOrFillCircle(disrupt_colour,0,disrupt_radius*2,0,0,true);
	drawBody();
}





void cl_disruptor::drawMaterialise()
{
	drawBody();
}




void cl_disruptor::drawExplode()
{
	if (x_scale > 0) drawBody();
	else
	{
		drawOrFillCircle(
			fireball_colour,0,
			fireball_diam,fireball_diam,draw_x,y,true);
		drawExplodeBits();
	}
}




void cl_disruptor::drawBody()
{
	objDrawLine(ORANGE,2,-30,30,30,-30);
	objDrawLine(ORANGE,2,-30,-30,30,30);

	for(int i=0;i < NUM_TRIANGLES;++i)
		objDrawOrFillPolygon(main_colour,DIS_TRI_VTX,tri_vtx[i],true);

	objDrawOrFillPolygon(DARK_RED,DIS_SQ_VTX,sq_vtx,true);
}
