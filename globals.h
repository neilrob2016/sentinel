#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>

#include "build_date.h"

#define VERSION "1.1.3"

#if defined(ALSA) && !defined(SOUND)
#error "The ALSA build option can only be used with SOUND"
#endif

#ifdef MAINFILE
#define EXTERN
#else
#define EXTERN extern
#endif

// Sound
#define ALSA_DEVICE "sysdefault"

// Screen
#define SCR_WIDTH        800
#define SCR_HEIGHT       600
#define SCR_TOP_HEIGHT   60
#define SCR_TOP         (SCR_HEIGHT - SCR_TOP_HEIGHT)
#define SCR_X_MID       (SCR_WIDTH / 2)
#define SCR_Y_MID       (SCR_HEIGHT / 2)

// Landscape
#define LANDSCAPE_MULT   10  // Change RADAR_MULT is this is changed
#define LANDSCAPE_WIDTH  (SCR_WIDTH * LANDSCAPE_MULT)

// Finely tuned. Alter them much and game becomes unplayable.
#define GRAVITY             0.15
#define FRICTION            0.995

// Object values
#define TRACTOR_BEAM_LEN 70
#define SHIP_MAX_CARGO   3
#define SHIP_DIAM        40
#define POD_DIAM         40

// Hills
#define MAX_HILLS           (LANDSCAPE_WIDTH / 100)
#define MIN_LEVEL_TOP_HILLS 4
#define MIN_HILL_HEIGHT     10
#define HILL_TOP            0
#define HILL_BOT            1

// Object counts
#define MAX_BOMB         12
#define MAX_POD          25
#define MAX_SHIP_LASER   20
#define MAX_STALAG_LASER 20
#define MAX_LAIR         4
#define MAX_SILO         5
#define MAX_MISSILE      5
#define MAX_BULLET       10
#define MAX_PLASMA       10
#define MAX_GRYPPER      30
#define MAX_ZOMBIE       MAX_POD
#define MAX_SKYTH        5
#define MAX_STALAG       8
#define MAX_DISRUPTOR    6
#define MAX_STRIKER      3
#define MAX_POWERUPS     6

// +2 for ship & moonbase
#define NUM_OBJECTS (\
	2 + \
	MAX_BOMB + \
	MAX_POD + \
	MAX_SHIP_LASER + \
	MAX_STALAG_LASER + \
	MAX_LAIR + \
	MAX_SILO + \
	MAX_MISSILE + \
	MAX_BULLET + \
	MAX_PLASMA + \
	MAX_GRYPPER + \
	MAX_ZOMBIE + \
	MAX_SKYTH + \
	MAX_STALAG + \
	MAX_DISRUPTOR + \
	MAX_STRIKER + \
	MAX_POWERUPS)

#define NUM_SHOW_OBJECTS 13

// Number of astronomical objects (stars/planets). Never changes
#define NUM_ASTROS 200

// Vertex counts
#define SHIP_CENTRE_VTX 4
#define SHIP_TRI_VTX    3
#define SHIP_FLAME_VTX  3

#define MOONBASE_MAIN_VTX 4
#define MOONBASE_WIN_VTX  4
#define MOONBASE_LEG_VTX  3

#define LAIR_MAIN_VTX    8
#define LAIR_LEG_VTX     3
#define LAIR_CANNON_VTX  4

#define SILO_VTX 4

#define MISSILE_MID_VTX   5
#define MISSILE_FIN_VTX   3
#define MISSILE_FLAME_VTX 3

#define GRYPPER_BODY_VTX 8
#define GRYPPER_TOP_VTX  3
#define GRYPPER_MID_VTX  4

#define STALAG_BREECH_VTX 6
#define STALAG_BARREL_VTX 4
#define STALAG_SQUARE_VTX 4

#define ZOMBIE_VTX  3
#define SKYTH_VTX   3
#define STRIKER_VTX 10

#define DIS_TRI_VTX 3
#define DIS_SQ_VTX  4

#define MAX_VERTEXES 10

// Maths
#define DEGS_PER_RADIAN  57.29578
#define SIN(A)           sin((A) / DEGS_PER_RADIAN)
#define COS(A)           cos((A) / DEGS_PER_RADIAN)
#define SGN(X)           ((X) < 0 ? -1 : ((X) > 0 ? 1 : 0))

// Misc
#define FOR_ALL_OBJECTS(O) for(O=0;O < NUM_OBJECTS;++O)
#define IN_HEARING_DISTANCE() (distToObject(ship) < SCR_WIDTH)

#define BONUS_LIFE_INC 15000

/////////////////////////////////// ENUMS ////////////////////////////////////

enum en_colour
{
	// Colour mixes
	GREEN       = 0,
	TURQUOISE   = 15,
	LIGHT_BLUE  = 20,
	SKY_BLUE    = 25,
	BLUE        = 30,
	PURPLE      = 40,
	MAUVE       = 45,
	RED         = 60,
	ORANGE      = 68,
	YELLOW      = 75,
	GREEN2      = 89,
	BLACK       = 90,
	DARK_GREY   = 92,
	GREY        = 97,
	LIGHT_GREY  = 100,
	WHITE       = 105,

	// RGB brightness
	BLACK2       = 106,
	DARK_RED     = 111,
	MEDIUM_RED   = 115,
	RED2         = 121,
	BLACK3       = 122,
	DARK_GREEN   = 126,
	MEDIUM_GREEN = 130,
	GREEN3       = 137,
	BLACK4       = 138,
	DARK_BLUE    = 142,
	MEDIUM_BLUE  = 147,
	BLUE2        = 153,

	// Mixed brightness
	BLACK5        = 154,
	DARK_MAUVE    = 158,
	MEDIUM_MAUVE  = 162,
	MAUVE2        = 169,
	BLACK6        = 170,
	STEEL_BLUE    = 174,
	CLOUD_BLUE    = 178,
	TURQUOISE2    = 185,
	BLACK7        = 186,
	KHAKI         = 190,
	MEDIUM_YELLOW = 194,
	YELLOW2       = 201,

	NUM_COLOURS
};

#define NUM_FULL_COLOURS BLACK2

#ifdef MAINFILE
en_colour hypcols[3] = { ORANGE,RED,PURPLE };
#else
extern en_colour hypcols[3];
#endif

enum en_game_stage
{
	GAME_STAGE_DEMO_PLAY,
	GAME_STAGE_SHOW_CAST,
	GAME_STAGE_SHOW_KEYS,
	GAME_STAGE_GAME_START,
	GAME_STAGE_LEVEL_INIT,
	GAME_STAGE_LEVEL_START,
	GAME_STAGE_PLAY,
	GAME_STAGE_LEVEL_COMPLETE,
	GAME_STAGE_PLAYER_DIED,
	GAME_STAGE_GAME_OVER
};


enum en_object_stage
{
	STAGE_INACTIVE,
	STAGE_MATERIALISE,
	STAGE_RUN,
	STAGE_EXPLODE
};


enum en_object_type
{
	TYPE_SHIP,
	TYPE_BOMB,
	TYPE_POD,
	TYPE_MOONBASE,
	TYPE_LAIR,
	TYPE_SILO,
	TYPE_SHIP_LASER,
	TYPE_STALAG_LASER,
	TYPE_MISSILE,
	TYPE_BULLET,
	TYPE_PLASMA,
	TYPE_GRYPPER,
	TYPE_ZOMBIE,
	TYPE_SKYTH,
	TYPE_STALAG,
	TYPE_DISRUPTOR,
	TYPE_STRIKER,

	TYPE_SHIELD_POWERUP,
	TYPE_LASER_POWERUP,
	TYPE_HEALTH_POWERUP,
	TYPE_HYPERSPACE_POWERUP,
	TYPE_TRACTORBEAM_POWERUP,
	TYPE_REVERSE_G_POWERUP
};


#define case_POWERUPS \
	case TYPE_SHIELD_POWERUP: \
	case TYPE_LASER_POWERUP: \
	case TYPE_HEALTH_POWERUP: \
	case TYPE_HYPERSPACE_POWERUP: \
	case TYPE_TRACTORBEAM_POWERUP: \
	case TYPE_REVERSE_G_POWERUP
	
enum en_text_type
{
	TEXT_SENTINEL,
	TEXT_COPYRIGHT,
	TEXT_PRESS_S,
	TEXT_PAUSED,
	TEXT_LEVEL_START,
	TEXT_GAME_OVER,
	TEXT_LEVEL_COMPLETE,
	TEXT_LEVEL_INST,
	TEXT_HIGH_SCORE,
	TEXT_ALL_PODS_CAPTURED,
	TEXT_POWERUP,
	TEXT_ALL_PODS_SAVED,
	TEXT_BONUS_LIFE,

	// MUST go at end
	TEXT_BONUS_SCORE
};

// 5 is the max number of on screen bonus texts at one time
#define NUM_TEXTS   (TEXT_BONUS_SCORE + 5)

// Sounds in order of priority. Lowest -> highest.
enum en_sound
{
        SND_SILENCE,

	// Non continuous
	SND_BOUNCE,
        SND_SHIP_HIT,

	// Continuous
	SND_MISSILE_THRUST,
	SND_SHIP_THRUST,
	SND_SHIP_SHIELD,
	SND_STRIKER,
	SND_DISRUPTOR,

	// Non continuous
	SND_SHIP_GRABBING,
	SND_STALAG_LASER,
	SND_BULLET,
	SND_SHIP_LASER,
	SND_SHIP_DROP,
	SND_SHIP_GRABBED,
	SND_POWERUP_DISAPPEAR,
	SND_ENEMY_MATERIALISE,
	SND_GRYPPER_EXPLODE,
	SND_ZOMBIE_EXPLODE,
	SND_SKYTH_EXPLODE,
	SND_STALAG_EXPLODE,
	SND_SILO_OR_MISSILE_EXPLODE,
	SND_STRIKER_EXPLODE,
	SND_DISRUPTOR_EXPLODE,
	SND_POD_EXPLODE,
	SND_GRYPPER_LAUNCH,
	SND_ZOMBIE_LAUNCH,
	SND_POD_SAVED,
	SND_GRYPPER_GRAB,
	SND_POD_CAPTURED,
	SND_PLASMA,
	SND_CANNON_EXPLODE,
        SND_BONUS_SCORE,
	SND_SMALL_BOMB_EXPLODE,
	SND_MEDIUM_BOMB_EXPLODE,
	SND_LARGE_BOMB_EXPLODE,
	SND_MISSILE_LAUNCH,
	SND_LAIR_EXPLODE,
	SND_POWERUP_ACTIVATE,
	SND_SHIP_DEMATERIALISE,
	SND_SHIP_MATERIALISE,
	SND_ALL_PODS_CAPTURED,
        SND_HIGH_SCORE,
	SND_BONUS_LIFE,
	SND_POWERUP_APPEAR,
        SND_SHIP_EXPLODE,
        SND_LEVEL_COMPLETE,
        SND_GAME_OVER,
        SND_START,

        NUM_SOUNDS
};


//////////////////////////// STRUCTS & CLASSES ///////////////////////////////

struct st_vertex
{
	double x;
	double y;
};



struct st_hill_vertex
{
	double x;
	double y;
	double step;
	double angle;
	int col;
};




struct st_hill_inside
{
	int tb;
	int hl;
	double y; // Set by insideHill()
};




struct st_hill_height 
{
	int hl;
	double y;
};




// Base class for all in game objects
class cl_object
{
public:
	struct st_explode
	{
		double x;
		double y;
		double diam;
		double xspd;
		double yspd;
	} *explode_bits;
	int explode_bits_cnt;
	double explode_colour;
	double explode_bits_add;
	double fireball_diam;

	en_object_type type;
	en_object_stage stage;

	double main_colour;
	double diam;
	double radius;
	double x;
	double y;
	double prev_x;
	double prev_y;
	double draw_x;
	double xspd;
	double yspd;
	double angle;
	double x_scale;
	double y_scale;
	int mass;
	int stage_timer;
	int flash_timer;  // For when object needs to flash briefly
	int health;       // Dead at zero
	int damage;       // How many points of damage we cause
	int hl;           // Hill - used by stalag and striker
	bool do_apc_check;

	cl_object(en_object_type ty, double dm);

	// Called when inactive object needs to start up
	virtual void activate(cl_object *activator) { }

	// Called on active objects when level reset after player lost life
	virtual void reset() { }

	void setStage(en_object_stage st);
	void setRandomGroundLocation(double min_dist, double y_offset);
	void setExplodeBits();

	virtual void run() = 0;
	virtual void runMaterialise() { }
	virtual void runExplode() { }
	void runExplodeBits();

	virtual void draw() = 0;
	virtual void drawMaterialise() { }
	virtual void drawExplode() { }

	void calcForces(cl_object *obj, double &force_x, double &force_y);
	void addForceAbove(double force_x, double force_y);
	void addForceBelow(double force_x, double force_y);

	virtual bool haveCollided(cl_object *obj);
	
	void updatePrevious();
	void updateXY(double &xp, double &yp);
	void limitToMaxSpeed(double max_speed);
	double bounceOffHill(st_hill_inside *hi, double inv_fric);
	void bounceOffObject(cl_object *obj);
	void headToObject(cl_object *obj);
	double distToObject(cl_object *obj);

	virtual void collidedWith(cl_object *obj) { }
	bool subDamage(cl_object *obj);

	void setDrawX();
	void drawExplodeBits();
	void drawMan(double col, bool arms_up);

	void objDrawLine(
		double col,
		double thick, double x1, double y1, double x2, double y2);
	void objDrawOrFillCircle(
		double col,
		double thick, double diam, double x1, double y1, bool fill);
	void objDrawOrFillPolygon(
		double col, int num_vtx, st_vertex *vert, bool fill);
	void objDrawOrFillRectangle(
		double col, 
		double thick, 
		double w, double h, double x1, double y1, bool fill);
	void objDrawOrFillHorizArc(
		double col,
		double thick,
		double w, double h, double x1, double y1, bool fill);
	void objDrawOrFillVertArc(
		double col,
		double thick,
		double w, double h, double x1, double y, bool fill);
};




/*** Pod, bomb and zombie base class ***/
class cl_sphere: public cl_object
{
public:
	cl_object *grabber;
	cl_object *last_grabber;
	double angle_add;
	bool released;

	cl_sphere(en_object_type ty, double dm);
	void activate(cl_object *activator);
	void reset();

	bool grab(cl_object *g);
	void release();

	void run();
	void spin();
};




class cl_pod: public cl_sphere
{
public:
	int wave_arms_timer;
	int beam_col;

	cl_pod();

	void activate(cl_object *activator);
	void collidedWith(cl_object *obj);
	bool hitTop(cl_object *obj);

	void run();
	void runExplode();
	
	void draw();
	void drawExplode();
};




class cl_bomb: public cl_sphere
{
public:
	double blast_radius;
	bool released_fixed;
	int strobe_colour;
	int explode_timer;
	int inc_at;
	int shockwave_diam;
	char text[2];

	cl_bomb();
	void activate(cl_object *activator);
	void reset();
	void setMass(int m);
	void collidedWith(cl_object *obj);

	void run();
	void runExplode();
	void draw();
	void drawExplode();
	bool canDrawShockwave();
};




// Most complex class in game because it runs half of it
class cl_ship: public cl_object
{
public:
	enum en_autoplay_stage
	{
		AUTO_START,
		AUTO_LAUNCH,
		AUTO_GOTO_POD,
		AUTO_GRAB_POD,
		AUTO_GOTO_MOONBASE,
		AUTO_WAIT
	} autoplay_stage;

	static st_vertex centre_vtx[SHIP_CENTRE_VTX];
	static st_vertex left_vtx[SHIP_TRI_VTX];
	static st_vertex right_vtx[SHIP_TRI_VTX];
	static st_vertex top_vtx[SHIP_TRI_VTX];
	static st_vertex flame_vtx[SHIP_FLAME_VTX];
	cl_sphere *cargo[SHIP_MAX_CARGO];
	cl_pod *autoplay_pod;
	bool thrust_on;
	bool shooting;
	bool dual_laser;
	bool shield_powerup;
	double thrust;
	double max_thrust;
	double max_thrust_inc;
	double angle_add;
	double level_angle_add;
	double max_speed;
	double beam_col_start;
	double gun_colour;
	double hyper_x;
	double hyper_diam;
	double hyper_col;
	double autoplay_angle;
	int hyperspace_cnt;
	int start_hyperspace_cnt;
	int cargo_cnt;
	int grab_timer;
	int drop_timer;
	int shoot_timer;
	int laser_shoot_every;
	int max_thrust_gauge_timer;
	int tractor_beam_energy;
	int tractor_beam_blink;
	int shield_timer;
	int autoplay_timer;
	int flame_timer;
	int spin_timer;
	int blink_timer;

	cl_ship();
	void activate(cl_object *activator);
	void reset();
	void resetPowerups();

	void thrustOn();
	void thrustOff();
	void incMaxThrust();
	void decMaxThrust();
	void fixMaxThrust();
	void grab();
	void drop();
	void dropAll();
	void rotateAnti();
	void rotateClock();
	void rotateStop();
	void shootStart();
	void shootStop();
	void hyperspace();
	void useShield();

	void incHealth(int hl);

	void autoplay();
	void setAutoplayStage(en_autoplay_stage astg);
	void autoplayChoosePod();
	void autoplayFly();

	void collidedWith(cl_object *obj);
	void damageCollision(cl_object *obj);
	void explode();

	void run();
	void updateThrust();
	void updateTimers();
	void updateCargo();
	void updateBeamEnergy();

	void grabNearestSphere();
	void dropCargo(int pos, bool play_sound);
	void dropBottomSphere();
	void scrollLandscape();

	void runMaterialise();
	void runExplode();

	void draw();
	void drawTractorBeam(cl_object *from, cl_object *to);

	void drawMaterialise();
	void drawExplode();

	void drawBody(
		double top_col,
		double left_col, double cent_col, double right_col);

	void drawHealthGauge();
	void drawMaxThrustGauge();
	void drawTractorBeamEnergyGauge();
	void drawPowerupIcons();
};


///////////// BASES ///////////////

// Rescue base
class cl_moonbase: public cl_object
{
public:
	static st_vertex main_vtx[MOONBASE_MAIN_VTX];
	static st_vertex leg1_vtx[MOONBASE_LEG_VTX];
	static st_vertex leg2_vtx[MOONBASE_LEG_VTX];
	int win_colour;
	int leg_colour;
	int grab_dist;
	int arc_radius;
	int rescue_flash;

	cl_moonbase();
	void activate(cl_object *activator);
	void reset();
	void collidedWith(cl_object *obj);
	void run();
	void draw();
};




// Enemy base
class cl_lair: public cl_object
{
public:
	static st_vertex main_vtx[LAIR_MAIN_VTX];
	static st_vertex leg1_vtx[LAIR_LEG_VTX];
	static st_vertex leg2_vtx[LAIR_LEG_VTX];
	static st_vertex breech_vtx[LAIR_CANNON_VTX];
	static st_vertex barrel_vtx[LAIR_CANNON_VTX];
	enum en_cannon
	{
		LEFT_CANNON,
		RIGHT_CANNON
	} cannon_fired;
	struct
	{
		double angle;
		double breech_colour;
		double barrel_colour;
		double fireball_diam;
		int shoot_flash;
		int hit_flash;
		int health;
		int explode_timer; // For when cannon is disabled
	} cannon[2];
	int leg_colour;
	int num_prisoners;
	int wave_arms_timer;
	int grypper_launch_every;
	int grypper_light_timer;
	int zombie_launch_every;
	int zombie_light_timer;
	int cannon_shoot_every;
	double cannon_move_speed;
	double scale_inc;
	double circ_colour;

	cl_lair();
	void activate(cl_object *activator);
	void reset();
	void collidedWith(cl_object *obj);
	void checkCannonHit(cl_object *obj);
	int allCannonsDestroyed();

	void run();
	void moveAndFireCannon(
		en_cannon lr, double ship_ang, double min_ang, double max_an);
	void runCannon(en_cannon lr, double bottom_ang);
	void updateCannonFireballs();
	void runExplode();
	void shudder();

	void draw();
	void drawLaunchLight(int light_col);
	void drawExplode();
	void drawBody();
};


//////////// ENEMIES //////////////

// Subsidiary enemy base that launches missiles
class cl_silo: public cl_object
{
public:
	static st_vertex left_vtx[SILO_VTX];
	static st_vertex mid_vtx[SILO_VTX];
	static st_vertex right_vtx[SILO_VTX];
	int colour[3];
	int launch_every;

	cl_silo();
	void activate(cl_object *activator);
	void reset();
	void setColours(bool strobe);
	void collidedWith(cl_object *obj);

	void run();
	void runExplode();

	void draw();
	void drawExplode();
};




class cl_grypper: public cl_object
{
public:
	enum en_action
	{
		ACT_RISE,
		ACT_GOTO_POD,
		ACT_GRAB_POD,
		ACT_RETURN_TO_LAIR,
		ACT_DELIVER_POD,
		ACT_CRUISE
	} action;

	static st_vertex body_vtx[GRYPPER_BODY_VTX];
	static st_vertex top_vtx[GRYPPER_TOP_VTX];
	double x_speed;
	double y_speed;
	cl_lair *lair;
	cl_pod *pod;
	int cruise_height;
	int strobe_colour;
	int y_angle;
	int shoot_distance;
	int shoot_every;
	int wait_timer;

	cl_grypper();
	void activate(cl_object *activator);
	void reset();

	void collidedWith(cl_object *obj);
	void addScore();

	void run();
	void actRise();
	void actGotoPod();
	void actGrabPod();
	void actReturnToLair();
	void actDeliverPod();
	void actCruise();

	void pickRandomPod();
	void moveHorizontal();
	void movePod();
	void dropPod();

	void runExplode();

	void draw();
	void drawExplode();
};




class cl_skyth: public cl_object
{
public:
	static st_vertex left_vtx[SKYTH_VTX];
	static st_vertex right_vtx[SKYTH_VTX];
	double xdist;
	int mat_dist;
	int flash_timer_top;
	int flash_timer_bot;
	int shoot_every;

	cl_skyth();
	void activate(cl_object *activator);
	void reset();
	void setSpeed();

	void collidedWith(cl_object *obj);

	void run();
	void runMaterialise();
	void runExplode();

	void draw();
	void drawMaterialise();
	void drawExplode();
};




class cl_zombie: public cl_sphere
{
public:
	static st_vertex blade_vtx[ZOMBIE_VTX];
	
	cl_zombie();
	void activate(cl_object *activator);
	void collidedWith(cl_object *obj);

	void run();
	void runExplode();
	void draw();
	void drawExplode();
};




class cl_stalag: public cl_object
{
public:
	static st_vertex breech_vtx[STALAG_BREECH_VTX];
	static st_vertex barrel_vtx[STALAG_BARREL_VTX];
	static st_vertex square_vtx[4][STALAG_SQUARE_VTX];
	double angle_add;
	double fireball_colour;
	double col_add;
	int shoot_every;
	int strobe_colour;

	cl_stalag();
	void activate(cl_object *activator);
	void reset();

	void collidedWith(cl_object *obj);
	void run();
	void runExplode();
	void draw();
	void drawExplode();
};




class cl_disruptor: public cl_object
{
public:
	static st_vertex tri_vtx[4][DIS_TRI_VTX];
	static st_vertex sq_vtx[DIS_SQ_VTX];
	int y_angle;
	int main_col_add;
	int max_disrupt_radius;
	int disrupt_radius;
	int disrupt_radius_add;
	double disrupt_colour;
	double disrupt_col_add;
	double fireball_colour;
	double fireball_col_add;

	cl_disruptor();
	void activate(cl_object *activator);
	void reset();

	void collidedWith(cl_object *obj);
	void run();
	void runMaterialise();
	void runExplode();

	void draw();
	void drawMaterialise();
	void drawExplode();
	void drawBody();
};




class cl_striker: public cl_object
{
public:
	static st_vertex body_vtx[STRIKER_VTX];
	double max_bolt_length;
	double bolt_length;
	double bolt_add;
	double init_bolt_add;
	double col_add;
	int bolt_timer;
	int bolt_return_timer;

	cl_striker();
	void activate(cl_object *activator);
	void reset();
	bool haveCollided(cl_object *obj);

	void collidedWith(cl_object *obj);
	void run();
	void runExplode();

	void draw();
	void drawExplode();
};


/////////// WEAPONS //////////////

// Laser base class
class cl_laser: public cl_object
{
public:
	double x1;
	double y1;
	double x2;
	double y2;
	double width;
	double draw_x2;

	cl_laser(en_object_type ty);
	void activate(cl_object *activator);
	void reset();
	bool haveCollided(cl_object *obj);

	void run();
	void stageRun();

	void runExplode();

	void draw();
	void drawExplode();
};



class cl_ship_laser: public cl_laser
{
public:
	cl_ship_laser();
	void activate(cl_object *activator);
	void collidedWith(cl_object *obj);
	void run();
};



class cl_stalag_laser: public cl_laser
{
public:
	cl_stalag_laser();
	void activate(cl_object *activator);
	void collidedWith(cl_object *obj);
	void run();
};




// Fired by silo
class cl_missile: public cl_object
{
public:
	static st_vertex mid_vtx[MISSILE_MID_VTX];
	static st_vertex left_vtx[MISSILE_FIN_VTX];
	static st_vertex right_vtx[MISSILE_FIN_VTX];
	static st_vertex flame_vtx[MISSILE_FLAME_VTX];
	double max_speed;
	double turn_rate;
	int fuel;

	cl_missile();
	void activate(cl_object *activator);
	void reset();
	void collidedWith(cl_object *obj);

	void run();
	void turnToHeading(double heading_ang, double ang_inc);

	void runExplode();

	void draw();
	void drawExplode();
};




// Fired by enemies
class cl_bullet: public cl_object
{
public:
	cl_bullet();

	void activate(cl_object *activator);
	void reset();
	void collidedWith(cl_object *obj);

	void run();
	void runExplode();
	void draw();
	void drawExplode();
};




// Fired by lair plasma cannon
class cl_plasma: public cl_object
{
public:
	double arc_size;
	double arc_colour;
	double arc_thick;
	double col_add;
	double ax;
	double ay;
	double ah;

	cl_plasma();
	void activate(cl_object *activator);
	void reset();
	void collidedWith(cl_object *obj);
	void run();
	void runExplode();
	void draw();
	void drawExplode();
};



/////////// POWER UPS /////////////

class cl_powerup: public cl_object
{
public:
	double draw_colour;
	double min_y;
	double max_y;

	cl_powerup(en_object_type ty);

	void activate(cl_object *activator);
	void setSpeed();
	void reset();
	void collidedWith(cl_object *obj);
	bool doDraw();
	void run();
};




class cl_shield_powerup: public cl_powerup
{
public:
	cl_shield_powerup();
	void run();
	void draw();
};




class cl_laser_powerup: public cl_powerup
{
public:
	double x1,y1;
	double x2,y2;
	double x3,y3;
	double x4,y4;

	cl_laser_powerup();
	void activate(cl_object *activator);
	void run();
	void draw();
};




class cl_health_powerup: public cl_powerup
{
public:
	double y_add;
	double col_add;
	double ang_add;

	cl_health_powerup();
	void run();
	void draw();
};




class cl_hyperspace_powerup: public cl_powerup
{
public:
	double width;
	double mult;

	cl_hyperspace_powerup();
	void activate(cl_object *activator);
	void run();
	void draw();
};




class cl_tractorbeam_powerup: public cl_powerup
{
public:
	double spiral_radius;
	double start_angle;

	cl_tractorbeam_powerup();
	void activate(cl_object *activator);
	void run();
	void draw();
};




class cl_reverse_g_powerup: public cl_powerup
{
public:
	cl_reverse_g_powerup();
	void collidedWith(cl_object *obj);
	void run();
	void draw();
};



/////////// MISC TYPES //////////////

// Astronomical background objects are a standalone class
class cl_astro
{
public:
	enum
	{
		TYPE_STAR,
		TYPE_PLANET,
		TYPE_RING_PLANET
	} type;
	enum
	{
		RING_VERT,
		RING_HORIZ
	};
	double x;
	double y;
	double draw_x;
	int diam;
	int colour;
	int ring_colour[2];
	bool ring_type[2];

	cl_astro();
	void setLocation();
	int getColour();
	void draw();
};



// Used for all animated text
class cl_text
{
public:
	en_text_type type;
	char *text;
	bool active;
	double colour;
	double x;
	double y;
	double x2;
	double y2;
	double thick;
	double angle;
	double angle2;
	double x_scale;
	double y_scale;
	double add;
	int timer;
	int len;
	int pause_col;

	cl_text(en_text_type tp);

	void activate();
	void activateBonus(int bonus, cl_object *obj);
	void activatePowerup(cl_object *obj);
	void activateAllPodsSaved(int bonus);
	void draw();
};



/////////////////////////// CHARACTER DEFINITIONS //////////////////////////

/*** All characters are based on a 11x11 pixel size matrix ***/
#define CHAR_SIZE 10  // Should be 11 , my mistake. Too late to fix now.
#define CHAR_HALF (CHAR_SIZE / 2)
#define CHAR_GAP 5

struct st_char_template
{
	int cnt;
	XPoint data[1];
};

#ifdef MAINFILE
struct st_space { int cnt; } char_space = { 0 };

struct st_a { int cnt; XPoint data[6]; } char_A = 
{
	6, 
	{{ 0,10 }, { 5,0 },
	{ 5,0 }, { 10,10 },
	{ 2,6 }, { 8,6 }}
};

struct st_b { int cnt; XPoint data[20]; } char_B = 
{
	20,
	{{ 0,0 }, { 8,0 },
	{ 8,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 5,5 },
	{ 5,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 8,10 },
	{ 8,10 }, { 0,10 },
	{ 0,10 }, { 0,0 },
	{ 0,5 }, { 5,5 }}
};

struct st_c { int cnt; XPoint data[14]; } char_C = 
{
	14,
	{{10,1 } , { 9,0 },
	{ 9,0 } , { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 9,10 },
	{ 9,10 }, { 10,9 }}
};

struct st_d { int cnt; XPoint data[12]; } char_D = 
{
	12,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,8 },
	{ 10,8 }, { 7,10 },
	{ 7,10 }, { 0,10 },
	{ 0,10 }, { 0,0 }}
};

struct st_e { int cnt; XPoint data[8]; } char_E = 
{
	8,
	{{ 0,0 }, { 10,0 },
	{ 0,0 } , { 0,10 },
	{ 0,5 }, { 7,5 },
	{ 0,10 }, { 10,10 }}
};

struct st_f { int cnt; XPoint data[6]; } char_F =
{
	6,
	{{ 0,0 } , { 10,0 },
	{ 0,0 }, { 0,10 },
	{ 0,5 }, { 7,5 }}
};

struct st_g { int cnt; XPoint data[18]; } char_G =
{ 
	18,
	{{ 10,2 } , { 7,0 },
	{ 7,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 7,10 },
	{ 7,10 }, { 10,7 },
	{ 10,7 }, { 10,5 },
	{ 10,5 }, { 5,5 }}
};

struct st_h { int cnt; XPoint data[6]; } char_H =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,5 }, { 10,5 },
	{ 10,0 }, { 10,10 }}
};

struct st_i { int cnt; XPoint data[6]; } char_I =
{
	6,
	{{ 0,0 }, { 10,0 },
	{ 5,0 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_j { int cnt; XPoint data[12]; } char_J =
{
	12, 
	{{ 0,0 }, { 10,0 },
	{ 7,0 }, { 7,7 },
	{ 7,7 }, { 5,10 },
	{ 5,10 }, { 2,10 },
	{ 2,10 }, { 1,8 },
	{ 1,8 }, { 1,6 }}
};

struct st_k { int cnt; XPoint data[6]; } char_K =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,7 }, { 10,0 },
	{ 3,5 }, { 10,10 }}
};

struct st_l { int cnt; XPoint data[4]; } char_L =
{
	4,
	{{ 0,0 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_m { int cnt; XPoint data[8]; } char_M =
{
	8,
	{{ 0,0 } , { 0,10 },
	{ 0,0 }, { 5,5 },
	{ 5,5 }, { 10,0 },
	{ 10,0 }, { 10,10 }}
};

struct st_n { int cnt; XPoint data[6]; } char_N =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,0 }, { 10,10 },
	{ 10,10 }, { 10,0 }}
};

struct st_o { int cnt; XPoint data[16]; } char_O =
{
	16,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 }}
};

struct st_p { int cnt; XPoint data[12]; } char_P =
{
	12,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 7,6 },
	{ 7,6 }, { 0,6 },
	{ 0,0 }, { 0,10 }}
};

struct st_q { int cnt; XPoint data[18]; } char_Q =
{
	18,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 },
	{ 0,10 }, { 4,6 }}
};

struct st_r { int cnt; XPoint data[14]; } char_R =
{
	14,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 7,6 },
	{ 7,6 }, { 0,6 }, 
	{ 0,0 }, { 0,10 },
	{ 3,6 }, { 9,10 }}
};

struct st_s { int cnt; XPoint data[22]; } char_S =
{
	22,
	{{ 10,1 }, { 9,0 },
	{ 9,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,4 }, 
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 }, 
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 }}
};

struct st_t { int cnt; XPoint data[4]; } char_T =
{
	4,
	{{ 0,0 }, { 10,0 },
	{ 5,0 }, { 5,10 }}
};

struct st_u { int cnt; XPoint data[10]; } char_U =
{
	10,
	{{ 0,0 } , { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 },
	{ 10,8 }, { 10,0 }}
};

struct st_v { int cnt; XPoint data[4]; } char_V =
{
	4,
	{{ 0,0 }, { 5,10 },
	{ 5,10 }, { 10,0 }}
};

struct st_w { int cnt; XPoint data[8]; } char_W =
{
	8,
	{{ 0,0 }, { 2,10 },
	{ 2,10 }, { 5,4 },
	{ 5,4 }, { 8,10 },
	{ 8,10 }, { 10,0 }}
};

struct st_x { int cnt; XPoint data[4]; } char_X =
{
	4,
	{{ 0,0 }, { 10,10 },
	{ 10,0 }, { 0,10 }}
};

struct st_y { int cnt; XPoint data[4]; } char_Y =
{
	4,
	{{ 0,0 }, { 6,4 },
	{ 10,0 }, { 2,10 }}
};

struct st_z { int cnt; XPoint data[6]; } char_Z =
{
	6,
	{{ 0,0 }, { 10,0 },
	{ 10,0 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_1 { int cnt; XPoint data[6]; } char_1 =
{
	6,
	{{ 2,3 }, { 5,0 },
	{ 5,0 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_2 { int cnt; XPoint data[12]; } char_2 =
{
	12,
	{{ 0,2 }, { 2,0 },
	{ 2,0 }, { 8,0 },
	{ 8,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_3 { int cnt; XPoint data[16]; } char_3 =
{
	16,
	{{ 0,1 }, { 1,0 },
	{ 1,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 },
	{ 3,5 }, { 10,5 }}
};

struct st_4 { int cnt; XPoint data[6]; } char_4 =
{
	6,
	{{ 10,5 }, { 0,5 },
	{ 0,5 }, { 5,0 },
	{ 5,0 }, { 5,10 }}
};

struct st_5 { int cnt; XPoint data[14]; } char_5 =
{
	14,
	{{ 10,0 }, { 0,0 },
	{ 0,0 }, { 0,5 },
	{ 0,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 0,10 }}
};

struct st_6 { int cnt; XPoint data[18]; } char_6 =
{
	18,
	{{ 10,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,9 },
	{ 0,9 }, { 1,10 },
	{ 1,10 }, { 9,10 },
	{ 9,10 }, { 10,9 },
	{ 10,9 }, { 10,6 },
	{ 10,6 }, { 9,5 },
	{ 9,5 }, { 0,5 }}
};	

struct st_7 { int cnt; XPoint data[4]; } char_7 =
{ 
	4,
	{{ 0,0 }, { 10,0 },
	{ 10,0 }, { 2,10 }}
};

struct st_8 { int cnt; XPoint data[30]; } char_8 =
{
	30,
	{{ 1,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 },
	{ 0,9 }, { 0,6 },
	{ 0,6 }, { 1,5 },
	{ 1,5 }, { 0,4 },
	{ 0,4 }, { 0,1 },
	{ 0,1 }, { 1,0 }, 
	{ 1,5 }, { 9,5 }}
};

struct st_9 { int cnt; XPoint data[18]; } char_9 =
{
	18,
	{{ 1,10 }, { 9,10 },
	{ 9,10 }, { 10,9 },
	{ 10,9 }, { 10,1 },
	{ 10,1 }, { 9,0 },
	{ 9,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,4 },
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 10,5 }}
};

struct st_0 { int cnt; XPoint data[18]; } char_0 =
{
	18,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 }, 
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 },
	{ 3,10 }, { 7,0 }}
};

struct st_qmark { int cnt; XPoint data[16]; } char_qmark =
{
	16,
	{{ 0,2 }, { 2,0 },
	{ 2,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 9,5 },
	{ 9,5 }, { 5,5 },
	{ 5,5 }, { 5,7 },
	{ 5,9 }, { 5,10 }}
};

struct st_exmark { int cnt; XPoint data[8]; } char_exmark =
{
	8,
	{{ 4,0 }, { 6,0 },
	{ 6,0 }, { 5,7 },
	{ 5,7 }, { 4,0 },
	{ 5,9 }, { 5,10 }}
};

struct st_plus { int cnt; XPoint data[4]; } char_plus =
{
	4,
	{{ 5,0 }, { 5,10 },
	{ 0,5 }, { 10,5 }}
};

struct st_minus { int cnt; XPoint data[2]; } char_minus =
{
	2,
	{{ 0,5 }, { 10,5 }}
};

struct st_star { int cnt; XPoint data[8]; } char_star =
{
	8,
	{{ 0,5 }, { 10,5 },
	{ 5,0 }, { 5,10 },
	{ 1,1 }, { 9,9 },
	{ 1,9 }, { 9,1 }}
};

struct st_equals { int cnt; XPoint data[4]; } char_equals =
{
	4,
	{{ 0,3 }, { 10,3 },
	{ 0,7 }, { 10,7 }}
};

struct st_dot { int cnt; XPoint data[10]; } char_dot =
{
	8,
	{{ 4,8 }, { 6,8 },
	{ 6,8 }, { 6,10 },
	{ 6,10 }, { 4,10 },
	{ 4,10 }, { 4,8 }}
};

struct st_comma { int cnt; XPoint data[4]; } char_comma =
{
	4,
	{{ 7,6 }, { 6,9 },
	{ 6,9 }, { 4,10 }}
};

struct st_lrbracket { int cnt; XPoint data[10]; } char_lrbracket =
{
	10,
	{{ 6,0 }, { 5,0 },
	{ 5,0 }, { 3,2 },
	{ 3,2 }, { 3,8 },
	{ 3,8 }, { 5,10 },
	{ 5,10 }, { 6,10 }}
};

struct st_rrbracket { int cnt; XPoint data[10]; } char_rrbracket =
{
	10,
	{{ 3,0 }, { 4,0 },
	{ 4,0 }, { 6,2 },
	{ 6,2 }, { 6,8 },
	{ 6,8 }, { 4,10 },
	{ 4,10 }, { 3,10 }}
};

struct st_lcbracket { int cnt; XPoint data[12]; } char_lcbracket =
{
	12,
	{{ 6,0 }, { 4,1 },
	{ 4,1 }, { 4,4 },
	{ 4,4 }, { 2,5 },
	{ 2,5 }, { 4,6 },
	{ 4,6 }, { 4,9 },
	{ 4,9 }, { 6,10 }}
};

struct st_rcbracket { int cnt; XPoint data[12]; } char_rcbracket =
{
	12,
	{{ 4,0 }, { 6,1 },
	{ 6,1 }, { 6,4 },
	{ 6,4 }, { 8,5 },
	{ 8,5 }, { 6,6 },
	{ 6,6 }, { 6,9 },
	{ 6,9 }, { 4,10 }}
};

struct st_lsbracket { int cnt; XPoint data[6]; } char_lsbracket =
{
	6,
	{{ 7,0 }, { 3,0 },
	{ 3,0 }, { 3,10 },
	{ 3,10 }, { 7,10 }}
};

struct st_rsbracket { int cnt; XPoint data[6]; } char_rsbracket =
{
	6,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 7,10 },
	{ 7,10 }, { 3,10 }}
};

struct st_dollar { int cnt; XPoint data[26]; } char_dollar =
{
	26,
	{{ 10,2 }, { 9,1 },
	{ 9,1 }, { 1,1 },
	{ 1,1 }, { 0,2 },
	{ 0,2 }, { 0,4 }, 
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,8 },
	{ 10,8 }, { 9,9 }, 
	{ 9,9 }, { 1,9 },
	{ 1,9 }, { 0,8 },
	{ 4,0 }, { 4,10 },
	{ 6,0 }, { 6,10 }}
};

struct st_hash { int cnt; XPoint data[8]; } char_hash =
{
	8,
	{{ 0,3 }, { 10,3 },
	{ 0,7 }, { 10,7 },
	{ 3,0 }, { 3,10 },
	{ 7,0 }, { 7,10 }}
};

struct st_fslash { int cnt; XPoint data[4]; } char_fslash =
{
	2,
	{{ 10,0 }, { 0,10 }}
};

struct st_bslash { int cnt; XPoint data[4]; } char_bslash =
{
	2,
	{{ 0,0 }, { 10,10 }}
};

struct st_less { int cnt; XPoint data[4]; } char_less =
{
	4,
	{{ 10,0 }, { 0,5 },
	{ 0,5 }, { 10,10 }}
};

struct st_greater { int cnt; XPoint data[4]; } char_greater =
{
	4,
	{{ 0,0 }, { 10,5 },
	{ 10,5 }, { 0,10 }}
};

struct st_underscore { int cnt; XPoint data[2]; } char_underscore =
{
	2,
	{{ 0,10 }, { 10,10 }}
};

struct st_bar { int cnt; XPoint data[2]; } char_bar =
{
	2,
	{{ 5,0 }, { 5,10 }}
};

struct st_squote { int cnt; XPoint data[2]; } char_squote =
{
	2,
	{{ 6,0 }, { 4,3 }}
};

struct st_dquote { int cnt; XPoint data[4]; } char_dquote =
{
	4,
	{{ 3,0 }, { 3,2 },
	{ 7,0 }, { 7,2 }}
};

struct st_bquote { int cnt; XPoint data[2]; } char_bquote =
{
	2,
	{{ 4,0 }, { 6,3 }}
};

struct st_colon { int cnt; XPoint data[16]; } char_colon =
{
	16,
	{{ 4,1 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 4,1 },
	{ 4,7 }, { 6,7 },
	{ 6,7 }, { 6,9 },
	{ 6,9 }, { 4,9 },
	{ 4,9 }, { 4,7 }}
};

struct st_semicolon { int cnt; XPoint data[12]; } char_semicolon =
{
	12,
	{{ 4,1 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 4,1 },
	{ 6,6 }, { 6,8 },
	{ 6,8 }, { 4,10 }}
};	

struct st_at { int cnt; XPoint data[36]; } char_at =
{
	36,
	{{ 7,6 }, { 7,4 },
	{ 7,4 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 3,4 },
	{ 3,4 }, { 3,6 },
	{ 3,6 }, { 4,7 },
	{ 4,7 }, { 6,7 },
	{ 6,7 }, { 7,6 },
	{ 7,6 }, { 8,7 },
	{ 8,7 }, { 9,6 },
	{ 9,6 }, { 9,2 },
	{ 9,2 }, { 7,0 },
	{ 7,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 }}
};

struct st_hat { int cnt; XPoint data[4]; } char_hat =
{
	4,
	{{ 5,0 }, { 1,5 },
	{ 5,0 }, { 9,5 }}
};

struct st_tilda { int cnt; XPoint data[6]; } char_tilda =
{
	6,
	{{ 1,2 }, { 3,0 },
	{ 3,0 }, { 5,2 },
	{ 5,2 }, { 7,0 }}
};

struct st_ampersand { int cnt; XPoint data[26]; } char_ampersand =
{
	26,
	{{ 9,9 }, { 8,10 },
	{ 8,10 }, { 1,3 },
	{ 1,3 }, { 1,1 }, 
	{ 1,1 }, { 2,0 },
	{ 2,0 }, { 5,0 },
	{ 5,0 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 0,7 },
	{ 0,7 }, { 0,9 },
	{ 0,9 }, { 1,10 }, 
	{ 1,10 }, { 6,10 },
	{ 6,10 }, { 8,8 },
	{ 8,8 }, { 8,7 }}
};

struct st_percent { int cnt; XPoint data[18]; } char_percent =
{
	18,
	{{ 0,10 }, { 10,0 },

	{ 0,0 }, { 4,0 },
	{ 4,0 }, { 4,4 },
	{ 4,4 }, { 0,4 },
	{ 0,4 }, { 0,0 },

	{ 6,6 }, { 10,6 },
	{ 10,6 }, { 10,10 },
	{ 10,10 }, { 6,10 },
	{ 6,10 }, { 6,6 }}
};
#endif


////////////////////////////////// GLOBALS //////////////////////////////////

EXTERN Display *display;
EXTERN Drawable drw;
EXTERN Window win;
EXTERN GC gc[NUM_COLOURS];

EXTERN cl_ship *ship;
EXTERN cl_moonbase *moonbase;
EXTERN cl_object *object[NUM_OBJECTS];
EXTERN cl_object *show_object[NUM_SHOW_OBJECTS];
EXTERN cl_astro *astro[NUM_ASTROS];
EXTERN cl_text *textobj[NUM_TEXTS];

EXTERN char *alsa_device;
EXTERN int win_width;
EXTERN int win_height;
EXTERN int win_refresh;
EXTERN int refresh_cnt;
EXTERN int level;
EXTERN int score;
EXTERN int lives;
EXTERN int high_score;
EXTERN int game_stage_timer;
EXTERN int level_stage_timer;
EXTERN int screen_x_shake;
EXTERN int screen_y_shake;
EXTERN int pod_start;
EXTERN int pods_saved;
EXTERN int max_hill_height;
EXTERN int num_hills[2];
EXTERN int apc_hill_col[2];
EXTERN int skyth_appear_at;
EXTERN int cannon_destroyed;
EXTERN int bonus_life_score;
EXTERN int reverse_g_timer;
EXTERN int zero_g_timer;
EXTERN double g_x_scale;
EXTERN double g_y_scale;
EXTERN double avg_scale;
EXTERN double screen_x_shift;
EXTERN double background_x_shift;
EXTERN double gravity;
EXTERN double start_gravity;
EXTERN double cast_angle;
EXTERN bool paused;
EXTERN bool all_pods_captured;
EXTERN bool activated_high_score_text;

EXTERN en_game_stage game_stage;
EXTERN st_hill_vertex hill[2][MAX_HILLS];
EXTERN XPoint draw_vtx[MAX_VERTEXES];  // For objects and radar
EXTERN st_char_template *ascii_table[256];

EXTERN char version_text[10];
EXTERN char lives_text[2];
EXTERN char score_text[10];
EXTERN char high_score_text[10];

#ifdef SOUND
EXTERN bool do_sound;
EXTERN bool do_fragment;
EXTERN bool do_soundtest;
#endif

//////////////////////////// FORWARD DECLARATIONS ////////////////////////////

// main.cc
void setScore(int sc);

// hills.cc
void createHills();
st_hill_height *getHillHeight(int tb, double x);
st_hill_inside *insideHill(double radius, double x, double y);

// maths.cc
double getAngle(double x, double y, double x2, double y2);
double angleDiff(double ang, double pang);
void incAngle(double &ang, double add);
void rotatePoint(double x, double y, double ang, double *x_res, double *y_res);
double getXDistance(double x1, double x2);
double getDrawX(double x);
void updateXY(double &x, double &y, double xspd, double &yspd);

// common.cc
bool activateObjects(en_object_type type, int num, cl_object *activator);
void activateBonusText(int bonus, cl_object *obj);
void setLives(int lv);
void setHighScore(int hs);
void setScore(int sc);
void incScore(int val);

// draw.cc
void drawDemoScreen();
void drawCastScreen();
void drawKeysScreen();
void drawPlayScreen();
void drawLightning(double col, double len, double ang, double x, double y);

void drawCenteredText(
	const char *mesg,
	double col,
	double thick,
	double ang,
	double gap,
	double x_scale, double y_scale, double y);
void drawText(
	const char *mesg,
	double col,
	double thick,
	double ang,
	double gap,
	double x_scale, double y_scale, double x, double y);
void drawChar(
	char c,
	double col,
	double thick,
	double ang, double x_scale, double y_scale, double x, double y);
void drawLine(
	double col, double thick, double x1, double y1, double x2, double y2);
void drawOrFillCircle(
	double col,
	double thick,
	double x_diam, double y_diam, double x, double y, bool fill);
void drawOrFillPolygon(
	double col, double thick, int num_points, XPoint *points, bool fill);
void drawOrFillRectangle(
	double col,
	double thick, double w, double h, double x, double y, bool fill);
void drawOrFillHorizArc(
	double col,
	double thick, double w, double h, double x, double y, bool fill);
void drawOrFillVertArc(
	double col,
	double thick, double w, double h, double x, double y, bool fill);


// sound.cc
void startSoundDaemon();
void playSound(en_sound snd);
void echoOn();
void echoOff();
void tellSoundShipThrust(u_char thrust);
