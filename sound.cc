/*****************************************************************************
  This is code for the sound daemon that communicates with the main game parent 
  process through shared memory and parent interface functions. This generates 
  sin, square, triangle, sawtooth, whitenoise & FM sounds and has low pass 
  filter, distortion and echo functionality.
 *****************************************************************************/

#include "globals.h"

#ifdef SOUND
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef ALSA
#include <alsa/asoundlib.h>
#else
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#endif

#define PCM_FREQ       20000
#define SNDBUFF_SIZE   (PCM_FREQ / 20) /* Sample is 1/20th sec */
#define ECHOBUFF_SIZE  (SNDBUFF_SIZE * 7)
#define ECHO_MULT      0.4
#define MAINLOOP_DELAY 40000
#define NUM_CHANS      3
#define MAX_SHORT      32767
#define MIN_SHORT      -32768

#define LOW_VOLUME    8000
#define MED_VOLUME    (LOW_VOLUME * 2)
#define HIGH_VOLUME   (LOW_VOLUME * 3)

#define PRIORITY(SND) (shm->snd < SND)

// Prevent wrapping - clip instead 
#define CLIP_AND_SET_BUFFER() \
	if (res > MAX_SHORT) res = MAX_SHORT; \
	else \
	if (res < MIN_SHORT) res = MIN_SHORT; \
	sndbuff[i] = (short)res;

// PCM buffers
short sndbuff[SNDBUFF_SIZE];
short echobuff[ECHOBUFF_SIZE];

#ifdef ALSA
snd_pcm_t *handle;
#else
int sndfd;
#endif

bool echo_on;
int echo_write_pos;
double echo_mult;


// Channel globals 
double sin_ang[NUM_CHANS];
double sin_fm_ang1[NUM_CHANS];
double sin_fm_ang2[NUM_CHANS];

double tri_val[NUM_CHANS];
double tri_inc[NUM_CHANS];

int sq_vol[NUM_CHANS];
int sq_cnt[NUM_CHANS];

double saw_val[NUM_CHANS];

// Shared mem
int shmid;

struct st_sharmem
{
	u_char echo;
	u_char snd;
	u_char ship_thrust;
} *shm;

enum en_note
{
	NOTE_SILENCE,

	NOTE_A,
	NOTE_AS,
	NOTE_B,
	NOTE_C,  /* Middle C */

	NOTE_CS,
	NOTE_D,
	NOTE_DS,
	NOTE_E,

	NOTE_F,
	NOTE_FS,
	NOTE_G,
	NOTE_GS,

	NOTE_A2,
	NOTE_AS2,
	NOTE_B2,
	NOTE_C2,

	NOTE_CS2,
	NOTE_D2,
	NOTE_DS2,
	NOTE_E2,

	NOTE_F2,
	NOTE_FS2,
	NOTE_G2,
	NOTE_GS2,

	NUM_NOTES
};


/* http://www.intmath.com/Trigonometric-graphs/music.php */
double note_freq[NUM_NOTES] =
{
	0,

	220,
	233.08,
	246.94,
	261.63,

	277.18,
	293.66,
	311.63,
	329.63,

	349.23,
	369.99,
	392,
	415.3,

	440,
	466.16,
	493.88,
	523.25,

	554.37,
	586.33,
	622.25,
	659.26,

	698.46,
	739.99,
	783.99,
	830.61
};


// Forward declarations
void checkEcho();

// Sounds
void playBounce();
void playShipHit();

void playMissileThrust();
void playShipThrust();
void playShipShield();
void playStriker();
void playDisruptor();

void playShipGrabbing();
void playStalagLaser();
void playBullet();
void playShipLaser();
void playShipDrop();
void playShipGrabbed();
void playPowerupDisappear();
void playEnemyMaterialise();
void playGrypperExplode();
void playZombieExplode();
void playSkythExplode();
void playStalagExplode();
void playSiloOrMissileExplode();
void playStrikerExplode();
void playDisruptorExplode();
void playPodExplode();
void playGrypperLaunch();
void playZombieLaunch();
void playPodSaved();
void playGrypperGrab();
void playPodCaptured();
void playPlasma();
void playCannonExplode();
void playBonusScore();
void playSmallBombExplode();
void playMediumBombExplode();
void playLargeBombExplode();
void playMissileLaunch();
void playLairExplode();
void playPowerupActivate();
void playShipDematerialise();
void playShipMaterialise();
void playAllPodsCaptured();
void playHighScore();
void playBonusLife();
void playPowerupAppear();
void playShipExplode();
void playLevelComplete();
void playGameOver();
void playStart();

void resetSoundChannels();
void resetSoundBuffer();
void resetEchoBuffer();
void writeSound();

void playBellSound(double freq, int cnt, en_sound snd);
void playOrganSound(double freq, int filt, en_sound snd);

void addSin(int ch, double vol, double freq, int reset);
void addSinFM(
	int ch,
	double vol, double freq,
	double fm_vol, double fm_freq, int reset);
void addTriangle(int ch, double vol, double freq, int reset);
void addSquare(int ch, double vol, double freq, int reset);
void addSawtooth(int ch, double vol, double freq, int reset);
void addNoise(double vol, int gap, int reset);
void addDistortion(short clip);
void filter(int sample_size, int reset);

// Play functions. Priorities are lowest -> highest
void (*playfunc[NUM_SOUNDS])() = 
{
	NULL,

	// Non-continuous
	playBounce,
	playShipHit,

	// Continous
	playMissileThrust,
	playShipThrust,
	playShipShield,
	playStriker,
	playDisruptor,

	// Non-continous
	playShipGrabbing,
	playStalagLaser,
	playBullet,
	playShipLaser,
	playShipDrop,
	playShipGrabbed,
	playPowerupDisappear,
	playEnemyMaterialise,
	playGrypperExplode,
	playZombieExplode,
	playSkythExplode,
	playStalagExplode,
	playSiloOrMissileExplode,
	playStrikerExplode,
	playDisruptorExplode,
	playPodExplode,
	playGrypperLaunch,
	playZombieLaunch,
	playPodSaved,
	playGrypperGrab,
	playPodCaptured,
	playPlasma,
	playCannonExplode,
	playBonusScore,
	playSmallBombExplode,
	playMediumBombExplode,
	playLargeBombExplode,
	playMissileLaunch,
	playLairExplode,
	playPowerupActivate,
	playShipDematerialise,
	playShipMaterialise,
	playAllPodsCaptured,
	playHighScore,
	playBonusLife,
	playPowerupAppear,
	playShipExplode,
	playLevelComplete,
	playGameOver,
	playStart
};

#endif


///////////////////////// PARENT INTERFACE FUNCTIONS //////////////////////////

void initALSA();
void initOpenSound();
void closedown();
void soundLoop();

/*** Create the shared memory and spawn off the child sound daemon process.
     I could have used pthreads but fork() is probably more portable plus
     if the sound daemon crashes it won't bring down the game process. 
     This function runs whatever do_sound is set to since player may want
     to switch sound on in the game later ***/
void startSoundDaemon()
{
#ifdef SOUND
	int i;

	shmid = -1;
	echo_on = false;

#ifdef ALSA
	initALSA();
#else
	initOpenSound();
#endif
	/* Set up shared memory. 10 attempts at finding a random key that 
           is currently unused. Using ftok() is a waste of time.
	   The shared memory has the following layout:

	    0         1       2
	    --------- ------- -------------
	   | echo on | sound | ship thrust |
	    --------- ------- -------------
	*/
	for(i=0;i < 10 && shmid == -1;++i)
	{
		shmid = shmget(
			(key_t)random(),
			sizeof(struct st_sharmem),IPC_CREAT | IPC_EXCL | 0600
			);
	}
	if (shmid == -1)
	{
		printf("SOUND: shmget(): %s\n",strerror(errno));
		closedown();
		return;
	}

	if ((shm = (struct st_sharmem *)shmat(shmid,NULL,0)) == (void *)-1)
	{
		printf("SOUND: shmat(): %s\n",strerror(errno));
		closedown();
		return;
	}

	// Mark for deletion when both processes have exited
	shmctl(shmid,IPC_RMID,0);

	bzero(shm,sizeof(struct st_sharmem));

	// Spawn off sound daemon as child process
	switch(fork())
	{
	case -1:
		printf("SOUND: fork(): %s\n",strerror(errno));
		closedown();
		return;

	case 0:
		// Child
		soundLoop();
		exit(0);
	}

	// Parent ends up here
#ifdef ALSA
	snd_pcm_close(handle);
#else
	close(sndfd);
#endif
#endif
}


#ifdef SOUND
#ifdef ALSA

/*** Use ALSA ***/
void initALSA()
{
	snd_pcm_hw_params_t *params;
	u_int freq = PCM_FREQ;
	int err;

	if ((err = snd_pcm_open(&handle,alsa_device,SND_PCM_STREAM_PLAYBACK,0)) < 0)
	{
		printf("SOUND: snd_pcm_open(): %s\n",snd_strerror(err));
		handle = NULL;
		return;
	}

	// Allocate hardware params structure
	if ((err = snd_pcm_hw_params_malloc(&params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_malloc(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Init structure 
	if ((err = snd_pcm_hw_params_any(handle,params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_any(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
	
	// Set interleaved regardless of mono or stereo 
	if ((err = snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_access(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Set number of channels. 1 in this case because we want mono 
	if ((err = snd_pcm_hw_params_set_channels(handle,params,1)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_channels(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Set word format
	if ((err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_format(): %s\n",snd_strerror(err));
		closedown();
		return;
	}

	// Set PCM frequency
	if ((err = snd_pcm_hw_params_set_rate_near(handle,params,&freq,0)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_rate_near(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
	if (freq != PCM_FREQ)
	{
		printf("ERROR: Device PCM freq %dHz is not requested freq %dHz\n",
			freq,PCM_FREQ);
		closedown();
		return;
	}

	// Do actual set of parameters on device
	if ((err = snd_pcm_hw_params(handle,params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
	snd_pcm_hw_params_free(params);	

	// Not sure what this is for
	if ((err = snd_pcm_prepare(handle)) < 0)
	{
		printf("SOUND: snd_pcm_prepare(): %s\n",snd_strerror(err));
		closedown();
		return;
	}
}


#else

void initOpenSound()
{
	u_int tmp;
	int frag;

	// Open sound device
	if ((sndfd=open("/dev/dsp",O_WRONLY)) == -1 &&
	    (sndfd=open("/dev/audio",O_WRONLY)) == -1)
	{
		printf("SOUND: Can't open /dev/dsp or /dev/audio: %s\n",strerror(errno));
		if (errno == EBUSY)
			puts("SOUND: Consider using 'aoss' wrapper script");
		return;
	}

	// Reset it
	if (ioctl(sndfd,SNDCTL_DSP_RESET) == -1)
	{
		printf("SOUND: Can't reset sound device: %s\n",
			strerror(errno));
		closedown();
		return;
	}

	// Set endian type
	tmp = AFMT_S16_NE;
	if (ioctl(sndfd,SNDCTL_DSP_SETFMT,&tmp) == -1)
	{
		printf("SOUND: ioctl(SNDCTL_DSP_SETFMT) failed: %s\n",
			strerror(errno));
		closedown();
		return;
	}

	tmp = PCM_FREQ;
	if (ioctl(sndfd,SNDCTL_DSP_SPEED,&tmp) == -1)
	{
		printf("SOUND: ioctl(SNDCTL_DSP_SPEED) failed: %s\n",
			strerror(errno));
		closedown();
		return;
	}
	if (tmp != PCM_FREQ)
	{
		printf("SOUND: Device PCM freq %dHz is not requested freq %dHz\n",
			tmp,PCM_FREQ);
		closedown();
		return;
	}

	/* Set so short sounds get played immediately, not buffered. This 
	   value seemd to work without messing the sounds up when played
	   direct however it seems to cause issues with background sounds
	   when sound done via aoss. *shrug*
	   http://manuals.opensound.com/developer/SNDCTL_DSP_SETFRAGMENT.html
	*/
	if (do_fragment)
	{
		frag = (2 << 16) | 10;
		if (ioctl(sndfd,SNDCTL_DSP_SETFRAGMENT,&frag))
		{
			printf("SOUND: ioctl(SNDCTL_DSP_SETFRAGMENT) failed: %s\n",
				strerror(errno));
			closedown();
			return;
		}
	}
}
#endif /* ALSA */
#endif /* SOUND */


/*** Obvious really ***/
void closedown()
{
#ifdef SOUND
#ifdef ALSA
	snd_pcm_close(handle);
	handle = NULL;
#else
	close(sndfd);
	sndfd = -1;
#endif
#endif
}



/*** Play a sound but do some checks beforehand ***/
void playSound(en_sound snd)
{
#ifdef SOUND
#ifdef ALSA
	if (do_sound && 
	    handle && 
	    game_stage >= GAME_STAGE_GAME_START && snd > shm->snd)
#else
	if (do_sound && 
	    sndfd != -1 && 
	    game_stage >= GAME_STAGE_GAME_START && snd > shm->snd)
#endif
		shm->snd = snd;
#endif
}




void echoOn()
{
#ifdef SOUND
#ifdef ALSA
	if (handle) shm->echo = 1;
#else
	if (sndfd != -1) shm->echo = 1;
#endif
#endif
}




void echoOff()
{
#ifdef SOUND
#ifdef ALSA
	if (handle) shm->echo = 0;
#else
	if (sndfd != -1) shm->echo = 0;
#endif
#endif
}




void tellSoundShipThrust(u_char thrust)
{
#ifdef SOUND
#ifdef ALSA
	if (handle) shm->ship_thrust = thrust;
#else
	if (sndfd != -1) shm->ship_thrust = thrust;
#endif
#endif
}


///////////////////////////////// MAIN LOOP //////////////////////////////////

#ifdef SOUND

/*** Child process sits here, watches the shared memory segment and plays the 
     sounds ***/
void soundLoop()
{
	u_char snd;
	int check_cnt;

	resetSoundChannels();
	resetEchoBuffer();
	filter(0,1);

	if (do_soundtest)
	{
		// Play all the sounds then exit
		for(snd=1;snd < NUM_SOUNDS;++snd)
		{
			printf("%d\n",snd);
			resetSoundBuffer();
			playfunc[snd]();
		}
		sleep(1);
		exit(0);
	}

	for(check_cnt=0;;check_cnt = (check_cnt + 1) % 20)
	{
		/* Samples take 1/20th of a second to play so pause to let
		   the device buffer empty and so we don't kill the CPU.
		   Using the DSP_SYNC ioctl() leads to nasty stuttering. */
		usleep(MAINLOOP_DELAY);

		checkEcho();

		/* Sounds are played once then stop. For continuous sounds
		   they must be called constantly. Loop on SM_SND because
		   the parent process updates this value */
		while(shm->snd != SND_SILENCE)
		{
			snd = shm->snd;
			assert(snd < NUM_SOUNDS);

			shm->snd = SND_SILENCE;
			resetSoundBuffer();
			playfunc[snd]();

			checkEcho();
		}

		if (echo_on)
		{
			// Let any echos play out. Reset sound buffer so the 
			// echos fade away instead of building on each other.
			resetSoundBuffer();
			writeSound();
		}

		// See if parent has died by checking if we've been reparented.
		// No need to check it on every loop however hence check_cnt
		if (!check_cnt && getppid() == 1)
		{
			puts("SOUND: Parent process dead - exiting");
			closedown();
			exit(0);
		}
	}
}




/*** Check for echo request and set flag on or off appropriately ***/
void checkEcho()
{
	if (shm->echo && !echo_on)
	{
		echo_on = true;
		resetEchoBuffer();
	}
	else if (!shm->echo) echo_on = false;
}


////////////////////////////// HIGH LEVEL SOUNDS //////////////////////////////

///// NON-CONTINOUS SOUNDS 

/*** Anything bouncing off a hill ***/
void playBounce()
{
	addTriangle(0,HIGH_VOLUME,70,1);
	writeSound();
	addTriangle(0,HIGH_VOLUME,60,1);
	writeSound();
}




void playShipHit()
{
	for(int i=0;i < 2 && PRIORITY(SND_SHIP_HIT);++i)
	{
		addTriangle(0,HIGH_VOLUME,50,1);
		addTriangle(1,HIGH_VOLUME,30,0);
		writeSound();
	}
}



///// CONTINOUS SOUNDS 

/*** Noise when missile under power. This is call continously by missile
     when under power so no loop. ***/
void playMissileThrust()
{
	addNoise(MED_VOLUME,50,1);
	writeSound();
}




/*** Ship thruster has fired. Vary volume and freq based on thrust ***/
void playShipThrust()
{
	double thrust = (double)shm->ship_thrust;
	double volume = (int)round((double)HIGH_VOLUME / 255 * thrust);

	// Thrust value is 0 -> 255.
	addTriangle(0,volume,40 + thrust/10,1);
	addTriangle(1,volume,42 + thrust/10,0);
	addNoise(volume,(int)(10 + thrust / 10),0);
	writeSound();
}




void playShipShield()
{
	addSquare(0,MED_VOLUME,60,1);
	addSquare(1,MED_VOLUME,61,0);
	addSquare(2,MED_VOLUME,62,0);
	filter(15,0);
	writeSound();
}




void playDisruptor()
{
	addSinFM(0,MED_VOLUME,1,200,1.2*50,1);
	addSinFM(1,MED_VOLUME,3,200,1.6*50,0);
	writeSound();
}




void playStriker()
{
	addSquare(0,HIGH_VOLUME,30,1);
	addSquare(1,HIGH_VOLUME,35,0);
	addSquare(2,HIGH_VOLUME,40,0);
	writeSound();
}


///// NON-CONTINOUS SOUNDS 


/*** Ship is trying to grab something.***/
void playShipGrabbing()
{
	for(int i=0;i < 15 && PRIORITY(SND_SHIP_GRABBING);++i)
	{
		addSinFM(0,MED_VOLUME,1,300,1.2*100,1);
		addSinFM(1,MED_VOLUME,1,300,1.6*100,0);
		writeSound();
	}
}




void playStalagLaser()
{
	int freq;
	int i;

	for(i=0;i < 5 && PRIORITY(SND_STALAG_LASER);++i)
	{
		freq = 20 + (int)random() % 50;
		addTriangle(0,MED_VOLUME,freq,1);
		addTriangle(1,MED_VOLUME,freq+10,0);
		addNoise(MED_VOLUME,4 + i * 20,0);
		writeSound();
	}
}




void playBullet()
{
	int freq;
	int i;

	for(i=0,freq=120;i < 3 && PRIORITY(SND_BULLET);++i,freq-=20)
	{
		addTriangle(0,HIGH_VOLUME,freq,1);
		addTriangle(1,HIGH_VOLUME,freq+2,0);
		addDistortion(LOW_VOLUME);
		writeSound();
	}
}




void playShipLaser()
{
	int freq;
	int i;

	for(i=0;i < 4 && PRIORITY(SND_SHIP_LASER);++i)
	{
		freq = 50 + (int)random() % 50;
		addTriangle(0,MED_VOLUME,freq,1);
		addTriangle(1,MED_VOLUME,freq+2,0);
		addNoise(MED_VOLUME,2 + i * 5,0);
		writeSound();
	}
}




void playShipDrop()
{
	double vol = HIGH_VOLUME;

	for(double f=700;f > 500 && PRIORITY(SND_SHIP_DROP);f-=5)
	{
		addSinFM(0,vol,f,40,f*1.2,1);
		addSinFM(1,vol,f,40,f*2.4,0);
		vol *= 0.9;
		writeSound();
	}
}




void playShipGrabbed()
{
	double freq = 80;
	for(int i=0;i < 40 && PRIORITY(SND_SHIP_GRABBED);++i)
	{
		addSinFM(0,HIGH_VOLUME,freq,50,freq * 1.2,1);
		addSinFM(1,HIGH_VOLUME,freq+1,50,freq * 2.4,0);
		if (freq < 120) freq += 4;
		writeSound();
	}
}




void playPowerupDisappear()
{
	en_note tune[] =
	{
		NOTE_E,NOTE_D,NOTE_C,NOTE_B,NOTE_A
	};

	for(int i=0;i < 5 && PRIORITY(SND_POWERUP_DISAPPEAR);++i)
		playOrganSound(note_freq[tune[i]] / 2,5,SND_POWERUP_APPEAR);
}




void playEnemyMaterialise()
{
	for(int i=20;i > 5 && PRIORITY(SND_ENEMY_MATERIALISE);i -= 2)
	{
		addNoise(LOW_VOLUME,i * 2,i % 2);
		writeSound();
	}
}




void playGrypperExplode()
{
	for(int i=60;i && PRIORITY(SND_GRYPPER_EXPLODE);i-=10)
	{
		addSquare(0,MED_VOLUME,i,1);
		addSquare(1,MED_VOLUME,i+1,0);
		addNoise(MED_VOLUME,60-i,0);
		filter(5,!i);
		writeSound();
	}
}




void playZombieExplode()
{
	for(int i=0,freq=100;i < 15 && PRIORITY(SND_ZOMBIE_EXPLODE);++i,freq-=5)
	{
		addSquare(0,MED_VOLUME,freq,1);
		addSquare(1,MED_VOLUME,freq+1,0);
		addNoise(HIGH_VOLUME,i*5,0);
		filter(5,!i);
		writeSound();
	}
}




void playSkythExplode()
{
	int freq = 100;

	for(int i=0;i < 10 && PRIORITY(SND_SKYTH_EXPLODE);++i)
	{
		addSawtooth(0,HIGH_VOLUME,freq,1);
		addSawtooth(1,HIGH_VOLUME,freq+2,0);
		filter(i*5,!i);
		freq -= 10;
		writeSound();
	}

}




void playStalagExplode()
{
	for(int i=1;i < 25 && PRIORITY(SND_STALAG_EXPLODE);++i)
	{
		addTriangle(0,HIGH_VOLUME,100 - i*4,1);
		addNoise(HIGH_VOLUME,10+i*3,0);
		writeSound();
	}
}




void playSiloOrMissileExplode()
{
	for(int i=0;i < 15 && PRIORITY(SND_SILO_OR_MISSILE_EXPLODE);++i)
	{
		addSquare(0,HIGH_VOLUME,1+random() % 10,1);
		addNoise(HIGH_VOLUME,20+i*2,0);
		writeSound();
	}
}




void playStrikerExplode()
{
	double freq = 50;
	double vol = HIGH_VOLUME;

	for(int i=0;i < 30 && PRIORITY(SND_STRIKER_EXPLODE);++i)
	{
		addSquare(0,vol,freq,1);
		addSquare(1,vol,freq+1,0);
		addSquare(2,vol,freq+2,0);
		filter(2+i,0);
		addNoise(vol,2+i,0);
		writeSound();

		//if (freq > 20) freq *= 0.95;
		vol *= 0.95;
	}
}




void playDisruptorExplode()
{
	double fm_freq = 50;

	for(int i=0;i < 25 && PRIORITY(SND_DISRUPTOR_EXPLODE);++i)
	{
		addSinFM(0,HIGH_VOLUME,1,200,1.2*fm_freq,1);
		addSinFM(1,HIGH_VOLUME,3,200,1.6*fm_freq,0);
		fm_freq *= 0.95;
		writeSound();
	}
}




void playPodExplode()
{
	double freq = 100;

	for(int i=0;i < 10 && PRIORITY(SND_POD_EXPLODE);++i)
	{
		addSquare(0,MED_VOLUME,freq,1);
		addSquare(1,MED_VOLUME,freq+10,0);
		filter(i,0);
		addNoise(HIGH_VOLUME,10+i,0);
		writeSound();
		freq *= 0.9;
	}
}




void playGrypperLaunch()
{
	playBellSound(300,3,SND_GRYPPER_LAUNCH);
	playBellSound(300,3,SND_GRYPPER_LAUNCH);
}




void playZombieLaunch()
{
	playBellSound(600,3,SND_ZOMBIE_LAUNCH);
	playBellSound(600,3,SND_ZOMBIE_LAUNCH);
}




void playPodSaved()
{
	int freq;
	int i;

	for(i=0;i < 10 && PRIORITY(SND_POD_SAVED);++i)
	{
		freq = 200 + (int)random() % 200;
		addSin(0,MED_VOLUME,freq,i);
		addSin(1,MED_VOLUME,freq+10,0);
		addSin(2,MED_VOLUME,freq+20,0);
		writeSound();
	}
}




/*** Grypper grabbed pod ***/
void playGrypperGrab()
{
	int freq;
	for(int i=0;i < 5 && PRIORITY(SND_GRYPPER_GRAB);++i)
	{
		freq = 100 - i * 10;
		addSawtooth(0,MED_VOLUME,freq,1);
		addSawtooth(1,MED_VOLUME,freq+2,0);
		filter(i*2,0);
		writeSound();
	}
}




/*** Pod dropped into lair ***/
void playPodCaptured()
{
	for(int freq=300;freq >= 50 && PRIORITY(SND_POD_CAPTURED);freq -= 50)
	{
		addSinFM(0,HIGH_VOLUME,freq,50,(double)freq * 1.2,1);
		writeSound();
	}
}




void playPlasma()
{
	double vol = HIGH_VOLUME;
	int i;

	for(i=0;i < 10 && PRIORITY(SND_PLASMA);++i)
	{
		addNoise(vol,20+i,1);
		addTriangle(0,vol,100-i*5,0);
		vol *= 0.9;
		writeSound();
	}
}




void playCannonExplode()
{
	double vol = HIGH_VOLUME;
	for(int i=1;i < 25 && PRIORITY(SND_CANNON_EXPLODE);++i)
	{
		addTriangle(0,vol,50-i,1);
		addNoise(vol,10+i*3,0);
		addDistortion((short)round(vol*0.8));
		if (i > 15) vol *= 0.75;
		writeSound();
	}
}




void playBonusScore()
{
	int start = 100;
	int freq = start;

	for(int i=0;i < 12 && PRIORITY(SND_BONUS_SCORE);++i)
	{
		addTriangle(0,MED_VOLUME,freq,1);
		addTriangle(1,MED_VOLUME,freq+2,0);
		addTriangle(2,MED_VOLUME,freq+4,0);
		writeSound();
		if (freq >= start + 300)
		{
			start += 100;
			freq = start;
		}
		else freq += 50;
	}
}




void playSmallBombExplode()
{
	double vol = HIGH_VOLUME;

	for(int i=0;i < 20 && PRIORITY(SND_SMALL_BOMB_EXPLODE);++i)
	{
		addSquare(0,vol,10 + random() % 10,1);
		filter(10+i,0);
		addNoise(vol,10+i*3,0);
		writeSound();

		if (i > 15) vol *= 0.7;
	}
}




void playMediumBombExplode()
{
	double vol = HIGH_VOLUME;

	for(int i=0;i < 25 && PRIORITY(SND_MEDIUM_BOMB_EXPLODE);++i)
	{
		addSquare(0,vol,10 + random() % 30,1);
		filter(4+i,0);
		addNoise(vol,2+i*2,0);
		writeSound();

		if (i > 20) vol *= 0.8;
	}
}




void playLargeBombExplode()
{
	double vol = HIGH_VOLUME;

	for(int i=0;i < 40 && PRIORITY(SND_LARGE_BOMB_EXPLODE);++i)
	{
		addSquare(0,vol,10 + random() % 30,1);
		filter(2+i,0);
		addNoise(vol,2+i,0);
		writeSound();

		if (i > 30) vol *= 0.9;
	}
}




void playMissileLaunch()
{
	int i;
	int j;
	double nf;

	for(i=0;i < 2 && PRIORITY(SND_MISSILE_LAUNCH);++i)
	{
		for(j=NOTE_C;j < NOTE_GS && PRIORITY(SND_MISSILE_LAUNCH);++j)
		{
			nf = note_freq[j];
			addSinFM(0,MED_VOLUME,nf,50+i,nf * 1.3,1);
			writeSound();
		}
	}
}




/*** Shudder then explode ***/
void playLairExplode()
{
	double freq = 80;
	double vol = 100;
	int add = 50;
	int i;

	for(i=0;i < 30 && PRIORITY(SND_LAIR_EXPLODE);++i)
	{
		addSinFM(0,HIGH_VOLUME,freq,vol,freq * 1.2,1);
		addSinFM(1,HIGH_VOLUME,freq+3,vol,freq * 2.4,0);
		writeSound();

		freq += 4;
		vol += add;
		if (vol >= 400 || vol <= 100)
		{
			if (add < 10)
				add -= 20;
			else
				add += 20;
			add = -add;
		}
	}

	vol = HIGH_VOLUME;
	for(i=0;i < 40 && PRIORITY(SND_LAIR_EXPLODE);++i)
	{
		freq = random() % (150 - i);
		addTriangle(0,vol,freq,1);
		addTriangle(1,vol,freq+5,0);
		addNoise(vol,i * 3,0);
		if (i > 20) vol *= 0.9;
		writeSound();
	}
}




void playPowerupActivate()
{
	int freq;
	int start;

	freq = start = 400;
	for(int i=0;i < 15 && PRIORITY(SND_POWERUP_ACTIVATE);++i)
	{
		addSinFM(0,MED_VOLUME,freq,50,120,1);
		addSinFM(1,MED_VOLUME,freq+1,50,160,0);
		writeSound();

		freq += 100;
		if (freq == start + 300)
		{
			start += 100;
			freq = start;
		}	
	}
}




void playShipDematerialise()
{
	for(int i=0;i < 10 && PRIORITY(SND_SHIP_DEMATERIALISE);++i)
	{
		addNoise(HIGH_VOLUME,i*10,!i);
		filter(i,!i);
		writeSound();
	}
}




void playShipMaterialise()
{
	for(int i=0;i < 10 && PRIORITY(SND_SHIP_MATERIALISE);++i)
	{
		addNoise(HIGH_VOLUME,100-i*10,!i);
		writeSound();
	}
}




void playAllPodsCaptured()
{
	int i;
	int j;

	for(i=0;i < 5 && PRIORITY(SND_ALL_PODS_CAPTURED);++i)
	{
		for(j=NOTE_A;
		    j < NOTE_GS2 && PRIORITY(SND_ALL_PODS_CAPTURED);j+=2)
		{
			addSawtooth(0,HIGH_VOLUME,note_freq[j],!i);
			addSawtooth(1,HIGH_VOLUME,note_freq[j+1],0);
			addSawtooth(2,HIGH_VOLUME,note_freq[j+2],0);
			filter(i * 3,!i);
			writeSound();
		}
	}
}




void playHighScore()
{
	int freq;
	int i;

	for(i=0,freq=100;i < 30 && PRIORITY(SND_HIGH_SCORE);++i)
	{
		addSin(0,MED_VOLUME,freq,1);
		addSin(1,MED_VOLUME,freq+2,0);
		writeSound();
		freq += 200;
		if (freq == 1100) freq = 100;
	}
}




void playBonusLife()
{
	double freq = 100;
	double vol = MED_VOLUME;
	int i;

	for(i=0;i < 20 && vol > 0 && PRIORITY(SND_BONUS_LIFE);++i)
	{
		addSquare(0,vol,freq,1);
		addSawtooth(1,vol,freq+2,0);
		addSawtooth(2,vol,freq+4,0);
		filter(i*2,0);
		writeSound();

		addSquare(0,vol,freq+50,1);
		addSquare(1,vol,freq+52,0);
		addSquare(2,vol,freq+54,0);
		filter(i*2,0);
		writeSound();

		if (i == 4) freq = 150;
		else
		if (i == 8) freq = 200;
		else
		if (i > 10) vol *= 0.8;
	}
}




void playPowerupAppear()
{
	en_note tune[] =
	{
		NOTE_A,NOTE_B,NOTE_C,
		NOTE_A2,NOTE_B2,NOTE_C2
	};

	for(int i=0;i < 6 && PRIORITY(SND_POWERUP_APPEAR);++i)
		playOrganSound(note_freq[tune[i]],5,SND_POWERUP_APPEAR);
}




void playShipExplode()
{
	int vol;
	int freq;
	int i;

	for(i=0;i < 40 && PRIORITY(SND_SHIP_EXPLODE);i += 2)
	{
		addSquare(0,MED_VOLUME,random() % 50,!i);
		addNoise(MED_VOLUME,(int)random() % 30,0);
		writeSound();
	}

	for(i=0,vol=HIGH_VOLUME;
	    i < 70 && PRIORITY(SND_SHIP_EXPLODE);++i,vol-=100)
	{
		freq = (int)random() % (100 - i);
		addTriangle(0,vol,freq,1);
		addTriangle(1,vol,freq+5,0);
		addNoise(vol,10 + i * 2,0);
		writeSound();
	}
}




void playLevelComplete()
{
	int freq;
	int start;
	int i;

	start = freq = 100;
	for(i=1;i < 41 && PRIORITY(SND_LEVEL_COMPLETE);++i)
	{
		addSawtooth(0,MED_VOLUME,freq,1);
		addSawtooth(1,MED_VOLUME,freq+1,0);
		addSquare(2,MED_VOLUME,freq+2,0);
		filter(5 + abs(i % 10 - 5),!i);
		writeSound();

		if (i && !(i % 3))
		{
			start += 25;
			freq = start;
		}
		else freq += 200;
	}
}




void playGameOver()
{
	en_note tune[] =
	{
		NOTE_C2,NOTE_A2,NOTE_F,NOTE_D,
		NOTE_A2,NOTE_F,NOTE_D,NOTE_B,
		NOTE_F,NOTE_D,NOTE_B,NOTE_A,

		NOTE_A,NOTE_A2,NOTE_A2,NOTE_F,
		NOTE_F,NOTE_F,NOTE_F,NOTE_F,
		NOTE_F,NOTE_F,NOTE_F,NOTE_F,
		NOTE_F,NOTE_F,NOTE_F,NOTE_F
	};
	double freq;
	u_int i;
	int sub;

	for(i=sub=0;
	    i < sizeof(tune)/sizeof(en_note) && PRIORITY(SND_GAME_OVER);++i) 
	{
		if (i <= 10)
			freq = note_freq[tune[i]];
		else
			freq = note_freq[tune[i]] / 2;

		if (i > 20) sub += 30;
		playOrganSound(freq-sub,i,SND_GAME_OVER);
	}
}




/*** Player has pressed 'S' key ***/
void playStart()
{
	for(int i=0;i < 7 && PRIORITY(SND_START);++i)
	{
		if (!(i % 3))
			playBellSound(note_freq[NOTE_D+i],3,SND_START);
		else
			playBellSound(0,3,SND_START); // Introduce a gap
	}
}


/////////////////////////// MID LEVEL SOUNDS ///////////////////////////////


void playBellSound(double freq, int cnt, en_sound snd)
{
	double volume = MED_VOLUME;
	int i;

	for(i=0;i < cnt && PRIORITY(snd);++i)
	{
		addSinFM(0,volume,freq,100,freq*1.2,1);
		addSinFM(1,volume,freq-10,100,freq*1.6,0);
		volume -= (i ? 500 : 3000);

		writeSound();
	}
}




void playOrganSound(double freq, int filt, en_sound snd)
{
	for(int i=0;i < 3 && PRIORITY(snd);++i)
	{
		addSin(0,MED_VOLUME,freq,!filt);
		addSin(1,MED_VOLUME,freq*2+1,0);
		addSin(2,MED_VOLUME,freq/2+2,0);
		filter(filt,!i);

		writeSound();
	}
}


///////////////////////////// LOW LEVEL SOUNDS ///////////////////////////////

void resetSoundChannels()
{
	bzero(sin_ang,sizeof(sin_ang));
	bzero(sin_fm_ang1,sizeof(sin_fm_ang1));
	bzero(sin_fm_ang2,sizeof(sin_fm_ang2));
	bzero(tri_val,sizeof(tri_val));
	bzero(tri_inc,sizeof(tri_inc));
	bzero(sq_vol,sizeof(sq_vol));
	bzero(sq_cnt,sizeof(sq_cnt));
	bzero(saw_val,sizeof(saw_val));
}




void resetSoundBuffer()
{
	bzero(sndbuff,sizeof(sndbuff));
}




void resetEchoBuffer()
{
	bzero(echobuff,sizeof(echobuff));
	echo_write_pos = 0;
}




/*** Play sound as-is or with an echo ***/
void writeSound()
{
	int res;
	int i;
	int j;
	int len;
#ifdef ALSA
	int frames;
#else
	int bytes;
#endif

	if (echo_on)
	{
		// Update echo buffer
		for(i=0,j=echo_write_pos;i < SNDBUFF_SIZE;++i)
		{
			echobuff[j] = (short)((double)echobuff[j] * ECHO_MULT);
			res = (int)echobuff[j] + sndbuff[i];
			CLIP_AND_SET_BUFFER();
			echobuff[j] = sndbuff[i];
			j = (j + 1) % ECHOBUFF_SIZE;
		}
		echo_write_pos = j;
	}

#ifdef ALSA
	/* Write sound data to device. ALSA uses frames (frame = size of format
	   * number of channels so mono 16 bit = 2 bytes, stereo = 4 bytes) */
	for(len = sizeof(sndbuff)/sizeof(short),frames=1;
	    frames > 0 && len > 0;len -= frames)
	{
		/* Occasionally get underrun and need to recover because stream
		   won't work again until you do */
		if ((frames = snd_pcm_writei(handle,sndbuff,len)) < 0)
			snd_pcm_recover(handle,frames,1);
	}
#else
	/* Opensound uses write() so length is done in bytes */
	for(len = sizeof(sndbuff),bytes=0;bytes != -1 && len > 0;len -= bytes)			bytes = write(sndfd,sndbuff,len);
#endif
}




/*** This has channels because starting from the previous angle prevents
     a clicking sound. If we didn't have this then playing 2 sin notes at
     the same time would be a mess. ***/
void addSin(int ch, double vol, double freq, int reset)
{
	double ang_inc;
	int res;
	int i;
	
	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	ang_inc = 360 / (PCM_FREQ / freq);

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = sndbuff[i] + (int)(vol * SIN(sin_ang[ch]));
		CLIP_AND_SET_BUFFER();

		incAngle(sin_ang[ch],ang_inc);
	}
}




/*** As above except with frequency modulation of the sin wave ***/
void addSinFM(
	int ch, 
	double vol, double freq, 
	double fm_vol, double fm_freq, int reset)
{
	double ang_inc;
	double fm_ang_inc;
	int res;
	int i;
	
	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	ang_inc = 360 / ((double)PCM_FREQ / freq);
	fm_ang_inc = (double)360 / ((double)PCM_FREQ / fm_freq);

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = sndbuff[i] + (int)(vol * SIN(sin_fm_ang1[ch] + 
		                         fm_vol * SIN(sin_fm_ang2[ch])));
		CLIP_AND_SET_BUFFER();

		incAngle(sin_fm_ang1[ch],ang_inc);
		incAngle(sin_fm_ang2[ch],fm_ang_inc);
	}
}




/*** Triangle waveform ***/
void addTriangle(int ch, double vol, double freq, int reset)
{
	double inc;
	double period;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = ((double)PCM_FREQ / freq) / 4;
	if (period < 1) period = 1;
	inc = vol / period;
	if (!tri_inc[ch] || fabs(tri_inc[ch]) != inc) tri_inc[ch] = inc;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = sndbuff[i] + (short)tri_val[ch];
		CLIP_AND_SET_BUFFER();

		tri_val[ch] += tri_inc[ch];
		if (tri_val[ch] >= vol)
		{
			tri_val[ch] = vol;
			tri_inc[ch] = -tri_inc[ch];
		}
		else if (tri_val[ch] <= -vol)
		{
			tri_val[ch] = -vol;
			tri_inc[ch] = -tri_inc[ch];
		}
	}
}




/*** Square wave. A proper square wave function would need to do some 
     interpolation to produce every frequency due to PCM sample size limits.
     This one doesn't. ***/
void addSquare(int ch, double vol, double freq, int reset)
{
	int period;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = (int)((PCM_FREQ / freq) / 2);
	if (period < 1) period = 1;
	if (fabs(sq_vol[ch]) != fabs(vol)) sq_vol[ch] = (int)vol;

	for(i=0;i < SNDBUFF_SIZE;++i,++sq_cnt[ch])
	{
		if (!(sq_cnt[ch] % period)) sq_vol[ch] = -sq_vol[ch];
		res = sndbuff[i] + sq_vol[ch];	
		CLIP_AND_SET_BUFFER();
	}
}




/*** Sawtooth waveform - sharp drop followed by gradual climb, then a sharp
     drop again etc etc ***/
void addSawtooth(int ch, double vol, double freq, int reset)
{
	double inc;
	double period;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = (double)(PCM_FREQ / freq) - 1;
	if (period < 1) period = 1;
	inc = (vol / period) * 2;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = sndbuff[i] + (short)saw_val[ch];
		CLIP_AND_SET_BUFFER();

		if (saw_val[ch] >= vol)
			saw_val[ch] = -vol;
		else
			saw_val[ch] += inc;
	}
}




/*** Create noise. 'gap' is the gap between new random values. Between these
     the code interpolates. The larger the gap the more the noise becomes pink 
     noise rather than white since the max frequency drops ***/
void addNoise(double vol, int gap, int reset)
{
	// Static otherwise we get a clicking sound on each call
	static double prev_res = 0;
	double res;
	double inc;
	double target;
	short svol;
	int i;
	int j;

	assert(gap <= SNDBUFF_SIZE);

	if (gap < 1) gap = 1;
	if (reset) resetSoundBuffer();

	svol = (short)vol;
	inc = 0;
	target = 0;
	for(i=0,j=0;i < SNDBUFF_SIZE;++i,j=(j+1) % gap)
	{
		res = prev_res + inc;
		prev_res = res;

		if (!j)
		{
			target = (random() % (svol * 2 + 1)) - svol;
			inc = (target - res) / gap;
		}
		res = sndbuff[i] + res;

		CLIP_AND_SET_BUFFER();
	}
}




/*** Distort by clipping ***/
void addDistortion(short clip)
{
	for(int i=0;i < SNDBUFF_SIZE;++i)
	{
		if (sndbuff[i] > clip) sndbuff[i] = clip;
		else
		if (sndbuff[i] < -clip) sndbuff[i] = -clip;
	}
}




/*** This low pass filter works by setting each point to the average of the 
     previous sample_size points ***/
void filter(int sample_size, int reset)
{
	static double avg[SNDBUFF_SIZE];
	static int a = 0;
	double tot;
	int i;
	int j;

	assert(sample_size < SNDBUFF_SIZE);
	if (sample_size < 1) sample_size = 1;

	if (reset)
	{
		bzero(avg,sizeof(avg));
		a = 0;
	}

	for(i=0;i < SNDBUFF_SIZE;++i,a = (a + 1) % sample_size)
	{
		avg[a] = sndbuff[i];
		for(j=0,tot=0;j < sample_size;++j) tot += avg[j];
		sndbuff[i] = (short)(tot / sample_size);
	}
}

#endif
