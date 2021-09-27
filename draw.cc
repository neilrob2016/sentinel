#include "globals.h"

#define CIRCLE   23040
#define SEMI    (CIRCLE / 2)
#define QUART   (CIRCLE / 4)

#define CAST_ANGLE_ADD ((double)360 / NUM_SHOW_OBJECTS)

#define RADAR_WIDTH   470
#define RADAR_HEIGHT  60

// Radar shows 60% (6/LANDSCAPE_MULT) of the landscape. 
#define RADAR_MULT    6 
#define RADAR_VIEW    (SCR_WIDTH * RADAR_MULT)
#define RADAR_X_MULT  ((double)RADAR_WIDTH / RADAR_VIEW)
#define RADAR_Y_MULT  ((double)RADAR_HEIGHT / SCR_TOP)

#define RADAR_LEFT   (SCR_X_MID - RADAR_WIDTH / 2)
#define RADAR_RIGHT  (RADAR_LEFT + RADAR_WIDTH)
#define RADAR_TOP     SCR_HEIGHT
#define RADAR_BOT    (RADAR_TOP - RADAR_HEIGHT)

#define RADAR_SCR_VIS_WIDTH ((double)RADAR_WIDTH / RADAR_MULT)
#define RADAR_SCR_VIS_LEFT  (RADAR_LEFT + RADAR_WIDTH/2 - RADAR_SCR_VIS_WIDTH/2)
#define RADAR_SCR_VIS_RIGHT  RADAR_SCR_VIS_LEFT + RADAR_SCR_VIS_WIDTH

//////////////////////////////////// MISC ////////////////////////////////////

/*** Set the thickness of the graphics context for the given colour ***/
void setThickness(double col, double thick)
{
	thick *= avg_scale;
	if (thick < 1) thick = 1;
	XSetLineAttributes(
		display,
		gc[(int)round(col)],
		(int)round(thick),LineSolid,CapRound,JoinRound);
}



////////////////////////////// ATTRACT SCREENS ///////////////////////////////

/*** Draw a rotating carousel showing who does what ***/
void drawCastScreen()
{
	static int text_col = GREEN;
	static int arrow_col = GREEN;
	cl_object *obj;
	double ang;
	char *name;
	char *text;
	int i;
	int o;
	int acol;
	
	drawCenteredText("THE CAST",text_col,6,0,0,2,4,SCR_HEIGHT - 30);
	if (++text_col >= GREEN2) text_col = GREEN;

	// Draw objects first
	ang = cast_angle;
	for(o=0;o < NUM_SHOW_OBJECTS;++o)
	{
		obj = show_object[o];

		// For colour cycling etc
		++obj->stage_timer;
		obj->run();

		// Set x,y positions on carousel
		obj->draw_x = SCR_X_MID + SIN(ang) * 350;
		obj->x = SCR_X_MID; // Just to be safe
		obj->y = (SCR_Y_MID - 20) + COS(ang) * 220;

		// Reset anything run() may have updated
		obj->x_scale = obj->y_scale = 1;

		obj->draw();

		incAngle(ang,-CAST_ANGLE_ADD);
	}

	// Draw arrow pointing at object at top
	acol = arrow_col;
	for(i=320;i < 420;i+=5)
	{
		drawLine(acol,8,SCR_X_MID,i,SCR_X_MID,i+5);
		acol = (acol + 1) % GREEN2;
	}
	drawLine(acol,8,SCR_X_MID,420,SCR_X_MID - 30,380);
	drawLine(acol,8,SCR_X_MID,420,SCR_X_MID + 30,380);
	if ((arrow_col -= 2) < GREEN) arrow_col += GREEN2;

	// Draw descriptive text over the top of objects
	ang = cast_angle;
	for(o=0;o < NUM_SHOW_OBJECTS;++o)
	{
		if (ang < CAST_ANGLE_ADD  / 2 || ang > 360 - CAST_ANGLE_ADD / 2)
		{
			// Info about object
			switch(show_object[o]->type)
			{
			case TYPE_SHIP:
				name = (char *)"YOUR SHIP";
				text = (char *)"THE LAST PLANETARY DEFENCE SHIP";
				break;

			case TYPE_POD:
				name = (char *)"POD";
				text = (char *)"GRAB AND RETURN TO MOONBASE TO SAVE THE MAN!";
				break;

			case TYPE_BOMB:
				name = (char *)"BOMB";
				text = (char *)"USE AGAINST ENEMY LAIRS";
				break;

			case TYPE_MOONBASE:
				name = (char *)"MOONBASE";
				text = (char *)"DROP PODS HERE";
				break;

			case TYPE_LAIR:
				name = (char *)"LAIR";
				text = (char *)"CANNON PROTECTED ENEMY COMMAND CENTRE";
				break;

			case TYPE_GRYPPER:
				name = (char *)"GRYPPER";
				text = (char *)"NO POD IS SAFE FROM THESE!";
				break;

			case TYPE_ZOMBIE:
				name = (char *)"ZOMBIE";
				text = (char *)"A CAPTURED MAN'S FATE!";
				break;

			case TYPE_SKYTH:
				name = (char *)"SKYTH";
				text = (char *)"A LATE ARRIVAL AT THE PARTY";
				break;

			case TYPE_DISRUPTOR:
				name = (char *)"DISRUPTOR";
				text = (char *)"DISRUPTS TRACTOR BEAMS AND REDUCES HEALTH";
				break;

			case TYPE_SILO:
				name = (char *)"SILO";
				text = (char *)"LAUNCHES MISSILES";
				break;

			case TYPE_MISSILE:
				name = (char *)"MISSILE";
				text = (char *)"THESE HAVE YOUR NAME ON THEM";
				break;

			case TYPE_STALAG:
				name = (char *)"STALAG";
				text = (char *)"ROOF MOUNTED LASER GUN";
				break;
			
			case TYPE_STRIKER:
				name = (char *)"STRIKER";
				text = (char *)"A BOLT FROM THE BLUE!";
				break;

			default:
				assert(0);
			}
			drawCenteredText(name,text_col,3,0,0,1,3,SCR_Y_MID - 20);
			drawCenteredText(text,LIGHT_BLUE,2,0,0,1,1,SCR_Y_MID - 60);
		}
		incAngle(ang,-CAST_ANGLE_ADD);
	}
	incAngle(cast_angle,0.5);
}


#define KX  50
#define KX2 210
#define KX3 100
#define KY  490
#define KY2 250

void drawKeysScreen()
{
	static int col = GREEN;

	drawCenteredText("CONTROL KEYS",col,6,0,0,2,4,SCR_HEIGHT - 30);
	if (++col >= GREEN2) col = GREEN;

	drawText("UP ARROW",MAUVE,2,0,0,1,1.5,KX,KY);
	drawText(": THRUST",WHITE,2,0,0,1,1.5,KX2,KY);

	drawText("L&R ARROWS",MAUVE,2,0,0,1,1.5,KX,KY - 30);
	drawText(": ROTATE",WHITE,2,0,0,1,1.5,KX2,KY - 30);

	drawText("SHIFT",MAUVE,2,0,0,1,1.5,KX,KY - 60);
	drawText(": FIRE LASER",WHITE,2,0,0,1,1.5,KX2,KY - 60);

	drawText("TAB/ALT",MAUVE,2,0,0,1,1.5,KX,KY - 90);
	drawText(": GRAB POD OR BOMB",WHITE,2,0,0,1,1.5,KX2,KY - 90);

	drawText("SPACEBAR",MAUVE,2,0,0,1,1.5,KX,KY - 120);
	drawText(": DROP EVERYTHING",WHITE,2,0,0,1,1.5,KX2,KY - 120);

	drawText("R CONTROL",MAUVE,2,0,0,1,1.5,KX,KY - 150);
	drawText(": USE SHIELD (IF AVAILABLE)",WHITE,2,0,0,1,1.5,KX2,KY - 150);

	drawText("ESCAPE",MAUVE,2,0,0,1,1.5,KX,KY - 180);
	drawText(": QUIT",WHITE,2,0,0,1,1.5,KX2,KY - 180);


	drawText("'S'",ORANGE,2,0,0,1,1.5,KX,KY2);
	drawText(": START",WHITE,2,0,0,1,1.5,KX3,KY2);

	drawText("'P'",ORANGE,2,0,0,1,1.5,KX,KY2 - 30);
	drawText(": PAUSE",WHITE,2,0,0,1,1.5,KX3,KY2 - 30);

	drawText("'A'",ORANGE,2,0,0,1,1.5,KX,KY2 - 60);
	drawText(": INCREASE THRUST",WHITE,2,0,0,1,1.5,KX3,KY2 - 60);

	drawText("'Z'",ORANGE,2,0,0,1,1.5,KX,KY2 - 90);
	drawText(": DECREASE THRUST",WHITE,2,0,0,1,1.5,KX3,KY2 - 90);

	drawText("'D'",ORANGE,2,0,0,1,1.5,KX,KY2 - 120);
	drawText(": DROP BOTTOM ITEM",WHITE,2,0,0,1,1.5,KX3,KY2 - 120);

	drawText("'H'",ORANGE,2,0,0,1,1.5,KX,KY2 - 150);
	drawText(": HYPERSPACE",WHITE,2,0,0,1,1.5,KX3,KY2 - 150);
#ifdef SOUND
	drawText("'V'",ORANGE,2,0,0,1,1.5,KX,KY2 - 180);
	drawText(": SOUND ON/OFF",WHITE,2,0,0,1,1.5,KX3,KY2 - 180);
#endif
}

///////////////////////////////// MAIN SCREEN /////////////////////////////////

void drawHills();
void drawObject(cl_object *obj);
void drawRadar();
void convertToRadarXY(double &x, double &y);
void drawRadarHill(int tb, double col, double x1, double y1, double x2, double y2);


/*** The main game screen ***/
void drawPlayScreen()
{
	char text[50];
	static double col = 0;
	static double add = 0.25;
	int o;

	// Pulse background colour if gravity affected
	if (reverse_g_timer)
	{
		col += add;
		if (col <= 0 || col >= DARK_MAUVE - BLACK5) add = -add;
		drawOrFillRectangle(col+BLACK5,0,SCR_WIDTH,SCR_HEIGHT,0,0,true);
	}
	else if (zero_g_timer)
	{
		col += add;
		if (col <= 0 || col >= DARK_BLUE - BLACK4) add = -add;
		drawOrFillRectangle(col+BLACK4,0,SCR_WIDTH,SCR_HEIGHT,0,0,true);
	}
	else
	{
		col = 0;
		add = 0.25;
	}

	// Background stars
	for(o=0;o < NUM_ASTROS;++o) astro[o]->draw();

	// Hills 
	drawHills();

	// Objects. Start at 1 to skip ship
	for(o=1;o < NUM_OBJECTS;++o) drawObject(object[o]);

	// Draw ship last so its always on top 
	drawObject(ship);
	
	// Fill in top of screen with black
	drawOrFillRectangle(BLACK,0,SCR_WIDTH,SCR_TOP_HEIGHT,0,SCR_TOP,true);

	// Draw radar and dividing line below 
	drawRadar();
	drawLine(ORANGE,1,0,SCR_TOP,SCR_WIDTH,SCR_TOP);

	ship->drawHealthGauge();
	ship->drawMaxThrustGauge();
	ship->drawTractorBeamEnergyGauge();
	ship->drawPowerupIcons();

	drawText("SCORE ",WHITE,2,0,0,0.8,1,10,SCR_HEIGHT - 13);
	drawText(score_text,TURQUOISE,2,0,0,1,1.5,75,SCR_HEIGHT - 15);

	drawText("HIGH  ",WHITE,2,0,0,0.8,1,10,SCR_HEIGHT - 38);
	if (score < high_score || game_stage_timer % 20 < 10)
		drawText(high_score_text,PURPLE,2,0,0,1,1.5,75,SCR_HEIGHT - 40);

	drawText("LIVES ",WHITE,2,0,0,0.8,2,SCR_WIDTH - 85,SCR_HEIGHT - 16);
	drawText(lives_text,TURQUOISE,2,0,0,1.5,4.5,SCR_WIDTH - 15,SCR_HEIGHT - 28);

	// Draw texts over the top of everything
	for(o=0;o < NUM_TEXTS;++o) if (textobj[o]->active) textobj[o]->draw();

	// Draw gravity countdowns
	if (reverse_g_timer)
	{
		sprintf(text,"REVERSE-G: %03d\n",reverse_g_timer);
		drawCenteredText(text,TURQUOISE,2,0,0,1,1,20);
	}
	else if (zero_g_timer)
	{
		sprintf(text,"ZERO-G: %03d\n",zero_g_timer);
		drawCenteredText(text,TURQUOISE,2,0,0,1,1,20);
	}

	// Flicker screen when ship blows up
	if ((game_stage == GAME_STAGE_PLAY || 
	     game_stage == GAME_STAGE_DEMO_PLAY) &&
	    ship->stage == STAGE_EXPLODE && 
	    (ship->stage_timer < 4 || 
	     (ship->stage_timer > 29 && ship->stage_timer < 35)))
	{
		drawOrFillRectangle(
			ship->stage_timer % 2 ? BLACK : WHITE,
			0,SCR_WIDTH,SCR_HEIGHT,0,0,true);
	}

	if (game_stage == GAME_STAGE_DEMO_PLAY)
		drawText(version_text,WHITE,1,0,0,0.75,1,10,10);

#ifdef SOUND
	if (!do_sound && game_stage >= GAME_STAGE_GAME_START)
	{
		drawText("S",WHITE,2,0,0,1.5,1.5,SCR_WIDTH - 15,15);
		drawText("/",RED,5,0,0,1.5,1.5,SCR_WIDTH - 15,15);
	}
#endif
}




/*** Draw top and bottom ***/
void drawHills()
{
	XPoint pnts[4]; 
	double draw_x;
	double first_draw_x;
	double prev_draw_x;
	int i;
	int tb;
	short edge;

	draw_x = 0;
	first_draw_x = 0;
	prev_draw_x = 0;

	if (all_pods_captured)
	{
		if (++apc_hill_col[HILL_TOP] == MAUVE) 
			apc_hill_col[HILL_TOP] = BLUE;

		if (++apc_hill_col[HILL_BOT] == YELLOW) 
			apc_hill_col[HILL_BOT] = RED;
	}

	for(tb=HILL_BOT,edge=0;;)
	{
		for(i=0;i < num_hills[tb];++i)
		{
			draw_x = getDrawX(hill[tb][i].x);

			if (i)
			{
				if (prev_draw_x < draw_x)
				{
					pnts[0].x = (short)prev_draw_x;
					pnts[0].y = (short)hill[tb][i-1].y;

					pnts[1].x = (short)draw_x;
					pnts[1].y = (short)hill[tb][i].y;

					pnts[2].x =(short)draw_x;
					pnts[2].y = edge; 

					pnts[3].x = (short)prev_draw_x;
					pnts[3].y = edge;

					drawOrFillPolygon(
						all_pods_captured ? apc_hill_col[tb] : hill[tb][i].col,
						0,4,pnts,true);
				}
			}
			else first_draw_x = draw_x;

			prev_draw_x = draw_x;
		}
		if (first_draw_x > draw_x) 
		{
			pnts[0].x = (short)draw_x;
			pnts[0].y = (short)hill[tb][i-1].y;

			pnts[1].x = (short)first_draw_x;
			pnts[1].y = (short)hill[tb][0].y;

			pnts[2].x =(short)first_draw_x;
			pnts[2].y = edge; 

			pnts[3].x = (short)draw_x;
			pnts[3].y = edge;

			drawOrFillPolygon(
				all_pods_captured ? apc_hill_col[tb] : hill[tb][0].col,
				0,4,pnts,true);
		}
		if (tb == HILL_TOP || level < MIN_LEVEL_TOP_HILLS) break;
		tb = HILL_TOP;
		edge = SCR_TOP;
	}
}




/*** Call the appropriate object draw function ***/
void drawObject(cl_object *obj)
{
	if (obj->stage == STAGE_INACTIVE) return;

	obj->setDrawX();

	switch(obj->stage)
	{
	case STAGE_MATERIALISE:
		obj->drawMaterialise();
		break;
		
	case STAGE_RUN:
		obj->draw();
		break;

	case STAGE_EXPLODE:
		obj->drawExplode();
		break;

	default:
		assert(0);
	}
}


/////////////////////////////////// RADAR ////////////////////////////////////

/*** Draw radar at the top of the screen ***/
void drawRadar()
{
	cl_object *obj;
	cl_moonbase *mb;
	cl_disruptor *dr;
	cl_bomb *bm;
	double x;
	double y;
	double w;
	double h;
	double first_x;
	double first_y;
	double prev_x;
	double prev_y;
	int tb;
	int i;

	x = 0;
	y = 0;
	prev_x = 0;
	prev_y = 0;
	first_x = 0;
	first_y = 0;

	// Draw radar hills
	for(tb=HILL_BOT;;)
	{
		for(i=0;i < num_hills[tb];++i)
		{
			x = hill[tb][i].x;
			y = hill[tb][i].y;
			convertToRadarXY(x,y);

			if (i) drawRadarHill(tb,hill[tb][i].col,prev_x,prev_y,x,y);
			else
			{
				first_x = x;
				first_y = y;
			}

			prev_x = x;
			prev_y = y;
		}
		drawRadarHill(tb,hill[tb][0].col,x,y,first_x,first_y);

		if (tb == HILL_TOP || level < MIN_LEVEL_TOP_HILLS) break;
		tb = HILL_TOP;
	}

	// Show object locations
	FOR_ALL_OBJECTS(i)
	{
		obj = object[i];
		if (obj->stage != STAGE_RUN &&
		    obj->stage != STAGE_EXPLODE) continue;

		x = obj->x;
		y = obj->y;
		convertToRadarXY(x,y);
		if (x < RADAR_LEFT || x > RADAR_RIGHT) continue;

		// Draw disruptor ring, moonbase arc and bomb shockwave
		switch(obj->type)
		{
		case TYPE_MOONBASE:
			if (obj->stage == STAGE_EXPLODE) continue;
			mb = (cl_moonbase *)obj;
			w = mb->arc_radius * RADAR_X_MULT * 2;
			h = mb->arc_radius * RADAR_Y_MULT;
			if (x - w >= RADAR_LEFT && x + w <= RADAR_RIGHT)
				drawOrFillHorizArc(DARK_MAUVE,2,w,h,x,y,false);
			break;

		case TYPE_DISRUPTOR:
			if (obj->stage == STAGE_EXPLODE) continue;
			dr = (cl_disruptor *)obj;
			w = dr->disrupt_radius * RADAR_X_MULT;
			h = dr->disrupt_radius * RADAR_Y_MULT;
			if (x - w >= RADAR_LEFT && x + w <= RADAR_RIGHT)
				drawOrFillCircle(dr->disrupt_colour,0,w,h,x,y,true);
			break;

		case TYPE_BOMB:
			if (obj->stage == STAGE_RUN) break;
			bm = (cl_bomb *)obj;
			if (bm->canDrawShockwave())
			{
				w = bm->shockwave_diam * RADAR_X_MULT;
				h = bm->shockwave_diam * RADAR_Y_MULT;
				if (x - w < RADAR_LEFT || x + w > RADAR_RIGHT)
					break;
				
				// If bottom of circle will be outside radar
				// then just draw an arc
				if (y - h / 2 < RADAR_BOT)
					drawOrFillHorizArc(YELLOW,1,w,h/2,x,y,false);
				else
				 	drawOrFillCircle(YELLOW,1,w,h,x,y,false);
			}
			continue;

		case TYPE_LAIR:
			// So drawn when exploding
			break;

		default:
			if (obj->stage == STAGE_EXPLODE) continue;
		}

		// Draw objects dot
		drawOrFillCircle(obj->main_colour,0,5,5,x,y,true);
	}

	// Draw side lines
	drawLine(ORANGE,2,RADAR_LEFT,RADAR_BOT,RADAR_LEFT,RADAR_TOP);
	drawLine(ORANGE,2,RADAR_RIGHT,RADAR_BOT,RADAR_RIGHT,RADAR_TOP);
		

	// Draw lines showing visible portion
	// Horizontal
	drawLine(
		TURQUOISE,2,
		RADAR_SCR_VIS_LEFT,RADAR_BOT,
		RADAR_SCR_VIS_RIGHT,RADAR_BOT);
	drawLine(
		TURQUOISE,2,
		RADAR_SCR_VIS_LEFT,RADAR_TOP,
		RADAR_SCR_VIS_RIGHT,RADAR_TOP);

	// Vertical 
	drawLine(
		TURQUOISE,2,
		RADAR_SCR_VIS_LEFT,RADAR_BOT,
		RADAR_SCR_VIS_LEFT,RADAR_BOT + 10);
	drawLine(
		TURQUOISE,2,
		RADAR_SCR_VIS_LEFT,RADAR_TOP,
		RADAR_SCR_VIS_LEFT,RADAR_TOP - 10);
	drawLine(
		TURQUOISE,2,
		RADAR_SCR_VIS_RIGHT,RADAR_BOT,
		RADAR_SCR_VIS_RIGHT,RADAR_BOT + 10);
	drawLine(
		TURQUOISE,2,
		RADAR_SCR_VIS_RIGHT,RADAR_TOP,
		RADAR_SCR_VIS_RIGHT,RADAR_TOP - 10);
}




/*** Convert landscape location to radar screen location ***/
void convertToRadarXY(double &x, double &y)
{
	x -= screen_x_shift;
	if (x > LANDSCAPE_WIDTH / 2 + SCR_X_MID) 
		x -= LANDSCAPE_WIDTH;
	else
	if (x < -(LANDSCAPE_WIDTH / 2 - SCR_X_MID)) 
		x += LANDSCAPE_WIDTH;
	x = RADAR_SCR_VIS_LEFT + x * RADAR_X_MULT;
	y = RADAR_BOT + y * RADAR_Y_MULT;
}




/** Draw the 4 point polygon that makes up the section of hill ***/
void drawRadarHill(
	int tb, double col, double x1, double y1, double x2, double y2)
{
	XPoint pnts[4]; 
	double ratio;

	// Last hill back to first hill which will be off radar anyway if
	// x2 < x1 so just return.
	if (x2 < x1) return;

	if (x1 < RADAR_LEFT)
	{
		if (x2 < RADAR_LEFT) return;

		// Clip to left
		x1 = RADAR_LEFT;
		ratio = (x2 - RADAR_LEFT) / (x2 - x1);
		y1 = y2 + (y1 - y2) * ratio;
	}
	else if (x2 > RADAR_RIGHT)
	{
		if (x1 > RADAR_RIGHT) return;

		// Clip to right
		x2 = RADAR_RIGHT;
		ratio = ((double)RADAR_RIGHT - x1) / (x2 - x1);
		y2 = y1 + (y2 - y1) * ratio;
	}
	
	// Line of hill
	pnts[0].x = (short)round(x1);
	pnts[0].y = (short)round(y1);
	pnts[1].x = (short)round(x2);
	pnts[1].y = (short)round(y2);

	// Line at top or bottom of radar
	pnts[2].x = (short)round(x2);
	pnts[3].x = (short)round(x1);
	if (tb == HILL_BOT)
	{
		pnts[2].y = RADAR_BOT;
		pnts[3].y = RADAR_BOT;
	}
	else
	{
		pnts[2].y = RADAR_TOP;
		pnts[3].y = RADAR_TOP;
	}

	drawOrFillPolygon(
		all_pods_captured ? apc_hill_col[tb] : col,0,4,pnts,true);
}


///////////////////////////////// LIGHTNING //////////////////////////////////


/*** Draw a lightning bolt from x,yi. If angle == -1 then set random angle ***/
void drawLightning(double col, double len, double ang, double x, double y)
{
	double lx;
	double ly;
	double rlx;
	double rly;
	double prev_lx;
	double prev_ly;
	double rprev_lx;
	double rprev_ly;
	double yadd;
	int cnt;
	int i;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	// Draw 2 to 4 individual segments. Create it as a vertical line 
	// with x offsets to start with then rotate
	cnt = 3 + (int)random() % 5;
	yadd = len / cnt;
	prev_lx = 0;
	prev_ly = 0;

	for(i=0;i < cnt;++i)
	{
		ly = prev_ly + yadd;

		// Create x offset unless final line in which case no offset
		// so end point of lightning is in line with start point
		if (i < cnt - 1)
		{
			lx = len / (10 + random() % 10);

			if (!i && random() % 2) lx = -lx;
			else
			if (prev_lx > 0) lx = -lx;
		}
		else lx = 0;
 
		// Rotate start and end points to required angle
		rotatePoint(prev_lx,prev_ly,ang,&rprev_lx,&rprev_ly);
		rotatePoint(lx,ly,ang,&rlx,&rly);

		drawLine(col,3,x + rprev_lx,y + rprev_ly,x + rlx,y + rly);

		prev_lx = lx;
		prev_ly = ly;
	}
}


///////////////////////////////// TEXT DRAWING ////////////////////////////////

/*** Draw some text in the centre of the screen ***/
void drawCenteredText(
	const char *mesg,
	double col,
	double thick,
	double ang,
	double gap,
	double x_scale, double y_scale, double y)
{
	double len;

	if (!gap) gap = CHAR_GAP;

	len = strlen(mesg) - 1;
	len = len * (double)CHAR_SIZE * x_scale +
	      len * gap * x_scale;

	drawText(mesg,col,thick,ang,gap,x_scale,y_scale,SCR_X_MID - len/2,y);
}




/*** Draw some text ***/
void drawText(
	const char *mesg,
	double col,
	double thick,
	double ang,
	double gap,
	double x_scale, double y_scale, double x, double y)
{
	int len = (int)strlen(mesg);
	int i;

	if (!gap) gap = CHAR_GAP;

	for(i=0;i < len;++i)
	{
		drawChar(
			mesg[i],col,thick,ang,x_scale,y_scale,
			x + (double)i * (double)CHAR_SIZE * x_scale +
			    (double)i * gap  * x_scale,
			y);
	}
}




/*** Draw a character ***/
void drawChar(
	char c,
	double col,
	double thick,
	double ang, double x_scale, double y_scale, double x, double y)
{
	st_char_template *tmpl;
	double x1, y1, x2, y2;

	if (!(tmpl = ascii_table[(int)c])) return;

	// Draw character
	for(int i=0;i < tmpl->cnt;i+=2)
	{
		rotatePoint(tmpl->data[i].x,tmpl->data[i].y,ang,&x1,&y1);
		x1 = x + x1 * x_scale;
		y1 = y + y1 * y_scale;

		rotatePoint(tmpl->data[i+1].x,tmpl->data[i+1].y,ang,&x2,&y2);
		x2 = x + x2 * x_scale;
		y2 = y + y2 * y_scale;

		drawLine(col,thick,x1,y1,x2,y2);
	}
}


///////////////////////////// BASIC DRAWING OPS //////////////////////////////

void drawLine(
	double col, double thick, double x1, double y1, double x2, double y2)
{
	if (refresh_cnt) return;

	x1 = (x1 + screen_x_shake) * g_x_scale;
	x2 = (x2 + screen_x_shake) * g_x_scale;

	// Just check X is on screen since Y probably usually is
	if ((x1 < 0 && x2 < 0) || (x1 > win_width && x2 > win_width)) return;

	y1 = (SCR_HEIGHT - y1 + screen_y_shake) * g_y_scale;
	y2 = (SCR_HEIGHT - y2 + screen_y_shake) * g_y_scale;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	setThickness(col,thick);
	XDrawLine(
		display,drw,gc[(int)round(col)],
		(int)round(x1),(int)round(y1),(int)round(x2),(int)round(y2));
}




/*** Draw or fill a circle ***/
void drawOrFillCircle(
	double col,
	double thick,
	double x_diam, double y_diam, double x, double y, bool fill)
{
	double x_radius;
	double y_radius;
	double xp;
	double yp;

	if (refresh_cnt) return;

	x_radius = x_diam / 2;
	y_radius = y_diam / 2;

	// Don't draw circle if its too small or outside the window. 
	if (x_diam <= 0 || y_diam <= 0 ||
	    x < -x_radius || x >= (double)SCR_WIDTH + x_radius) return;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	x_diam *= g_x_scale;
	y_diam *= g_y_scale;

	// X seems not to draw arcs with a diam < 2
	if (x_diam < 2) x_diam = 2;
	if (y_diam < 2) y_diam = 2;

	xp = (x - x_radius + screen_x_shake) * g_x_scale;
	yp = ((double)SCR_HEIGHT - y - y_radius + screen_y_shake) * g_y_scale;

	if (fill)
	{
		XFillArc(
			display,drw,gc[(int)round(col)],
			(int)round(xp),(int)round(yp),
			(int)round(x_diam),(int)round(y_diam),
			0,CIRCLE);
	}
	else
	{
		setThickness(col,thick);
		XDrawArc(
			display,drw,gc[(int)round(col)],
			(int)round(xp),(int)round(yp),
			(int)round(x_diam),(int)round(y_diam),
			0,CIRCLE);
	}
}




/*** Draw or fill a polygon. The scale argument is used for efficiency in
     cl_polygon::draw() and only makes a difference if window is resized ***/
void drawOrFillPolygon(
	double col, double thick, int num_points, XPoint *points, bool fill)
{
	int cnt;
	int i;

	if (refresh_cnt) return;

	for(i=cnt=0;i < num_points;++i)
	{
		points[i].x = (short)round(((double)(points[i].x + screen_x_shake) * g_x_scale));
		points[i].y = (short)round(((double)SCR_HEIGHT - points[i].y + screen_y_shake) * g_y_scale);
		if (points[i].x < 0) --cnt;
		else
		if (points[i].x > win_width) ++cnt;
	}

	// If all points either to left or right of the screen don't bother
	// drawing the polygon
	if (abs(cnt) == num_points) return;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	if (fill)
	{
		XFillPolygon(
			display,drw,gc[(int)round(col)],
			points,num_points,Nonconvex,CoordModeOrigin);
	}
	else
	{
		setThickness(col,thick);
		XDrawLines(
			display,drw,gc[(int)round(col)],
			points,num_points,CoordModeOrigin);

		// Draw from last point back to start. Not done automatically
		// by X.
		XDrawLine(
			display,drw,gc[(int)round(col)],
			points[num_points-1].x,
			points[num_points-1].y,
			points[0].x,points[0].y);
	}
}




/*** Draw or fill a rectangle. This draws up so x,y is bottom left. ***/
void drawOrFillRectangle(
	double col,
	double thick, double w, double h, double x, double y, bool fill)
{
	if (refresh_cnt) return;

	x = (x + screen_x_shake) * g_x_scale;
	y = ((double)SCR_HEIGHT - (y + h) + screen_y_shake) * g_y_scale;
	w *= g_x_scale;
	h *= g_y_scale;

	if (w < 1) w = 1;
	if (h < 1) h = 1;
	if (col < 0 || col >= NUM_COLOURS) col = 0;

	if (fill)
	{
		XFillRectangle(
			display,drw,gc[(int)round(col)],
			(int)round(x),(int)round(y),
			(int)round(w),(int)round(h));
	}
	else
	{
		setThickness(col,thick);
		XDrawRectangle(
			display,drw,gc[(int)round(col)],
			(int)round(x),(int)round(y),
			(int)round(w),(int)round(h));	
	}
}




/*** Draw a horizontal arc over or under. Because of the odd way XDrawArc() 
     works this isn't as simple as it should be. x is the horizontal middle of 
     the arc ***/
void drawOrFillHorizArc(
	double col,
	double thick, double w, double h, double x, double y, bool fill)
{
	double ax;
	double ay;
	double h2;

	if (refresh_cnt) return;

	w = fabs(w);
	h2 = fabs(h);

	// Don't draw if we're offscreen
	if (y < -h2 || y > SCR_HEIGHT + h2 ||
	    x < -w || x > SCR_WIDTH + w) return;

	if (col < 0 || col >= NUM_COLOURS) col = 0;

	setThickness(col,thick);

	// Move x,y from centre to top left
	ax = (x - w / 2 + screen_x_shake) * g_x_scale;
	ay = ((double)SCR_HEIGHT - y - h2 + screen_y_shake) * g_y_scale;

	// * 2 because the height to XDrawArc is the full height of the
	// circle regardless of how much of it is being drawn
	h = h * 2 * g_y_scale;
	w *= g_x_scale;

	// XDrawArc can't cope with negative heights. Have to use the angle
	// arguments instead and shift the y position
	if (h > 0)
	{
		// 3 -> 9 o'clock over
		if (fill)
		{
			XFillArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(w),(int)round(h),
				0,SEMI);
		}
		else
		{
			XDrawArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(w),(int)round(h),
				0,SEMI);
		}
	}
	else
	{
		// 9 -> 3 o'clock under
		if (fill)
		{
			XFillArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(w),(int)round(-h),
				SEMI,SEMI);
		}
		else
		{
			XDrawArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(w),(int)round(-h),
				SEMI,SEMI);
		}
	}
}




/*** Draw a left or right side arc. y is the vertical middle ***/
void drawOrFillVertArc(
	double col,
	double thick, double w, double h, double x, double y, bool fill)
{
	double ax;
	double ay;
	double w2;

	if (refresh_cnt) return;

	h = fabs(h);
	w2 = fabs(w);

	// Don't draw if we're offscreen
	if (y < -h || y > SCR_HEIGHT + h ||
	    x < -w2 || x > SCR_WIDTH + w2) return;

	if (col < 0 || col >= NUM_COLOURS) col = 0;
	setThickness(col,thick);

	ax = (x - w2 + screen_x_shake) * g_x_scale;
	ay = ((double)SCR_HEIGHT - y - h / 2 + screen_y_shake) * g_y_scale;
	h *= g_y_scale;
	w = w * 2 * g_x_scale;

	if (w > 0)
	{
		// 12 -> 6 o'clock right
		if (fill)
		{
			XFillArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(w),(int)round(h),
				SEMI+QUART,SEMI);
		}
		else
		{
			XDrawArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(w),(int)round(h),
				SEMI+QUART,SEMI);
		}
	}
	else
	{
		// 6 -> 12 o'clock left
		if (fill)
		{
			XFillArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(-w),(int)round(h),
				QUART,SEMI);
		}
		else
		{
			XDrawArc(
				display,drw,gc[(int)round(col)],
				(int)round(ax),(int)round(ay),
				(int)round(-w),(int)round(h),
				QUART,SEMI);
		}
	}
}
