#include "globals.h"

cl_plasma::cl_plasma(): cl_object(TYPE_PLASMA,20)
{
	damage = 15;
}




void cl_plasma::activate(cl_object *activator)
{
	cl_lair *lair = (cl_lair *)activator;

	setStage(STAGE_RUN);
	main_colour = SKY_BLUE;
	arc_colour = ORANGE;
	explode_colour = YELLOW;
	arc_size = radius;
	col_add = 2;
	fireball_diam = 10;
	angle = 0;
	health = 3;
	flash_timer = 0;

	xspd = SIN(lair->cannon[lair->cannon_fired].angle);
	yspd = COS(lair->cannon[lair->cannon_fired].angle);
	x = lair->x + xspd * 50;
	y = lair->y + yspd * 50;

	if (all_pods_captured)
	{
		xspd *= 20;
		yspd *= 20;
	}
	else
	{
		xspd *= 10;
		yspd *= 10;
	}
	playSound(SND_PLASMA);
}




void cl_plasma::reset()
{
	setStage(STAGE_INACTIVE);
}




void cl_plasma::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_POD:	
	case TYPE_BOMB:
	case TYPE_SHIP:
		setStage(STAGE_EXPLODE);
		break;

	case TYPE_SHIP_LASER:
		if (subDamage(obj))
		{
			incScore(25);
			setStage(STAGE_EXPLODE);
		}
		else flash_timer = 3;
		break;

	default:
		break;
	}
}




void cl_plasma::run()
{
	if (flash_timer) --flash_timer;

	if (stage_timer == 100 || y > SCR_TOP)
	{
		setStage(STAGE_INACTIVE);
		return;
	}
	updateXY(x,y);
	if (insideHill(0,x,y))
	{
		setStage(STAGE_EXPLODE);
		return;
	}
	if (--arc_size < -radius) arc_size = radius;

	main_colour += col_add;
	if (main_colour <= SKY_BLUE || main_colour >= RED) 
		col_add = -col_add;
	arc_colour += col_add / 4;
	arc_thick = 2 + (random() % 5);
}




void cl_plasma::runExplode()
{
	if (stage_timer < 10) fireball_diam += 20;
	else if (stage_timer < 30) fireball_diam -= 8;
	else setStage(STAGE_INACTIVE);

	explode_colour -= 0.5;
	if (explode_colour < RED) explode_colour = RED2;
}




void cl_plasma::draw()
{
	objDrawOrFillCircle(flash_timer ? YELLOW : main_colour,0,diam,0,0,true);
	if (!(stage_timer % 3))
	{
		objDrawOrFillHorizArc(arc_colour,arc_thick,diam,arc_size,0,0,false);
		objDrawOrFillVertArc(arc_colour,arc_thick,arc_size,diam,0,0,false);
	}
}




void cl_plasma::drawExplode()
{
	// Draw fireball
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);

	// Draw 3 lightning bolts
	for(int i=0;i < 3;++i) 
	{
		drawLightning(
			random() % SKY_BLUE,
			fireball_diam/2,(double)(random() % 360),draw_x,y);
	}
}
