/*******************************************************************/
/*  main.cpp Version 0.16.0                                        */
/*                                                                 */
/*  It can:                                                        */
/*   - Move in all directions                                      */
/*   - rotate counter-clockwise                                    */
/*   - It's mobility has been corrected                            */
/*   - Improved repetition timing, it can be adjusted in the       */
/*     keyHist.repetitionTime field.                               */
/*   - Make a completed line dissappear                            */
/*   - Complete a game (window just closes)                        */
/*                                                                 */
/*  It cannot:                                                     */
/*   - Put a game ending label before closing screen               */
/*   - Keep score                                                  */
/*   - Show us the next piece that's coming. Tetris games usually  */
/*     display it in a rectangle located to the right of the stage */
/*   - When the scoring system works, increment gravity speed      */
/*                                                                 */
/*  BUGS:                                                          */
/*   - It still has that bug where after a certain number of       */
/*     tetris pieces are placed, the stage dissapears.             */
/*   - Once a piece lands, the sustaintment mechanism does not     */
/*     deactivate                                                  */
/*                                                                 */
/*  TO-DO: (take it step by step)                                  */
/*     (DONE)Let's have only one vertex object. So we have to merge*/
/*     AcumulatedShapes and g_vertex_buffer_data into one          */
/*     as well as ColorAcumulatedShapes and g_color_buffer_data    */
/*     (DONE)Let's have only one MVP object and be careful with    */
/*     the buffers, remember to put vertex buffers into buffer 0   */
/*     and color buffers into buffer 1, this is the only way since */
/*     the vertex and fragment shader files written in TFSL were   */
/*     written that way.                                           */
/*   - Let's list the following 4 pieces in the right side of the  */
/*     stage.                                                      */
/*   - It's not fun, but let's put the text and line counter on it */
/*     already.                                                    */
/*******************************************************************/

/*******************************************************************/
/*  Libraries                                                      */
/*******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>

// NEW!!  User defined library
extern "C" {
    #include "inc/math_functions.h"
    #include "inc/globalVariables.h"
}
/*******************************************************************/
/*  Macros  New!!  defined in user header files                    */
/*******************************************************************/

/*******************************************************************/
/*  Global variables                                               */
/*******************************************************************/
		                        /***********************************
		                         * Get the very first time of them all
		                         **********************************/
double firstTime = glfwGetTime(); //is called only once

		                        /***********************************
	  	                         * Camera, MVP's                   
		                         **********************************/
// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
// Camera matrix
glm::mat4 View = glm::lookAt(
	glm::vec3(5,10, 30), // Camera is at (4,3,-3), in World Space
	glm::vec3(5,10,  0), // and looks at this point
	glm::vec3(0, 1,  0)  // Head is up (set to 0,-1,0 to look upside-down)
);
// Model matrix : an identity matrix (model will be at the origin)
glm::mat4 Model = glm::mat4(1.0f);

                                                                 /*********************************
								  * 'Boolean' indicating if there's  
								  *  an active tetris figure          
								  *********************************/
//FALSE=0, TRUE=1
//The initial value of 'landed' is set to true so we get a fresh
//tetris figure at the beginning of the game.
int landed = TRUE;

                                                                 /*********************************
								  * Stage-size vertex matrix so our
								  * figures land here
								  *********************************/
//40 squares per section, 4 sections, 6 vertexes per square, 3 coordenates per vertex
GLfloat AcumulatedShapes[3240+18*3]; //static const GLfloat g_vertex_buffer_data[] = { //18*3 
GLfloat colorAcumulatedShapes[3240+18*3]; //static const GLfloat g_color_buffer_data[] = { //18*3

                                                                 /*********************************
								  * Movility variables. Regulate!!
								  *********************************/
float gravitySpeed = 1.0f; // in coordenate units/second
	//Key sustaining effect
float sustainedKeyTime = 0.5f;

                                                                 /*********************************
								  * blueprint3.c global variables
								  *********************************/
char layout[18][10+1] = {
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //17
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //16
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //15
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //14
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //13
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //12
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //11
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, //10
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 9
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 8
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 7
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 6
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 5
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 4
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 3
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 2
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}, // 1
		{ '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '\0'}  // 0
		// 0    1    2    3    4    5    6    7    8    9
};

                                                                 /*********************************
								  * So we don't have to declare everytime
								  * inside the for statement
								  *********************************/
int i = 0;
int j = 0;
                                                                 /*********************************
								  * keypress history to improve
								  * movility
								  *********************************/
struct key_motion_history
{
	int lastKey = GLFW_KEY_UNKNOWN; //Can take: Any key value defined in the library glfw3.h
									// ../../test/OpenGL-tutorial_v0014_33/external/glfw-3.0.3/include/GLFW/glfw3.h
	int consecutive = 0;
	double t1 = 0.0;
	double dt = 0.0;
	float repetitionTime = 0.1f;
} keyHist;
                                                                 /*********************************
								  * blinking integer will tell us 
								  * whether or not a line completition
								  * display process is active
								  *********************************/
int blinking = 0;
int completedLines[4] = {400,400,400,400}; //up to 4 complete lines at once, and
                                           //that's a great play. '400' is default value

/*******************************************************************/
/*  Prototypes                                                     */
/*******************************************************************/
//NEW!! line removal functions!!
void getCompletedLines(void);
GLfloat getRGBvalue(char tetrisPieceType, int RGBindex);

void keyPress(int key);
void executeMotionForKey(int key);

void printTetrisPieceOnLayout(char *matrixName);
void removeFromStage(void);
void paintStageBlack(void);

int calcDownMotion(void);
int calcHorizontalMotion(int direction);
int calcFirstTetrisPiece(void);
void moveTetrisPiece(void);
int calcRotation(char tetrisPieceType);
/*******************************************************************/
/*  Main funtion                                                   */
/*******************************************************************/
int main(void)
{
		                         /**********************************
		                         * Initialize everything           *
		                         **********************************/
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "My first Tetris-like game", NULL, NULL);
	if (window == NULL){
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

		                        /***********************************
		                         * Matrices, more than one must be *
		                         * declared to get more shapes     *
		                         **********************************/
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("C:\\Users\\carreong\\Documents\\I-Training\\OpenGLTutorials\\OpenGL-tutorial_v0014_33\\tutorial04_colored_cube\\TransformVertexShader.vertexshader", "C:\\Users\\carreong\\Documents\\I-Training\\OpenGLTutorials\\OpenGL-tutorial_v0014_33\\tutorial04_colored_cube\\ColorFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	//Handle acumulated tetris figures no longer in the game
	//GLuint MatrixID2 = glGetUniformLocation(programID, "MVP2");


		                        /***********************************
		                         * 2D text code, borrowed from
		                         * tutorial11.cpp
		                         ***********************************/

	GLuint vertexbuffer;
	//glGenBuffers(1, &vertexbuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint colorbuffer;
	//glGenBuffers(1, &colorbuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	// -- NEW!! AcumulatedShapes initialization so it spans entire stage -- 
		                        /***********************************
		                         * Initialize the entire stage
		                         * Since layout looks like this:
		                         * {
								 * w { [0][0] [0][1] [0][2] ... [0][9] } w
								 * w { [1][0] [1][1] [1][2] ... [1][9] } w
								 *       .
								 *       .
								 *       .
								 * w { [n][0] [n][1] [n][2] ... [n][9] } w
		                         * }
								 * where: w = wall
								 *        n = 17
		                         **********************************/
	//Initialize AcumulatedShapes which will be our STAGE, really
	GLfloat c = 0.0f;
	GLfloat r = 0.0f;
	for ( int row = 0; row < 18; row++)
	{
		c = 0.0f;
		for ( int col = 0; col < 10; col++)
		{
		//This the STAGE's first block
		//  0.0f,	18.0f,	0.0f,
		//  1.0f,	17.0f,	0.0f,
		//  0.0f,	17.0f,	0.0f,
		//
		//  0.0f,	18.0f,	0.0f,
		//  1.0f,	17.0f,	0.0f,
		//  1.0f,	18.0f,	0.0f,

		//so the STAGE's next blocks should be:
		//  col  ,	18-row, 0.0f,
		//  col+1,	17-row,	0.0f,
		//  col  ,	17-row,	0.0f,
		//
		//  col  ,	18-row, 0.0f,
		//  col+1,	17-row,	0.0f,
		//  col+1,	18-row,	0.0f,

			AcumulatedShapes[ 0+180*row+18*col] = c     ; AcumulatedShapes[ 1+180*row+18*col] = 18.0f-r; AcumulatedShapes[ 2+180*row+18*col] = 0.0f;
			AcumulatedShapes[ 3+180*row+18*col] = c+1.0f; AcumulatedShapes[ 4+180*row+18*col] = 17.0f-r; AcumulatedShapes[ 5+180*row+18*col] = 0.0f;
			AcumulatedShapes[ 6+180*row+18*col] = c     ; AcumulatedShapes[ 7+180*row+18*col] = 17.0f-r; AcumulatedShapes[ 8+180*row+18*col] = 0.0f;

			AcumulatedShapes[ 9+180*row+18*col] = c     ; AcumulatedShapes[10+180*row+18*col] = 18.0f-r; AcumulatedShapes[11+180*row+18*col] = 0.0f;
			AcumulatedShapes[12+180*row+18*col] = c+1.0f; AcumulatedShapes[13+180*row+18*col] = 17.0f-r; AcumulatedShapes[14+180*row+18*col] = 0.0f;
			AcumulatedShapes[15+180*row+18*col] = c+1.0f; AcumulatedShapes[16+180*row+18*col] = 18.0f-r; AcumulatedShapes[17+180*row+18*col] = 0.0f;

			c += 1.0f;
		}
		r += 1.0f;
	}
		                        /***********************************
		                         * Container walls and floor with
                                         * corresponding color 
		                         **********************************/
	//wall 1
	AcumulatedShapes[3239+3* 0+1]= -1.0f; AcumulatedShapes[3239 +3* 0+ 2] = 17.0f; AcumulatedShapes[3239 +3* 0+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 1+1]= -1.0f; AcumulatedShapes[3239 +3* 1+ 2] =  0.0f; AcumulatedShapes[3239 +3* 1+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 2+1]=  0.0f; AcumulatedShapes[3239 +3* 2+ 2] =  0.0f; AcumulatedShapes[3239 +3* 2+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 3+1]= -1.0f; AcumulatedShapes[3239 +3* 3+ 2] = 17.0f; AcumulatedShapes[3239 +3* 3+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 4+1]=  0.0f; AcumulatedShapes[3239 +3* 4+ 2] = 17.0f; AcumulatedShapes[3239 +3* 4+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 5+1]=  0.0f; AcumulatedShapes[3239 +3* 5+ 2] =  0.0f; AcumulatedShapes[3239 +3* 5+ 3] = 0.0f;
	//wall 2
	AcumulatedShapes[3239+3* 6+1]= 11.0f; AcumulatedShapes[3239 +3* 6+ 2] = 17.0f; AcumulatedShapes[3239 +3* 6+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 7+1]= 11.0f; AcumulatedShapes[3239 +3* 7+ 2] =  0.0f; AcumulatedShapes[3239 +3* 7+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 8+1]= 10.0f; AcumulatedShapes[3239 +3* 8+ 2] =  0.0f; AcumulatedShapes[3239 +3* 8+ 3] = 0.0f;
	AcumulatedShapes[3239+3* 9+1]= 11.0f; AcumulatedShapes[3239 +3* 9+ 2] = 17.0f; AcumulatedShapes[3239 +3* 9+ 3] = 0.0f;
	AcumulatedShapes[3239+3*10+1]= 10.0f; AcumulatedShapes[3239 +3*10+ 2] = 17.0f; AcumulatedShapes[3239 +3*10+ 3] = 0.0f;
	AcumulatedShapes[3239+3*11+1]= 10.0f; AcumulatedShapes[3239 +3*11+ 2] =  0.0f; AcumulatedShapes[3239 +3*11+ 3] = 0.0f;
    //floor
	AcumulatedShapes[3239+3*12+1]= -1.0f; AcumulatedShapes[3239 +3*12+ 2] =  0.0f; AcumulatedShapes[3239 +3*12+ 3] = 0.0f;
	AcumulatedShapes[3239+3*13+1]= 11.0f; AcumulatedShapes[3239 +3*13+ 2] = -1.0f; AcumulatedShapes[3239 +3*13+ 3] = 0.0f;
	AcumulatedShapes[3239+3*14+1]= -1.0f; AcumulatedShapes[3239 +3*14+ 2] = -1.0f; AcumulatedShapes[3239 +3*14+ 3] = 0.0f;
	AcumulatedShapes[3239+3*15+1]= -1.0f; AcumulatedShapes[3239 +3*15+ 2] =  0.0f; AcumulatedShapes[3239 +3*15+ 3] = 0.0f;
	AcumulatedShapes[3239+3*16+1]= 11.0f; AcumulatedShapes[3239 +3*16+ 2] = -1.0f; AcumulatedShapes[3239 +3*16+ 3] = 0.0f;
	AcumulatedShapes[3239+3*17+1]= 11.0f; AcumulatedShapes[3239 +3*17+ 2] =  0.0f; AcumulatedShapes[3239 +3*17+ 3] = 0.0f;

    for( i=1; i<=18*3; i++)
    {
        colorAcumulatedShapes[3240 + i] = 1.000f;
    }

	paintStageBlack();

	srand (time(NULL));

		                        /***********************************
		                         * Let's fix:
                                 * list the following 4 pieces in the right side of the  
                                 * stage.
		                         **********************************/
    //since tetris pieces are characters, we need a string then.
    char randomTetrisPieces[5+1];
    //Fill this string with random characters for the first time.

		                        /***********************************
		                         * THE LOOP
		                         **********************************/
	do{
		                        /***********************************
		                         * clear the screen and call shader*
		                         **********************************/
		// Clear the screen                               ; // Use our shader
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glUseProgram(programID);

		                        /***********************************
		                         * NEW!! the 'if' statement is not new, I
		                         * just need to make a comment here, this
								 * control statement serves as the, 
								 * how to call it, 'engine' or 'heart
								 * of the engine' if you will. It is the 
								 * most important logical structure that will
								 * take us to different stages, parts of a game.
								 * Refer to notes or to the file:
								 * architechture_blueprint_08182015.txt
								 **********************************
								 * NEW!! logic structure designed in files:
                                         * 'blueprint5.cpp'  and  'blueprint5-2.txt'
		                         **********************************/
		if(landed == TRUE)
        {
            if(blinking == 10)
            {
			//case "No active piece, line-blinking process completed":
              // REQUIRES:
              //    landed = 1
              //    sizeof(completedLines) > 0
              //    blinking = 10
				// FROM:                  TO:
				//  ---  ---             
				// |   ||   |           
				//  ---  ---           
				//  ---  ---  ---  ---    ---  --- 
				// |   ||   ||   ||   |  |   ||   |
				//  ---  ---  ---  ---    ---  --- 
                int colNum = 0;
                for( i = 3; i >= 0; i--)//for elements in 'completedLines'
                {
                    if(completedLines[i] == 400) continue;
                    //The for statement below only copies the higher row values only to the
                    //one completed line. The rest of them are missing.
                    for(j=completedLines[i]; j>0; j--)
                    {
                        for( colNum = 0; colNum < 10; colNum++)//for ten columns in a row
                        {
                        //since completedLines[i] is our row number, and completedLines will turn
                        //out to be sorted in ascending order, the completed line that is found to
                        //be lower in the stage will be eliminated first, which is the ideal
                        //sequence for us.
                            if( !strcmp(layout[j] ,"__________") && !strcmp(layout[j-1] ,"__________") ) continue;
                            for(int till18 = 0; till18<18; till18+=3)
                            {
                              //colorAcumulatedShapes[ [0-17]+v+180*rowNum+18*COL] = colorAcumulatedShapes[i+v+180*(rowNum-1)+18*COL]
                                //colorAcumulatedShapes[ till18+0+180*j+18*colNum ] = colorAcumulatedShapes[ till18+0+180*j-1+18*colNum ];
                                colorAcumulatedShapes[ till18+0+180*j+18*colNum ] = getRGBvalue(layout[j-1][colNum],0);
                                //colorAcumulatedShapes[ till18+1+180*j+18*colNum ] = colorAcumulatedShapes[ till18+1+180*j-1+18*colNum ];
                                colorAcumulatedShapes[ till18+1+180*j+18*colNum ] = getRGBvalue(layout[j-1][colNum],1);
                                //colorAcumulatedShapes[ till18+2+180*j+18*colNum ] = colorAcumulatedShapes[ till18+2+180*j-1+18*colNum ];
                                colorAcumulatedShapes[ till18+2+180*j+18*colNum ] = getRGBvalue(layout[j-1][colNum],2);
                            }
                            layout[j][colNum] = layout[j-1][colNum];
                        }
                    }
                }
                //remember to include the new zeroth, row!
                if( !strcmp(layout[0] ,"__________") )
                {
                    for( colNum = 0; colNum < 10; colNum++)//for ten columns in a row
                    {
                        for(int till18 = 0; till18<18; till18+=3)
                        {
                            colorAcumulatedShapes[ i+0+180*0+18*colNum ] = 0.0f;
                            colorAcumulatedShapes[ i+1+180*0+18*colNum ] = 0.0f;
                            colorAcumulatedShapes[ i+2+180*0+18*colNum ] = 0.0f;
                        }
                        layout[0][ colNum ] = '_';
                    }
                }
                //reset both 'blinking' and 'completedLines'
                blinking = 0;
                for( i=0;i<4;i++)
                    completedLines[i] = 400;
            }
			//case "No active piece, line blinking process on course":
            else if( blinking > 0 ) //since blinking == 10 was tested above, this condition alone
            {                        //guarantees 0 < blinking < 10
              // REQUIRES:
              //    landed = 1
              //    sizeof(completedLines) > 0
              //    0 < blinking <= 10
				if( (blinking & 1) == 1) //if 'blinking' holds an odd value
				{
					//modify colorAcumulatedShapes accordingly
                        //go to that specific part of colorAcumulatedShapes and
                        //turn everything black
                    for(i = 0; i < 4; i++)//for elements in 'completedLines'
                    {
                        if(completedLines[i] == 400) continue;
                        for(int colNum = 0; colNum < 10; colNum++)//for ten columns in a row
                        {
                            for(int till18 = 0; till18<18; till18+=3)
                            {
                          //colorAcumulatedShapes[ i+v+180*ROW              +18*COL ]
                                colorAcumulatedShapes[ till18+0+180*completedLines[i]+18*colNum ] = 0.0f;
                                colorAcumulatedShapes[ till18+1+180*completedLines[i]+18*colNum ] = 0.0f;
                                colorAcumulatedShapes[ till18+2+180*completedLines[i]+18*colNum ] = 0.0f;
                            }
                        }
                    }
				}
                else
                {
                    //go for even number completed-line color
                    // restore original colors from chars still found in layout[][]
                    for( i = 0; i < 4; i++)//for elements in 'completedLines'
                    {
                        if(completedLines[i] == 400) continue;
                        for(int colNum = 0; colNum < 10; colNum++)//for ten columns in a row
                        {
                            for(int till18 = 0; till18<18; till18+=3)
                            {
                          //colorAcumulatedShapes[ i+vertexNum+180*ROW+18*COL ]
                                colorAcumulatedShapes[ till18+0+180*completedLines[i]+18*colNum ] = getRGBvalue(layout[completedLines[i]][colNum],0);
                                colorAcumulatedShapes[ till18+1+180*completedLines[i]+18*colNum ] = getRGBvalue(layout[completedLines[i]][colNum],1);
                                colorAcumulatedShapes[ till18+2+180*completedLines[i]+18*colNum ] = getRGBvalue(layout[completedLines[i]][colNum],2);
                            }
                        }
                    }
				}
                blinking++;
            }

			//case "Piece just landed, did the game end?":
            else if( calcFirstTetrisPiece() != 4 )
            {
              // REQUIRES:
              //    landed = 1
              //    calcFirstTetrisPiece() != 4, in other words, couldn't initialize a new 
              //       tetris piece
                //display ending message
                break; //!!
            }
			//case "Piece just landed, either initialize another one or start blinking process":
            else //blinking must be zero then
            {
              // REQUIRES:
              //    landed = 1
              //    calcFirstTetrisPiece() == 4, we continue on the game
              //    blinking == 0
                //maybe this functionality can be added to the function
                //that lands the tetris piece in the first place. Anyways:
                // Find the  number of completed lines if any:
                /*GLOBAL VAR*/ 
                getCompletedLines();
                if ( completedLines[0] == 400 )
                {        
			        //No key is currently present, so reset all sustaining values
			        keyHist.lastKey = GLFW_KEY_UNKNOWN; keyHist.t1 = 0.0f;
			        keyHist.consecutive = 0; keyHist.dt = 0.0f;
			        landed = FALSE;
                    moveTetrisPiece();
		        }
                else
                    blinking = 1; //blinking process activated !!
			//break;
            }
        }
		else
		{ 
			double timeDiff = glfwGetTime() - firstTime;
			// Right key
			if ((glfwGetKey( window, GLFW_KEY_RIGHT ) == GLFW_PRESS) )
			{
				keyPress(GLFW_KEY_RIGHT);
			}
			// Left key
			else if ((glfwGetKey( window, GLFW_KEY_LEFT ) == GLFW_PRESS) )
			{
				keyPress(GLFW_KEY_LEFT);
			}
			// Up key rotation
			else if ((glfwGetKey( window, GLFW_KEY_UP ) == GLFW_PRESS) )
			{
				keyPress(GLFW_KEY_UP);
			}
			// Down key
			else if ((glfwGetKey( window, GLFW_KEY_DOWN ) == GLFW_PRESS) )
			{
				keyPress(GLFW_KEY_DOWN);
			}
			else 
			{
				//No key is currently present, so reset all sustaining values
				keyHist.lastKey = GLFW_KEY_UNKNOWN; keyHist.t1 = 0.0f;
				keyHist.consecutive = 0; keyHist.dt = 0.0f;
		    }
			if(timeDiff >= gravitySpeed) //gravity
			{
				calcDownMotion(); //This one can land current tetrisPiece
				firstTime += timeDiff;
				moveTetrisPiece();
			}
		}
		                        /***********************************
		                         * NEW!! Walls, floor, and everything
		                         **********************************/
		//First MVP is ready. It is static so no change here
        glm::mat4 MVP = Projection * View * Model; 
		// Define container walls and floor matrix
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// 1rst attribute buffer : vertices of the containing walls and floor
		glEnableVertexAttribArray(0);

		//Define a vertexBuffer for acumulated shapes:
		//GLuint vertexbuffer;
		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(AcumulatedShapes), AcumulatedShapes, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : colors of the containing walls and floor
		glEnableVertexAttribArray(1);
		//GLuint colorbuffer;
		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(colorAcumulatedShapes), colorAcumulatedShapes, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw containing walls and floor
		glDrawArrays(GL_TRIANGLES, 0, 1098 ); // 3240 = 1080*3, 1080*3+18*3 = 1098*3 

		                        /***********************************
		                         * Wrap up this screen             *
		                         **********************************/
		glDisableVertexAttribArray(0); //vertices from both walls and AcumulatedShapes
		glDisableVertexAttribArray(1); //colors from both walls and AcumulatedShapes

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	//PUBLISH CURRENT SCREEN!!
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

		                        /***********************************
		                         * GAME ENDS!!                     *
		                         **********************************/
	// Cleanup VBO and shader
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

/******************************************************/
/* NEW Function definitions                           */
/******************************************************/

/************************************************
 * Function 
 *
 *      int calcFirstTetrisPiece(void)
 *
 * Will put data of one of the 7 tetris figures into
 * the tetris.curr matrix if those newly created
 * coordenates are not blocked by some previous
 * ttris figure already in place. Returns the number of
 * characters of the tetris.curr matrix allowed to be 
 * to be placed into the 'layout' matrix. 
 ************************************************/
int calcFirstTetrisPiece(void)
{
	//Our return value
	int allowedBlocks;
    //get a number between 0 and 5
	int randomNumber = rand() % 6;

    //return one of the 7 shapes with a case statement:
    switch(randomNumber)
    {
        case 0 :                  //col                       //row
			tetrisPiece.curr[0][1] = 5; tetrisPiece.curr[0][0] = 0;
			tetrisPiece.curr[1][1] = 6; tetrisPiece.curr[1][0] = 0;
			tetrisPiece.curr[2][1] = 5; tetrisPiece.curr[2][0] = 1;
			tetrisPiece.curr[3][1] = 6; tetrisPiece.curr[3][0] = 1;
			tetrisPiece.type = 'q';
            break; 
        case 1 :                  //col                       //row
			tetrisPiece.curr[0][1] = 6; tetrisPiece.curr[0][0] = 1; //center
			tetrisPiece.curr[1][1] = 6; tetrisPiece.curr[1][0] = 0; //  0,-1
			tetrisPiece.curr[2][1] = 5; tetrisPiece.curr[2][0] = 1; // -1, 0
			tetrisPiece.curr[3][1] = 7; tetrisPiece.curr[3][0] = 1; //  1, 0
			tetrisPiece.type = 't';
            break; 
        case 2 :                  //col                       //row
			tetrisPiece.curr[0][1] = 5; tetrisPiece.curr[0][0] = 0; //center
			tetrisPiece.curr[1][1] = 4; tetrisPiece.curr[1][0] = 0;
			tetrisPiece.curr[2][1] = 6; tetrisPiece.curr[2][0] = 0;
			tetrisPiece.curr[3][1] = 7; tetrisPiece.curr[3][0] = 0;
			tetrisPiece.type = 'l';
            break; 
        case 3 :                  //col                       //row
			tetrisPiece.curr[0][1] = 6; tetrisPiece.curr[0][0] = 1; //center
			tetrisPiece.curr[1][1] = 5; tetrisPiece.curr[1][0] = 1;
			tetrisPiece.curr[2][1] = 6; tetrisPiece.curr[2][0] = 0;
			tetrisPiece.curr[3][1] = 7; tetrisPiece.curr[3][0] = 0;
			tetrisPiece.type = 's';
            break; 
        case 4 :                  //col                       //row
			tetrisPiece.curr[0][1] = 5; tetrisPiece.curr[0][0] = 1; //center
			tetrisPiece.curr[1][1] = 6; tetrisPiece.curr[1][0] = 1;
			tetrisPiece.curr[2][1] = 4; tetrisPiece.curr[2][0] = 0;
			tetrisPiece.curr[3][1] = 5; tetrisPiece.curr[3][0] = 0;
			tetrisPiece.type = 'z';
            break; 
        case 5 :                  //col                       //row
			tetrisPiece.curr[1][1] = 5; tetrisPiece.curr[1][0] = 1;
			tetrisPiece.curr[0][1] = 6; tetrisPiece.curr[0][0] = 1; //center
			tetrisPiece.curr[2][1] = 7; tetrisPiece.curr[2][0] = 1;
			tetrisPiece.curr[3][1] = 7; tetrisPiece.curr[3][0] = 0;
			tetrisPiece.type = 'L';
            break; 
        default :                  //col                       //row
			tetrisPiece.curr[1][1] = 5; tetrisPiece.curr[1][0] = 1;
			tetrisPiece.curr[0][1] = 6; tetrisPiece.curr[0][0] = 1; //center
			tetrisPiece.curr[2][1] = 7; tetrisPiece.curr[2][0] = 1;
			tetrisPiece.curr[3][1] = 5; tetrisPiece.curr[3][0] = 0;
			tetrisPiece.type = 'i';
    }
                                                                 /*********************************
								  * Erase the .next coordenates since we
								  * are restarting everything
								  *********************************/
	tetrisPiece.next[0][0] = 400; //interpreted as NULL 

                                                                 /*********************************
								  * Now that the piece has been defined,
								  * place it in the canvas if posible
								  *********************************/
	//count the number of blocks that can be placed
	for(allowedBlocks=0; allowedBlocks<4; allowedBlocks++)
	{
		if(layout[tetrisPiece.curr[allowedBlocks][0]][tetrisPiece.curr[allowedBlocks][1]]!='_') //if layout[row][col] is not free
		{
			break; //one of the blocks can't be placed
		}
	}
	return allowedBlocks;
}
/************************************************
 * Function 
 *
 *      int calcDownMotion(void);
 *
 * Will place the current tetrisPiece character type 
 * one place below their tetris.curr position in the 
 * layout matrix Returns the number of tetrisPiece
 * blocks that are free to move.
 ************************************************/
int calcDownMotion(void)
{
	int allowedBlocks = 0;  //at the end, this must be equal to 4
                                                                 /*********************************
								  * If any block of our tetrisPiece
								  * is located in row 17, we should
								  * end the function and return FALSE
								  *********************************/
	if( getMinY() == 17 ) 
	{
		landed = TRUE;
		return 0;
	}
                                                                 /*********************************
								  * If any other character exists
								  * directly below one of the blocks
								  * of our tetris figure, we should
								  * end the function and return FALSE
								  *********************************/
	for(i=0; i < 4; i++)
    {
		if(layout[tetrisPiece.curr[i][0]+1][tetrisPiece.curr[i][1]] != '_')
		{
			//one of our blocks has a non clear value below
			//there is a chance that the space below this block 'i',
			//also belongs to our current tetrisPiece, let's find out:
			for(j=0; j<4; j++)
    		{
				//  _ _
				// |_|_|<-- are we this piece? tetrisPiece.curr[i][1] 
				// |_|_|
				//    ^.... tetrisPiece.curr[i][1] == tetrisPiece.curr[j][1] ?

				if(j==i)
				{
					//No point in comparing this block against itself
					continue;
				}
				if( //If block 'i' is directly above block 'j'
					tetrisPiece.curr[j][0] == tetrisPiece.curr[i][0]+1 &&
					tetrisPiece.curr[j][1] == tetrisPiece.curr[i][1] 
				  )
				{
					//this is good the space below tetrisPiece 'i' is
					//one of it's siblings
					allowedBlocks++;
					break;
				}
			}
		}
		else
		{
			//there's a free space directly below this tetrisPiece block
			allowedBlocks++;
		}
	}
	if(allowedBlocks == 4)//do all 4 blocks in current figure have
	{                     //free space below to move to?
		//Yes, move this piece down.
		tetrisPiece.next[0][0] = tetrisPiece.curr[0][0] + 1;
		tetrisPiece.next[1][0] = tetrisPiece.curr[1][0] + 1;
		tetrisPiece.next[2][0] = tetrisPiece.curr[2][0] + 1;
		tetrisPiece.next[3][0] = tetrisPiece.curr[3][0] + 1;

		tetrisPiece.next[0][1] = tetrisPiece.curr[0][1];
		tetrisPiece.next[1][1] = tetrisPiece.curr[1][1];
		tetrisPiece.next[2][1] = tetrisPiece.curr[2][1];
		tetrisPiece.next[3][1] = tetrisPiece.curr[3][1];
	}
	else
	{
		landed = TRUE;
		//printf("allowedBlocks = %d\n",allowedBlocks);
	}

	return allowedBlocks;
}

/************************************************
 * Function 
 *
 *      void moveTetrisPiece(void)
 *
 * Will place our approved tetris piece values into
 * the layout matrix. Will check tetrisPiece.b and 
 * tetrisPiece.a to know which one has values on
 * it and will prioritize 'a' over 'b'.
 *
 * TODO: I don't exactly like this function. I think its
 *       processes can be performed in main, so we call
 * 		 all functions from main and not within the functions.
 ************************************************/
void moveTetrisPiece(void)
{
                                                                 /*********************************
								  * If there's an 'next' value, use
								  * it and then exit 
								  *********************************/
	if(tetrisPiece.next[0][0] != 400)
	{
								 /*********************************
								  * Values where found in the 'next' position
								  * matrix, that will be our new location.
								  * Remember to remove chars on the layout
								  * that we don't need
								  *********************************/
		for(i=0; i<4; i++)
		{
			//remove from old coordenates from layout
			layout[tetrisPiece.curr[i][0]][tetrisPiece.curr[i][1]] = '_';
		}
		//remove old coordenates from stage
		removeFromStage();

		for(i=0; i<4; i++)
		{
			//place in new coordenates
			layout[tetrisPiece.next[i][0]][tetrisPiece.next[i][1]] = tetrisPiece.type;
			//Current coordenates are now the coordenates we used before
			tetrisPiece.curr[i][0] = tetrisPiece.next[i][0];
			tetrisPiece.curr[i][1] = tetrisPiece.next[i][1];
		}
		//Place the tetrisPiece in its new place.
		printTetrisPieceOnLayout("next");

		return;
	}
                                                                 /*********************************
								  * If the 'next' values have not been 
								  * set, this is probably a new tetris 
								  * piece that has 'current' values only
								  *********************************/
	else if(tetrisPiece.curr[0][0] != 400)
	{
		for(i=0; i<4; i++)
		{
			layout[tetrisPiece.curr[i][0]][tetrisPiece.curr[i][1]] = tetrisPiece.type;
		}
		//Place this new tetrisPiece in the stage via the current coordenates
		printTetrisPieceOnLayout("current");
	}
	else
	{
		fprintf(stderr,"Couldn't move the tetris piece. Undefined motion coordenates.\n");
	}
	return;
}
/************************************************
 * Function 
 *
 *      int calcHorizontalMotion(void);
 *
 * Will place the current tetrisPiece character type 
 * one place below their tetris.curr position in the 
 * layout matrix Returns the number of tetrisPiece
 * blocks that are free to move.
 ************************************************/
int calcHorizontalMotion(int direction)
{
	int allowedBlocks = 0;  //at the end, this must be equal to 4
                                                                 /*********************************
								  * Determine the direction in 
								  * which our tetrisPiece will move 
								  * based on the 'direction' parameter
								  *********************************/
	signed int rightOrLeft;        //direction on the 'x' axis 
	if(direction<=0)
	{
		rightOrLeft = -1; //zero is to the left
		if( getMinX() == 0 ) 
		{
			return 0;
		}
	}
	else
	{
		rightOrLeft = 1;  //any positive number translates to 'right'
		if( getMaxX() == 9 ) 
		{
			return 0;
		}
	}
                                                                 /*********************************
								  * If we are trying to move horizontally
								  * in the direction of one of the walls,
								  * end the function and return FALSE
								  *********************************/
	for(i=0; i < 4; i++)
    {
		if(layout[tetrisPiece.curr[i][0]][tetrisPiece.curr[i][1]+rightOrLeft] != '_')
		{
			//one of our blocks has a non clear value in the direction of 'rightOrLeft' 
			//there is a chance that the taken space belongs to our very own tetrisPiece
			//let's find out:
			for(j=0; j<4; j++)
    		{
				//  _ _
				// |_|_|
				// |_|_|<-- tetrisPiece.curr[j][1]
				//  ^.... are we this piece? tetrisPiece.curr[i][1]

				if(j==i)
				{
					//No point in comparing this block against itself
					continue;
				}
				if( //If block 'i' is next to block 'j'
					tetrisPiece.curr[j][0] == tetrisPiece.curr[i][0] &&
					tetrisPiece.curr[j][1] == tetrisPiece.curr[i][1]+rightOrLeft
				  )
				{
					//The space next to tetrisPiece 'i' is its sibling, block 'j'
					allowedBlocks++;
					break;
				}
			}
		}
		else
		{
			//there's an available space next to block 'i'
			allowedBlocks++;
		}
	}
	if(allowedBlocks == 4)//do all 4 blocks in current figure have
	{                     //free space below to move to?
		//Yes, move this piece in 'rightOrLeft' direction
		tetrisPiece.next[0][0] = tetrisPiece.curr[0][0];
		tetrisPiece.next[1][0] = tetrisPiece.curr[1][0];
		tetrisPiece.next[2][0] = tetrisPiece.curr[2][0];
		tetrisPiece.next[3][0] = tetrisPiece.curr[3][0];

		tetrisPiece.next[0][1] = tetrisPiece.curr[0][1]+rightOrLeft;
		tetrisPiece.next[1][1] = tetrisPiece.curr[1][1]+rightOrLeft;
		tetrisPiece.next[2][1] = tetrisPiece.curr[2][1]+rightOrLeft;
		tetrisPiece.next[3][1] = tetrisPiece.curr[3][1]+rightOrLeft;
	}
	else
	{
		printf("calcHorizontalMotion: allowedBlocks = %d\n",allowedBlocks);
	}

	return allowedBlocks;
}

/************************************************
 * Function 
 *
 *      int calcRotation(char tetrisPieceType)
 *
 * Rotates tetrisPiece. Checks if spaces are free.
 * Returns number of blocks that will fall in valid
 * coordenates that have been placed into tetrisPiece.next
 ************************************************/
int calcRotation(char tetrisPieceType)
{
	int allowedBlocks = 0;
                                                                 /*********************************
								  * If current tetrisPiece is a square,
								  * do nothing and return successfuly
								  *********************************/
	if(tetrisPieceType == 'q')
	{
		return allowedBlocks;
	}
	else
	{
                                                                 /*********************************
								  * Matrix that will contain unitary
                                                                  * coordenates, these are nothing more 
                                                                  * than the coordenates with respect to 
                                                                  * the center of the tetrisPiece.
								  *********************************/

		signed int currUnitaryCoords[4][2];
		signed int newUnitaryCoords[4][2];

		//these are x coordenates from now on so we keep everything in quadrant 1
		currUnitaryCoords[1][0] = tetrisPiece.curr[1][0] - tetrisPiece.curr[0][0]; 
		currUnitaryCoords[2][0] = tetrisPiece.curr[2][0] - tetrisPiece.curr[0][0]; 
		currUnitaryCoords[3][0] = tetrisPiece.curr[3][0] - tetrisPiece.curr[0][0]; 

		//these are y coordenates from now on so we keep everything in quadrant 1
		currUnitaryCoords[1][1] = tetrisPiece.curr[1][1] - tetrisPiece.curr[0][1]; 
		currUnitaryCoords[2][1] = tetrisPiece.curr[2][1] - tetrisPiece.curr[0][1]; 
		currUnitaryCoords[3][1] = tetrisPiece.curr[3][1] - tetrisPiece.curr[0][1]; 
		//DEBUG:
		//printf("                       // x ;                         //y;\n", currUnitaryCoords[0][1], currUnitaryCoords[1][1]);
		//printf("currUnitaryCoords[1][0] = %d; currUnitaryCoords[1][1] = %d;\n", currUnitaryCoords[0][1], currUnitaryCoords[1][1]);
		//printf("currUnitaryCoords[2][0] = %d; currUnitaryCoords[2][1] = %d;\n", currUnitaryCoords[0][1], currUnitaryCoords[2][1]);
		//printf("currUnitaryCoords[3][0] = %d; currUnitaryCoords[3][1] = %d;\n", currUnitaryCoords[0][1], currUnitaryCoords[3][1]);

			              //x  =   - y                       
		//printf("\t\t x = -y\n");
		newUnitaryCoords[1][0] = 0-currUnitaryCoords[1][1];
		//printf("newUnitaryCoords[1][0] = 0-%d;\n", currUnitaryCoords[1][1]);
		newUnitaryCoords[2][0] = 0-currUnitaryCoords[2][1];
		//printf("newUnitaryCoords[2][0] = 0-%d;\n", currUnitaryCoords[2][1]);
		newUnitaryCoords[3][0] = 0-currUnitaryCoords[3][1];
		//printf("newUnitaryCoords[3][0] = 0-%d;\n", currUnitaryCoords[3][1]);

		                  //y  =   x                       
		newUnitaryCoords[1][1] = currUnitaryCoords[1][0];
		newUnitaryCoords[2][1] = currUnitaryCoords[2][0];
		newUnitaryCoords[3][1] = currUnitaryCoords[3][0];
		//printf("newUnitaryCoords[1][1] = %d;\n", newUnitaryCoords[1][1]);
		//printf("newUnitaryCoords[2][1] = %d;\n", newUnitaryCoords[2][1]);
		//printf("newUnitaryCoords[3][1] = %d;\n", newUnitaryCoords[3][1]);
		
                                                                 /*********************************
								  * Put the rotated unitary coordenates 
								  * stored in newUnitaryCoords into regular
								  * cartesian coords.
								  *********************************/
		//origin
		tetrisPiece.next[0][1] = tetrisPiece.curr[0][1];
		tetrisPiece.next[0][0] = tetrisPiece.curr[0][0];

		//All 'x' components or, in our case, columns
		tetrisPiece.next[1][1] = tetrisPiece.curr[0][1] + newUnitaryCoords[1][1];
		//printf("tetrisPiece.next[1][1] = tetrisPiece.curr[0][1] + newUnitaryCoords[1][1];\n");
		//printf("tetrisPiece.next[1][1] = %d                      + %d                     ;\n",tetrisPiece.curr[0][1] , newUnitaryCoords[1][1]);
		tetrisPiece.next[2][1] = tetrisPiece.curr[0][1] + newUnitaryCoords[2][1];
		//printf("tetrisPiece.next[2][1] = tetrisPiece.curr[0][1] + newUnitaryCoords[2][1];\n");
		//printf("tetrisPiece.next[2][1] = %d                      + %d                     ;\n",tetrisPiece.curr[0][1] , newUnitaryCoords[2][1]);
		tetrisPiece.next[3][1] = tetrisPiece.curr[0][1] + newUnitaryCoords[3][1];
		//printf("tetrisPiece.next[3][1] = tetrisPiece.curr[0][1] + newUnitaryCoords[3][1];\n");
		//printf("tetrisPiece.next[3][1] = %d                      + %d                     ;\n",tetrisPiece.curr[0][1] , newUnitaryCoords[3][1]);

		//All 'y' components or, in our case, rows
		tetrisPiece.next[1][0] = tetrisPiece.curr[0][0] + newUnitaryCoords[1][0];
		//printf("tetrisPiece.next[1][0] = tetrisPiece.curr[0][0] + newUnitaryCoords[1][0];\n");
		//printf("tetrisPiece.next[1][0] = %d                      + %d                     ;\n",tetrisPiece.curr[0][0] , newUnitaryCoords[1][0]);
		tetrisPiece.next[2][0] = tetrisPiece.curr[0][0] + newUnitaryCoords[2][0];
		//printf("tetrisPiece.next[2][0] = tetrisPiece.curr[0][0] + newUnitaryCoords[2][0];\n");
		//printf("tetrisPiece.next[2][0] = %d                      + %d                     ;\n",tetrisPiece.curr[0][0] , newUnitaryCoords[2][0]);
		tetrisPiece.next[3][0] = tetrisPiece.curr[0][0] + newUnitaryCoords[3][0];
		//printf("tetrisPiece.next[3][0] = tetrisPiece.curr[0][0] + newUnitaryCoords[3][0];\n");
		//printf("tetrisPiece.next[3][0] = %d                      + %d                     ;\n",tetrisPiece.curr[0][0] , newUnitaryCoords[3][0]);
	}
	//DEBUG:
	//printf("tetrisPiece.next[0][1] = %d; tetrisPiece.next[0][0] = %d;\n", tetrisPiece.next[0][1], tetrisPiece.next[0][0]);
	//printf("tetrisPiece.next[1][1] = %d; tetrisPiece.next[1][0] = %d;\n", tetrisPiece.next[1][1], tetrisPiece.next[1][0]);
	//printf("tetrisPiece.next[2][1] = %d; tetrisPiece.next[2][0] = %d;\n", tetrisPiece.next[2][1], tetrisPiece.next[2][0]);
	//printf("tetrisPiece.next[3][1] = %d; tetrisPiece.next[3][0] = %d;\n", tetrisPiece.next[3][1], tetrisPiece.next[3][0]);
                                                                 /*********************************
								  * Now that the piece has been defined,
								  * place it in the canvas if posible
								  *********************************/
	//count the number of blocks that can be placed
	for(allowedBlocks=0; allowedBlocks<4; allowedBlocks++)
	{
		if(layout[tetrisPiece.next[i][0]][tetrisPiece.next[i][1]]!='_') //if layout[row][col] is free
		{
			break; //one of the blocks can't be placed
		}
	}
	return allowedBlocks;
}

/******************************************************************/
/* FUNCTION:                                                      */
/*     void paintStageBlack()                                     */
/*                                                                */
/* It's only cfunction is to paint the AcumulatedShapes canvas    */
/* black, which is its default color                              */
/******************************************************************/
void paintStageBlack(void)
{
	for (i = 0; i < 3240; i++)
	{
		colorAcumulatedShapes[i] = 0.0f; //All black
	}
}

/******************************************************************/
/* FUNCTION:                                                      */
/*     void printTetrisPieceOnLayout(char *matrixName)            */
/*                                                                */
/* Function to display the equivalence of the 'int layout' matrix */
/* in terms of the 'GLfloat AcumulatedShapes' matrix.             */
/******************************************************************/
void printTetrisPieceOnLayout(char *matrixName)
{
  if(strcmp(matrixName,"current") == 1) //strcmp returns -1 when false
  {
                                                                 /*********************************
								  * Every character in the 'int layout [18][10]  
								  * matrix has an equivalence here. From:
								  *   ______  ______
								  *  |      ||      |
								  *  |[0][0]||[0][1]|
								  *  |      ||      |
								  *   ------  ------ 
								  *  |      ||      |
								  *  |[1][0]||[1][1]|
								  *  |      ||      |
								  *   ------  ------ 
								  *   To:
								  *   _________________  _________________
								  *  | 0.0f,18.0f,0.0f,|| 1.0f,18.0f,0.0f,|
								  *  | 1.0f,17.0f,0.0f,|| 2.0f,17.0f,0.0f,|
								  *  | 0.0f,17.0f,0.0f,|| 1.0f,17.0f,0.0f,|
								  *  | 0.0f,18.0f,0.0f,|| 1.0f,18.0f,0.0f,|
								  *  | 1.0f,17.0f,0.0f,|| 2.0f,17.0f,0.0f,|
								  *  | 1.0f,18.0f,0.0f,|| 2.0f,18.0f,0.0f,|
								  *   -----------------  -----------------
								  *   _________________  _________________
								  *  | 0.0f,17.0f,0.0f,|| 1.0f,17.0f,0.0f,|
								  *  | 1.0f,16.0f,0.0f,|| 2.0f,16.0f,0.0f,|
								  *  | 0.0f,16.0f,0.0f,|| 1.0f,16.0f,0.0f,|
								  *  | 0.0f,17.0f,0.0f,|| 1.0f,17.0f,0.0f,|
								  *  | 1.0f,16.0f,0.0f,|| 2.0f,16.0f,0.0f,|
								  *  | 1.0f,17.0f,0.0f,|| 2.0f,17.0f,0.0f,|
								  *   -----------------  -----------------
								  *********************************/
							//  No,. I don't need to traverse throught the entire matrix!! I already have all the info I need
							//  On tetrisPiece.next[][] that's the reason I've been feeding that matrix for!!
							//      colorAcumulatedShapes[ i+180*row+18*col]   = (float)  8/255;
							//      colorAcumulatedShapes[ i+180*tetrisPiece[0][1]+18*tetrisPiece[0][0] ]   = (float)  8/255;
							//
							//
							//	I could use a for statement to go through the tetrisPiece.next a matrix that has 4 spots:
                                                                 /****************************************************************************
								  * There's an operation such that:
								  *    x,y    x,y
								  *   _____  _____
								  *  |     ||     |
								  *  | 0,0 || 1,0 | 
								  *  |     ||     |
								  *   -----  ----- 
								  *  |     ||     |
								  *  | 0,1 || 1,1 | 
								  *  |     ||     |
								  *   -----  ----- 
								  *            x        ,      y                       x        ,      y
								  *   _____________________________________   _____________________________________
								  *  |                                     | |                                     |
								  *  |                                     | |                                     |
								  *  | tetrisPiece[0][0],tetrisPiece[0][1] | | tetrisPiece[1][0],tetrisPiece[1][1] |
								  *  |                                     | |                                     |
								  *  |                                     | |                                     |
								  *   -------------------------------------   -------------------------------------   
								  *  |                                     | |                                     |
								  *  |                                     | |                                     |
								  *  | tetrisPiece[3][0],tetrisPiece[3][1] | | tetrisPiece[2][0],tetrisPiece[2][1] |
								  *  |                                     | |                                     |
								  *  |                                     | |                                     |
								  *   -------------------------------------   -------------------------------------   
								  *
								  * There's an operation such that:
								  *   ___________________________  ___________________________
								  *  | c+x=AS[z], k-y=AS[z],AS[z]|| c+x=AS[z], k-y=AS[z],AS[z]|   AS[3240] = {0.0f,18.0f,0.0f.....}
								  *  | 0+0=0    ,18-0=18   , 0   || 0+1=1    ,18-0=18   , 0   |   AS[0]  =  0.0f
								  *  | 1+0=1    ,17-0=17   , 0   || 1+1=2    ,17-0=17   , 0   |   AS[1]  = 18.0f
								  *  | 0+0=0    ,17-0=17   , 0   || 0+1=1    ,17-0=17   , 0   |   AS[2]  =  0.0f
								  *  | 0+0=0    ,18-0=18   , 0   || 0+1=1    ,18-0=18   , 0   |   AS[3]  =  1.0f
								  *  | 1+0=1    ,17-0=17   , 0   || 1+1=2    ,17-0=17   , 0   |   AS[4]  = 17.0f
								  *  | 1+0=1    ,18-0=18   , 0   || 1+1=2    ,18-0=18   , 0   |   AS[5]  =  0.0f
								  *   ---------------------------  ---------------------------    AS[6]  =  0.0f
								  *  | c+x=AS[z], k-y=AS[z],AS[z]|| c+x=AS[z], k-y=AS[z],AS[z]|   AS[7]  = 17.0f
								  *  | 0+0=0    ,18-1=17   , 0   || 0+1=1    ,18-1=17   , 0   |   AS[8]  =  0.0f
								  *  | 1+0=1    ,17-1=16   , 0   || 1+1=2    ,17-1=16   , 0   |   AS[9]  =  0.0f
								  *  | 0+0=0    ,17-1=16   , 0   || 0+1=1    ,17-1=16   , 0   |   AS[10] = 18.0f
								  *  | 0+0=0    ,18-1=17   , 0   || 0+1=1    ,18-1=17   , 0   |      .
								  *  | 1+0=1    ,17-1=16   , 0   || 1+1=2    ,17-1=16   , 0   |      .
								  *  | 1+0=1    ,18-1=17   , 0   || 1+1=2    ,18-1=17   , 0   |      .
								  *   ---------------------------  ---------------------------    AS[3240] = 0.0f
								  *
								  *  We managed to find the values for AS[z] from tetrisPiece.next[ ][ ] but I don't actually 
								  *  need that, what I need is to find the 'z' index so I can go to colorAcumulatedShapes[z] 
								  *  to change its values. I need to find the 'z' index out of the tetrisPiece.next[ ][ ] 
								  *  coordenates.
								  *   ______________  ______________
								  *  |              ||              |
								  *  | [0  ,  0]    || [0  ,  1]    |
								  *  |              ||              |
								  *  |    0-17      ||   18-35      |
								  *  |              ||              |
								  *   --------------  -------------- 
								  *  |              ||              |
								  *  |  [0  ,  0]   ||  [1  ,  0]   |
								  *  |              ||              |
								  *  |180*0~180*0+17||180*1-180*1+17|
								  *  |              ||              |
								  *   --------------  -------------- 
								  *   
								  *   --------------------------  -------------------------- 
								  *  |                          ||                          |
								  *  |      [0   ,   0]         ||      [0   ,   1]         |
								  *  |                          ||                          |
								  *  |180*0+18*0 , 180*0+18*0+17||180*0+18*1 , 180*0+18*1+17|
								  *  |                          ||                          | 
								  *   --------------------------  --------------------------
								  *   --------------------------  -------------------------- 
								  *  |                          ||                          |
								  *  |      [1   ,   0]         ||      [1   ,   1]         |
								  *  |                          ||                          |
								  *  |180*1+18*0 , 180*1+18*0+17||180*1+18*1 , 180*1+18*1+17|
								  *  |                          ||                          | 
								  *   --------------------------  --------------------------
								  *
								  *  Which takes us to this:
								  *   --------------------------- 
								  *  |                           |
								  *  |       [x    ,   y]        |
								  *  |                           |
								  *  |180*x+18*y+i , 180*x+18*y+i|
								  *  |                           |
								  *   --------------------------  
								  *
								  *  So our formula could be:
								  * colorAcumulatedShapes[ i+180*tetrisPiece[0][1]+18*tetrisPiece[0][0] ]   = getColorFromType(tetrisPiece.type);
								  *   --------------------------- 
								  *  |                           |
								  *  |       (x    ,   y)        |
								  *  |                           |
								  *  |180*x+18*y+i , 180*x+18*y+i|
								  *  |                           |
								  *   --------------------------  
								  *******************************************************************************************/
			for( int tetrisSquareNumber=0;tetrisSquareNumber<4;tetrisSquareNumber++ ) 
			{
				for( int i=0;i<18;i+=3 )
				{
					colorAcumulatedShapes[ i + 180*tetrisPiece.next[tetrisSquareNumber][0]+18*tetrisPiece.next[tetrisSquareNumber][1]] = getRGBvalue(tetrisPiece.type, 0);
					colorAcumulatedShapes[ i+1+180*tetrisPiece.next[tetrisSquareNumber][0]+18*tetrisPiece.next[tetrisSquareNumber][1]] = getRGBvalue(tetrisPiece.type, 1);
					colorAcumulatedShapes[ i+2+180*tetrisPiece.next[tetrisSquareNumber][0]+18*tetrisPiece.next[tetrisSquareNumber][1]] = getRGBvalue(tetrisPiece.type, 2);
				}
			}
  }
  else
  {
			for( int tetrisSquareNumber=0;tetrisSquareNumber<4;tetrisSquareNumber++ ) 
			{
				for( int i=0;i<18;i+=3 )
				{
					colorAcumulatedShapes[ i + 180*tetrisPiece.curr[tetrisSquareNumber][0]+18*tetrisPiece.curr[tetrisSquareNumber][1]] = getRGBvalue(tetrisPiece.type, 0);
					colorAcumulatedShapes[ i+1+180*tetrisPiece.curr[tetrisSquareNumber][0]+18*tetrisPiece.curr[tetrisSquareNumber][1]] = getRGBvalue(tetrisPiece.type, 1);
					colorAcumulatedShapes[ i+2+180*tetrisPiece.curr[tetrisSquareNumber][0]+18*tetrisPiece.curr[tetrisSquareNumber][1]] = getRGBvalue(tetrisPiece.type, 2);
					//printf("                                          i = %d\n",i);
					//printf("180*tetrisPiece.curr[tetrisSquareNumber][1] = %d\n",180*tetrisPiece.curr[tetrisSquareNumber][1]);
					//printf(" 18*tetrisPiece.curr[tetrisSquareNumber][0] = %d\n", 18*tetrisPiece.curr[tetrisSquareNumber][0] );
					//printf("i + 180*tetrisPiece.curr[tetrisSquareNumber][1]+18*tetrisPiece.curr[tetrisSquareNumber][0] = %d\n",i+180*tetrisPiece.curr[tetrisSquareNumber][1]+18*tetrisPiece.curr[tetrisSquareNumber][0] );
				}
			}
  }

  return;   
}

/******************************************************************/
/*                                                                */
/* FUNCTION:                                                      */
/*     void removeFromStage(void)                                 */
/*                                                                */
/* Function to display the equivalence of the 'int layout' matrix */
/* in terms of the 'GLfloat AcumulatedShapes' matrix.             */
/******************************************************************/
void removeFromStage(void)
{
	for( int tetrisSquareNumber=0;tetrisSquareNumber<4;tetrisSquareNumber++ ) 
	{
		for( int i=0;i<18;i+=3 )
		{
			colorAcumulatedShapes[ i + 180*tetrisPiece.curr[tetrisSquareNumber][0]+18*tetrisPiece.curr[tetrisSquareNumber][1]] = 0.0f;
			colorAcumulatedShapes[ i+1+180*tetrisPiece.curr[tetrisSquareNumber][0]+18*tetrisPiece.curr[tetrisSquareNumber][1]] = 0.0f;
			colorAcumulatedShapes[ i+2+180*tetrisPiece.curr[tetrisSquareNumber][0]+18*tetrisPiece.curr[tetrisSquareNumber][1]] = 0.0f;
		}
	}
    return;   
}

/******************************************************************/
/* FUNCTION:                                                      */
/*     void keyPress(int key)                                     */
/*                                                                */
/* New algorithm to implement sustaintment.                       */
/* ---|-------|---------------|---------------------------> time  */
/*  press   repeat        press again                             */
/*   t1  sustainedKeyTime    1.0                                  */
/*             ^     ^------- No sustaintment, just press it      */
/*             |              again and reset t1                  */
/*         sustaintment, execute motion again, reset nothing      */
/******************************************************************/
void keyPress(int key)
{
	if( keyHist.lastKey != key ) 
	{ 
		//user presses this key for the first time
		//printf("Move() and initialize 'keyHist'\n");
		keyHist.lastKey = key;
		keyHist.t1 = glfwGetTime();
		keyHist.consecutive = 2 + 1;
		executeMotionForKey(key);
	}
	else 
	{
		//This means t1 has been set  and we COULD have sustaintment
		//let's figure out:
		keyHist.dt = glfwGetTime() - keyHist.t1;

		// ---|-------|-----|---------------------------> time
		//  press   repeat press again
		//    t1      0.6   1.0 
		//             ^     ^------- No sustaintment, just press it again and reset t1
		//             |
		//         sustaintment, execute motion again, reset nothing given by the 'sustainedKeyTime' constant
		if( (keyHist.dt >= sustainedKeyTime) && (keyHist.consecutive == 3) )
		{
			//sustained key!!
			//printf("(keyHist.dt >= gravitySpeed) == TRUE;\n");
			keyHist.consecutive++;
			executeMotionForKey(key);
		}
		else if( keyHist.dt >= keyHist.consecutive*keyHist.repetitionTime)
		{
			//sustained key!!
			//printf("(keyHist.dt >= keyHist.consecutive*0.5f) == FALSE; keyHist.consecutive*0.5f = %f\n", keyHist.consecutive*0.5f);
			keyHist.consecutive++;
			executeMotionForKey(key);
		}
	}
    return;   
}

/******************************************************************/
/* FUNCTION:                                                      */
/*     void executeMotionForKey(int key);                         */
/*                                                                */
/* A simple switch statement that executes according to the key   */
/* we pressed.                                                    */
/*                                                                */
/******************************************************************/
void executeMotionForKey(int key)
{
	switch(key)
	{
		case GLFW_KEY_RIGHT:
			calcHorizontalMotion(1);
			moveTetrisPiece();
			break;
		case GLFW_KEY_LEFT:
			calcHorizontalMotion(0);
			moveTetrisPiece();
			break;
		case GLFW_KEY_DOWN:
			calcDownMotion(); //This one can land current tetrisPiece
			moveTetrisPiece();
			break;
		case GLFW_KEY_UP:
			calcRotation(tetrisPiece.type);
			moveTetrisPiece();
			break;
		default:
		break;
	}
    return;   
}

/******************************************************************/
/* NEW!!                                                          */
/* FUNCTION:                                                      */
/*    float getRGBvalue(char tetrisPieceType, int RGBindex)       */
/*                                                                */
/* This function needs accepts one of the characters that         */
/* represents a tetris piece and an integer from 0 to 2.          */
/*                                                                */
/* Returns the R, B or G value of the color the type of tetris    */
/* piece should have. This function could greatly simpify the     */
/* void printTetrisPieceOnLayout(char *matrixName) function       */
/*                                                                */
/******************************************************************/
GLfloat getRGBvalue(char tetrisPieceType, int RGBindex)
{
    GLfloat RGBvalues[3]; //TO-IMPROVE: make this one a global variable. or even better,
                        //make this function return the entire array

    if(tetrisPieceType == '_')
        return (GLfloat)0.0f;

	switch(tetrisPieceType)
    {
		case 'q': //1) squares will be green
		    RGBvalues[0] = (float)  8/255;
		    RGBvalues[1] = (float)168/255;
		    RGBvalues[2] = (float) 74/255;
			break;
		case 't': //2)
		    RGBvalues[0] = (float)233/255;
		    RGBvalues[1] = (float)233/255;
		    RGBvalues[2] = (float)129/255;
			break;
		case 'l': //3)
		    RGBvalues[0] = (float) 31/255;
		    RGBvalues[1] = (float)124/255;
		    RGBvalues[2] = (float)116/255;
			break;
		case 's': //4)
		    RGBvalues[0] = 1.0f;
		    RGBvalues[1] = 1.0f;
		    RGBvalues[2] = 1.0f;
			break;
		case 'z': //5)
		    RGBvalues[0] = (float) 92/255;
		    RGBvalues[1] = (float)107/255;
		    RGBvalues[2] = (float)153/255;
			break;
		case 'L': //6)
		    RGBvalues[0] = (float)228/255;
		    RGBvalues[1] = (float)142/255;
		    RGBvalues[2] = (float) 36/255;
			break;
		default : //7) case 'i':
		    RGBvalues[0] = (float)288/255;
		    RGBvalues[1] = (float) 30/255;
		    RGBvalues[2] = (float) 37/255;
			break;
	}

    return RGBvalues[RGBindex];
}

/******************************************************************/
/* NEW!!                                                          */
/* FUNCTION:                                                      */
/*    void getCompletedLines(void)                                */
/*                                                                */
/* This function feeds the int completedLines[4] global variable  */
/* the row indexes where layout is found to not to have the '_'   */
/* character.                                                     */
/*                                                                */
/******************************************************************/
void getCompletedLines(void)
{
    int completedLinesIndex = 0;
    for(int rowNum=0; rowNum<18; rowNum++) //traverse through every row of 'layout'
    {
        if(strchr(layout[rowNum],'_') == NULL) //strchr returns 1 when false
        {
            completedLines[completedLinesIndex] = rowNum;
            completedLinesIndex++;
        }
    }
    return;
}
