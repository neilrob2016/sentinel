#include "globals.h"

/*** Constructor - set up values that never change ***/
cl_text::cl_text(en_text_type tp)
{
	type = tp;
	active = false;
	x_scale = 1;
	y_scale = 1;
	thick = 1;
	colour = WHITE;
	pause_col = 0;

	switch(type)
	{
	case TEXT_SENTINEL:
		text = (char *)"SENTINEL";
		break;

	case TEXT_COPYRIGHT:
		text = (char *)"COPYRIGHT (C) NEIL ROBERTSON 2012-2016";
		y = 210;
		y_scale = 3;
		thick = 2;
		colour = TURQUOISE;
		break;

	case TEXT_PRESS_S:
		text = (char *)"PRESS 'S' TO START";
		y = 130;
		y_scale = 4;
		thick = 3;
		break;

	case TEXT_PAUSED:
		text = (char *)"PAUSED";
		x_scale = 8;
		y_scale = 8;
		x = 100;
		y = SCR_Y_MID;
		break;

	case TEXT_LEVEL_START:
		x_scale = 5;
		y_scale = 10;
		y = SCR_Y_MID;
		thick = 15;
		break;

	case TEXT_LEVEL_INST:
		y = SCR_Y_MID - 120;
		x_scale = 1;
		y_scale = 2;
		thick = 3;
		break;

	case TEXT_GAME_OVER:
		text = (char *)"GAME OVER";
		x_scale = 5;
		y_scale = 15;
		y = SCR_Y_MID;
		thick = 15;
		break;

	case TEXT_LEVEL_COMPLETE:
		text = (char *)"LEVEL COMPLETE";
		y = SCR_Y_MID;
		x_scale = 3.5;
		y_scale = 15;
		thick = 10;
		break;

	case TEXT_HIGH_SCORE:
		text = (char *)"NEW HIGH SCORE!";
		len = (int)strlen(text);
		y = SCR_Y_MID;
		break;

	case TEXT_ALL_PODS_CAPTURED:
		text = (char *)"ALL PODS CAPTURED!";
		y = SCR_Y_MID;
		x_scale = 5;
		y_scale = 8;
		thick = 10;
		break;

	case TEXT_BONUS_SCORE:
		text = NULL;
		x_scale = 1;
		y_scale = 1.5;
		thick = 3;
		break;

	case TEXT_POWERUP:
		text = NULL;
		x_scale = 1;
		y_scale = 1;
		thick = 3;
		break;

	case TEXT_ALL_PODS_SAVED:
		text = NULL;
		x_scale = 1.5;
		y_scale = 4;
		y = 150;
		x = 85;
		thick = 4;
		break;

	case TEXT_BONUS_LIFE:
		text = (char *)"BONUS LIFE!";
		len = (int)strlen(text);
		y = SCR_Y_MID;
		break;

	default:
		assert(0);
	}
}





/*** Reset values that will change ***/
void cl_text::activate()
{
	char txt[20];
	active = true;
	timer = 0;
	angle = 0;
	add = 0;

	switch(type)
	{
	case TEXT_SENTINEL:
		x = SCR_WIDTH;
		y = SCR_TOP;
		x_scale = 0;
		y_scale = 0;
		thick = 1;
		colour = GREEN;
		break;

	case TEXT_COPYRIGHT:
		x = SCR_WIDTH + 10;
		break;

	case TEXT_PRESS_S:
		break;

	case TEXT_PAUSED:
		colour = BLACK2;
		pause_col = 0;
		thick = 1;
		add = 0.5;
		break;

	case TEXT_LEVEL_START:
		sprintf(txt,"LEVEL %d",level);
		text = strdup(txt);
		add = 5;
		colour = GREEN;
		break;	
		
	case TEXT_LEVEL_INST:
		switch(level)
		{
		case 1:
		case 2:
			text = (char *)"RESCUE PODS, KILL ZOMBIES";
			break;

		case 3:
			text = (char *)"RESCUE PODS, KILL ZOMBIES, DESTROY 2 CANNON";
			break;

		case 4:
			text = (char *)"RESCUE PODS, KILL ZOMBIES, DESTROY ALL CANNON";
			break;

		case 5:
			text = (char *)"RESCUE PODS, KILL ZOMBIES, DESTROY 2 LAIRS";
			break;

		default:
			text = (char *)"RESCUE PODS, KILL ZOMBIES, DESTROY ALL LAIRS";

		}
		x = SCR_WIDTH;
		break;

	case TEXT_GAME_OVER:
		x = SCR_WIDTH;
		colour = ORANGE;
		add = -0.5;
		break;

	case TEXT_LEVEL_COMPLETE:
		colour = TURQUOISE;
		add = 1;
		break;

	case TEXT_HIGH_SCORE:
		x = SCR_WIDTH;
		colour = GREEN;
		add = 3;
		break;

	case TEXT_ALL_PODS_CAPTURED:
		x = SCR_WIDTH;
		colour = RED;
		break;

	case TEXT_BONUS_LIFE:
		x = SCR_WIDTH;
		angle = 0;
		add = 3;
		colour = GREEN;
		break;

	default:
		assert(0);
	}
}




/*** Just for bonus texts during game ***/
void cl_text::activateBonus(int bonus, cl_object *obj)
{
	char txt[20];

	active = true;
	angle = 0;
	angle2 = 0;
	timer = 0;
	x2 = obj->x - 50;
	y2 = obj->y;

	if (text) free(text);
	sprintf(txt,"BONUS %d",bonus);
	text = strdup(txt);
}




/*** Just for powerups ***/
void cl_text::activatePowerup(cl_object *obj)
{
	active = true;
	angle = 0;
	angle2 = 0;
	timer = 0;
	x2 = obj->x;
	y2 = obj->y;
	colour = GREEN2;
	x_scale = 1;
	y_scale = 1;
	thick = 1;

	switch(obj->type)
	{
	case TYPE_SHIELD_POWERUP:
		text = (char *)"SHIELD";
		break;

	case TYPE_LASER_POWERUP:
		text = (char *)"DUAL LASER";
		break;

	case TYPE_HEALTH_POWERUP:
		text = (char *)"HEALTH";
		break;

	case TYPE_HYPERSPACE_POWERUP:
		text = (char *)"HYPERSPACE";
		break;

	case TYPE_TRACTORBEAM_POWERUP:
		text = (char *)"TRACTORBEAM";
		break;

	case TYPE_REVERSE_G_POWERUP:
		text = (char *)"REVERSE-G";
		break;

	case TYPE_DISRUPTOR:
		text = (char *)"ZERO-G";
		break;
		
	default:
		assert(0);
	}
}




/*** All pods saved message at end of level ***/
void cl_text::activateAllPodsSaved(int bonus)
{
	char txt[50];

	active = true;
	timer = 0;

	if (text) free(text);
	sprintf(txt,"ALL PODS SAVED, BONUS = %05d",bonus);
	text = strdup(txt);
	colour = GREEN;
}




/*** Run and draw but doesn't need a seperate run() function because they're
     non interacting and unaffected by pausing ***/
void cl_text::draw()
{
	static en_colour pause_bot_col[6] = 
	{
		BLACK2,
		BLACK3,
		BLACK4,
		BLACK5,
		BLACK6,
		BLACK7
	};
	static en_colour pause_top_col[6] = 
	{
		RED2,
		GREEN3,
		BLUE2,
		MAUVE2,
		TURQUOISE2,
		YELLOW2
	};
	double col;
	int i;

	++timer;

	switch(type)
	{
	case TEXT_SENTINEL:
		if (game_stage != GAME_STAGE_DEMO_PLAY || timer > 300)
		{
			active = false;
			return;
		}
		drawCenteredText(text,colour,thick,0,0,x_scale,y_scale,y);

		// Get bigger and swoop in
		if (x_scale < 6.2)
		{
			x_scale += 0.1;
			y_scale += 0.3;
			thick += 0.3;
			incAngle(angle,2.3);
		}
		if (timer > 250)
		{
			// Disappear off top of screen
			add += 2;
			y += add;
		}
		else y = SCR_TOP - SCR_Y_MID * SIN(angle);

		if ((colour += 2) >= GREEN2) colour = GREEN;
		return;

	case TEXT_COPYRIGHT:
		if (game_stage != GAME_STAGE_DEMO_PLAY || timer > 400)
		{
			active = false;
			return;
		}
		if (timer < 80) return;

		// Get to rough centre
		if (x > 120) 
		{
			x -= 10;
			break;
		}

		// Disappear off to left after a while
		if (timer > 280) 
		{
			add += 2;
			x -= add;
			break;
		}

		// Draw in centre
		drawCenteredText(text,colour,thick,0,0,x_scale,y_scale,y);
		return;

	case TEXT_PRESS_S:
		if (game_stage != GAME_STAGE_DEMO_PLAY)
		{
			active = false;
			return;
		}
		if (timer < 150 || (timer % 40) < 20) return;
		drawCenteredText(text,colour,thick,0,0,x_scale,y_scale,y);
		return;

	case TEXT_PAUSED:
		if (!paused) 
		{
			active = false;
			return;
		}
		drawCenteredText(text,colour,thick,0,0,x_scale,y_scale,y);

		colour += add;
		thick += (add * 2);
		if (colour <= pause_bot_col[pause_col])
		{
			pause_col = (int)(pause_col + 1) % 6;
			colour = pause_bot_col[pause_col];
			add = -add;
		}
		else if (colour >= pause_top_col[pause_col]) add = -add;
		return;

	case TEXT_LEVEL_START:
		if (game_stage != GAME_STAGE_LEVEL_START)
		{
			free(text);
			active = false;
			return;
		}
		drawCenteredText(text,colour,thick,angle,0,x_scale,y_scale,y);

		incAngle(angle,add);
		if ((int)angle == 30 || (int)angle == 330) add = -add;
		colour = ((int)colour + 4) % GREEN2;
		return;

	case TEXT_LEVEL_INST:
		if (game_stage != GAME_STAGE_LEVEL_START)
		{
			active = false;
			return;
		}
		x -= 10;
		switch(level)
		{
		case 1:
		case 2:
			if (x < 220) x = 220;
			break;

		case 3:
			if (x < 83) x = 83;
			break;

		case 4:
			if (x < 68) x = 68;
			break;

		case 5:
			if (x < 92) x = 92;
			break;

		default:
			if (x < 76) x = 76;
		}
		break;

	case TEXT_GAME_OVER:
		if (game_stage != GAME_STAGE_GAME_OVER)
		{
			active = false;
			return;
		}
		if (x > 100)
		{
			x -= 20;
			incAngle(angle,-10);
		}
		else
		{
			x = 100;
			angle = 0;
			if (timer % 30 < 15) return;
		}
		colour += add;
		if (colour == RED || colour == ORANGE) add = -add;
		break;

	case TEXT_HIGH_SCORE:
		// Draw a sinusoidal wave of characters moving left
		angle2 = angle;
		x2 = x;
		col = colour;
		for(i=0;i < len;++i)
		{
			y2 = y + SIN(angle2) * 100;
			drawChar(text[i],col,8,0,5,5,x2,y2);
			incAngle(angle2,-20);
			x2 += 70;
			col += 3;
		}
		if (x2 < 0) 
		{
			active = false;
			return;
		}
		incAngle(angle,10);
		x -= 15;
		colour += add;
		if (colour <= GREEN || colour >= BLUE) add = -add;
		return;

	case TEXT_LEVEL_COMPLETE:
		if (game_stage != GAME_STAGE_LEVEL_COMPLETE)
		{
			active = false;
			return;
		}
		if ((timer % 40) > 20) return;

		drawCenteredText(text,colour,thick,0,0,x_scale,y_scale,y);
		colour += add;
		if (colour >= BLUE || colour <= TURQUOISE) 
			add = -add;
		return;

	case TEXT_ALL_PODS_CAPTURED:
		if ((x -= 25) < -1200) active = false;
		else
		if ((timer % 20) > 10) return;
		if (++colour == YELLOW) colour = RED;
		break;

	case TEXT_BONUS_SCORE:
		// Move around in a circle then zip off. getDrawX() because 
		// text must scroll with landscape, not stay fixed in place
		x = getDrawX(x2 + SIN(angle2) * timer * 3);
		if (timer < 55)
		{
			y = y2 + COS(angle2) * timer * 3;
			incAngle(angle2,18);
		}
		else if (y < SCR_HEIGHT) y += 40;
		else
		{
			active = false;
			return;
		}
		colour = random() % NUM_COLOURS;
		break;

	case TEXT_POWERUP:
		// Same algo as above
		x = getDrawX(x2 + SIN(angle2) * timer * 8);
		if (timer < 32)
		{
			y = y2 + COS(angle2) * timer * 10;
			incAngle(angle2,9);
		}
		else if (y < SCR_HEIGHT)
		{
			y += 20;
			incAngle(angle,18);
		}
		else
		{
			active = false;
			return;
		}
		colour = (int)(colour + 1) % GREEN2;

		x_scale += 0.1;
		y_scale += 0.2;
		thick += 0.2;
		break;
		
	case TEXT_ALL_PODS_SAVED:
		if (game_stage != GAME_STAGE_LEVEL_COMPLETE)
		{
			active = false;
			return;
		}
		colour += 2;
		if (colour >= GREEN2) colour = GREEN;
		break;

	case TEXT_BONUS_LIFE:
		x2 = x;
		col = colour;
		for(i=0;i < len;++i)
		{
			if (i % 2)
				y2 = y + SIN(angle+180) * 50;
			else
				y2 = y + SIN(angle) * 50;
			drawChar(text[i],col,10,0,5,5,x2,y2);
			col += 5;
			x2 += 70;
		}
		if (x2 < 0)
		{
			active = false;
			return;
		}
		incAngle(angle,5);
		x -= 15;
		colour += add;
		if (colour <= GREEN || colour >= PURPLE) add = -add;
		return;

	default:
		assert(0);
	}
	drawText(text,colour,thick,angle,0,x_scale,y_scale,x,y);
}
