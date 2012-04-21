#include <math.h>
#include "font.h"

#define FONT_SIZE 16
SDL_Color red = {255,0,0}, blue = {0,0,255};

void glBegin2D()
{
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

    glOrtho(0.0, (GLdouble)SCREEN_WIDTH, (GLdouble)SCREEN_HEIGHT, 0.0, 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void glEnd2D()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

static int power_of_two(int input)
{
	int value = 1;

	while (value < input)
	{
		value <<= 1;
	}
	return value;
}

GLuint SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord)
{
	GLuint texture;
	int w, h;
	SDL_Surface *image;
	SDL_Rect area;
	Uint32 saved_flags;
	Uint8  saved_alpha;

	w = power_of_two(surface->w);
	h = power_of_two(surface->h);

	texcoord[0] = 0.0f;			/* Min X */
	texcoord[1] = 0.0f;			/* Min Y */
	texcoord[2] = (GLfloat)surface->w / w;	/* Max X */
	texcoord[3] = (GLfloat)surface->h / h;	/* Max Y */

	image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
#else
			0xFF000000,
			0x00FF0000,
			0x0000FF00,
			0x000000FF
#endif
		       );

	if (image == NULL) return 0;

	saved_flags = surface->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
	saved_alpha = surface->format->alpha;
	if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA) SDL_SetAlpha(surface, 0, 0);

	/* Copy the surface into the GL texture image */
	area.x = 0;
	area.y = 0;
	area.w = surface->w;
	area.h = surface->h;
	SDL_BlitSurface(surface, &area, image, &area);

	/* Restore the alpha blending attributes */
	if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA) SDL_SetAlpha(surface, saved_flags, saved_alpha);

	/* Create an OpenGL texture for the image */
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
	SDL_FreeSurface(image); /* No longer needed */

	return texture;
}

TTF_Font *font;

void InitFont()
{
	font = TTF_OpenFont("fonts/arial.ttf", FONT_SIZE);
	if (font == NULL)
	{
		printf("TTF_OpenFont: %s\n", SDL_GetError());
		return;
	}
}

void FreeFont()
{
	TTF_CloseFont(font);
}

void glPrint(std::string message, int x, int y, SDL_Color color)
{
	int w, h;
	GLfloat texcoord[4];
	GLfloat texMinX, texMinY;
	GLfloat texMaxX, texMaxY;
	SDL_Surface *text;
	GLuint fonttexture;

	text = TTF_RenderText_Blended(font, message.c_str(), color);
	fonttexture = SDL_GL_LoadTexture(text, texcoord);
	SDL_FreeSurface(text);

	w = text->w;
	h = text->h;

	texMinX = texcoord[0];
	texMinY = texcoord[1];
	texMaxX = texcoord[2];
	texMaxY = texcoord[3];

	glBegin2D();
	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, fonttexture);
	glColor3f(1.0,1.0,1.0);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(texMinX, texMinY); glVertex2i(x,   y  );
		glTexCoord2f(texMaxX, texMinY); glVertex2i(x+w, y  );
		glTexCoord2f(texMinX, texMaxY); glVertex2i(x,   y+h);
		glTexCoord2f(texMaxX, texMaxY); glVertex2i(x+w, y+h);
	glEnd();
	glEnable(GL_LIGHTING);
	glEnd2D();

	glDeleteTextures(1, &fonttexture);
}

