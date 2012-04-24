#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <time.h>
#include <rfftw.h>
#include "SDL.h"
#include "font.h"
#include "debug.h"
#include "serial.h"
#include "config.h"
#include "pong.h"
#include "p300.h"

// drawing stuff
SDL_Surface *surface;
unsigned int point_buffer[X_SIZE], running_buffer[X_SIZE], xline=0;

// logging stuff
FILE* logFile = NULL;

// FFT stuff
fftw_real in[FFT_SIZE], out[FFT_SIZE], power_spectrum[FFT_SIZE/2+1];
rfftw_plan p;

void Quit(int returnCode)
{
	SDL_ShowCursor(SDL_ENABLE);

    printf("Quiting...\n");
    closeSerial();
    if (logFile) fclose(logFile);
    rfftw_destroy_plan(p);

    FreeFont();
    SDL_Quit();
    exit(returnCode);
}

int resizeWindow(int width, int height)
{
    GLfloat ratio;

    if (height == 0) height = 1;

    ratio = (GLfloat)width / (GLfloat)height;

    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    gluOrtho2D(0, X_SIZE, 0, ADC_RESOLUTION);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    return TRUE;
}

void handleKeyPress(SDL_keysym *keysym)
{
    switch (keysym->sym)
	{
	case SDLK_ESCAPE:
	    Quit(0);
	    break;
	case SDLK_F1:
	    SDL_WM_ToggleFullScreen(surface);
	    break;
	default:
	    break;
	}

    return;
}

int initGL(void)
{
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    return TRUE;
}

int drawGLScene(void)
{
    static GLint T0 = 0, Frames = 0;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, 0.0f);

    unsigned int s = point_buffer[xline] = readSerialValue();
    for (int i=1; i<X_SIZE; i++)
    {
        running_buffer[i-1] = running_buffer[i];
    }
    running_buffer[X_SIZE-1] = point_buffer[xline];

    // log the output to a file
    fprintf(logFile, "%d,%d\n", time(NULL), point_buffer[xline]);

    // send sample to P300 buffer
    p300AddSample(s);

    xline++;
    xline %= X_SIZE;

        for (int i=0; i<FFT_SIZE; i++)
        {
            in[i] = running_buffer[(X_SIZE-FFT_SIZE)+i] - ADC_RESOLUTION/2;
        }

        rfftw_one(p, in, out);
        power_spectrum[0] = out[0]*out[0];  /* DC component */
        for (int i=1; i < (FFT_SIZE+1)/2; i++)  /* (k < N/2 rounded up) */
        {
            power_spectrum[i] = out[i]*out[i] + out[FFT_SIZE-i]*out[FFT_SIZE-i];
        }

        if (FFT_SIZE % 2 == 0) /* N is even */
        {
            power_spectrum[FFT_SIZE/2] = out[FFT_SIZE/2]*out[FFT_SIZE/2];  /* Nyquist freq. */
        }

    // draw FFT
    glBegin(GL_LINES);
        for (int i=0; i<=FFT_SIZE/2; i++)
        {
            glColor4f(0,1,0,1);
            glVertex2i(i*2, 0);
            glVertex2i(i*2, power_spectrum[i]/FFT_SCALE_FACTOR);
            glVertex2i(i*2+1, 0);
            glVertex2i(i*2+1, power_spectrum[i]/FFT_SCALE_FACTOR);
        }
    glEnd();

    // draw points here
    glBegin(GL_POINTS);
        for (int i=0; i<X_SIZE; i++)
        {
            glColor4f(1,0,0,1);
            glVertex2f(i, running_buffer[i]*((float)SCREEN_HEIGHT/ADC_RESOLUTION));
        }
    glEnd();

    glBegin(GL_LINES);
        for (int i=1; i<X_SIZE; i++)
        {
            glColor4f(1,0,0,1);
            glVertex2f(i, running_buffer[i-1]*((float)SCREEN_HEIGHT/ADC_RESOLUTION));
            glVertex2f(i, running_buffer[i]*((float)SCREEN_HEIGHT/ADC_RESOLUTION));
        }
    glEnd();

    float delta=0, theta=0, alpha=0, beta=0, gamma=0, mu=0, total=0;

    for (int i=0; i<(FFT_SIZE/2); i++) total += power_spectrum[i];
    for (int i=0; i<4; i++) delta += power_spectrum[i];
    for (int i=4; i<=8; i++) theta += power_spectrum[i];
    for (int i=8; i<=13; i++) alpha += power_spectrum[i];
    for (int i=14; i<=30; i++) beta += power_spectrum[i];
    for (int i=30; i<=100; i++) gamma += power_spectrum[i];
    for (int i=8; i<=13; i++) mu += power_spectrum[i];
    delta /= total; theta /= total; alpha /= total; beta /= total; gamma /= total; mu /= total;

    // do BCI paddle control
    const float ALPHA_MIN=0.01f, ALPHA_MAX=0.04f;
    posy = (SCREEN_HEIGHT - 64) * (alpha - ALPHA_MIN) / (ALPHA_MAX - ALPHA_MIN);
    // TODO: low-pass filter

/*    // 0.03 BELOW THAT MOVE PADDLE DOWN, OTHERWISE MOVE UP
    const float MU_THRESHOLD = 0.03f;
    if (mu < MU_THRESHOLD)
        paddle1yvel = 0.1f;
    else
        paddle1yvel = -0.1f; */

    // update and draw pong game state
	pongUpdateAndRender();

    // update and draw P300 state machine
    p300UpdateAndRender();

    // draw text
    glPrint(format("Delta=%f; Theta=%f; Alpha=%f", delta, theta, alpha), 10, 10, blue);
    glPrint(format("Beta=%f; Gamma=%f; Mu=%f", beta, gamma, mu), 10, 30, blue);

    SDL_GL_SwapBuffers();

    Frames++;
    {
        GLint t = SDL_GetTicks();
        if (t - T0 >= 5000)
        {
            GLfloat seconds = (t - T0) / 1000.0;
            GLfloat fps = Frames / seconds;
            printf("%d frames in %g seconds = %g FPS\n", Frames, seconds, fps);
            T0 = t;
            Frames = 0;
        }
    }

    return TRUE;
}

int main(int argc, char **argv)
{
    for (int i=0; i<X_SIZE; i++)
    {
        point_buffer[i] = ADC_RESOLUTION/2; //0;
        running_buffer[i] = ADC_RESOLUTION/2; //0;
    }

    int videoFlags;
    int done = FALSE;
    SDL_Event event;
    const SDL_VideoInfo *videoInfo;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
	    fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
	    Quit(1);
	}

#ifdef WIN32
	freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
#endif

    videoInfo = SDL_GetVideoInfo();

    if (!videoInfo)
	{
	    fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
	    Quit(1);
	}

    videoFlags  = SDL_OPENGL;
    videoFlags |= SDL_GL_DOUBLEBUFFER;
    videoFlags |= SDL_HWPALETTE;
    videoFlags |= SDL_RESIZABLE;

    if (videoInfo->hw_available)
        videoFlags |= SDL_HWSURFACE;
    else
        videoFlags |= SDL_SWSURFACE;

    if (videoInfo->blit_hw)
        videoFlags |= SDL_HWACCEL;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    surface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, videoFlags);

    if (!surface)
	{
	    fprintf(stderr,  "Video mode set failed: %s\n", SDL_GetError());
	    Quit(1);
	}

    initGL();

    resizeWindow(SCREEN_WIDTH, SCREEN_HEIGHT);

    if (TTF_Init() == -1) 
    {
        printf("Unable to initialize SDL_ttf: %s \n", TTF_GetError());
        Quit(1);
    }

    // init font
    InitFont();

    // open serial
    openSerial();

    // open log file
    logFile = fopen(LOG_FILENAME, "w");

    if (logFile == NULL)
    {
        fprintf(stderr, "gl_plot main(): Can't open log output file %s!\n", LOG_FILENAME);
        Quit(1);
    }

    // init FFT
    printf("Initializing FFTW 2...\n");
    p = rfftw_create_plan(FFT_SIZE, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
    printf("Done!\n");

    // init pong
    printf("Initializing pong...\n");
	srand((unsigned)time(0)); // Init Random Number Generator
    pongInit();

    // init P300
    p300init();

    while (!done)
	{
	    while (SDL_PollEvent(&event))
		{
		    switch(event.type)
			{
			case SDL_VIDEORESIZE:
			    surface = SDL_SetVideoMode(event.resize.w, event.resize.h, SCREEN_BPP, videoFlags);
			    if (!surface)
				{
				    fprintf(stderr, "Could not get a surface after resize: %s\n", SDL_GetError());
				    Quit(1);
				}
			    resizeWindow(event.resize.w, event.resize.h);
			    break;
			case SDL_KEYDOWN:
				handleKeyPress(&event.key.keysym);
				pongHandleKeyDown(&event.key.keysym);
				p300HandleKeyDown(&event.key.keysym);
				break;
			case SDL_KEYUP:
				pongHandleKeyUp(&event.key.keysym);
				p300HandleKeyUp(&event.key.keysym);
				break;
			case SDL_QUIT:
			    done = TRUE;
			    break;
			default:
			    break;
			}
		}

		drawGLScene();
	}

    Quit(0);

    return(0);
}
