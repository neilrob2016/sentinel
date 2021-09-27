#include "globals.h"

cl_bullet::cl_bullet(): cl_object(TYPE_BULLET,12)
{
	damage = 2;
}




void cl_bullet::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	x = activator->x;
	y = activator->y;

	headToObject(ship);

	if (activator->type == TYPE_SKYTH)
	{
		xspd *= 30;
		yspd *= 30;
	}
	else
	{
		xspd *= 10;
		yspd *= 10;
	}

	explode_colour = ORANGE;
	fireball_diam = 6;

	playSound(SND_BULLET);
}




void cl_bullet::reset()
{
	setStage(STAGE_INACTIVE);
}




void cl_bullet::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_SHIP || 
	    obj->type == TYPE_SHIP_LASER ||
	    obj->type == TYPE_POD || obj->type == TYPE_BOMB)
		setStage(STAGE_EXPLODE);
}




void cl_bullet::run()
{
	if (stage_timer == 40)
	{
		setStage(STAGE_INACTIVE);
		return;
	}
	updateXY(x,y);
	if (insideHill(0,x,y)) setStage(STAGE_EXPLODE);
	main_colour = MAUVE + random() % (YELLOW - MAUVE);
}




void cl_bullet::runExplode()
{
	if (stage_timer < 5)
	{
		fireball_diam += 2;
		explode_colour += 2;
	}
	else if (stage_timer < 10)
	{
		fireball_diam -= 2;
		explode_colour -= 4;
	}
	else setStage(STAGE_INACTIVE);
}




void cl_bullet::draw()
{
	objDrawOrFillCircle(main_colour,0,diam,0,0,true);
}




void cl_bullet::drawExplode()
{
	draw_x = getDrawX(x);
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
}
