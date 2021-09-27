#include "globals.h"

st_vertex cl_skyth::left_vtx[SKYTH_VTX] =
{
	{ -30,5 },
	{ -40,0 },
	{ -30,-5 }
};


st_vertex cl_skyth::right_vtx[SKYTH_VTX] =
{
	{ 30,5 },
	{ 40,0 },
	{ 30,-5 }
};


/////////////////////////////////// INIT /////////////////////////////////////

cl_skyth::cl_skyth(): cl_object(TYPE_SKYTH,50)
{
	explode_bits_cnt = 30;
	damage = 10;
}




void cl_skyth::activate(cl_object *activator)
{
	setStage(STAGE_MATERIALISE);
	setExplodeBits();

	if (random() % 2)
		x = ship->x + SCR_WIDTH * 2;
	else
		x = ship->x - SCR_WIDTH * 2;
	y = SCR_Y_MID;
	updateXY(x,y);

	setSpeed();

	x_scale = y_scale = 0;
	mat_dist = 200;

	main_colour = MEDIUM_GREEN;
	explode_colour = GREEN3;
	flash_timer_top = 0;
	flash_timer_bot = 0;
	shoot_every = level < 7 ? 55 - level * 5 : 20;
	health = 1;

	playSound(SND_ENEMY_MATERIALISE);
}




void cl_skyth::reset()
{
	setStage(STAGE_INACTIVE);
}




void cl_skyth::setSpeed()
{
	double max_speed;
	int ran;

	xdist = getXDistance(x,ship->x);

	max_speed = xspd = xdist > 0 ? ship->max_speed + 3 : -ship->max_speed - 3;

	// If getting close to ship then start moving towards it in y axis
	if (fabs(xdist) < 500) 
	{
		ran = 5 + (int)random() % 10;
		yspd = ship->y > y ? ran : (ship->y < y ? -ran : 0);
	}
	else yspd = random() % 21 - 10;

	limitToMaxSpeed(max_speed);
}


/////////////////////////////////// RUN /////////////////////////////////////

/*** Damaged by laser, explode if we hit ship ***/
void cl_skyth::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_BOMB:
		if (!obj->damage) return;
		// Fall through

	case TYPE_SHIP_LASER:
		if (subDamage(obj))
			incScore(100);
		else
			flash_timer = 3;
		break;

	case TYPE_SHIP:
		// Just explode
		incScore(100);
		setStage(STAGE_EXPLODE);
		break;

	default:
		break;
	}
}




/*** Head for ship ***/
void cl_skyth::run()
{
	st_hill_inside *hi;

	if (!(stage_timer % 5)) setSpeed();

	prev_x = x;
	prev_y = y;
	updateXY(x,y);

	// Make sure we haven't moved inside a hill
	if ((hi = insideHill(radius,x,y)))
	{
		// Back out and move away
		x = prev_x;
		y = prev_y;
		xspd = 0;
		yspd = yspd ? fabs(yspd) : 10;
		if (hi->tb == HILL_TOP) yspd = -yspd;
	}

	if (flash_timer) --flash_timer;

	if (flash_timer_top) --flash_timer_top;
	if (flash_timer_bot) --flash_timer_bot;

	if (xdist < SCR_WIDTH && !(random() % shoot_every))
	{
		activateObjects(TYPE_BULLET,1,this);

		// Flash when bullets are fired
		if (ship->y > y) 
			flash_timer_top = 3;
		else
			flash_timer_bot = 3;
	}
}




/*** Sub materialise bits dist and then do scaling ***/
void cl_skyth::runMaterialise()
{
	if (mat_dist > 0) mat_dist -= 20;
	else if (x_scale < 1)
	{
		x_scale += 0.1;
		y_scale += 0.1;
	}
	else setStage(STAGE_RUN);
}




void cl_skyth::runExplode()
{
	if (stage_timer == 1) playSound(SND_SKYTH_EXPLODE);

	if (stage_timer == 30) setStage(STAGE_INACTIVE);
	else
	{
		runExplodeBits();
		explode_colour -= 0.5;
	}
}



/////////////////////////////////// DRAW /////////////////////////////////////

void cl_skyth::draw()
{
	objDrawOrFillRectangle(
		flash_timer ? WHITE : main_colour,0,60,10,-30,-5,true);

	objDrawOrFillHorizArc(
		flash_timer_top ? RED : LIGHT_BLUE,0,30,10,0,5,true);
	objDrawOrFillHorizArc(
		flash_timer_bot ? RED : LIGHT_BLUE,0,30,-10,0,-5,true);

	if (stage_timer % 30 < 5)
	{
		objDrawOrFillPolygon(RED,SKYTH_VTX,left_vtx,true);
		objDrawOrFillPolygon(MAUVE,SKYTH_VTX,right_vtx,true);
	}
	else
	{
		objDrawOrFillPolygon(DARK_RED,SKYTH_VTX,left_vtx,true);
		objDrawOrFillPolygon(DARK_MAUVE,SKYTH_VTX,right_vtx,true);
	}
}




void cl_skyth::drawMaterialise()
{
	if (mat_dist)
	{
		drawOrFillCircle(GREEN,0,10,10,draw_x-mat_dist,y-mat_dist,true);
		drawOrFillCircle(GREEN,0,10,10,draw_x,y-mat_dist,true);
		drawOrFillCircle(GREEN,0,10,10,draw_x+mat_dist,y-mat_dist,true);
		drawOrFillCircle(GREEN,0,10,10,draw_x+mat_dist,y,true);

		drawOrFillCircle(GREEN,0,10,10,draw_x+mat_dist,y+mat_dist,true);
		drawOrFillCircle(GREEN,0,10,10,draw_x,y+mat_dist,true);
		drawOrFillCircle(GREEN,0,10,10,draw_x-mat_dist,y+mat_dist,true);
		drawOrFillCircle(GREEN,0,10,10,draw_x-mat_dist,y,true);
	}
	else draw();
}




void cl_skyth::drawExplode()
{
	drawExplodeBits();
}
