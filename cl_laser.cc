#include "globals.h"

#define LASER_LEN      50
#define LASER_HALF_LEN (LASER_LEN / 2)

///////////////////////////////////// INIT ///////////////////////////////////

/*** Constructor ***/
cl_laser::cl_laser(en_object_type ty): cl_object(ty,LASER_LEN)
{
}




/*** Location, angle and speed must be set by child class ***/
void cl_laser::activate(cl_object *activator)
{
	width = 5;

	if (x < 0) x += LANDSCAPE_WIDTH;
	else 
	if (x >= LANDSCAPE_WIDTH) x -= LANDSCAPE_WIDTH;

	// Don't start if we're inside a hill
	if (insideHill(0,x,y)) return;

	setStage(STAGE_RUN);

	// Beam goes from x1,y1 -> x2,y2
	x1 = x - xspd * LASER_HALF_LEN;
	y1 = y - yspd * LASER_HALF_LEN;
	x2 = x + xspd * LASER_HALF_LEN;
	y2 = y + yspd * LASER_HALF_LEN;

	xspd = xspd * 30 + activator->xspd;
	yspd = yspd * 30 + activator->yspd;

	explode_colour = ORANGE;
	fireball_diam = 6;
}




void cl_laser::reset()
{
	setStage(STAGE_INACTIVE);
}

/////////////////////////////////// RUNTIME //////////////////////////////////


/*** Move and bounce off walls ***/
void cl_laser::run()
{
	st_hill_inside *hi;

	updatePrevious();

	updateXY(x,y);
	updateXY(x1,y1);
	updateXY(x2,y2);

	if ((hi = insideHill(0,x,y)))
	{
		if (type == TYPE_STALAG_LASER)
		{
			setStage(STAGE_EXPLODE);
			return;
		}
		angle = bounceOffHill(hi,1);
		x1 = x - SIN(angle) * LASER_HALF_LEN;
		y1 = y - COS(angle) * LASER_HALF_LEN;
		x2 = x + SIN(angle) * LASER_HALF_LEN;
		y2 = y + COS(angle) * LASER_HALF_LEN;
	}

	explode_colour -= 0.1;
	if (width > 1) width -= 0.1;
}




#define SEGMENT_CNT 5

/*** Laser is a special case so its needs its own version of this function ***/
bool cl_laser::haveCollided(cl_object *obj)
{
	double xadd = (x2 - x1) / SEGMENT_CNT;
	double yadd = (y2 - y1) / SEGMENT_CNT;
	double xp = x1;
	double yp = y1;
	int i;

	// Check 5 spots along the line of the beam to see if we're inside
	// the other objects circumference
	for(i=0;i < SEGMENT_CNT;++i)
	{
		if (hypot(xp - obj->x,yp - obj->y) < obj->radius) return true;
		xp += xadd;
		yp += yadd;
	}
	return false;
}




void cl_laser::runExplode()
{
	if (stage_timer < 8)
	{
		fireball_diam += 3;
		++explode_colour;
	}
	else if (stage_timer < 15)
	{
		--fireball_diam;
		--explode_colour;
	}
	else setStage(STAGE_INACTIVE);
}


///////////////////////////////////// DRAW ///////////////////////////////////

/*** Draw line ***/
void cl_laser::draw()
{
	draw_x = getDrawX(x1);
	draw_x2 = getDrawX(x2);
	if (fabs(draw_x - draw_x2) <= LASER_LEN) 
		drawLine(round(main_colour),round(width),draw_x,y1,draw_x2,y2);
}




/*** Draw explosion ball ***/
void cl_laser::drawExplode()
{
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
}
