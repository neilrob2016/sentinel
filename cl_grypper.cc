#include "globals.h"

#define CRUISE_HEIGHT (SCR_HEIGHT / 2)

st_vertex cl_grypper::body_vtx[GRYPPER_BODY_VTX] =
{
	{ -20, 20 },
	{  20, 20 },
	{  20, 0 },
	{  30,-20 },
	{  10, 0 },
	{ -10, 0 },
	{ -30,-20 },
	{ -20, 0 }
};


st_vertex cl_grypper::top_vtx[GRYPPER_TOP_VTX] =
{
	{ -20,20 },
	{   0,30 },
	{  20,20 }
};


///////////////////////////////// INIT ////////////////////////////////////


cl_grypper::cl_grypper(): cl_object(TYPE_GRYPPER,40)
{
	main_colour = SKY_BLUE;
	damage = 5;
	explode_bits_cnt = 10;
}




void cl_grypper::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setExplodeBits();
	explode_colour = SKY_BLUE;

	if (game_stage != GAME_STAGE_SHOW_CAST)
	{
		assert(activator);
		lair = (cl_lair *)activator;
		x = lair->x;
		y = lair->y + lair->radius + radius - 10;
	}
	cruise_height = CRUISE_HEIGHT + ((int)random() % 61) - 30;
	pod = NULL;
	strobe_colour = 0;
	action = ACT_RISE;
	flash_timer = 0;
	health = 1;
	y_speed = 5;
	do_apc_check = true;
	wait_timer = 0;

	shoot_distance = SCR_X_MID + level * 10;
	shoot_every = level < 7 ? 55 - level * 5 : 20;
	x_speed = 5 + level;

	if (IN_HEARING_DISTANCE()) playSound(SND_GRYPPER_LAUNCH);
}




/*** Inactivate - lair will re-create us ***/
void cl_grypper::reset()
{
	if (pod) pod->release();
	setStage(STAGE_INACTIVE);
}



////////////////////////////////// RUNTIME //////////////////////////////////

void cl_grypper::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_BOMB:
		if (!obj->damage) return;
		// Fall through

	case TYPE_SHIP_LASER:
		if (subDamage(obj)) addScore();
		else flash_timer = 2;
		break;

	case TYPE_SHIP:
		// Just explode if we hit the ship
		addScore();
		setStage(STAGE_EXPLODE);
		break;

	default:
		break;
	}
}




void cl_grypper::addScore()
{
	incScore(25);
	if (pod && pod->grabber == this) activateBonusText(50,this);
}




void cl_grypper::run()
{
	int o;

	if (flash_timer) --flash_timer;
	strobe_colour = (strobe_colour + 1) % 15;

	// Speed up if all_pods_captured. Don't need to slow down as it 
	// continues to end of level.
	if (all_pods_captured && do_apc_check)
	{
		x_speed *= 3;
		y_speed *= 2;
		shoot_every /= 2;
		do_apc_check = false;
	}

	// Has pod we're heading for been grabbed by someone else?
	if (pod && 
	    (pod->stage != STAGE_RUN || (pod->grabber && pod->grabber != this)))
	{
		pod = NULL;
		action = ACT_RISE;
	}

	// Is our lair destroyed?
	if (lair && lair->stage == STAGE_INACTIVE)
	{
		// Find another lair to go to
		FOR_ALL_OBJECTS(o)
		{
			if (object[o]->type == TYPE_LAIR &&
			    object[o]->stage != STAGE_INACTIVE) 
			{
				lair = (cl_lair *)object[o];
				action = ACT_RISE;
				break;
			}
		}

		if (o == NUM_OBJECTS)
		{
			// No lairs left
			lair = NULL;
			shoot_distance *= 2;
			shoot_every /= 2;
			dropPod();
			action = ACT_RISE;
		}
	}

	// Shoot bullet at ship if its nearby
	if (distToObject(ship) < shoot_distance && !(random() % shoot_every))
		activateObjects(TYPE_BULLET,1,this);

	// State machine switch
	switch(action)
	{
	case ACT_RISE:
		actRise();
		break;

	case ACT_GOTO_POD:
		actGotoPod();
		break;

	case ACT_GRAB_POD:
		actGrabPod();
		break;

	case ACT_RETURN_TO_LAIR:
		actReturnToLair();
		break;

	case ACT_DELIVER_POD:
		actDeliverPod();
		break;

	case ACT_CRUISE:
		actCruise();
		break;

	default:
		assert(0);
	}
}




/*** Rise to cruising altitude ***/
void cl_grypper::actRise()
{
	if (y < cruise_height && !insideHill(radius,x,y)) 
	{
		y += y_speed;
		movePod();
	}
	else if (pod)
	{
		xspd = SGN(getXDistance(x,lair->x)) * x_speed;
		action = ACT_RETURN_TO_LAIR;
	}
	else if (!lair) action = ACT_CRUISE;
	else
	{
		pickRandomPod();
		action = ACT_GOTO_POD;
		stage_timer = 0;
		y_angle = 0;
	}
}




/*** Move horizontally to pods X position ***/
void cl_grypper::actGotoPod()
{
	moveHorizontal();

	if (pod)
	{
		// If we've selected a pod see if we're there
		if (fabs(getXDistance(x,pod->x)) <= x_speed)
			action = ACT_GRAB_POD;
		return;
	}

	// Pick a new one 
	if (!(stage_timer % 20)) pickRandomPod();
}




/*** Drop down to pod to grab it ***/
void cl_grypper::actGrabPod()
{
	double dist;

	// If pod has moved too far give up 
	dist = getXDistance(x,pod->x);
	if (y < pod->y || fabs(dist) > x_speed)
	{
		pod = NULL;
		action = ACT_RISE;
		return;
	}
	yspd = (y > pod->y ? -y_speed : (y < pod->y ? y_speed : 0));
	xspd = (dist > 0 ? 1 : (dist < 0 ? -1 : 1));
	updateXY(x,y);

	if (distToObject(pod) < 1)
	{
		if (pod->grab(this))
			playSound(SND_GRYPPER_GRAB);
		else
			pod = NULL;

		// Doesn't matter if we can grab it or not, rise anyway
		action = ACT_RISE;
	}
}




/*** Rise up and return to lair ***/
void cl_grypper::actReturnToLair()
{
	moveHorizontal();
	movePod();

	if (fabs(getXDistance(x,lair->x)) <= x_speed)
		action = ACT_DELIVER_POD;
}




/*** Deliver pod back on the lair ***/
void cl_grypper::actDeliverPod()
{
	if (distToObject(lair) > pod->diam * 3) 
	{
		y -= y_speed;
		movePod();
		return;
	}
	dropPod();
	wait_timer = 50;
	action = ACT_RISE;
}




/*** Pick a pod or set random x direction to move ***/
void cl_grypper::pickRandomPod()
{
	int o;
	int cnt;

	if (wait_timer) --wait_timer;
	else
	{
		o = (int)random() % MAX_POD;

		for(cnt=0;cnt < MAX_POD;++cnt)
		{
			pod = (cl_pod *)object[pod_start + o];
			if (pod->type == TYPE_POD && 
			    pod->stage == STAGE_RUN && 
			   !pod->grabber &&
			    fabs(pod->xspd) + fabs(pod->yspd) < 1)
			{
				xspd = SGN(getXDistance(x,pod->x)) * x_speed;
				return;
			}
			o = (o + 1) % MAX_POD;
		}
	}

	// Fly in some random direction until a pod is available
	if (action == ACT_RISE) xspd = (random() % 2) ? x_speed : -x_speed;
	pod = NULL;
}




/*** Lair has gone, just cruise around ***/
void cl_grypper::actCruise()
{
	if (!(stage_timer % 50))
		xspd = ((random() % 2) ? x_speed : -x_speed) * 3;
	moveHorizontal();
}




/*** xspd must already be set ***/
void cl_grypper::moveHorizontal()
{
	st_hill_inside *hi;

	y_angle = (y_angle + 10) % 360;
	yspd = SIN(y_angle) * y_speed;
	updateXY(x,y);

	// Won't happen very often
	if ((hi = insideHill(radius,x,y)))
	{
		y = getHillHeight(hi->tb,x)->y;
		y += (hi->tb == HILL_BOT ? radius : -radius);
	}
}




void cl_grypper::movePod()
{
	if (pod)
	{
		pod->y = y - radius - 12;
		pod->x = x;
	}
}




void cl_grypper::dropPod()
{
	if (pod)
	{
		pod->release();
		pod->xspd = 0;
		pod->yspd = 0;
		pod = NULL;
	}
}




void cl_grypper::runExplode()
{
	if (stage_timer == 1) playSound(SND_GRYPPER_EXPLODE);

	if (explode_colour < RED) ++explode_colour;
	else
	if (explode_colour >= RED && explode_colour < ORANGE) 
		explode_colour = RED2;
	else
	if (--explode_colour <= BLACK2) 
		setStage(STAGE_INACTIVE);

	runExplodeBits();
}

/////////////////////////////////// DRAW //////////////////////////////////

void cl_grypper::draw()
{
	objDrawOrFillPolygon(
		flash_timer ? WHITE : main_colour,
		GRYPPER_BODY_VTX,body_vtx,true);
	objDrawOrFillPolygon(strobe_colour+RED+8,GRYPPER_TOP_VTX,top_vtx,true);
	objDrawOrFillRectangle(strobe_colour+RED+6,0,10,5,-5,15,true);
	objDrawOrFillRectangle(strobe_colour+RED+4,0,10,5,-5,10,true);
	objDrawOrFillRectangle(strobe_colour+RED+2,0,10,5,-5,5,true);
	objDrawOrFillRectangle(strobe_colour+RED,0,10,5,-5,0,true);
}




void cl_grypper::drawExplode()
{
	drawExplodeBits();
}
