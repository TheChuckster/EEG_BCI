#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <time.h>
#include <string>
//#include <rfftw.h>
#include "font.h"
#include "debug.h"
#include "config.h"

#include "p300.h"

#define TRAINING_DATA_FILENAME "p300_data"
#define TESTING_DATA_FILENAME "p300_data.t"

#define P300_READY 0
#define P300_TRAINING 1
#define P300_TESTING 2

unsigned int p300_state = P300_READY;

#define NUM_COLORS 5
#define NUM_TRIALS 3
#define BUFFER_SIZE 512

const float color_choices[NUM_COLORS][3] = { {1,0,0}, {0,1,0}, {0,0,1}, {1,1,0}, {1,0,1} };
const std::string color_names[NUM_COLORS] = { "Red", "Green", "Blue", "Yellow", "Purple" };
unsigned int trialBuffer[NUM_TRIALS][NUM_COLORS][BUFFER_SIZE];
float targetBuffer[BUFFER_SIZE], nonTargetBuffer[BUFFER_SIZE], testBuffers[NUM_COLORS][BUFFER_SIZE];

int current_color = -1, trainingTarget = -1; // black
unsigned int bufferPtr = 0;
unsigned int current_trial = 0;
bool color_histogram[NUM_COLORS], classified_histogram[NUM_COLORS];
bool displayBuffers = false;

bool p300StartTrial(bool clear);
void p300trainSVM();

void p300setTrainingTarget()
{
    trainingTarget = 3;//rand() % NUM_COLORS;
}

void p300init()
{
    for (int i=0; i<NUM_COLORS; i++)
    {
        color_histogram[i] = classified_histogram[i] = false;
        for (int j=0; j<BUFFER_SIZE; j++)
        {
            testBuffers[i][j] = 0;
        }
    }

    for (int i=0; i<BUFFER_SIZE; i++)
    {
        targetBuffer[i] = nonTargetBuffer[i] = 0;
    }

    p300setTrainingTarget();
    p300trainSVM();
}

void p300trainSVM()
{
    printf("TODO: Training SVM!\n");

    // load binary data file, use it to train the SVM and get coefficients
}

void p300addTrainingExample()
{
    printf("Adding training example buffer!\n");

    // so we know trainingTarget, so iterate through all trials and add the buffers where training target is the color to targetBuffer and add the buffers where training target is NOT the color to non-target buffer, then divide targetBuffer by NUM_TRIALS (since each trial has exactly one target) and divide nonTargetBuffer by (NUM_TRIALS*(NUM_COLORS-1))

    // clear buffers first!
    for (int i=0; i<BUFFER_SIZE; i++)
    {
        targetBuffer[i] = nonTargetBuffer[i] = 0;
    }

    for (int i=0; i<NUM_TRIALS; i++)
    {
        for (int j=0; j<NUM_COLORS; j++)
        {
            for (int k=0; k<BUFFER_SIZE; k++)
            {
                if (j == trainingTarget)
                {
                    targetBuffer[k] += trialBuffer[i][j][k];
                } else {
                    nonTargetBuffer[k] += trialBuffer[i][j][k];
                }
            }
        }
    }

    for (int i=0; i<BUFFER_SIZE; i++)
    {
        targetBuffer[i] /= NUM_TRIALS;
        nonTargetBuffer[i] /= (NUM_TRIALS*(NUM_COLORS-1));
    }

    // then scale the data
    for (int i=0; i<BUFFER_SIZE; i++) // goes from 0 to ADC_RESOLUTION initially so subtract ADC_RESOLUTION/2 then divide by ADC_RESOLUTION/2 to get it in [-1, +1] range
    {
        targetBuffer[i] = (targetBuffer[i]-ADC_RESOLUTION/2)/(ADC_RESOLUTION/2);
        nonTargetBuffer[i] = (nonTargetBuffer[i]-ADC_RESOLUTION/2)/(ADC_RESOLUTION/2);
    }

    // then append it to a training SVM text file
    FILE *fp = fopen(TRAINING_DATA_FILENAME, "a");
    if (fp == NULL)
    {
        printf("p300addTrainingExample(): Unable to open training data file %s!\n", TRAINING_DATA_FILENAME);
        return;
    }

    // first do non-target buffer
    fprintf(fp, "-1 ");
    for (int i=0; i<BUFFER_SIZE; i++)
    {
        fprintf(fp, "%d:%f ", i+1, nonTargetBuffer[i]);
    }
    fprintf(fp, "\n");

    // now do target buffer
    fprintf(fp, "+1 ");
    for (int i=0; i<BUFFER_SIZE; i++)
    {
        fprintf(fp, "%d:%f ", i+1, targetBuffer[i]);
    }
    fprintf(fp, "\n");

    fclose(fp);

    // average the trials, append the trial to a binary data file, also call p300trainSVM() ?
    // TODO: update targetBuffer, nonTargetBuffer
}

void p300testAndReport()
{
    printf("Testing and reporting!\n");

    // so we have a bunch of colors with multiple trials each, so we need to go through each color and then each trial and add all the data for each color into the appropriate test buffer then scale all of the test buffers by NUM_TRIALS, then spit each one into the test data file (after scaling the data, of course)

    // clear buffers first!
    for (int i=0; i<NUM_COLORS; i++)
    {
        for (int j=0; j<BUFFER_SIZE; j++)
        {
            testBuffers[i][j] = 0;
        }
    }

    for (int i=0; i<NUM_TRIALS; i++)
    {
        for (int j=0; j<NUM_COLORS; j++)
        {
            for (int k=0; k<BUFFER_SIZE; k++)
            {
                testBuffers[j][k] += trialBuffer[i][j][k];
            }
        }
    }

    for (int i=0; i<NUM_COLORS; i++)
    {
        for (int j=0; j<BUFFER_SIZE; j++)
        {
            testBuffers[i][j] /= NUM_TRIALS;
        }
    }

    // then scale the data
    for (int i=0; i<NUM_COLORS; i++)
    {
        for (int j=0; j<BUFFER_SIZE; j++)
        {
            testBuffers[i][j] = (testBuffers[i][j]-ADC_RESOLUTION/2)/(ADC_RESOLUTION/2);
        }
    }

    // then write it to a testing SVM text file
    FILE *fp = fopen(TESTING_DATA_FILENAME, "w");
    if (fp == NULL)
    {
        printf("p300testAndReport(): Unable to open testing data file %s!\n", TESTING_DATA_FILENAME);
        return;
    }

    // do all test buffers with 0 for classification label
    for (int i=0; i<NUM_COLORS; i++)
    {
        fprintf(fp, "0 ");
        for (int j=0; j<BUFFER_SIZE; j++)
        {
            fprintf(fp, "%d:%f ", j+1, testBuffers[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    // so take the current trial and classify it using libSVM, then set the classification histogram
    // TODO: update targetBuffer, nonTargetBuffer (???)
}

void p300AddSample(unsigned int s)
{
    if (p300_state != P300_READY)
    {
        if (bufferPtr < BUFFER_SIZE) // still room left in the buffer
        {
            trialBuffer[current_trial][current_color][bufferPtr++] = s;
        } else { // buffer filled up
            // start a new trial unless we are finished then set p300_state to READY
            if (p300StartTrial(false)) // finished
            {
                // do SVM stuff here
                if (p300_state == P300_TRAINING)
                {
                    p300addTrainingExample();
                    p300setTrainingTarget(); // done training, set a new target
                } else if (p300_state == P300_TESTING)
                    p300testAndReport();
                else
                    printf("p300AddSample(): Unknown state %d\n", p300_state);

                // reset state
                p300_state = P300_READY;
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

                // might as well reset the state variables just to be safe
                current_color = -1;
                bufferPtr = 0;
                current_trial = 0;
            }
        }
    }
}

// returns true if all finished
bool p300StartTrial(bool clear)
{
    // reset buffer pointer
    bufferPtr = 0;

    if (clear)
    {
        // set trial to zero and clear histogram
        current_trial = 0;
        for (int i=0; i<NUM_COLORS; i++) color_histogram[i] = false;
    }

    // check if done with trial
    bool increment_trial = true;
    for (int i=0; i<NUM_COLORS; i++)
    {
        if (color_histogram[i] == false)
        {
            increment_trial = false;
            break;
        }
    }

    // done with trial
    if (increment_trial)
    {
        if (current_trial < NUM_TRIALS - 1) // check if finished all together
            current_trial++;
        else
            return true;

        for (int i=0; i<NUM_COLORS; i++) color_histogram[i] = false;
    }

    // pick a random color, set it in the histograph
    bool picked = false;
    while (!picked)
    {
        unsigned int next_color = rand() % NUM_COLORS;
        if (!color_histogram[next_color] && next_color != current_color)
        {
            current_color = next_color;
            color_histogram[current_color] = true;
            picked = true;
        }
    }

    glClearColor(color_choices[current_color][0], color_choices[current_color][1], color_choices[current_color][2], 0.0f);
    return false;
}

void p300UpdateAndRender()
{
    if (displayBuffers) // TODO: test and fix!!!
    {
        glBegin(GL_LINES);
            for (int i=1; i<BUFFER_SIZE; i++)
            {
                glColor4f(0,1,0,1);
                glVertex2f(i, targetBuffer[i-1]/2.0f*((float)SCREEN_HEIGHT/ADC_RESOLUTION));
                glVertex2f(i, targetBuffer[i]/2.0f*((float)SCREEN_HEIGHT/ADC_RESOLUTION));
            }
        glEnd();

        glBegin(GL_LINES);
            for (int i=1; i<BUFFER_SIZE; i++)
            {
                glColor4f(0,0,1,1);
                glVertex2f(i, nonTargetBuffer[i-1]/2.0f*((float)SCREEN_HEIGHT/ADC_RESOLUTION)+SCREEN_HEIGHT/2.0f);
                glVertex2f(i, nonTargetBuffer[i]/2.0f*((float)SCREEN_HEIGHT/ADC_RESOLUTION)+SCREEN_HEIGHT/2.0f);
            }
        glEnd();
    }

    // draw status text:
    std::string classified_str = "Classified: { ";
    // Classified: { Red Blue }
    for (int i=0; i<NUM_COLORS; i++)
    {
        if (classified_histogram[i])
        {
            classified_str += color_names[i] + " ";
        }
    }
    classified_str += " }";

    // add mode string to status text
    switch (p300_state)
    {
    case P300_READY:
        classified_str = "P300 Ready: " + classified_str;
        break;
    case P300_TRAINING:
        classified_str = "P300 Training Mode: " + classified_str;
        break;
    case P300_TESTING:
        classified_str = "P300 Testing Mode: " + classified_str;
        break;
    default:
        classified_str = "P300 Unknown State! " + classified_str;
    }

    // display training target too
    classified_str += " - Training Target: " + color_names[trainingTarget];

    glPrint(classified_str.c_str(), 10, 50, blue);
}

void p300HandleKeyDown(SDL_keysym *keysym)
{
	switch(keysym->sym)
	{
    case SDLK_F2: // switch to "training mode"
        if (p300_state == P300_READY)
        {
            p300_state = P300_TRAINING;
            p300StartTrial(true);
        }

        break;
    case SDLK_F3: // switch to "testing mode"
        if (p300_state == P300_READY)
        {
            p300_state = P300_TESTING;
            p300StartTrial(true);
        }

        break;
    case SDLK_F5:
        displayBuffers = !displayBuffers;
        break;
    default:
        break;
	}
}

void p300HandleKeyUp(SDL_keysym *keysym)
{
/*	switch(keysym->sym)
	{
	case SDLK_UP:
		paddle2yvel = 0;
		break;
	case SDLK_DOWN:
		paddle2yvel = 0;
		break;
	case SDLK_a:
		paddle1yvel = 0;
		break;
	case SDLK_z:
		paddle1yvel = 0;
		break;
	} */
}

