#include "globals.h"


/*** Try to activate the given quantity of the given object type regardless of 
     how many of this type already are activated ***/
bool activateObjects(en_object_type type, int num, cl_object *activator)
{
	int o;
	int cnt = 0;

	if (num < 1) return false;

	FOR_ALL_OBJECTS(o)
	{
		if (object[o]->type == type && 
		    object[o]->stage == STAGE_INACTIVE)
		{
			object[o]->activate(activator);
			if (++cnt == num) break;
		}
	}
	return (cnt == num);
}




/*** Inc score, play sound and find a free bonus text object then activate ***/
void activateBonusText(int bonus, cl_object *obj)
{
	incScore(bonus);
	playSound(SND_BONUS_SCORE);

	for(int i=TEXT_BONUS_SCORE;i < NUM_TEXTS;++i)
	{
		if (!textobj[i]->active)
		{
			textobj[i]->activateBonus(bonus,obj);
			return;
		}
	}
}




/*** Set the lives to the specific value ***/
void setLives(int lv)
{
	lives = lv;
	sprintf(lives_text,"%d",lives);
}




/*** Set high score value and string ***/
void setHighScore(int hs)
{
	high_score = hs;
	sprintf(high_score_text,"%06u",high_score % 1000000);
}




/*** Set the score to the specific value ***/
void setScore(int sc)
{
	score = sc % 1000000;
	sprintf(score_text,"%06u",score % 1000000);

	if (score > high_score && game_stage == GAME_STAGE_PLAY)
	{
		setHighScore(score);
		if (!activated_high_score_text)
		{
			textobj[TEXT_HIGH_SCORE]->activate();
			activated_high_score_text = true;
			playSound(SND_HIGH_SCORE);
		}
	}
	if (score >= bonus_life_score)
	{
		// Limit to under 10 else it won't fit on screen
		if (lives < 9)
		{
			setLives(lives+1);
			textobj[TEXT_BONUS_LIFE]->activate();
			playSound(SND_BONUS_LIFE);	
		}
		bonus_life_score += BONUS_LIFE_INC;
	}
}




/*** Update by given amount ***/
void incScore(int val)
{
	setScore(score + val);

}
