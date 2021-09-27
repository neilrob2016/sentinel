#include "globals.h"

#define BOUNCE_FRICTION 0.8

////////////////////////////////// SETUP //////////////////////////////////

cl_object::cl_object(en_object_type ty, double dm)
{
	setStage(STAGE_INACTIVE);
	type = ty;
	diam = dm;
	radius = diam / 2;

	mass = 1;
	health = 0;
	damage = 0;

	x = y = draw_x = 0;
	prev_x = prev_y = 0;
	xspd = yspd = 0;
	angle = 0;
	x_scale = 1;
	y_scale = 1;
	hl = -1;

	main_colour = WHITE;
	explode_bits = NULL;
	fireball_diam = 0;
	flash_timer = 0;
	do_apc_check = true;
}




void cl_object::setStage(en_object_stage st)
{
	stage = st;
	stage_timer = 0;
}




/*** Set x,y to a random ground location in the landscape ***/
void cl_object::setRandomGroundLocation(double min_dist, double y_offset)
{
	cl_object *obj;
	double dist;
	double x1;
	double x2;
	int o;

	if (game_stage == GAME_STAGE_SHOW_CAST) return;

	o = 0; // So 'continue' doesn't cause a crash

	do
	{
		x = random() % LANDSCAPE_WIDTH;
		y = getHillHeight(HILL_BOT,x)->y;

		// Type specific checks
		switch(type)
		{
		case TYPE_LAIR:
			/* Don't want lair stuck in a ditch so it can't fire 
			   its cannons properly. Make sure one or other side 
			   is lower */
			x1 = x - 40;
			if (x1 < 0) x1 += LANDSCAPE_WIDTH;

			x2 = x + 40;
			if (x2 >= LANDSCAPE_WIDTH) x2 -= LANDSCAPE_WIDTH;

			if (getHillHeight(HILL_BOT,x1)->y > y &&
			    getHillHeight(HILL_BOT,x2)->y > y) continue;
			y += y_offset;
			break;

		case TYPE_POD:
			// Check we're not within a screen width of the base
			y += y_offset;
			if (distToObject(moonbase) <= SCR_WIDTH) continue;
			break;

		default:
			y += y_offset;
			break;
		}


		// Make sure we don't appear on top of anything else. If this
		// loop completes then co-ords are ok and outside exits.
		FOR_ALL_OBJECTS(o)
		{
			obj = object[o];
			if (obj == this || obj->stage == STAGE_INACTIVE) 
				continue;

			if ((dist = distToObject(obj)) < min_dist) break;

			/* Keep lair/lair and lair/moonbase apart. Moonbase
			   is always activated first so will always be
			   "obj" not "this" when checking */
			if (type == TYPE_LAIR &&
			    (obj->type == TYPE_LAIR || 
			     obj->type == TYPE_MOONBASE) && 
			    dist < LANDSCAPE_WIDTH / 10) break;
		}
	} while(o < NUM_OBJECTS);
}




/*** Set up the pieces of an explosion ***/
void cl_object::setExplodeBits()
{
	double ang;
	double ang_inc;
	int i;

	if (explode_bits) delete explode_bits;

	explode_bits = new st_explode[explode_bits_cnt];

	explode_colour = YELLOW;

	ang = 0;
	ang_inc = (double)360 / explode_bits_cnt;

	for(i=0;i < explode_bits_cnt;++i)
	{
		explode_bits[i].x = 0;
		explode_bits[i].y = 0;
		explode_bits[i].diam = 10 + random() % 10;
		explode_bits[i].xspd = SIN(ang) * (random() % 20 + 2);
		explode_bits[i].yspd = COS(ang) * (random() % 20 + 2);

		ang += ang_inc;
	}
}



////////////////////////////////// RUNTIME //////////////////////////////////

/*** The bits that fly out from an explosion ***/
void cl_object::runExplodeBits()
{
	for(int i=0;i < explode_bits_cnt;++i)
	{
		explode_bits[i].x += explode_bits[i].xspd;
		explode_bits[i].y += explode_bits[i].yspd;
		if (explode_bits[i].diam > 1) explode_bits[i].diam *= 0.98;
	}
}


#define FORCE_DIV 150

/*** Calculate forces between us and the other object ***/
void cl_object::calcForces(cl_object *obj, double &force_x, double &force_y)
{
	double dist;
	double stretch;
	double angle;

	dist = hypot(getXDistance(x,obj->x),y - obj->y);

	if (dist > TRACTOR_BEAM_LEN)
	{
		stretch = dist - TRACTOR_BEAM_LEN;
		angle = getAngle(x,y,obj->x,obj->y);
		force_x = stretch * SIN(angle) / FORCE_DIV;
		force_y = -stretch * COS(angle) / FORCE_DIV;
	}
	else
	{
		force_x = 0;
		force_y = 0;
	}
}




/*** Add force from object above - ie pull us upwards ***/
void cl_object::addForceAbove(double force_x, double force_y)
{
	assert(mass);
	xspd -= force_x / mass;
	yspd += force_y / mass;
}




/*** Add force from object below - ie pull us downwards ***/
void cl_object::addForceBelow(double force_x, double force_y)
{
	assert(mass);
	xspd += force_x / mass;
	yspd -= force_y / mass;
}




/*** Set the speed for moving towards the object ***/
void cl_object::headToObject(cl_object *obj)
{
	double xd = getXDistance(x,obj->x);
	double yd = obj->y - y;
	double total = hypot(xd,yd);

	xspd = xd / total;
	yspd = yd / total;
}




/*** Return true if we've collided with them ***/
bool cl_object::haveCollided(cl_object *obj)
{
	return (distToObject(obj) <= 0);
}




/*** Set previous x,y vars ***/
void cl_object::updatePrevious()
{
	prev_x = x;
	prev_y = y;
}




/*** Add speed to given x,y co-ords and do sanity check ***/
void cl_object::updateXY(double &xp, double &yp)
{
	xp += xspd;
	yp += yspd;
	if (xp >= LANDSCAPE_WIDTH) xp -= LANDSCAPE_WIDTH;
	else
	if (xp < 0) xp += LANDSCAPE_WIDTH;

	// Some objects can't go off the top of the screen
	switch(type)
	{
	case TYPE_SHIP_LASER:
	case TYPE_MISSILE:
	case TYPE_PLASMA:
	case TYPE_BULLET:
	case TYPE_POD:
	case TYPE_BOMB:
		// These ones can
		break;

	default:
		if (yp > SCR_TOP - radius) {
			yp = SCR_TOP - radius;
			yspd = 0;
		}
		else if (yp < 0)
		{
			yp = radius;
			yspd = 0;
		}
	}
}




/*** Limit the total speed to the maximum given ***/
void cl_object::limitToMaxSpeed(double max_speed)
{
	double speed = hypot(xspd,yspd);
	double mult;

	max_speed = fabs(max_speed);
	if (speed > max_speed)
	{
		mult = max_speed / speed;
		xspd *= mult;
		yspd *= mult;
	}
}




/*** Bounce off the hill we've hit ***/
double cl_object::bounceOffHill(st_hill_inside *hi, double inv_fric)
{
	double move_ang;
	double new_ang;
	double new_y;
	double hyp;

	move_ang = getAngle(prev_x,prev_y,x,y);

	new_ang = move_ang + (angleDiff(move_ang,hill[hi->tb][hi->hl].angle) * 2);
	if (new_ang < 0) new_ang += 360;
	else
	if (new_ang >= 360) new_ang -= 360;

	hyp = hypot(xspd,yspd);
	xspd = hyp * SIN(new_ang);
	yspd = hyp * COS(new_ang);

	// If not laser then back out to prev x,y and add in bounce friction
	// Don't care if laser goes into hill.
	if (type != TYPE_SHIP_LASER)
	{
		x = prev_x;
		y = prev_y;
		xspd *= inv_fric;
		yspd *= inv_fric;

		// See if we're still inside the hill. If we are force y 
		// outside of it and make sure yspd is in opposite direction
		new_y = y + yspd;
		if ((hi->tb == HILL_BOT && new_y - radius < hi->y) ||
		    (hi->tb == HILL_TOP && new_y + radius > hi->y))
		{
			xspd = 0; // So hi->y still valid after update
			if (hi->tb == HILL_BOT) 
			{
				y = hi->y + radius + 1;
				yspd = fabs(yspd);
			}
			else
			{
				y = hi->y - radius - 1;
				yspd = -fabs(yspd);
			}
		}
	}

	updateXY(x,y);
	return new_ang;
}



#define MAX_SPEED 20

/*** Bounce off another object ***/
void cl_object::bounceOffObject(cl_object *obj)
{
	double xd;
	double yd;
	double total_dist;
	double overlap;

	assert(mass);

	xd = getXDistance(obj->x,x);
	yd = y - obj->y;
	total_dist = fabs(xd) + fabs(yd);
	if (!total_dist) total_dist = 0.1;  // Don't want to divide by zero 

	overlap = radius + obj->radius - hypot(xd,yd);

	xspd = (xspd + overlap * (xd/total_dist) / mass) * BOUNCE_FRICTION;
	yspd = (yspd + overlap * (yd/total_dist) / mass) * BOUNCE_FRICTION;

	// Don't produce values that see objects flying off the screen in
	// a microsecond
	if (fabs(xspd) > MAX_SPEED) xspd = MAX_SPEED * SGN(xspd);
	if (fabs(yspd) > MAX_SPEED) yspd = MAX_SPEED * SGN(yspd);
}




/*** Get the distance between the 2 objects - NOT the distance from the 
     2 centres ***/
double cl_object::distToObject(cl_object *obj)
{
	return hypot(getXDistance(x,obj->x),y - obj->y) - radius - obj->radius;
}




/*** Called in collidedWith(). Subtracts damage and sets explode stage if 
     health is zero or below and returns true. Else false. ***/
bool cl_object::subDamage(cl_object *obj)
{
	if ((health -= obj->damage) <= 0)
	{
		setStage(STAGE_EXPLODE);
		return true;
	}
	return false;
}



////////////////////////////////// DRAW //////////////////////////////////////


/*** Called by drawObject() ***/
void cl_object::setDrawX()
{
	draw_x = getDrawX(x);
}




/*** The bits that fly out from an explosion ***/
void cl_object::drawExplodeBits()
{
	for(int i=0;i < explode_bits_cnt;++i)
	{
		drawOrFillCircle(
			explode_colour,0,
			explode_bits[i].diam,
			explode_bits[i].diam,
			getDrawX(x + explode_bits[i].x),
			y + explode_bits[i].y,
			true);
	}
}



#define R1 ((double)POD_DIAM / 3)
#define R2 ((double)POD_DIAM / 4)
#define R3 ((double)POD_DIAM / 6)

/*** Draw a man in either a pod or lair ***/
void cl_object::drawMan(double col, bool arms_up)
{
	objDrawLine(col,1,0,R3,0,-R3);

	// Arms
	if (arms_up)
	{
		objDrawLine(col,1,0,R3,-R2,R2);
		objDrawLine(col,1,0,R3,R2,R2);
	}
	else
	{
		objDrawLine(col,1,0,R3,-R2,0);
		objDrawLine(col,1,0,R3,R2,0);
	}

	// Legs
	objDrawLine(col,1,0,-R3,-R2,-R1);
	objDrawLine(col,1,0,-R3,R2,-R1);

	// Head
	objDrawOrFillCircle(ORANGE,0,10,0,R2,true);
}




/*** Draw line taking into account object location and angle of rotation. 
     draw_x must be set before calling ***/
void cl_object::objDrawLine(
	double col, double thick, double x1, double y1, double x2, double y2)
{
	double x3,y3;
	double x4,y4;

	rotatePoint(x1,y1,angle,&x3,&y3);
	rotatePoint(x2,y2,angle,&x4,&y4);

	x3 = x3 * x_scale + draw_x;
	y3 = y3 * y_scale + y;
	x4 = x4 * x_scale + draw_x;
	y4 = y4 * y_scale + y;

	drawLine(col,thick,x3,y3,x4,y4);
}




/*** Rotate local x,y point around 0,0 by angle then draw ***/
void cl_object::objDrawOrFillCircle(
	double col, double thick, double diam, double x1, double y1, bool fill)
{
	double x_diam;
	double y_diam;
	double x2;
	double y2;

	rotatePoint(x1,y1,angle,&x2,&y2);

	x2 = x2 * x_scale + draw_x;
	y2 = y2 * y_scale + y;
	x_diam = diam * x_scale;
	y_diam = diam * y_scale;

	drawOrFillCircle(col,thick,x_diam,y_diam,x2,y2,fill);
}




/*** As above for polygons ***/
void cl_object::objDrawOrFillPolygon(
	double col, int num_vtx, st_vertex *vert, bool fill)
{
	double xd;
	double yd;
	int i;

	assert(num_vtx <= MAX_VERTEXES);

	for(i=0;i < num_vtx;++i)
	{
		rotatePoint(vert[i].x,vert[i].y,angle,&xd,&yd);

		draw_vtx[i].x = (short)round(draw_x + xd * x_scale);
		draw_vtx[i].y = (short)round(y + yd * y_scale);
	}
	drawOrFillPolygon(col,0,num_vtx,draw_vtx,fill);
}




/*** Draw rectangle adding draw_x,y to the offsets. This draws up so x,y
     is bottom left position. Rotation not supported. ***/
void cl_object::objDrawOrFillRectangle(
	double col,
	double thick, 
	double w, double h, double x1, double y1, bool fill)
{
	drawOrFillRectangle(
		col,thick,
		w * x_scale,
		h * y_scale,
		draw_x + x1 * x_scale,
		y + y1 * y_scale,
		fill);
}




/*** Draw arc going from left to right. Rotation not supported ***/
void cl_object::objDrawOrFillHorizArc(
	double col,
	double thick, double w, double h, double x1, double y1, bool fill)
{
	drawOrFillHorizArc(
		col,thick,
		w * x_scale,
		h * y_scale,
		draw_x + x1 * x_scale,
		y + y1 * y_scale,
		fill);
}




/*** Draw arc going from top to bottom. Rotation not supported ***/
void cl_object::objDrawOrFillVertArc(
	double col,
	double thick, double w, double h, double x1, double y1, bool fill)
{
	drawOrFillVertArc(
		col,thick,
		w * y_scale,
		h * x_scale,
		draw_x + x1 * x_scale,
		y + y1 * y_scale,
		fill);
}
