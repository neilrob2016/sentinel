#include "globals.h"

//////////////////////////////// SHAPE DEFS /////////////////////////////////

st_vertex cl_lair::main_vtx[LAIR_MAIN_VTX] =
{
	{ -45,  0 },
	{ -20, 40 },
	{ -30, 45 },
	{  30, 45 },
	{  20, 40 },
	{  45,  0 },
	{  30,-40 },
	{ -30,-40 }
};


st_vertex cl_lair::leg1_vtx[LAIR_LEG_VTX] =
{
	{ -20,-40 },
	{ -10,-40 },
	{ -40,-60 }
};


st_vertex cl_lair::leg2_vtx[LAIR_LEG_VTX] =
{
	{ 20,-40 },
	{ 10,-40 },
	{ 40,-60 }
};


st_vertex cl_lair::breech_vtx[LAIR_CANNON_VTX] =
{
	{ -20,30 },
	{  -8,50 },
	{   8,50 },
	{  20,30 }
};


st_vertex cl_lair::barrel_vtx[LAIR_CANNON_VTX] =
{
	{  -8,50 },
	{  -5,70 },
	{   5,70 },
	{   8,50 }
};

////////////////////////////////// SETUP /////////////////////////////////////

cl_lair::cl_lair(): cl_object(TYPE_LAIR,100)
{
	explode_bits_cnt = 300;
}




void cl_lair::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setRandomGroundLocation(0,radius);
	setExplodeBits();
	reset();

	leg_colour = WHITE;
	circ_colour = DARK_BLUE;
	explode_colour = YELLOW;
	num_prisoners = 0;
	wave_arms_timer = 0;
	fireball_diam = 5;
	grypper_light_timer = 0;
	zombie_light_timer = 0;
	do_apc_check = true;

	health = level < 10 ? 60 + level * 4 : 100;
	cannon_shoot_every = level < 10 ? 30 - level : 20;
	grypper_launch_every = level < 10 ? 200 - level * 10 : 100;
	zombie_launch_every = level < 20 ? 320 - level * 10 : 120;
	cannon_move_speed = level < 5 ? 0.5 + 0.1 * level : 1;

	// Set up cannons
	cannon[0].angle = 270;  // Left
	cannon[0].breech_colour = LIGHT_GREY;
	cannon[0].barrel_colour = TURQUOISE;
	cannon[0].shoot_flash = 0;
	cannon[0].hit_flash = 0;
	cannon[0].health = health / 4;
	cannon[0].fireball_diam = 0;
	cannon[0].explode_timer = 0;

	cannon[1].angle = 90;   // Right
	cannon[1].breech_colour = LIGHT_GREY;
	cannon[1].barrel_colour = TURQUOISE;
	cannon[1].shoot_flash = 0;
	cannon[1].hit_flash = 0;
	cannon[1].health = health / 4;
	cannon[1].fireball_diam = 0;
	cannon[1].explode_timer = 0;
}




void cl_lair::reset()
{
	main_colour = RED;
	flash_timer = 0;
	x_scale = y_scale = 1;
	scale_inc = 0;
}



///////////////////////////////// COLLISIONS /////////////////////////////////

void cl_lair::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_BOMB:
		if (!obj->damage) return;

		// Release 1 pod
		if (num_prisoners)
		{
			--num_prisoners;
			activateObjects(TYPE_POD,1,this);
		}

		// Fall through

	case TYPE_SHIP_LASER:
		checkCannonHit(obj);
		if (subDamage(obj)) 
		{
			x_scale = y_scale = 1;
			flash_timer = 0;
			circ_colour = DARK_BLUE;
		}
		else flash_timer = 2;
		break;

	case TYPE_POD:
		if (((cl_pod *)obj)->hitTop(this))
		{
			++num_prisoners;
			scale_inc = 0.1;
			reset();
			playSound(SND_POD_CAPTURED);
		}
		break;

	default:
		return;
	}
}




/*** This sees which side cannon - if any - was affected ***/
void cl_lair::checkCannonHit(cl_object *obj)
{
	int lr;

	// Doesn't count if cannon already dead or we're hit on the top
	if (obj->y - y >= 40) return;

	// Find side we were hit on
	lr = (getXDistance(x,obj->x) < 0 ? LEFT_CANNON : RIGHT_CANNON);
	if (cannon[lr].health > 0)
	{
		cannon[lr].health -= obj->damage;
		if (cannon[lr].health <= 0)
		{
			cannon[lr].breech_colour = MEDIUM_RED;
			cannon[lr].barrel_colour = DARK_RED;
			cannon[lr].shoot_flash = 0;
			cannon[lr].hit_flash = 0;
			cannon[lr].fireball_diam = 10;
			++cannon_destroyed;
			playSound(SND_CANNON_EXPLODE);
		}
		else cannon[lr].hit_flash = 5;
	}
}


//////////////////////////////////// RUN //////////////////////////////////////

void cl_lair::run()
{
	double ship_ang;

	// Timers
	if (flash_timer) --flash_timer;
	if (zombie_light_timer) --zombie_light_timer;
	if (grypper_light_timer) --grypper_light_timer;
	if (num_prisoners)
	{
		if (wave_arms_timer) 
			--wave_arms_timer;
		else
			wave_arms_timer = 20 + (int)random() % 100;
	}

	if (all_pods_captured && do_apc_check)
	{
		cannon_shoot_every /= 2;
		grypper_launch_every /= 2;
		do_apc_check = false;
	}

	// If we're soon going to explode...
	if (health < 5) shudder();

	// Run cannon
	ship_ang = getAngle(x,y,ship->x,ship->y);
	if (ship_ang > 180)
		moveAndFireCannon(LEFT_CANNON,ship_ang,225,315);
	else 
		moveAndFireCannon(RIGHT_CANNON,ship_ang,45,135);
	runCannon(LEFT_CANNON,225);
	runCannon(RIGHT_CANNON,135);

	// Launch grypper but only a short time past the point that skyths
	// start to appear unless APC
	if ((all_pods_captured || level_stage_timer < skyth_appear_at + 1000) &&
	    !(stage_timer % grypper_launch_every) &&
	     activateObjects(TYPE_GRYPPER,1,this)) grypper_light_timer = 20;

	// Launch zombie. If all_pods_captured just fire them out as fast as
	// possible.
	if (num_prisoners && 
	    (all_pods_captured || !(stage_timer % zombie_launch_every)) &&
	    activateObjects(TYPE_ZOMBIE,1,this))
	{
		--num_prisoners;
		zombie_light_timer = 20;
		circ_colour = MAUVE2;
	}
	else if (circ_colour > BLACK5) circ_colour -= 0.5;

	// Swallowed pod
	if (scale_inc)
	{
		x_scale += scale_inc;
		y_scale += scale_inc;
		main_colour -= scale_inc * 50;

		if (x_scale >= 1.4) scale_inc = -scale_inc;
		else if (x_scale <= 1) reset();
	}
}




/*** Move cannon up and down and fire ***/
void cl_lair::moveAndFireCannon(
	en_cannon lr, double ship_ang, double min_ang, double max_ang)
{
	double ang_diff;

	if (cannon[lr].health <= 0) return;

	// Move in direction of ship
	ang_diff = angleDiff(cannon[lr].angle,ship_ang);
	cannon[lr].angle += cannon_move_speed * SGN(ang_diff);

	if (cannon[lr].angle < min_ang)
		cannon[lr].angle = min_ang;
	else if (cannon[lr].angle > max_ang)
		cannon[lr].angle = max_ang;

	// Shoot cannon if pointing in direction of ship. cannon_fired variable 
	// read by cl_plasma object
	if (fabs(ang_diff) < 20 && 
	   !(stage_timer % cannon_shoot_every) &&
	   distToObject(ship) < SCR_X_MID) 
	{
		cannon_fired = lr;
		activateObjects(TYPE_PLASMA,1,this);
		cannon[lr].shoot_flash = 8;
	}
}




/*** Everything thats not moving towards ship and firing ***/
void cl_lair::runCannon(en_cannon lr, double bottom_ang)
{
	if (cannon[lr].hit_flash) --cannon[lr].hit_flash;
	if (cannon[lr].shoot_flash) --cannon[lr].shoot_flash;

	if (cannon[lr].health > 0) return;

	if (++cannon[lr].explode_timer == 1) incScore(300);

	// If cannon destroyed then drop it down
	if (lr == LEFT_CANNON && cannon[lr].angle > bottom_ang)
		cannon[lr].angle -= cannon_move_speed;
	else if (lr == RIGHT_CANNON && cannon[lr].angle < bottom_ang)
		cannon[lr].angle += cannon_move_speed;

	// Do fireball diameter
	if (cannon[lr].explode_timer < 10)
		cannon[lr].fireball_diam += 20;
	else if (cannon[lr].explode_timer < 50)
		cannon[lr].fireball_diam -= 5;
}




void cl_lair::runExplode()
{
	if (stage_timer == 1) playSound(SND_LAIR_EXPLODE);

	if (stage_timer < 60)
	{
		shudder();
		main_colour = random() % NUM_FULL_COLOURS;

		cannon[LEFT_CANNON].barrel_colour = random() % NUM_FULL_COLOURS;
		cannon[LEFT_CANNON].breech_colour = cannon[LEFT_CANNON].barrel_colour;

		cannon[RIGHT_CANNON].barrel_colour = random() % NUM_FULL_COLOURS;
		cannon[RIGHT_CANNON].breech_colour = cannon[RIGHT_CANNON].barrel_colour;
	}
	else if (stage_timer == 60) 
	{
		incScore(1000 + 200 * num_prisoners);
		x_scale = y_scale = 1;

		// If all pods captured then release as zombies. They will
		// probably have been released anyway but this is just in case
		if (all_pods_captured)
			activateObjects(TYPE_ZOMBIE,num_prisoners,this);
		else
			activateObjects(TYPE_POD,num_prisoners,this);
	}
	else if (stage_timer < 140)
	{
		if (stage_timer > 80) runExplodeBits();
		if ((explode_colour -= 0.3) <= RED) explode_colour = RED2;
	}
	else
	{
		// If lair is destroyed before cannon - eg bomb hit - then
		// make sure global is updated
		if (cannon[LEFT_CANNON].health > 0) ++cannon_destroyed;
		if (cannon[RIGHT_CANNON].health > 0) ++cannon_destroyed;

		setStage(STAGE_INACTIVE);
		return;
	}
}




void cl_lair::shudder()
{
	x_scale = 0.8 + (double)(random() % 40) / 100;
	y_scale = 0.8 + (double)(random() % 40) / 100;
}



//////////////////////////////////// DRAW //////////////////////////////////////

void cl_lair::draw()
{
	// Launch lights - zombie light has priority
	if (zombie_light_timer) drawLaunchLight(RED);
	else
	if (grypper_light_timer) drawLaunchLight(BLUE);

	drawBody();

	if (num_prisoners)
		drawMan(WHITE,wave_arms_timer > 20 && ((wave_arms_timer / 5) % 2));
}




void cl_lair::drawLaunchLight(int light_col)
{
	if (stage_timer % 2)
		objDrawOrFillRectangle(light_col,0,40,SCR_HEIGHT,-20,0,true);
}




void cl_lair::drawExplode()
{
	if (stage_timer < 60)
	{
		drawBody();
		if (num_prisoners) drawMan(WHITE,true);
		return;
	}
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);

	if (stage_timer < 70) fireball_diam += 80;
	else
	{
		if (fireball_diam > 0) fireball_diam -= 15;
		if (stage_timer > 80) drawExplodeBits();
	}
}




void cl_lair::drawBody()
{
	int i;
	int j;
	double lx;
	double ly;
	double col;

	// Draw cannon first - want body to partially obscure 
	for(i=0;i < 2;++i)
	{
		angle = cannon[i].angle;

		objDrawOrFillPolygon(
			cannon[i].hit_flash ? WHITE : cannon[i].breech_colour,
			LAIR_CANNON_VTX,breech_vtx,true);

		if (cannon[i].hit_flash) col = WHITE;
		else if (cannon[i].shoot_flash) col = YELLOW;
		else col = cannon[i].barrel_colour;

		objDrawOrFillPolygon(col,LAIR_CANNON_VTX,barrel_vtx,true);
	}

	angle = 0;
	objDrawOrFillPolygon(
		flash_timer ? WHITE : main_colour,
		LAIR_MAIN_VTX,main_vtx,true);
	objDrawOrFillPolygon(leg_colour,LAIR_LEG_VTX,leg1_vtx,true);
	objDrawOrFillPolygon(leg_colour,LAIR_LEG_VTX,leg2_vtx,true);

	objDrawOrFillCircle(circ_colour,0,POD_DIAM,0,0,true);

	// Draw cannons blowing up
	for(i=0;i < 2 && stage == STAGE_RUN;++i)
	{
		if (cannon[i].health > 0 || cannon[i].explode_timer > 50) 
			continue;

		// Draw fireball
		if (cannon[i].fireball_diam)
		{
			lx = (i == LEFT_CANNON ? -40 : 40);

			// Don't use objDraw.. because don't want x_scale,
			// y_scale coming into it.
			drawOrFillCircle(
				ORANGE,0,
				cannon[i].fireball_diam,
				cannon[i].fireball_diam,
				draw_x + lx,y,true);
		}

		// Draw lightning
		if (i == LEFT_CANNON) 
			lx = draw_x - random() % 100;
		else 
			lx = draw_x + random() % 100;

		ly = y + (random() % 61) - 30;

		for(j=0;j < 3;++j)
		{
			drawLightning(
				random() % SKY_BLUE,
				80,(double)(random() % 360),lx,ly);
		}
	}
}
