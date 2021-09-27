#include "globals.h"

#define MIN_HILLS        (LANDSCAPE_WIDTH / 120)


/*** Create top and bottom hills ***/
void createHills()
{
	double sep;
	double x;
	double hx;
	double height;
	int tb;
	int i;
	int hill_bot_col;
	int hill_top_col = BLACK;

	// Vary hill colours depending on level. This is just for the sake
	// of variety, it doesn't affect gameplay.
	switch(level)
	{
	case 1:
	case 2:
		hill_bot_col = MEDIUM_GREEN;
		break;

	case 3:
		hill_bot_col = CLOUD_BLUE;
		break;

	case 4:
		hill_bot_col = CLOUD_BLUE;
		hill_top_col = SKY_BLUE;
		break;

	case 5:
	case 6:
		hill_bot_col = MEDIUM_YELLOW;
		hill_top_col = SKY_BLUE;
		break;

	case 7:
	case 8:
		hill_bot_col = MEDIUM_BLUE;
		hill_top_col = MEDIUM_MAUVE;
		break;

	case 9:
	case 10:
		hill_bot_col = MEDIUM_MAUVE;
		hill_top_col = MEDIUM_YELLOW;
		break;

	default:
		hill_bot_col = MEDIUM_RED;
		hill_top_col = MEDIUM_RED;
	}
	max_hill_height = level < 20 ? 100 + level * 5 : 200;

	for(tb=HILL_BOT;;)
	{
		num_hills[tb] = MIN_HILLS + (int)random() % (MAX_HILLS - MIN_HILLS);
		sep = (double)LANDSCAPE_WIDTH / (num_hills[tb] + 1);

		for(i=0,x=0,hx=0;i < num_hills[tb];++i)
		{
			hill[tb][i].x = hx;
			height = MIN_HILL_HEIGHT + ((int)random() % (max_hill_height - MIN_HILL_HEIGHT));

			if (tb == HILL_BOT)
			{
				hill[tb][i].y = height;
				hill[tb][i].col = hill_bot_col + ((int)random() % 7) - 3;
			}
			else
			{
				hill[tb][i].y = SCR_TOP - height * 0.75;
				hill[tb][i].col = hill_top_col + ((int)random() % 7) - 3;
			}

			// Set stepping and ang from previous hill vertex to us
			if (i) 
			{
				// Step is y height gain per x
				hill[tb][i-1].step = (hill[tb][i].y-hill[tb][i-1].y) / 
				                     (hill[tb][i].x-hill[tb][i-1].x);
	
				hill[tb][i-1].angle = getAngle(
					hill[tb][i-1].x,hill[tb][i-1].y,
					hill[tb][i].x,hill[tb][i].y
					);
			}
			x += sep;

			/* Adjust next X location by -20% to +20% to give 
			   actual X. (random() % 200) - 100 gives -100 -> 100 
			   so divide by 500 to get -0.2 -> +0.2 */
			hx = x + sep * ((double)(random() % 200) - 100) / 500;
		}

		// Set stepping and angle from last hill vertex to first tb will
		// always be at X = 0
		--i;
		hill[tb][i].step = (hill[tb][0].y - hill[tb][i].y) / 
		               ((double)LANDSCAPE_WIDTH - hill[tb][i].x);

		hill[tb][i].angle = getAngle(
			hill[tb][i].x,hill[tb][i].y,
			LANDSCAPE_WIDTH,hill[tb][0].y
			);

		if (tb == HILL_TOP || level < MIN_LEVEL_TOP_HILLS) break;
		tb = HILL_TOP;
	}
}




/*** Return a structure giving the hill and its height at the given x location.
     The hill is the one to the left of the X co-ordinate ***/
st_hill_height *getHillHeight(int tb, double x)
{
	static st_hill_height hh;
	int i;

	assert(x >= 0 && x < LANDSCAPE_WIDTH);

	if (tb == HILL_TOP) assert(level >= MIN_LEVEL_TOP_HILLS);

	for(i=1;i < num_hills[tb];++i)
	{
		hh.hl = i-1;
		if (x >= hill[tb][hh.hl].x && x < hill[tb][i].x) break;
	}
	if (i == num_hills[tb]) hh.hl = num_hills[tb] - 1;
	
	hh.y = hill[tb][hh.hl].y + 
	       ((x - hill[tb][hh.hl].x) * hill[tb][hh.hl].step);

	return &hh;
}




/*** Returns the hill we're inside at x,y else NULL ***/
st_hill_inside *insideHill(double radius, double x, double y)
{
	static st_hill_inside hi;
	st_hill_height *hh;
	int tb;

	for(tb=HILL_BOT;;)
	{
		hh = getHillHeight(tb,x);

		// Get hill height/depth at our current x location
		if (tb == HILL_BOT && y - radius <= hh->y)
		{
			hi.tb = HILL_BOT;
			hi.hl = hh->hl;
			hi.y = hh->y;
			return &hi;
		}
		else if (tb == HILL_TOP && y + radius >= hh->y)
		{
			hi.tb = HILL_TOP;
			hi.hl = hh->hl;
			hi.y = hh->y;
			return &hi;
		}
		if (tb == HILL_TOP || level < MIN_LEVEL_TOP_HILLS) break;
		tb = HILL_TOP;
	}
	return NULL;
}
