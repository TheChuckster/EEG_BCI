#ifndef FONT_H_
#define FONT_H_

#include <string>
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "config.h"

void glPrint(std::string message, int x, int y, SDL_Color color);
void InitFont();
void FreeFont();

void glBegin2D();
void glEnd2D();

extern SDL_Color red, blue;

#endif
