#include "globals.h"

//////////////////////////////// SHAPE DEFS /////////////////////////////////

st_vertex cl_missile::mid_vtx[MISSILE_MID_VTX] =
{
	{ -5,-15 },
	{ -5, 15 },
	{  0, 25 },
	{  5, 15 },
	{  5,-15 }
};


st_vertex cl_missile::left_vtx[MISSILE_FIN_VTX] =
{
	{ -15,-15 },
	{  -2,  0 },
	{  -2,-15 }
};


st_vertex cl_missile::right_vtx[MISSILE_FIN_VTX] =
{
	{ 15,-15 },
	{  2,  0 },
	{  2,-15 }
};


st_vertex cl_missile::flame_vtx[MISSILE_FLAME_VTX] =
{
	{ -8,-17 },
	{  8,-17 },
	{  0,-50 }
};


///////////////////////////////// METHODS ////////////////////////////////////


cl_missile::cl_missile(): cl_object(TYPE_MISSILE,20)
{
	main_colour = WHITE;
	damage = 20;
	explode_bits_cnt = 20;
}




void cl_missile::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setExplodeBits();

	if (game_stage != GAME_STAGE_SHOW_CAST)
	{
		assert(activator);
		x = activator->x;
		y = activator->y;
	}
	health = 3;
	xspd = 0;
	yspd = 0;
	angle = 0;
	turn_rate = 0;
	fireball_diam = 5;
	explode_colour = YELLOW;

	fuel = 190 + level * 10;
	max_speed = 10 + level;

	playSound(SND_MISSILE_LAUNCH);
}




void cl_missile::reset()
{
	// Get us offscreen
	setStage(STAGE_INACTIVE);
}




void cl_missile::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_SHIP_LASER:
		if (subDamage(obj)) incScore(200);
		break;

	case TYPE_SHIP:
		incScore(200);
		// Fall through

	case TYPE_POD:
	case TYPE_BOMB:
	case TYPE_LAIR:
	case TYPE_MOONBASE:
		setStage(STAGE_EXPLODE);
		break;

	default:
		break;
	}
}




/*** Fly baby fly! ***/
void cl_missile::run()
{
	if (game_stage == GAME_STAGE_SHOW_CAST) return;

	// See if we've hit a hill
	if (insideHill(0,x,y)) 
	{
		setStage(STAGE_EXPLODE);
		return;
	}

	if (fuel)
	{
		// Turn faster as we go faster
		if (turn_rate < 2 + level) turn_rate += 0.3;

		// We have fuel so head for ship but go straight up at launch
		--fuel;
		if (stage_timer > 10)
			turnToHeading(getAngle(x,y,ship->x,ship->y),turn_rate);
		xspd += SIN(angle);
		yspd += COS(angle);

		limitToMaxSpeed(max_speed);

		if (IN_HEARING_DISTANCE()) playSound(SND_MISSILE_THRUST);
	}
	// No fuel, fall to earth
	else turnToHeading(180,0.3);

	yspd -= gravity;
	updateXY(x,y);
}




/*** Turn our angle towards the one given at the given turn rate ***/
void cl_missile::turnToHeading(double heading_ang, double ang_inc)
{
	double ang_diff = angleDiff(angle,heading_ang);
	if (fabs(ang_diff) > ang_inc) 
		incAngle(angle,ang_diff > 0 ? ang_inc : -ang_inc);
}




void cl_missile::runExplode()
{
	if (stage_timer == 1 && IN_HEARING_DISTANCE()) 
		playSound(SND_SILO_OR_MISSILE_EXPLODE);
	else if (stage_timer < 10) fireball_diam += 10;
	else if (stage_timer < 50) fireball_diam -= 2;
	else
	{
		setStage(STAGE_INACTIVE);
		return;
	}

	explode_colour -= 0.5;
	if (explode_colour < RED) explode_colour = RED2;

	runExplodeBits();
}




void cl_missile::draw()
{
	objDrawOrFillPolygon(main_colour,MISSILE_MID_VTX,mid_vtx,true);
	objDrawOrFillPolygon(RED,MISSILE_FIN_VTX,left_vtx,true);
	objDrawOrFillPolygon(RED,MISSILE_FIN_VTX,right_vtx,true);
	if (fuel % 2)
	{
		objDrawOrFillPolygon(
			ORANGE + (random() % 11),
			MISSILE_FLAME_VTX,flame_vtx,true);
	}
}




void cl_missile::drawExplode()
{
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
	drawExplodeBits();
}
