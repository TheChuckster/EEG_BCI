#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctime>
#include <malloc.h>
#include <string>
#include "config.h"
#include "glyphs.h"

void drawsprite(int x, int y, float r, float g, float b);
void drawpaddlesprite(int x, int y, float r, float g, float b);
void updatepaddle1();
void updatepaddle2();
void drawglyph(int num, int x, int y, float r, float g, float b);
void drawline();
void drawscore();
void ResetVelocity();
int randomNum();

#define PADDLE_WIDTH 16
#define PADDLE_HEIGHT 128

unsigned int score1=0, score2=0;
float posy=SCREEN_HEIGHT/2, paddle1yvel=0;
float pos2y=SCREEN_HEIGHT/2, paddle2yvel=0;

float ballposx=SCREEN_WIDTH/2-8, ballposy=SCREEN_HEIGHT/2-8;
float ballvelx=0, ballvely=0;

const float vel_threshold = 0.1f;

const unsigned char sprite[] =
{
0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0
};

void updateball()
{
	ballposx += ballvelx;
	ballposy += ballvely;

	if (ballposy <= 0)
	{
		ballvely = -ballvely;
		ballposy = 0;
	} else if (ballposy >= SCREEN_HEIGHT - 16) {
		ballvely = -ballvely;
		ballposy = SCREEN_HEIGHT - 16;
	}

	if (ballposx <= 0)
	{
		ballvelx = -ballvelx;
		ballposx = 0;
	} else if (ballposx >= SCREEN_WIDTH - 16) {
		ballvelx = -ballvelx;
		ballposx = SCREEN_WIDTH - 16;
	}

	// COLLISION DETECTION
	// front checking for both paddles
	if ((ballposx >= 75) && (ballposx <= 75+PADDLE_WIDTH) && (ballposy >= posy-(PADDLE_HEIGHT-1)) && (ballposy <= posy+PADDLE_HEIGHT-1))
	{
		ballposx = 75+(PADDLE_WIDTH-1);
		ballvelx = -ballvelx;

		if (ballvelx > 1)
			ballvelx += vel_threshold;
		else if (ballvelx < 1)
			ballvelx -= vel_threshold;

		if (ballvely > 1)
			ballvely += vel_threshold;
		else if (ballvely < 1)
			ballvely -= vel_threshold;
	}

	if ((ballposx <= (SCREEN_WIDTH-91)) && (ballposx >= (SCREEN_WIDTH-91)-PADDLE_WIDTH) && (ballposy >= pos2y-(PADDLE_WIDTH-1)) && (ballposy <= pos2y+PADDLE_HEIGHT-1))
	{
		ballposx = (SCREEN_WIDTH-91)-PADDLE_WIDTH;
		ballvelx = -ballvelx;

		if (ballvelx > 1)
			ballvelx += vel_threshold;
		else if (ballvelx < 1)
			ballvelx -= vel_threshold;

		if (ballvely > 1)
			ballvely += vel_threshold;
		else if (ballvely < 1)
			ballvely -= vel_threshold;
	}

	// back checking for both paddles
	if ((ballposx == 75-PADDLE_WIDTH) && (ballposy >= posy-(PADDLE_WIDTH-1)) && (ballposy <= posy+PADDLE_HEIGHT-1))
		ballvelx = -ballvelx;

	if ((ballposx == (SCREEN_WIDTH-91)+PADDLE_WIDTH) && (ballposy >= pos2y-(PADDLE_WIDTH-1)) && (ballposy <= pos2y+PADDLE_HEIGHT-1))
		ballvelx = -ballvelx;

	// Top and bottom checking for paddle1
	if ((ballposy == posy-PADDLE_WIDTH) && (ballposx > 75-(PADDLE_WIDTH-1)) && (ballposx < 75+(PADDLE_WIDTH-1)))
		ballvely = -ballvely;

	if ((ballposy == posy+PADDLE_HEIGHT) && (ballposx > 75-(PADDLE_WIDTH-1)) && (ballposx < 75+(PADDLE_WIDTH-1)))
		ballvely = -ballvely;

	// Top and bottom checking for paddle 2
	if ((ballposy == pos2y-PADDLE_WIDTH) && (ballposx > (SCREEN_WIDTH-91)-(PADDLE_WIDTH-1)) && (ballposx < (SCREEN_WIDTH-91)+(PADDLE_WIDTH-1)))
		ballvely = -ballvely;

	if ((ballposy == pos2y+PADDLE_HEIGHT) && (ballposx > (SCREEN_WIDTH-91)-(PADDLE_WIDTH-1)) && (ballposx < (SCREEN_WIDTH-91)+(PADDLE_WIDTH-1)))
		ballvely = -ballvely;

	if (ballposx == 0)
	{
		score1++;

		ResetVelocity();
	}

	if (ballposx == SCREEN_WIDTH-16)
	{
		score2++;

		ResetVelocity();
	}
}

void ResetVelocity()
{
	if (ballvelx > 1)
		ballvelx = 1;
	else if (ballvelx < 1)
		ballvelx = -1;

	if (ballvely > 1)
		ballvely = 1;
	else if (ballvely < 1)
		ballvely = -1;
}

void pongUpdateAndRender()
{
	int tick = SDL_GetTicks();

	updateball();
	updatepaddle1();
	updatepaddle2();

	drawsprite(ballposx, ballposy, 0, 1, 0);

	drawpaddlesprite(75, posy, 1, 0, 0);
	drawpaddlesprite((SCREEN_WIDTH-91), pos2y, 0, 0, 1);

	drawscore();
	drawline();
}

void drawscore()
{
	drawglyph(score1, SCREEN_WIDTH/2+10, 25, 1, 1, 0);
	drawglyph(score2, SCREEN_WIDTH/2-12-10, 25, 1, 1, 0);
}

void drawglyph(int num, int x, int y, float r, float g, float b)
{
	const unsigned char *glyph;

	switch (num)
	{
	case 0:
		glyph = num0;
		break;
	case 1:
		glyph = num1;
		break;
	case 2:
		glyph = num2;
		break;
	case 3:
		glyph = num3;
		break;
	case 4:
		glyph = num4;
		break;
	case 5:
		glyph = num5;
		break;
	case 6:
		glyph = num6;
		break;
	case 7:
		glyph = num7;
		break;
	case 8:
		glyph = num8;
		break;
	case 9:
		glyph = num9;
		break;
	default:
		glyph = num0;
		break;
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

	glColor4f(r,g,b,1.0f);
	glBegin(GL_POINTS);
		for (int i = 0, c = 0; i < 20; i++)
		{
			for (int j = 0; j < 12; j++, c++)
			{
				if (glyph[c])
				{
					glVertex2i(x+j,y+i);
				}
			}
		}
	glPopMatrix();
}

void drawsprite(int x, int y, float r, float g, float b)
{
	glBegin(GL_POINTS);
	glColor4f(r,g,b,1.0f);

	for (int i = 0, c = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++, c++)
		{
			if (sprite[c])
			{
				glVertex2i(x+j,y+i);
			}
		}
	}
	glEnd();
}

#define LINE_SPACING 5
void drawline()
{
	glBegin(GL_LINES);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // white divider line in the middle
	for (int i=0; i<SCREEN_HEIGHT; i+=LINE_SPACING*2)
	{
		glVertex2i(SCREEN_WIDTH/2, i);
		glVertex2i(SCREEN_WIDTH/2, i+LINE_SPACING);
	}

	glEnd();
}

void pongInit()
{
	printf("SCREEN_HEIGHT=%d\n", SCREEN_HEIGHT);
	int i = randomNum();

	if (i == 0)
		ballvelx = -1;
	else
		ballvelx = 1;

	i = randomNum();

	if (i == 0)
		ballvely = -1;
	else
		ballvely = 1;
}

void drawpaddlesprite(int x, int y, float r, float g, float b)
{
	glBegin(GL_POINTS);
	glColor4f(r,g,b,1.0f);
	for (int i = 0, c = 0; i < PADDLE_HEIGHT; i++)
	{
		for (int j = 0; j < PADDLE_WIDTH; j++, c++)
		{
			glVertex2i(x+j,y+i);
		}
	}
	glEnd();
}

void pongHandleKeyDown(SDL_keysym *keysym)
{
	switch(keysym->sym)
	{
	case SDLK_UP:
		paddle2yvel = -1;
		break;
	case SDLK_DOWN:
		paddle2yvel = 1;
		break;
	/*case SDLK_a:
		paddle1yvel = -1;
		break;
	case SDLK_z:
		paddle1yvel = 1;
		break; */
	default:
		break;
	}
}

void pongHandleKeyUp(SDL_keysym *keysym)
{
	switch(keysym->sym)
	{
	case SDLK_UP:
		paddle2yvel = 0;
		break;
	case SDLK_DOWN:
		paddle2yvel = 0;
		break;
/*	case SDLK_a:
		paddle1yvel = 0;
		break;
	case SDLK_z:
		paddle1yvel = 0;
		break; */
    default:
        break;
	}
}

void updatepaddle1()
{
    //printf("updating...\n");
	posy += paddle1yvel;

	if (posy < 0)
		posy = 0;
	else if (posy > (SCREEN_HEIGHT - 64))
		posy = SCREEN_HEIGHT - 64;
}

void updatepaddle2()
{
	pos2y += paddle2yvel;

	if (pos2y < 0)
		pos2y = 0;
	else if (pos2y > (SCREEN_HEIGHT - 64))
		pos2y = SCREEN_HEIGHT - 64;
}

int randomNum() // Obtain a random integer between defined range
{
	int range=(1-0)+1; // Calculate range

	// Use rand()
	int retval=0+int(range*rand()/(RAND_MAX + 1.0));

	return retval; // Return the number
}
