#include "globals.h"

#define RADIUS (POD_DIAM / 2)
#define MAX_X_SPEED (double)20

st_vertex cl_zombie::blade_vtx[ZOMBIE_VTX] =
{
	{ -10,RADIUS },
	{   0,RADIUS + 15 },
	{  10,RADIUS }
};


//////////////////////////////////// INIT /////////////////////////////////////


cl_zombie::cl_zombie(): cl_sphere(TYPE_ZOMBIE,POD_DIAM+30)
{
	damage = 8;
	explode_bits_cnt = 50;
	mass = 5;
}




/*** Doesn't call cl_sphere::activate() since doesn't need to set a 
     ground location ***/
void cl_zombie::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setExplodeBits();
	angle = 0;

	if (game_stage != GAME_STAGE_SHOW_CAST)
	{
		assert(activator);
		x = activator->x;
		y = activator->y + activator->radius + radius;
		updateXY(x,y);
	}

	xspd = (random() % 11) - 5;
	yspd = 10 + random() % 10;

	explode_colour = YELLOW;
	main_colour = GREEN;
	flash_timer = 0;

	health = 2;

	if (IN_HEARING_DISTANCE()) playSound(SND_ZOMBIE_LAUNCH);
}




////////////////////////////////// RUNTIME ///////////////////////////////////

/*** Killed by laser, bounce off ship causing it damage ***/
void cl_zombie::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case_POWERUPS:
		return;

	case TYPE_BOMB:
		if (!obj->damage) break;
		// Fall through

	case TYPE_SHIP:
		incScore(50);
		setStage(STAGE_EXPLODE);
		break;

	case TYPE_SHIP_LASER:
		flash_timer = 3;
		if (subDamage(obj))
		{
			incScore(50);
			return;
		}
		if (obj->type == TYPE_SHIP_LASER) return;
		break;

	default:
		break;
	}

	bounceOffObject(obj);
	spin();
}




/*** Head for ship ***/
void cl_zombie::run()
{
	double xd;
	double speed_mult = all_pods_captured ? 3 : 1;

	cl_sphere::run();

	if (flash_timer) --flash_timer;

	xd = getXDistance(x,ship->x);

	if (xd > 0 && xspd < MAX_X_SPEED * speed_mult) xspd += speed_mult;
	else
	if (xd < 0 && xspd > -MAX_X_SPEED * speed_mult) xspd -= speed_mult;

	main_colour += 2;
	if (main_colour >= GREEN2) main_colour = GREEN;
}




void cl_zombie::runExplode()
{
	if (stage_timer == 1) playSound(SND_ZOMBIE_EXPLODE);

	if (stage_timer < 50)
	{
		if ((explode_colour -= 0.5) <= RED) explode_colour = RED2;
		runExplodeBits();
	}
	else setStage(STAGE_INACTIVE);
}


////////////////////////////////////// DRAW ///////////////////////////////////

void cl_zombie::draw()
{
	double tmp_angle;
	double tmp_col;
	int i;

	// Don't want man to rotate
	tmp_angle = angle;
	angle = 0;
	drawMan(RED,false);

	tmp_col = flash_timer ? WHITE : main_colour;

	// Draw saw by drawing the same blade with new angle
	for(angle=tmp_angle,i=0;i < 6;++i)
	{
		objDrawOrFillPolygon(tmp_col,ZOMBIE_VTX,blade_vtx,true);
		incAngle(angle,60);
	}

	angle = tmp_angle;

	// Draw pod circle 
	objDrawOrFillCircle(RED,3,POD_DIAM,0,0,false);
}




void cl_zombie::drawExplode()
{
	drawExplodeBits();
}
