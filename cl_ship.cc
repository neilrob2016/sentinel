#include "globals.h"

#define MAX_THRUST     1.5
#define MAX_THRUST_INC 0.02
#define THRUST_INC     0.1
#define THRUST_DEC     0.2

#define TIMER_CNT      30

#define POD_GRAB_DIST  (TRACTOR_BEAM_LEN * 2)

#define BEAM_STEP   2
#define BEAM_WIDTH  5
#define RING_DIAM   (POD_DIAM + 15)
#define RING_RADIUS (RING_DIAM / 2)

#define SHIP_DRAW_X_MAX (SCR_X_MID + 100)
#define SHIP_DRAW_X_MIN (SCR_X_MID - 100)

#define FLAME_TOP -8
#define FLAME_EDGE 8

#define MT_GAUGE_TIMER_START 50

#define MAX_TRACTOR_BEAM 2000

#define START_HEALTH 100

/////////////////////////////// SHAPE DEFS //////////////////////////////////

st_vertex cl_ship::centre_vtx[SHIP_CENTRE_VTX] =
{
	{ -12, 8 },
	{  12, 8 },
	{  12,-8 },
	{ -12,-8 }
};


st_vertex cl_ship::left_vtx[SHIP_TRI_VTX] = 
{
	{ -12,-8 },
	{ -25, 0 },
	{ -12, 8 }
};


st_vertex cl_ship::right_vtx[SHIP_TRI_VTX] = 
{
	{ 12,-8 },
	{ 25, 0 },
	{ 12, 8 }
};


st_vertex cl_ship::top_vtx[SHIP_TRI_VTX] =
{
	{ -12, 8 },
	{   0,15 },
	{  12, 8 }
};


st_vertex cl_ship::flame_vtx[SHIP_FLAME_VTX] =
{
	{ -FLAME_EDGE,FLAME_TOP },
	{  FLAME_EDGE,FLAME_TOP },
	{  0,FLAME_TOP } 
};


/////////////////////////////////// INIT /////////////////////////////////////

cl_ship::cl_ship(): cl_object(TYPE_SHIP,SHIP_DIAM)
{
	mass = 3;
	explode_bits_cnt = 500;
	cargo_cnt = 0;
	shield_timer = 0;
	hyperspace_cnt = 0;
	start_hyperspace_cnt = 0;
	resetPowerups();
}




/*** Called in main.cc initLevel() ***/
void cl_ship::activate(cl_object *activator)
{
	setAutoplayStage(AUTO_START);

	switch(level)
	{
	case 1:
	case 2:
		laser_shoot_every = 6;
		start_hyperspace_cnt = 1;
		break;

	case 3:
		laser_shoot_every = 5;
		start_hyperspace_cnt = 1;
		break;

	case 4:
		laser_shoot_every = 5;
		start_hyperspace_cnt = 2;
		break;

	case 5:
	case 6:
		laser_shoot_every = 4;
		start_hyperspace_cnt = 2;
		break;

	case 7:
	case 8:
		laser_shoot_every = 3;
		start_hyperspace_cnt = 2;
		break;

	default:
		laser_shoot_every = 2;
		start_hyperspace_cnt = 3;
		break;
	}

	if (hyperspace_cnt < start_hyperspace_cnt) 
		hyperspace_cnt = start_hyperspace_cnt;

	reset();
}




/*** Called in main.cc setGameStage() for GAME_STAGE_LEVEL_START when new game
     or level being reset ***/
void cl_ship::reset()
{
	assert(moonbase->stage != STAGE_INACTIVE);

	setStage(STAGE_RUN);
	setExplodeBits();

	if (game_stage != GAME_STAGE_SHOW_CAST)	
	{
		// We start sitting on the moonbase
		x = moonbase->x;
		y = moonbase->y + moonbase->radius + radius;
		screen_x_shift = x - SCR_X_MID;
		if (screen_x_shift < 0) screen_x_shift += LANDSCAPE_WIDTH;
		draw_x = getDrawX(x);

		// Protection at start. Half second extra each level
		shield_timer = 140 + 20 * level;
	}

	x_scale = 1;
	y_scale = 1;
	background_x_shift = 0;
	xspd = 0;
	yspd = 0;
	angle = 0;
	angle_add = 0;
	thrust = 0;
	max_speed = 19 + level;
	level_angle_add = (level < 6 ? 3 + 0.5 * level : 6);
	max_thrust = MAX_THRUST / 2;
	max_thrust_inc = 0;
	thrust_on = false;
	shooting = false;
	shoot_timer = 0;
	grab_timer = 0;
	drop_timer = 0;
	beam_col_start = TURQUOISE2;
	max_thrust_gauge_timer = 0;
	tractor_beam_energy = MAX_TRACTOR_BEAM;
	tractor_beam_blink = 0;
	flash_timer = 0;
	flame_timer = 0;
	spin_timer = 0;
	health = START_HEALTH;
	gun_colour = RED;

	if (game_stage == GAME_STAGE_SHOW_CAST) return;

	tellSoundShipThrust(0);

	dropCargo(0,false);
}




/*** Called in main.cc setGameStart() for GAME_STAGE_GAME_START at the start 
     of a new game and in runExplode() when life lost ***/
void cl_ship::resetPowerups()
{
	dual_laser = false;
	shield_powerup = false;
	hyperspace_cnt = 0;
}


///////////////////////////////// GLOBAL USE /////////////////////////////////

void cl_ship::incHealth(int h)
{
	health += h;
	if (health > START_HEALTH) health = START_HEALTH;
}


///////////////////////////////// PLAYER INPUT ///////////////////////////////

void cl_ship::thrustOn()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) thrust_on = true;
}




void cl_ship::thrustOff()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) thrust_on = false;
}




void cl_ship::incMaxThrust()
{
	if (stage == STAGE_RUN) 
	{
		max_thrust_inc = MAX_THRUST_INC;
		max_thrust_gauge_timer = MT_GAUGE_TIMER_START;
	}
}




void cl_ship::decMaxThrust()
{
	if (stage == STAGE_RUN) 
	{
		max_thrust_inc = -MAX_THRUST_INC;
		max_thrust_gauge_timer = MT_GAUGE_TIMER_START;
	}
}




void cl_ship::fixMaxThrust()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) 
		max_thrust_inc = 0;
}




void cl_ship::grab()
{
	// Use timer so we don't grab 1 object each iteration while use has
	// finger on key
	if (stage == STAGE_RUN && 
	    cargo_cnt < SHIP_MAX_CARGO &&
	    !grab_timer && tractor_beam_energy) 
	{
		grab_timer = TIMER_CNT;
		playSound(SND_SHIP_GRABBING);
	}
}




void cl_ship::drop()
{
	if (stage == STAGE_RUN && !drop_timer) 
		drop_timer = TIMER_CNT;
}




void cl_ship::dropAll()
{
	dropCargo(0,true);
}




void cl_ship::rotateAnti()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) 
		angle_add = -level_angle_add;
}




void cl_ship::rotateClock()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) 
		angle_add = level_angle_add;
}




void cl_ship::rotateStop()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) angle_add = 0;
}




void cl_ship::shootStart()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) shooting = true;
}




void cl_ship::shootStop()
{
	if (stage == STAGE_RUN || stage == STAGE_MATERIALISE) shooting = false;
}




void cl_ship::hyperspace()
{
	if (stage == STAGE_RUN && hyperspace_cnt)
	{
		hyper_diam = SCR_WIDTH;
		hyper_col = YELLOW;
		--hyperspace_cnt;
		dropAll();

		setStage(STAGE_MATERIALISE);
		playSound(SND_SHIP_DEMATERIALISE);
	}
}




void cl_ship::useShield()
{
	if (shield_powerup)
	{
		shield_timer = level < 10 ? 400 + level * 20 : 600;
		shield_powerup = false;
	}
}


///////////////////////////////// DEMO MODE //////////////////////////////////

/*** Calls key functions to simulate player. Doing this properly would require 
     some serious AI but I can't be bothered to attempt that. This is just
     enough code to give a reasonable demo for the short time it runs. It finds 
     a pod, attempts to grab it and return it to the moonbase. ***/
void cl_ship::autoplay()
{
	// Shoot at random regardless of stage
	if (!(random() % 20)) shootStart();
	else if (!(random() % 20)) shootStop();

	++autoplay_timer;

	switch(autoplay_stage)
	{
	case AUTO_START:
		thrustOn();
		setAutoplayStage(AUTO_LAUNCH);
		break;

	case AUTO_LAUNCH:
		if (autoplay_timer == 40)
		{
			autoplayChoosePod();
			autoplay_angle = 80 - gravity * 100;
			if (random() % 2) 
				autoplay_angle = 360 - autoplay_angle;
			setAutoplayStage(AUTO_GOTO_POD);
		}
		break;

	case AUTO_GOTO_POD:
		if(max_thrust < MAX_THRUST) incMaxThrust();

		autoplayFly();

		// See if we're near pod
		if (distToObject(autoplay_pod) < SCR_X_MID)
		{
			// Thrust off
			thrustOff();
			setAutoplayStage(AUTO_GRAB_POD);
		}

		break;

	case AUTO_GRAB_POD:
		if (y < SCR_Y_MID) grab();

		if (autoplay_timer > 40)
		{
			if (cargo_cnt)
			{
				// Head back towards base
				if (getXDistance(x,moonbase->x) < 0) 
					rotateAnti();
				else
					rotateClock();

				setAutoplayStage(AUTO_GOTO_MOONBASE);
			}
			else 
			{
				autoplayChoosePod();
				setAutoplayStage(AUTO_GOTO_POD);
			}
		}
		break;

	case AUTO_GOTO_MOONBASE:
		autoplayFly();

		// If we're near the moonbase drop everything 
		if (distToObject(moonbase) < SCR_X_MID)
		{
			dropAll();
			setAutoplayStage(AUTO_WAIT);
		}
		break;

	case AUTO_WAIT:
		autoplayFly();

		// Wait for pods to drop into moonbase
		if (autoplay_timer > 40)
		{
			autoplayChoosePod();
			setAutoplayStage(AUTO_GOTO_POD);
		}
		break;

	default:
		assert(0);
	}
}




void cl_ship::setAutoplayStage(en_autoplay_stage astg)
{
	autoplay_stage = astg;
	autoplay_timer = 0;
	rotateStop();
}




void cl_ship::autoplayChoosePod()
{
	int o;

	autoplay_pod = NULL;

	FOR_ALL_OBJECTS(o)
	{
		autoplay_pod = (cl_pod *)object[o];
		if (autoplay_pod->type == TYPE_POD &&
		    autoplay_pod->stage == STAGE_RUN) break;
	}
	assert(autoplay_pod);

	// Rotate to correct direction
	if (getXDistance(x,autoplay_pod->x) < 0) 
		rotateAnti();
	else
		rotateClock();
}




void cl_ship::autoplayFly()
{
	double diff = angleDiff(autoplay_angle,angle);

	if (diff < -5) rotateClock();
	else if (diff > 5) rotateAnti();
	else rotateStop();
	
	// If zero g point horizontal and go for it
	if (zero_g_timer)
	{
		if (autoplay_angle > 0 && autoplay_angle < 180)
			autoplay_angle = 90;
		else
			autoplay_angle = 270;
	}
	// Randomly modify angle a bit for some variation
	else if (!(random() % 30))
	{
		incAngle(autoplay_angle,random() % 41 - 20);

		// Don't go too vertical
		if (autoplay_angle < 20) autoplay_angle = 340;
		else
		if (autoplay_angle > 340) autoplay_angle = 20;
		else
		// And don't pile into the ground
		if (autoplay_angle > 80 && autoplay_angle < 180)
			autoplay_angle = 80;
		else if (autoplay_angle > 180 && autoplay_angle < 280)
			autoplay_angle = 280;
	}

	// Maintain altitude
	if (y < SCR_Y_MID || zero_g_timer)
		thrustOn();
	else
		thrustOff();
}



////////////////////////////////// RUNTIME ///////////////////////////////////

void cl_ship::collidedWith(cl_object *obj)
{
	switch(obj->type)
	{
	case TYPE_SHIP_LASER:
		break;

	case TYPE_SHIELD_POWERUP:
		shield_powerup = true;
		break;

	case TYPE_LASER_POWERUP:
		dual_laser = true;
		break;

	case TYPE_HEALTH_POWERUP:
		// First appears on level 4 so gives 40 health, get max health 
		// level 10+
		incHealth(level * 10);
		break;

	case TYPE_HYPERSPACE_POWERUP:
		++hyperspace_cnt;
		break;

	case TYPE_TRACTORBEAM_POWERUP:
		tractor_beam_energy = MAX_TRACTOR_BEAM;
		break;

	case TYPE_REVERSE_G_POWERUP:
		// Powerup object does the work here
		break;

	case TYPE_ZOMBIE:
		bounceOffObject(obj);
		// Fall through

	case TYPE_STALAG_LASER:
	case TYPE_MISSILE:
	case TYPE_BULLET:
	case TYPE_PLASMA:
	case TYPE_GRYPPER:
	case TYPE_SKYTH:
		if (!shield_timer) damageCollision(obj);
		break;

	case TYPE_DISRUPTOR:
		if (shield_timer) break;

		// If we've hit the disruptor itself then lose all damage
		// else if we're in disrupt radius then just subtract 1
		if (distToObject(obj) <= 0)
			subDamage(obj);
		else
			 --health;

		if (health <= 0)
		{
			explode();
			break;
		}

		flash_timer = 3;
		playSound(SND_SHIP_HIT);

		// Lose power
		if (!max_thrust_gauge_timer)
			max_thrust_gauge_timer = MT_GAUGE_TIMER_START;
		if (max_thrust > 0)
		{
			max_thrust -= MAX_THRUST_INC;
			if (max_thrust < 0) max_thrust = 0;
		}

		// Disruptor makes us drop all cargo
		dropAll();
		break;

	case TYPE_STRIKER:
		if (shield_timer) break;

		// If we've hit the striker itself then just bounce off but if
		// we've hit the bolt then its bad news
		if (distToObject(obj) <= 0)
			bounceOffObject(obj);
		else
			damageCollision(obj);
		break;

	default:
		bounceOffObject(obj);
	}
}




void cl_ship::damageCollision(cl_object *obj)
{
	if (subDamage(obj)) explode();
	else
	{
		flash_timer = 3;
		playSound(SND_SHIP_HIT);
	}
}




void cl_ship::explode()
{
	setStage(STAGE_EXPLODE);
	dropAll();
	playSound(SND_SHIP_EXPLODE);

	fireball_diam = 0;
	explode_colour = ORANGE;
}




/*** Main run function ***/
void cl_ship::run()
{
	st_hill_inside *hi;

	updateThrust();
	updateTimers();
	if (cargo_cnt) updateCargo();
	updateBeamEnergy();

	// Rotate
	incAngle(angle,angle_add);

	// Alter speed based on thrust and gravity
	xspd = (xspd + (thrust * SIN(angle) / mass)) * FRICTION;
	yspd = (yspd + (thrust * COS(angle) / mass)) * FRICTION - gravity;
	limitToMaxSpeed(max_speed);
	
	updatePrevious();
	updateXY(x,y);

	// Check for collision with hill
	if ((hi = insideHill(radius,x,y)))
	{
		// If all pods captured then hitting hill costs health
		if (all_pods_captured) 
		{
			if ((health -= 2) <= 0) 
				explode();
			else
				flash_timer = 5;
		}
		bounceOffHill(hi,0.3);
	}

	// Shift the landscape on screen
	if (draw_x > SHIP_DRAW_X_MAX) 
	{
		draw_x = SHIP_DRAW_X_MAX;
		scrollLandscape();
	}
	else if (draw_x < SHIP_DRAW_X_MIN)
	{
		draw_x = SHIP_DRAW_X_MIN;
		scrollLandscape();
	}

	// Guns colour
	if (gun_colour > RED) gun_colour -= 0.5;
}




/*** Update max thrust and thrust ***/
void cl_ship::updateThrust()
{
	// Do thrust 
	if (max_thrust_inc)
	{
		max_thrust += max_thrust_inc;
		if (max_thrust < 0) max_thrust = 0;
		else
		if (max_thrust > MAX_THRUST) max_thrust = MAX_THRUST;
	}
	if (thrust_on) 
	{
		thrust += THRUST_INC;
		if (thrust > max_thrust) thrust = max_thrust;
	}
	else
	{
		thrust -= THRUST_DEC;
		if (thrust < 0) thrust = 0;
	}
	tellSoundShipThrust((u_char)(thrust / MAX_THRUST * 255));

	if (thrust) playSound(SND_SHIP_THRUST);

	if (flash_timer) --flash_timer;
}




/*** Update timers and perform actions ***/
void cl_ship::updateTimers()
{
	if (grab_timer) 
	{
		if (grab_timer && tractor_beam_energy) 
			grabNearestSphere();
		if (grab_timer) --grab_timer;
	}
	else if (drop_timer)
	{
		if (drop_timer == TIMER_CNT) dropBottomSphere();
		--drop_timer;
	}

	if (shooting)
	{
		if (shoot_timer) --shoot_timer;
		else
		{
			// Activation object ptr is just used as a flag here
			if (dual_laser)
			{
				activateObjects(TYPE_SHIP_LASER,1,NULL);
				activateObjects(TYPE_SHIP_LASER,1,this);
			}
			else activateObjects(TYPE_SHIP_LASER,1,NULL);

			gun_colour = YELLOW;
			shoot_timer = laser_shoot_every;
		}
	}
	else shoot_timer = 0;

	if (max_thrust_gauge_timer) --max_thrust_gauge_timer;

	if (shield_timer)
	{
		--shield_timer;
		playSound(SND_SHIP_SHIELD);
	}

	// Miscellanious graphics timers
	spin_timer = (spin_timer + 1) % 15;
	flame_timer = (flame_timer + 1) % 3;
}




/*** Go through the cargo we've collected and see if we need to drop any then
     calc forces ***/
void cl_ship::updateCargo()
{
	double force_x;
	double force_y;
	int i;

	for(i=0;i < cargo_cnt;++i)
	{
		if (cargo[i]->stage != STAGE_RUN) 
		{
			// Drop all including and below
			dropCargo(i,true);
			break;
		}
	}
	if (!cargo_cnt) return;

	// Calculate interactive forces with cargo being carried which will
	// update our speed
	for(i=1;i < cargo_cnt;++i)
	{
		cargo[i]->calcForces(cargo[i-1],force_x,force_y);
		cargo[i]->addForceBelow(force_x,force_y);
		cargo[i-1]->addForceAbove(force_x,force_y);
	}

	// Calc interaction between ship and cargo just below it
	calcForces(cargo[0],force_x,force_y);
	cargo[0]->addForceAbove(force_x,force_y);
	addForceBelow(force_x,force_y);
}




/*** Inc/dec tractor beam energy and a few other things ***/
void cl_ship::updateBeamEnergy()
{
	if (cargo_cnt)
	{
		tractor_beam_energy -= 2;
		if (tractor_beam_energy <= 0)
		{
			dropAll();
			tractor_beam_energy = 0;
		}

		if (--beam_col_start < BLACK6) 
			beam_col_start = TURQUOISE2;

		if (tractor_beam_energy < MAX_TRACTOR_BEAM / 5) 
			tractor_beam_blink = (tractor_beam_blink + 1) % 14;
	}
	else if (tractor_beam_energy < MAX_TRACTOR_BEAM) 
		++tractor_beam_energy;
}




/*** Grab the sphere thats nearest to the ship or if we're carrying cargo 
     already then the bottom sphere ***/
void cl_ship::grabNearestSphere()
{
	double dist;
	double min_dist;
	double x2;
	double y2;
	cl_sphere *sph;
	cl_sphere *grabbed_sph;
	int o;

	tractor_beam_blink = 0;

	switch(cargo_cnt)
	{
	case 0:
		x2 = x;
		y2 = y;
		break;

	case SHIP_MAX_CARGO:
		return;

	default:
		x2 = cargo[cargo_cnt-1]->x;
		y2 = cargo[cargo_cnt-1]->y;
	}

	min_dist = POD_GRAB_DIST;
	grabbed_sph = NULL;

	FOR_ALL_OBJECTS(o)
	{
		sph = (cl_sphere *)object[o];
		if (sph->stage == STAGE_RUN &&
		    (sph->type == TYPE_POD || sph->type == TYPE_BOMB) &&
		    !sph->grabber)
		{
			dist = hypot(getXDistance(x2,sph->x),y2 - sph->y);
			if (dist <= min_dist + sph->radius)
			{
				min_dist = dist;
				grabbed_sph = sph;
			}
		}
	}
	if (grabbed_sph && grabbed_sph->grab(this))
	{
		cargo[cargo_cnt++] = grabbed_sph;
		playSound(SND_SHIP_GRABBED);
		grab_timer = 0;
	}
}




/*** Drop all cargo from bottom up to and including "pos" ***/
void cl_ship::dropCargo(int pos, bool play_sound)
{
	if (cargo_cnt)
	{
		for(int i=cargo_cnt-1;i >= pos;--i) cargo[i]->release();
		cargo_cnt = pos;
		if (play_sound) playSound(SND_SHIP_DROP);
	}
}



		
/*** Drop the sphere the bottom of the chain ***/
void cl_ship::dropBottomSphere()
{
	if (cargo_cnt)
	{
		cargo[cargo_cnt-1]->release();
		--cargo_cnt;
		playSound(SND_SHIP_DROP);
	}
}




void cl_ship::scrollLandscape()
{
	screen_x_shift += xspd;

	if (screen_x_shift >= LANDSCAPE_WIDTH) 
		screen_x_shift -= LANDSCAPE_WIDTH;
	else if (screen_x_shift < 0) 
		screen_x_shift += LANDSCAPE_WIDTH;
	
	background_x_shift += (xspd / 2);

	// Stars and planets do parallax movement
	if (background_x_shift >= LANDSCAPE_WIDTH) 
		background_x_shift -= LANDSCAPE_WIDTH;
	else if (background_x_shift < 0) 
		background_x_shift += LANDSCAPE_WIDTH;
}




/*** Disappear off to hyperspace then back again ***/
void cl_ship::runMaterialise()
{
	if (stage_timer < 11)
	{
		// Dematerialise
		if (hyper_diam > 0) hyper_diam -= 80;
		if (x_scale > 0)
		{
			x_scale -= 0.1;
			y_scale -= 0.1;
		}
		hyper_col -= 5;
		return;
	}

	if (stage_timer == 11)
	{
		// Set destination
		hyper_x = x + SCR_WIDTH + random() % 
		          (LANDSCAPE_WIDTH - SCR_WIDTH * 2);
		if (hyper_x >= LANDSCAPE_WIDTH) hyper_x -= LANDSCAPE_WIDTH;

		xspd = hyper_x < x ? -150 : 150;
		yspd = 0;

		y = SCR_Y_MID;
		return;
	}

	if (stage_timer < 1000)
	{
		// Move to destination
		scrollLandscape();
		updateXY(x,y);
		if (fabs(getXDistance(x,hyper_x)) <= 150)
		{
			xspd = 0;
			x = hyper_x;
			stage_timer = 1000;
		}
		return;
	}

	if (stage_timer == 1001) playSound(SND_SHIP_MATERIALISE);

	// Materialise
	hyper_diam += 80;
	if (x_scale < 1)
	{
		x_scale += 0.1;
		y_scale += 0.1;
		hyper_col += 5;
	}
	else setStage(STAGE_RUN);
}




/*** Explode stage ***/
void cl_ship::runExplode()
{
	if (stage_timer < 30) return;
	
	runExplodeBits();

	screen_x_shake = (int)random() % 21 - 10;
	screen_y_shake = (int)random() % 21 - 10;

	if (stage_timer < 50) 
	{
		fireball_diam += 20;
		if (explode_colour < YELLOW) explode_colour += 0.5;
	}
	else if (stage_timer < 170)
	{
		fireball_diam -= 4;
		explode_colour -= 0.20;
		if (explode_colour <= RED) explode_colour = RED2;
	}
	else 
	{
		screen_x_shake = 0;
		screen_y_shake = 0;

		// Lose power-ups when we die
		resetPowerups();
		hyperspace_cnt = start_hyperspace_cnt;

		setStage(STAGE_INACTIVE);
	}
}


////////////////////////////////// DRAW /////////////////////////////////////

void cl_ship::draw()
{
	int i;
	int edge;

	// Draw tractor beams first so ship body and flame overlays. Pods
	// will draw themselves.
	if (cargo_cnt)
	{
		for(i=0;tractor_beam_blink < 7 && i < cargo_cnt;++i)
		{
			if (i)
				// Sphere to sphere
				drawTractorBeam(cargo[i-1],cargo[i]);
			else
				// Ship to sphere
				drawTractorBeam(this,cargo[0]);
		}
	}

	// Draw ship body
	if (flash_timer) drawBody(YELLOW,YELLOW,YELLOW,YELLOW);
	else
	{
		drawBody(
			gun_colour,
			spin_timer < 5 ? TURQUOISE : LIGHT_BLUE,
			spin_timer >= 5 && spin_timer < 10 ? TURQUOISE : LIGHT_BLUE,
			spin_timer >= 10 && spin_timer < 15 ? TURQUOISE : LIGHT_BLUE);
	}

	// Draw flame
	if (thrust && flame_timer < 2) 
	{
		edge = FLAME_EDGE + (int)random() % 3 - 1;
		flame_vtx[0].x = -edge;
		flame_vtx[1].x = edge;
		flame_vtx[2].y = FLAME_TOP - (thrust / MAX_THRUST) * 
		                 (50 + random() % 30);
		objDrawOrFillPolygon(
			ORANGE + (random() % 11) - 5,
			SHIP_FLAME_VTX,flame_vtx,true);
	}

	// Draw grab sphere either around ship or bottom pod if we're
	// carrying any.
	if (grab_timer && tractor_beam_energy && !(stage_timer % 3)) 
	{
		if (cargo_cnt) 
		{
			drawOrFillCircle(
				TURQUOISE,10,
				POD_GRAB_DIST*2,
				POD_GRAB_DIST*2,
				cargo[cargo_cnt-1]->draw_x,
				cargo[cargo_cnt-1]->y,
				false);
		}
		else
		{
			objDrawOrFillCircle(
				TURQUOISE,10,POD_GRAB_DIST*2,
				0,0,false);
		}
	}

	// Draw shield. % 2 makes it shimmer. % 12 is so it flashes 2 seconds
	// before it runs out.
	if (shield_timer && shield_timer % 2 &&
	    (shield_timer > 80 || shield_timer % 12 < 6))
		objDrawOrFillCircle((stage_timer*4) % GREEN2,5,SHIP_DIAM * 2,0,0,false);
}




/*** This is the beam that links the ship -> sphere and sphere -> sphere ***/
void cl_ship::drawTractorBeam(cl_object *from, cl_object *to)
{
	double num_points;
	double xd;
	double yd;
	double xa;
	double ya;
	double px;
	double py;
	double col;
	int i;

	// Can't wait until draw() has set draw_x
	to->setDrawX();
	if (from != this)
	{
		from->setDrawX();

		// If draw_x wraps we could end up drawing a line across 
		// the screen
		if (fabs(from->draw_x - to->draw_x) > SCR_WIDTH) return;
	}

	// Draw a ring around "to" object
	drawOrFillCircle(
		beam_col_start,
		BEAM_WIDTH,RING_DIAM,RING_DIAM,to->draw_x,to->y,false);

	// Work out plot steppings. 
	xd = to->draw_x - from->draw_x;
	yd = to->y - from->y;

	if (fabs(xd) > fabs(yd))
	{
		xa = xd < 0 ? -BEAM_STEP : BEAM_STEP;
		if ((num_points = fabs(xd / BEAM_STEP))) 
			ya = yd / num_points;
		else
			ya = 0;
	}
	else
	{
		ya = yd < 0 ? -BEAM_STEP : BEAM_STEP;
		if ((num_points = fabs(yd / BEAM_STEP))) 
			xa = xd / num_points;
		else
			xa = 0;
	}

	// Set start values
	px = from->draw_x;
	py = from->y;
	col = beam_col_start;

	// Draw the points
	for(i=0;i < (int)num_points;++i)
	{
		// Draw from outside of ring to outside of next ring, unless
		// its the ship in which case draw from centre of ship.
		if ((from == this || 
		     hypot(from->draw_x - px, from->y - py) >= RING_RADIUS) &&
		    hypot(to->draw_x - px, to->y - py) >= RING_RADIUS)
		{
			drawOrFillCircle(col,0,BEAM_WIDTH,BEAM_WIDTH,px,py,true);
		}

		px += xa;
		py += ya;
		col -= 0.1;
		if (col < BLACK6) col = TURQUOISE2;
	}
}




void cl_ship::drawMaterialise()
{
	if (x_scale > 0) draw();
	if (hyper_diam > 0)
	{
		for(int i=0;i < 5;++i)
		{
			drawOrFillCircle(
				hyper_col,50,
				hyper_diam * i,hyper_diam * i,draw_x,y,false);
		}
	}
}




void cl_ship::drawExplode()
{
	int col;

	if (stage_timer < 30)
	{
		if (stage_timer % 2)
		{
			col = (int)random() % NUM_FULL_COLOURS;
			drawBody(col,col,col,col);
		}
	}
	else 
	{
		objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
		drawExplodeBits();
	}
}




void cl_ship::drawBody(
	double top_col, double left_col, double cent_col, double right_col)
{
	objDrawOrFillPolygon(top_col,SHIP_TRI_VTX,top_vtx,true);
	objDrawOrFillPolygon(left_col,SHIP_TRI_VTX,left_vtx,true);
	objDrawOrFillPolygon(cent_col,SHIP_CENTRE_VTX,centre_vtx,true);
	objDrawOrFillPolygon(right_col,SHIP_TRI_VTX,right_vtx,true);

	// Legs
	objDrawLine(cent_col,1,-10,-8,-15,-20);
	objDrawLine(cent_col,1,10,-8,15,-20);

	// Outside laser spikes
	if (dual_laser)
	{
		objDrawLine(top_col,1,-25,0,-25,5);
		objDrawLine(top_col,1,25,0,25,5);
	}
}


///////////////////////// Called by drawPlayScreen() /////////////////////////

#define MT_GAUGE_X  30
#define TR_GAUGE_X  (SCR_WIDTH - 30) 

#define GAUGE_HEIGHT  300
#define GAUGE_BOT     150
#define GAUGE_TOP    (GAUGE_BOT + GAUGE_HEIGHT)

#define H_GAUGE_HEIGHT     (SCR_TOP_HEIGHT - 10)
#define H_GAUGE_WIDTH       20
#define H_GAUGE_LEFT       (SCR_WIDTH - 155)
#define H_GAUGE_TOP        (SCR_HEIGHT - 5)
#define H_GAUGE_BOT        (H_GAUGE_TOP - H_GAUGE_HEIGHT)
#define H_GAUGE_BARS        10
#define H_GAUGE_BAR_HEIGHT (H_GAUGE_HEIGHT / H_GAUGE_BARS)

#define ICON_X (SCR_WIDTH - 115)
#define HYPER_Y (SCR_HEIGHT - 15)

/*** Top of the screen health monitor ***/
void cl_ship::drawHealthGauge()
{
	int col;
	int cnt;
	int i;

	cnt = (int)round(H_GAUGE_BARS * (double)health / START_HEALTH);

	col = RED;
	for(i=0;i < cnt;++i)
	{
		drawOrFillRectangle(
			col,0,
			H_GAUGE_WIDTH,H_GAUGE_BAR_HEIGHT,
			H_GAUGE_LEFT,H_GAUGE_BOT + H_GAUGE_BAR_HEIGHT * i,
			true);
		col += 3;
	}

	drawOrFillRectangle(
		PURPLE,2,
		H_GAUGE_WIDTH,H_GAUGE_HEIGHT,
		H_GAUGE_LEFT,H_GAUGE_BOT,
		false);
}




/*** Max thrust gauge on left of screen. Only visible when changing ***/
void cl_ship::drawMaxThrustGauge()
{
	double gy; 
	double ratio;
	double col;
	int i;

	if (max_thrust_gauge_timer)
	{
		// Vertical
		drawLine(
			LIGHT_BLUE,3,
			MT_GAUGE_X,GAUGE_BOT,MT_GAUGE_X,GAUGE_TOP);

		// Intermediate lines
		for(i=0,gy=GAUGE_BOT;i < 10;++i,gy+=GAUGE_HEIGHT/10)
			drawLine(LIGHT_BLUE,1,MT_GAUGE_X,gy,MT_GAUGE_X+10,gy);

		// Top line
		drawLine(
			LIGHT_BLUE,3,
			MT_GAUGE_X,GAUGE_TOP,MT_GAUGE_X+30,GAUGE_TOP);

		// Bottom line
		drawLine(
			LIGHT_BLUE,3,
			MT_GAUGE_X,GAUGE_BOT,MT_GAUGE_X+30,GAUGE_BOT);

		// Max thrust marker
		ratio = max_thrust / MAX_THRUST;
		gy = GAUGE_BOT + ratio * GAUGE_HEIGHT;
		col = GREEN2 - ratio * (GREEN2 - RED);
		drawLine(col,10,MT_GAUGE_X+4,gy,MT_GAUGE_X+20,gy);
	}
}




/*** Energy left in tractor mean on right of screen. Only visible when
     tractor beam being used ***/
void cl_ship::drawTractorBeamEnergyGauge()
{
	double gy;
	double ratio;
	double col;
	int i;

	if (cargo_cnt)
	{
		drawLine(
			LIGHT_BLUE,3,
			TR_GAUGE_X,GAUGE_BOT,TR_GAUGE_X,GAUGE_TOP);

		for(i=0,gy=GAUGE_BOT;i < 10;++i,gy+=GAUGE_HEIGHT/10)
			drawLine(LIGHT_BLUE,1,TR_GAUGE_X,gy,TR_GAUGE_X-10,gy);

		drawLine(
			LIGHT_BLUE,3,
			TR_GAUGE_X,GAUGE_TOP,TR_GAUGE_X-30,GAUGE_TOP);

		drawLine(
			LIGHT_BLUE,3,
			TR_GAUGE_X,GAUGE_BOT,TR_GAUGE_X-30,GAUGE_BOT);

		// Beam energy remaining marker
		ratio = (double)tractor_beam_energy / MAX_TRACTOR_BEAM;
		gy = GAUGE_BOT + ratio * GAUGE_HEIGHT;
		col = RED + ratio * (GREEN2 - RED);
		drawLine(col,10,TR_GAUGE_X-4,gy,TR_GAUGE_X-20,gy);
	}
}




/*** Power up icons in top right ***/
void cl_ship::drawPowerupIcons()
{
	char text[5];

	if (hyperspace_cnt)
	{
		// Hyperspace available icon
		int i = (stage_timer / 10) % 3;
		drawOrFillCircle(hypcols[i],1,25,25,ICON_X,HYPER_Y,false);
		drawOrFillCircle(hypcols[(i+1)%3],1,15,15,ICON_X,HYPER_Y,false);
		drawOrFillCircle(hypcols[(i+2)%3],1,7,7,ICON_X,HYPER_Y,false);

	}

	if (shield_timer)
	{
		// Shield timer countdown in place of icon
		sprintf(text,"%03d",shield_timer);
		drawText(
			text,(stage_timer*4) % GREEN2,
			1,0,0,0.6,1.5,ICON_X-8,HYPER_Y - 30);
	}
	else if (shield_powerup)
	{
		// Shield icon
		drawOrFillCircle(
			(stage_timer*4) % GREEN2,
			2,23,23,ICON_X,HYPER_Y - 28,false);
	}
}
