/*****************************************************************************
 SENTINEL

 A 1980s style Scramble/Defender cross arcade game for X Windows.

 Copyright (C) Neil Robertson 2012-2016
 *****************************************************************************/

#define MAINFILE
#include "globals.h"

#define MAINLOOP_DELAY 25000

// Module globals
char *disp;
XdbeSwapInfo swapinfo;
XColor g_colour[NUM_COLOURS];
int screen;
bool use_db;

int pod_cnt_at_start;
int lair_cnt_at_start;
int skyth_appear_every;
int disruptor_appear_every;

int tractorbeam_powerup_at;
int shield_powerup_at;
int health_powerup_at;
int laser_powerup_at;
int reverse_g_powerup_at;
int hyperspace_powerup_at;

// Forward declarations
void parseCmdLine(int argc, char **argv);
void Xinit();
void init();
void setGameStage(en_game_stage stage);
void initLevel();
void setScaling();

void mainloop();
u_int getTime();
void processXEvents();
void runGameStage();
void runPlayStage();
void duringLevel();


////////////////////////////////// START ///////////////////////////////////

int main(int argc, char **argv)
{
	srandom((u_int)time(0));

	parseCmdLine(argc,argv);
#ifdef SOUND
	startSoundDaemon();
	if (do_soundtest)
	{
		// Just wait until child process exits
		int status = 0;
		wait(&status);
		exit(0);
	}
#endif
	Xinit();
	init();
	mainloop();
	return 0;
}




/*** Get the command line arguments ***/
void parseCmdLine(int argc, char **argv)
{
	const char *opt[] =
	{
		"size",
		"disp",
		"ref",
		"nodb",
#ifdef SOUND
		"nosnd",
		"nofrag",
		"sndtest",
#ifdef ALSA
		"adev",
#endif
#endif
		"ver",
	};
	enum
	{
		OPT_SIZE,
		OPT_DISP,
		OPT_REF,
		OPT_NODB,
#ifdef SOUND
		OPT_NOSND,
		OPT_NOFRAG,
		OPT_SNDTEST,
#ifdef ALSA
		OPT_ADEV,
#endif
#endif
		OPT_VER,

		OPT_END
	};
	int i,o;
	float mult;

	disp = NULL;
	mult = 1;
	use_db = true;
	win_refresh = 1;
#ifdef SOUND
	do_sound = true;
	do_soundtest = false;
	do_fragment = true;
	alsa_device = (char *)ALSA_DEVICE;
#endif

	for(i=1;i < argc;++i)
	{
		if (argv[i][0] != '-') goto USAGE;
		for(o=0;o != OPT_END;++o)
			if (!strcasecmp(opt[o],argv[i]+1)) break;

		switch(o)
		{
		case OPT_VER:
			puts("-=[ SENTINEL ]=-\n");
			puts("Copyright (C) Neil Robertson 2012-2016\n");
			printf("Version     : %s\nBuild date  : %s\nSound system: ",
				VERSION,BUILD_DATE);
#ifdef SOUND
#ifdef ALSA
			puts("ALSA");
#else
			puts("OpenSound");
#endif
#else
			puts("<no sound>");
#endif
                        exit(0);

		case OPT_NODB:
			use_db = false;
			continue;
#ifdef SOUND
		case OPT_NOSND:
			do_sound = false;
			continue;

		case OPT_NOFRAG:
			do_fragment = false;
			continue;

		case OPT_SNDTEST:
			do_soundtest = true;
			continue;
#endif
		}

		if (i++ == argc - 1) goto USAGE;

		switch(o)
		{
		case OPT_SIZE:
			if ((mult = atof(argv[i])) < 0) goto USAGE;
			break;

		case OPT_DISP:
			disp = argv[i];
			break;

		case OPT_REF:
			if ((win_refresh = atoi(argv[i])) < 1) goto USAGE;
			break;

#ifdef ALSA
		case OPT_ADEV:
			alsa_device = argv[i];
			break;
#endif

		default:
			goto USAGE;
		}
	}

	win_width = (int)(mult * SCR_WIDTH);
	win_height = (int)(mult * SCR_HEIGHT);
	setScaling();
	return;

	USAGE:
        printf("Usage: %s\n"
	       "       -disp <display>       : Set X display\n"
	       "       -size <win size mult> : Set window size compared to default. Eg: 2\n"
	       "                               means twice default size, 0.5 means half.\n"
	       "       -ref  <iterations>    : The number of mainloop iterations before the\n"
	       "                               window is redrawn. Default = 1\n"
#ifdef ALSA
	       "       -adev <ALSA device>   : Set the ALSA device to use. Default = '%s'\n"
#endif
	       "       -nodb                 : Don't use double buffering. For really old\n"
	       "                               systems.\n"
#ifdef SOUND
	       "       -nosnd                : Switch off sound.\n"
	       "       -nofrag               : If background sounds stutter try this though\n"
	       "                               some short sounds might not work properly.\n"
	       "       -sndtest              : Play all the sound effects then exit.\n"
#endif
	       "       -ver                  : Print version info then exit\n",
		argv[0]
#ifdef ALSA
		,ALSA_DEVICE
#endif
		);
	exit(1);
}


/////////////////////////////////// SETUP ///////////////////////////////////

/*** Set up X ***/
void Xinit()
{
	XGCValues gcvals;
	XColor unused;
	XTextProperty title_prop;
	Colormap cmap;
	u_int white;
	u_int black;
	int stage;
	int cnt;
	u_char r,g,b;
	char colstr[5];
	char title[50];
	char *titleptr;
	char *sound_system;

	if (!(display = XOpenDisplay(disp)))
	{
		printf("ERROR: Can't connect to: %s\n",XDisplayName(disp));
		exit(1);
	}
	screen = DefaultScreen(display);
	black = (u_int)BlackPixel(display,screen);
	white = (u_int)WhitePixel(display,screen);
	cmap = DefaultColormap(display,screen);

	win = XCreateSimpleWindow(
		display,
		RootWindow(display,screen),
		0,0,win_width,win_height,0,white,black);

	XSetWindowBackground(display,win,black);
#ifdef SOUND
#ifdef ALSA
	sound_system = (char *)"ALSA";
#else
	sound_system = (char *)"OpenSound";
#endif
#else
	sound_system = (char *)"<no sound>";
#endif
	sprintf(title,"SENTINEL v%s (%s)",VERSION,sound_system);
	titleptr = title;  // Function will crash if you pass address of array
	XStringListToTextProperty(&titleptr,1,&title_prop);
	XSetWMProperties(display,win,&title_prop,NULL,NULL,0,NULL,NULL,NULL);

	// Create colour GCs. 
	r = 0;
	g = 0xF;
	b = 0;
	stage = 1;
	
	for(cnt=0;cnt < NUM_COLOURS;++cnt) 
	{
		sprintf(colstr,"#%01X%01X%01X",r,g,b);
		if (!XAllocNamedColor(display,cmap,colstr,&g_colour[cnt],&unused)) 
		{
			printf("WARNING: Can't allocate colour %s\n",colstr);
			gcvals.foreground = white;
		}
		else gcvals.foreground = g_colour[cnt].pixel;

		gcvals.background = black;
		gc[cnt] = XCreateGC(
			display,win,GCForeground | GCBackground,&gcvals);

		switch(stage) 
		{
		case 1:
			// Green to turquoise
			if (++b == 0xF) ++stage;
			break;
	
		case 2:
			// Turquoise to blue
			if (!--g) ++stage;
			break;
	
		case 3:
			// Blue to mauve
			if (++r == 0xF) ++stage;
			break;
	
		case 4:
			// Mauve to red
			if (!--b) ++stage;
			break;
	
		case 5:
			// Red to yellow
			if (++g == 0xF) ++stage;
			break;
	
		case 6:
			// Yellow to green
			if (!--r) 
			{
				g = b = 0;
				++stage;
			} 
			break;

		case 7:
			// black to white
			++g;
			++b;
			if (r++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 8:
			// black to red
			if (r++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 9:
			// black to green
			if (g++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 10:
			// black to blue
			if (b++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 11:
			// black to mauve
			r++;
			if (b++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 12:
			// black to turquoise
			b++;
			if (g++ == 0xF)
			{
				r = g = b = 0;
				++stage;
			}
			break;

		case 13:
			// black to yellow
			r++;
			g++;
			break;
		}
	}

	if (use_db)
	{
		drw = (Drawable)XdbeAllocateBackBufferName(
			display,win,XdbeBackground);
		swapinfo.swap_window = win;
		swapinfo.swap_action = XdbeBackground;
	}
	else drw = win;

	XSelectInput(display,win,
		ExposureMask | 
		StructureNotifyMask |
		KeyPressMask | 
		KeyReleaseMask);

	XMapWindow(display,win);

	setScaling();
}




/*** Set up everything not directly X windows ***/
void init()
{
	int i;
	int j;

	sprintf(version_text,"V%s",VERSION);

	// Set up ascii tables. Taken from peniten-6
	for(i=0;i < 256;++i) ascii_table[i] = NULL;

	ascii_table[(int)'A'] = (st_char_template *)&char_A;
	ascii_table[(int)'B'] = (st_char_template *)&char_B;
	ascii_table[(int)'C'] = (st_char_template *)&char_C;
	ascii_table[(int)'D'] = (st_char_template *)&char_D;
	ascii_table[(int)'E'] = (st_char_template *)&char_E;
	ascii_table[(int)'F'] = (st_char_template *)&char_F;
	ascii_table[(int)'G'] = (st_char_template *)&char_G;
	ascii_table[(int)'H'] = (st_char_template *)&char_H;
	ascii_table[(int)'I'] = (st_char_template *)&char_I;
	ascii_table[(int)'J'] = (st_char_template *)&char_J;
	ascii_table[(int)'K'] = (st_char_template *)&char_K;
	ascii_table[(int)'L'] = (st_char_template *)&char_L;
	ascii_table[(int)'M'] = (st_char_template *)&char_M;
	ascii_table[(int)'N'] = (st_char_template *)&char_N;
	ascii_table[(int)'O'] = (st_char_template *)&char_O;
	ascii_table[(int)'P'] = (st_char_template *)&char_P;
	ascii_table[(int)'Q'] = (st_char_template *)&char_Q;
	ascii_table[(int)'R'] = (st_char_template *)&char_R;
	ascii_table[(int)'S'] = (st_char_template *)&char_S;
	ascii_table[(int)'T'] = (st_char_template *)&char_T;
	ascii_table[(int)'U'] = (st_char_template *)&char_U;
	ascii_table[(int)'V'] = (st_char_template *)&char_V;
	ascii_table[(int)'W'] = (st_char_template *)&char_W;
	ascii_table[(int)'X'] = (st_char_template *)&char_X;
	ascii_table[(int)'Y'] = (st_char_template *)&char_Y;
	ascii_table[(int)'Z'] = (st_char_template *)&char_Z;

	ascii_table[(int)'a'] = (st_char_template *)&char_A;
	ascii_table[(int)'b'] = (st_char_template *)&char_B;
	ascii_table[(int)'c'] = (st_char_template *)&char_C;
	ascii_table[(int)'d'] = (st_char_template *)&char_D;
	ascii_table[(int)'e'] = (st_char_template *)&char_E;
	ascii_table[(int)'f'] = (st_char_template *)&char_F;
	ascii_table[(int)'g'] = (st_char_template *)&char_G;
	ascii_table[(int)'h'] = (st_char_template *)&char_H;
	ascii_table[(int)'i'] = (st_char_template *)&char_I;
	ascii_table[(int)'j'] = (st_char_template *)&char_J;
	ascii_table[(int)'k'] = (st_char_template *)&char_K;
	ascii_table[(int)'l'] = (st_char_template *)&char_L;
	ascii_table[(int)'m'] = (st_char_template *)&char_M;
	ascii_table[(int)'n'] = (st_char_template *)&char_N;
	ascii_table[(int)'o'] = (st_char_template *)&char_O;
	ascii_table[(int)'p'] = (st_char_template *)&char_P;
	ascii_table[(int)'q'] = (st_char_template *)&char_Q;
	ascii_table[(int)'r'] = (st_char_template *)&char_R;
	ascii_table[(int)'s'] = (st_char_template *)&char_S;
	ascii_table[(int)'t'] = (st_char_template *)&char_T;
	ascii_table[(int)'u'] = (st_char_template *)&char_U;
	ascii_table[(int)'v'] = (st_char_template *)&char_V;
	ascii_table[(int)'w'] = (st_char_template *)&char_W;
	ascii_table[(int)'x'] = (st_char_template *)&char_X;
	ascii_table[(int)'y'] = (st_char_template *)&char_Y;
	ascii_table[(int)'z'] = (st_char_template *)&char_Z;

	ascii_table[(int)'0'] = (st_char_template *)&char_0;
	ascii_table[(int)'1'] = (st_char_template *)&char_1;
	ascii_table[(int)'2'] = (st_char_template *)&char_2;
	ascii_table[(int)'3'] = (st_char_template *)&char_3;
	ascii_table[(int)'4'] = (st_char_template *)&char_4;
	ascii_table[(int)'5'] = (st_char_template *)&char_5;
	ascii_table[(int)'6'] = (st_char_template *)&char_6;
	ascii_table[(int)'7'] = (st_char_template *)&char_7;
	ascii_table[(int)'8'] = (st_char_template *)&char_8;
	ascii_table[(int)'9'] = (st_char_template *)&char_9;

	ascii_table[(int)' '] = (st_char_template *)&char_space;
	ascii_table[(int)'?'] = (st_char_template *)&char_qmark;
	ascii_table[(int)'!'] = (st_char_template *)&char_exmark;
	ascii_table[(int)'+'] = (st_char_template *)&char_plus;
	ascii_table[(int)'-'] = (st_char_template *)&char_minus;
	ascii_table[(int)'*'] = (st_char_template *)&char_star;
	ascii_table[(int)'='] = (st_char_template *)&char_equals;
	ascii_table[(int)'.'] = (st_char_template *)&char_dot;
	ascii_table[(int)','] = (st_char_template *)&char_comma;
	ascii_table[(int)'('] = (st_char_template *)&char_lrbracket;
	ascii_table[(int)')'] = (st_char_template *)&char_rrbracket;
	ascii_table[(int)'{'] = (st_char_template *)&char_lcbracket;
	ascii_table[(int)'}'] = (st_char_template *)&char_rcbracket;
	ascii_table[(int)'['] = (st_char_template *)&char_lsbracket;
	ascii_table[(int)']'] = (st_char_template *)&char_rsbracket;
	ascii_table[(int)'$'] = (st_char_template *)&char_dollar;
	ascii_table[(int)'#'] = (st_char_template *)&char_hash;
	ascii_table[(int)'/'] = (st_char_template *)&char_fslash;
	ascii_table[(int)'\\'] = (st_char_template *)&char_bslash;
	ascii_table[(int)'>'] = (st_char_template *)&char_greater;
	ascii_table[(int)'<'] = (st_char_template *)&char_less;
	ascii_table[(int)'_'] = (st_char_template *)&char_underscore;
	ascii_table[(int)'|'] = (st_char_template *)&char_bar;
	ascii_table[(int)'\''] = (st_char_template *)&char_squote;
	ascii_table[(int)'"'] = (st_char_template *)&char_dquote;
	ascii_table[(int)'`'] = (st_char_template *)&char_bquote;
	ascii_table[(int)':'] = (st_char_template *)&char_colon;
	ascii_table[(int)';'] = (st_char_template *)&char_semicolon;
	ascii_table[(int)'@'] = (st_char_template *)&char_at;
	ascii_table[(int)'^'] = (st_char_template *)&char_hat;
	ascii_table[(int)'~'] = (st_char_template *)&char_tilda;
	ascii_table[(int)'&'] = (st_char_template *)&char_ampersand;
	ascii_table[(int)'%'] = (st_char_template *)&char_percent;

	// Adjust data values to have origin in centre, not top left and flip
	// Y axis the other way up. Can't be arsed to adjust all data manually.
	// too lazy to adjust all that data itself.
	for(i=32;i < 256;++i)
	{
		// A & a , B & b etc share the same struct so don't do this
		// operation twice on them
		if (ascii_table[i] && (i < 'a' || i > 'z'))
		{
			for(j=0;j < ascii_table[i]->cnt;++j)
			{
				ascii_table[i]->data[j].y = CHAR_SIZE - ascii_table[i]->data[j].y;
				ascii_table[i]->data[j].x -= CHAR_HALF;
				ascii_table[i]->data[j].y -= CHAR_HALF;
			}
		}
	}

	///// Create all the game objects /////
	bzero(object,sizeof(object));

	/* Stalag laser and strikers go before ship so their haveCollided() 
	   method gets called instead of the standard one when checking 
	   against ship */
	for(i=0,j=0;i < MAX_STALAG_LASER;++i,++j)
		object[j] = new cl_stalag_laser;
	for(i=0;i < MAX_STRIKER;++i,++j) object[j] = new cl_striker;

	object[j++] = ship = new cl_ship;
	object[j++] = moonbase = new cl_moonbase;

	// Lasers here so haveCollided() gets called instead of enemies
	for(i=0;i < MAX_SHIP_LASER;++i,++j) object[j] = new cl_ship_laser;
	for(i=0;i < MAX_BOMB;++i,++j) object[j] = new cl_bomb;
	pod_start = j;
	for(i=0;i < MAX_POD;++i,++j) object[j] = new cl_pod;
	for(i=0;i < MAX_LAIR;++i,++j) object[j] = new cl_lair;
	for(i=0;i < MAX_MISSILE;++i,++j) object[j] = new cl_missile;
	for(i=0;i < MAX_SILO;++i,++j) object[j] = new cl_silo;
	for(i=0;i < MAX_BULLET;++i,++j) object[j] = new cl_bullet;
	for(i=0;i < MAX_PLASMA;++i,++j) object[j] = new cl_plasma;
	for(i=0;i < MAX_GRYPPER;++i,++j) object[j] = new cl_grypper;
	for(i=0;i < MAX_ZOMBIE;++i,++j) object[j] = new cl_zombie;
	for(i=0;i < MAX_SKYTH;++i,++j) object[j] = new cl_skyth;
	for(i=0;i < MAX_STALAG;++i,++j) object[j] = new cl_stalag;
	for(i=0;i < MAX_DISRUPTOR;++i,++j) object[j] = new cl_disruptor;

	// Powerups
	object[j++] = new cl_shield_powerup;
	object[j++] = new cl_laser_powerup;
	object[j++] = new cl_health_powerup;
	object[j++] = new cl_tractorbeam_powerup;
	object[j++] = new cl_reverse_g_powerup;
	object[j++] = new cl_hyperspace_powerup;

	assert(j == NUM_OBJECTS);

	// Demo objects for show cast screen
	j = 0;
	show_object[j++] = new cl_ship;
	show_object[j++] = new cl_pod;
	show_object[j++] = new cl_bomb;
	show_object[j++] = new cl_moonbase;
	show_object[j++] = new cl_lair;
	show_object[j++] = new cl_grypper;
	show_object[j++] = new cl_zombie;
	show_object[j++] = new cl_skyth;
	show_object[j++] = new cl_disruptor;
	show_object[j++] = new cl_silo;
	show_object[j++] = new cl_missile;
	show_object[j++] = new cl_stalag;
	show_object[j++] = new cl_striker;
	assert(j == NUM_SHOW_OBJECTS);
	cast_angle = 0;

	// Background astronomics never interact with other objects so put 
	// them in their own list so we don't waste time scanning through them.
	for(i=0;i < NUM_ASTROS;++i) astro[i] = new cl_astro;

	// 1 instance of every text object except bonus score
	for(i=0;i < TEXT_BONUS_SCORE;++i) 
		textobj[i] = new cl_text((en_text_type)i);

	for(;i < NUM_TEXTS;++i) textobj[i] = new cl_text(TEXT_BONUS_SCORE);

	setHighScore(20000);
	setGameStage(GAME_STAGE_DEMO_PLAY);

	// Doesn't need to be set here, just to be clean
	start_gravity = GRAVITY;
}


///////////////////////////// GETTING STARTED /////////////////////////////////

/*** Set and carry out appropriate actions for game stage ***/
void setGameStage(en_game_stage stage)
{
	int o;
	int bonus;

	game_stage = stage;
	game_stage_timer = 0;
	screen_x_shake = 0;
	screen_y_shake = 0;
	paused = false;

	// Reset in case player just died
	if (reverse_g_timer) reverse_g_timer = 0;
	if (zero_g_timer) zero_g_timer = 0;
	gravity = start_gravity; 

	switch(stage)
	{
	case GAME_STAGE_DEMO_PLAY:
		level = 1 + (int)random() % 10;
		setGameStage(GAME_STAGE_GAME_START);
		game_stage = GAME_STAGE_DEMO_PLAY;
		textobj[TEXT_SENTINEL]->activate();
		textobj[TEXT_COPYRIGHT]->activate();
		textobj[TEXT_PRESS_S]->activate();
		break;

	case GAME_STAGE_SHOW_CAST:
		// Reset all demo objects
		for(o=0;o < NUM_SHOW_OBJECTS;++o)
			show_object[o]->activate(NULL);
		break;

	case GAME_STAGE_SHOW_KEYS:
		break;

	case GAME_STAGE_GAME_START:
		// Player pressed 'S'
		level_stage_timer = 0;
		reverse_g_timer = 0;
		zero_g_timer = 0;
		bonus_life_score = BONUS_LIFE_INC;
		activated_high_score_text = false;

		setScore(0);
		setLives(3);
		ship->resetPowerups();

		// New locations for astros
		for(o=0;o < NUM_ASTROS;++o) astro[o]->setLocation();
		// Fall through

	case GAME_STAGE_LEVEL_INIT:
		// Level started for first time
		level_stage_timer = 0;
		initLevel();
		game_stage = GAME_STAGE_LEVEL_START;
		textobj[TEXT_LEVEL_INST]->activate();
		// Fall through

	case GAME_STAGE_LEVEL_START:
		// Level (re)started. Reset objects
		FOR_ALL_OBJECTS(o) object[o]->reset();
		textobj[TEXT_LEVEL_START]->activate();
		break;

	case GAME_STAGE_PLAY:
		break;

	case GAME_STAGE_LEVEL_COMPLETE:
		textobj[TEXT_LEVEL_COMPLETE]->activate();
		echoOn();
		playSound(SND_LEVEL_COMPLETE);

		if (pods_saved == pod_cnt_at_start)
		{
			bonus = 1500 + level * 500;
			textobj[TEXT_ALL_PODS_SAVED]->activateAllPodsSaved(bonus);
			incScore(bonus);
		}
		break;

	case GAME_STAGE_PLAYER_DIED:
		setLives(lives-1);
		if (!lives)
		{
			game_stage = GAME_STAGE_GAME_OVER;
			textobj[TEXT_GAME_OVER]->activate();
			playSound(SND_GAME_OVER);
		}
		break;

	case GAME_STAGE_GAME_OVER:
		// Should never come here. Fall through

	default:
		assert(0);
	}
}




/*** Called only at the start of a new level, NOT when level needs resetting
     after player lost life ***/
void initLevel()
{
	int o;

	all_pods_captured = false;
	
	// Set hills
	apc_hill_col[HILL_TOP] = BLUE;
	apc_hill_col[HILL_BOT] = RED;
	createHills();

	// Inactivate then reactivate
	FOR_ALL_OBJECTS(o) object[o]->setStage(STAGE_INACTIVE);
	moonbase->activate(NULL);
	ship->activate(NULL);

	skyth_appear_at = 0;
	skyth_appear_every = 0;
	disruptor_appear_every = 0;
	cannon_destroyed = 0;

	// 2 second variability
	shield_powerup_at = 1000 + (int)random() % 80;
	health_powerup_at = 1400 + (int)random() % 80;
	tractorbeam_powerup_at = 1600 + (int)random() % 80;
	laser_powerup_at = 1800 + (int)random() % 80;
	reverse_g_powerup_at = 2200 + (int)random() % 80;
	hyperspace_powerup_at = 2600 + (int)random() % 80;

	// Start increasing gravity after level 5
	if (level > 5)
		gravity = GRAVITY * (0.5 + (0.1 * level));
	else
		gravity = GRAVITY;

	start_gravity = gravity;

	switch(level)
	{
	case 1:
		lair_cnt_at_start = 1;
		activateObjects(TYPE_BOMB,2,NULL);
		skyth_appear_at = 4800;  // 2 minutes
		skyth_appear_every = 400;
		break;

	case 2:
		lair_cnt_at_start = 2;
		activateObjects(TYPE_SILO,1,NULL);
		activateObjects(TYPE_BOMB,3,NULL);
		skyth_appear_at = 4800;
		skyth_appear_every = 400;
		break;

	case 3:
		lair_cnt_at_start = 2;
		activateObjects(TYPE_SILO,1,NULL);
		activateObjects(TYPE_BOMB,4,NULL);
		skyth_appear_at = 3600;  // 90 seconds
		skyth_appear_every = 400;
		break;

	case 4:
		lair_cnt_at_start = 2;
		activateObjects(TYPE_SILO,2,NULL);
		activateObjects(TYPE_BOMB,4,NULL);
		activateObjects(TYPE_DISRUPTOR,1,NULL);
		skyth_appear_at = 3600;
		skyth_appear_every = 300;
		disruptor_appear_every = 2000;
		break;

	case 5:
		lair_cnt_at_start = 3;
		activateObjects(TYPE_SILO,2,NULL);
		activateObjects(TYPE_STALAG,1,NULL);
		activateObjects(TYPE_BOMB,5,NULL);
		activateObjects(TYPE_DISRUPTOR,2,NULL);
		skyth_appear_at = 2400;
		skyth_appear_every = 300;
		disruptor_appear_every = 1600;
		break;

	case 6:
		lair_cnt_at_start = 3;
		activateObjects(TYPE_SILO,3,NULL);
		activateObjects(TYPE_STALAG,2,NULL);
		activateObjects(TYPE_STRIKER,1,NULL);
		activateObjects(TYPE_BOMB,6,NULL);
		activateObjects(TYPE_DISRUPTOR,2,NULL);
		skyth_appear_at = 2400;
		skyth_appear_every = 300;
		disruptor_appear_every = 1200;
		break;

	case 7:
		lair_cnt_at_start = 3;
		activateObjects(TYPE_SILO,3,NULL);
		activateObjects(TYPE_STALAG,3,NULL);
		activateObjects(TYPE_STRIKER,1,NULL);
		activateObjects(TYPE_BOMB,7,NULL);
		activateObjects(TYPE_DISRUPTOR,2,NULL);
		skyth_appear_at = 2400;
		skyth_appear_every = 200;
		disruptor_appear_every = 1200;
		break;

	// Level 8+
	default:
		lair_cnt_at_start = 4;
		activateObjects(TYPE_SILO,level-4,NULL);
		activateObjects(TYPE_STALAG,level-4,NULL);
		activateObjects(TYPE_STRIKER,level-6,NULL);
		activateObjects(TYPE_BOMB,level,NULL);
		activateObjects(TYPE_DISRUPTOR,2,NULL);
		skyth_appear_at = 2400;
		skyth_appear_every = 200;
		disruptor_appear_every = 800;
	}

	activateObjects(TYPE_LAIR,lair_cnt_at_start,NULL);
	pod_cnt_at_start = 9 + level;
	activateObjects(TYPE_POD,pod_cnt_at_start,NULL);
}




/*** Set the scaling factors according to window size ***/
void setScaling()
{
	g_x_scale = (double)win_width / SCR_WIDTH;
	g_y_scale = (double)win_height / SCR_HEIGHT;
	avg_scale = (g_x_scale + g_y_scale) / 2;
}



/////////////////////////////////// RUNTIME ///////////////////////////////////

/*** Get the events and draw the points.

     Main game code path:

     mainloop()
        runGameStage()
           runPlayStage()
              duringLevel()

 ***/
void mainloop()
{
	u_int tm1;
	u_int tm2;
	u_int diff;

	for(refresh_cnt=0;;refresh_cnt = (refresh_cnt + 1) % win_refresh)
	{
		tm1 = getTime();

		if (!use_db) XClearWindow(display,win);

		processXEvents();
		runGameStage();

		if (!refresh_cnt)
		{
			if (use_db) XdbeSwapBuffers(display,&swapinfo,1);
			XFlush(display);
		}

		// Timing check
		if ((tm2 = getTime()) > tm1)
		{
			diff = tm2 - tm1;
			if (diff < MAINLOOP_DELAY)
				usleep(MAINLOOP_DELAY - diff);
		}
		else usleep(MAINLOOP_DELAY);
	}
}




/*** Get the current time down to the microsecond. Value wraps once every
     1000 seconds ***/
u_int getTime()
{
	timeval tv;
	gettimeofday(&tv,0);
	return (u_int)(tv.tv_sec % 1000) * 1000000 + (u_int)tv.tv_usec;
}




/*** See whats on the X queue ***/
void processXEvents()
{
	XWindowAttributes wa;
	XEvent event;
	KeySym ksym;
	char key;

	while(XPending(display))
	{
		XNextEvent(display,&event);

		switch(event.type)
		{
		case Expose:
			// Some window managers can shrink a window when it
			// first appears and we get this in the expose event
			XGetWindowAttributes(display,win,&wa);
			win_width = wa.width;
			win_height = wa.height;
			setScaling();
			break;

		case ConfigureNotify:
			win_width = event.xconfigure.width;
			win_height = event.xconfigure.height;
			setScaling();
			break;

		case UnmapNotify:
			if (game_stage == GAME_STAGE_PLAY)
			{
				// Auto pause if window unmapped
				paused = true;
				textobj[TEXT_PAUSED]->activate();
			}
			break;

		case KeyRelease:
			if (game_stage != GAME_STAGE_PLAY) break;
			XLookupString(&event.xkey,&key,1,&ksym,NULL);

			switch(ksym)
			{
			case XK_a:
			case XK_A:
			case XK_z:
			case XK_Z:
				ship->fixMaxThrust();
				break;

			case XK_Up:
				ship->thrustOff();
				break;

			case XK_Left:
			case XK_Right:
				ship->rotateStop();
				break;

			case XK_Shift_L:
			case XK_Shift_R:
				ship->shootStop();
			}
			break;

		case KeyPress:
			XLookupString(&event.xkey,&key,1,&ksym,NULL);

			switch(ksym)
			{
			case XK_s:
			case XK_S:
				if (game_stage < GAME_STAGE_GAME_START)
				{
					level = 1;
					setGameStage(GAME_STAGE_GAME_START);
					echoOn();
					playSound(SND_START);
				}
				break;

			case XK_plus:
				// Undocumented key to skip through levels
				if (game_stage < GAME_STAGE_GAME_START)	
					level = 1;
				else
					++level;
				setGameStage(GAME_STAGE_GAME_START);
				break;

			case XK_Escape:
				if (game_stage <= GAME_STAGE_GAME_START)
					exit(0);
				setGameStage(GAME_STAGE_DEMO_PLAY);
			}
			if (game_stage != GAME_STAGE_PLAY) continue;

			switch(ksym)
			{
			case XK_p:
			case XK_P:
				if (paused) paused = false;
				else
				{
					textobj[TEXT_PAUSED]->activate();
					paused = true;
				}
				break;

			case XK_a:
			case XK_A:
				ship->incMaxThrust();
				break;

			case XK_z:
			case XK_Z:
				ship->decMaxThrust();
				break;

			case XK_d:
			case XK_D:
				ship->drop();
				break;

			case XK_space:
				ship->dropAll();
				break;

			case XK_Tab:
			case XK_Alt_L:
			case XK_Alt_R:
				ship->grab();
				break;
#ifdef SOUND
			case XK_v:
			case XK_V:
				do_sound = !do_sound;
				break;
#endif
			case XK_h:
			case XK_H:
				ship->hyperspace();
				break;

			case XK_Up:
				ship->thrustOn();
				break;

			case XK_Left:
				ship->rotateAnti();
				break;

			case XK_Right:
				ship->rotateClock();
				break;

			case XK_Shift_L:
			case XK_Shift_R:
				ship->shootStart();
				break;

			case XK_Control_R:
				ship->useShield();
				break;
			}
			break;

		default:
			break;
		}
	}
}




/*** Do something depending on the stage ***/
void runGameStage()
{
	++game_stage_timer;

	// Attract screens
	switch(game_stage)
	{
	case GAME_STAGE_SHOW_CAST:
		drawCastScreen();
		if (game_stage_timer == 720) 
			setGameStage(GAME_STAGE_SHOW_KEYS);
		return;

	case GAME_STAGE_SHOW_KEYS:
		drawKeysScreen();
		if (game_stage_timer == 200) 
			setGameStage(GAME_STAGE_DEMO_PLAY);
		return;

	default:
		break;
	}

	drawPlayScreen();

	switch(game_stage)
	{
	case GAME_STAGE_DEMO_PLAY:
		if (game_stage_timer == 800)
			setGameStage(GAME_STAGE_SHOW_CAST);
		else
		{
			ship->autoplay();
			runPlayStage();
		}
		break;

	case GAME_STAGE_GAME_START:
	case GAME_STAGE_LEVEL_INIT:
		// Should never get here
		assert(0);

	case GAME_STAGE_LEVEL_START:
		if (game_stage_timer == 100) setGameStage(GAME_STAGE_PLAY);
		break;

	case GAME_STAGE_PLAY:
		if (game_stage_timer == 40) echoOff();
		if (!paused) runPlayStage();
		break;

	case GAME_STAGE_LEVEL_COMPLETE:
		if (game_stage_timer == 150) 
		{
			echoOff();
			++level;
			setGameStage(GAME_STAGE_LEVEL_INIT);
		}
		break;

	case GAME_STAGE_PLAYER_DIED:
		if (game_stage_timer == 20)
			setGameStage(GAME_STAGE_LEVEL_START);
		break;

	case GAME_STAGE_GAME_OVER:
		if (game_stage_timer == 200)
			setGameStage(GAME_STAGE_DEMO_PLAY);
		break;

	default:
		assert(0);
	}
}




/*** Run all the objects and check for collisions ***/
void runPlayStage()
{
	cl_object *obj1;
	cl_object *obj2;
	int o1;
	int o2;
	int pods_left = 0;
	int prisoners_left = 0;
	int zombies_alive = 0;
	int lairs_alive = 0;

	duringLevel();

	// Run the objects
	FOR_ALL_OBJECTS(o1) 
	{
		obj1 = object[o1];
		++obj1->stage_timer;

		switch(obj1->stage)
		{
		case STAGE_INACTIVE:
			break;

		case STAGE_MATERIALISE:
			obj1->runMaterialise();
			break;

		case STAGE_RUN:
		case STAGE_EXPLODE:
			switch(obj1->type)
			{
			case TYPE_LAIR:
				++lairs_alive;
				prisoners_left += ((cl_lair *)(obj1))->num_prisoners;
				break;

			case TYPE_ZOMBIE:
				++zombies_alive;
				break;

			case TYPE_POD:
				++pods_left;
				break;

			default:
				break;
			}
			if (obj1->stage == STAGE_RUN)
				obj1->run();
			else
				obj1->runExplode();
			break;

		default:
			assert(0);
		}
	}

	// If no pods left, all zombies are destroyed and all prisoners are 
	// gone then check for level complete
	if (!pods_left && !zombies_alive && !prisoners_left)
	{
		switch(level)
		{
		case 1:
		case 2:
			setGameStage(GAME_STAGE_LEVEL_COMPLETE);
			return;

		case 3:
			// cannon_destroyed incremented by cl_lair
			if (cannon_destroyed >= 2)
			{
				setGameStage(GAME_STAGE_LEVEL_COMPLETE);
				return;
			}
			break;

		case 4:
			if (cannon_destroyed >= 4)
			{
				setGameStage(GAME_STAGE_LEVEL_COMPLETE);
				return;
			}
			break;

		case 5:
			if (lair_cnt_at_start - lairs_alive >= 2)
			{
				setGameStage(GAME_STAGE_LEVEL_COMPLETE);
				return;
			}
			break;

		default:
			if (!lairs_alive)
			{
				setGameStage(GAME_STAGE_LEVEL_COMPLETE);
				return;
			}
		}
	}
	if (ship->stage == STAGE_INACTIVE)
	{
		if (game_stage == GAME_STAGE_DEMO_PLAY)
			setGameStage(GAME_STAGE_SHOW_CAST);
		else
			setGameStage(GAME_STAGE_PLAYER_DIED);
		return;
	}

	// If no pods left (and none have been saved) then go into bezerk mode
	if (!all_pods_captured && !pods_left && !pods_saved)
	{
		all_pods_captured = true;

		// Make skyths appear in half the time and twice as often
		skyth_appear_at /= 2;
		skyth_appear_every /= 2;

		textobj[TEXT_ALL_PODS_CAPTURED]->activate();
		playSound(SND_ALL_PODS_CAPTURED);
	}

	// If all lairs gone then make skyths appear from now if they haven't
	// already started
	if (!lairs_alive && skyth_appear_at > level_stage_timer)
		skyth_appear_at = level_stage_timer + 1;

	// Check for collisions
	for(o1=0;o1 < NUM_OBJECTS-1;++o1)
	{
		obj1 = object[o1];
		if (obj1->stage == STAGE_RUN)
		{
			for(o2=o1+1;o2 < NUM_OBJECTS;++o2)
			{
				obj2 = object[o2];

				if (obj1 != obj2 &&
				    obj2->stage == STAGE_RUN &&
				    obj1->haveCollided(obj2))
				{
					obj1->collidedWith(obj2);
					obj2->collidedWith(obj1);
				}
			}
		}
	}
}




/*** Timed creation of objects and other activities done during a level rather 
     than at the start ***/
void duringLevel()
{
	++level_stage_timer;

	// Set by powerup
	if (reverse_g_timer && !--reverse_g_timer) gravity = start_gravity;
	else
	// Set when disruptor destroyed
	if (zero_g_timer && !--zero_g_timer) gravity = start_gravity;

	// Skyth appearances
	if (level_stage_timer == skyth_appear_at ||
	    (level_stage_timer > skyth_appear_at &&
             !(level_stage_timer % skyth_appear_every)))
		activateObjects(TYPE_SKYTH,1,NULL);

	// Disruptor appearances
	if (disruptor_appear_every && 
	    !(level_stage_timer % disruptor_appear_every))
		activateObjects(TYPE_DISRUPTOR,level < 7 ? 1 : 2,NULL);

	// Use game_stage_timer for powerups so that player gets another shot 
	// at them if ship destroyed 
	if (game_stage_timer == tractorbeam_powerup_at)
		activateObjects(TYPE_TRACTORBEAM_POWERUP,1,NULL);

	if (level >= 3 && 
	    game_stage_timer == shield_powerup_at && !ship->shield_powerup)
		activateObjects(TYPE_SHIELD_POWERUP,1,NULL);

	if (level >= 4)
	{
		if (game_stage_timer == health_powerup_at)
			activateObjects(TYPE_HEALTH_POWERUP,1,NULL);
		else
		if (game_stage_timer == laser_powerup_at && !ship->dual_laser)
			activateObjects(TYPE_LASER_POWERUP,1,NULL);
	}

	if (level >= 5)
	{
		if (game_stage_timer == reverse_g_powerup_at)
			activateObjects(TYPE_REVERSE_G_POWERUP,1,NULL);
		else
		if (game_stage_timer == hyperspace_powerup_at)
			activateObjects(TYPE_HYPERSPACE_POWERUP,1,NULL);
	}
}
