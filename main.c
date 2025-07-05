#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include "mybib.h"
#include "perlin.h"

GLfloat fAspect;
GLfloat eye_x, eye_y, eye_z;
GLfloat center_x, center_y, center_z;
GLfloat up_x, up_y, up_z;
ObjModel meuModelo;
GLboolean ortho = GL_FALSE;
GLboolean usePerlinTerrain = GL_FALSE;

// Vertex selection variables
GLboolean selectVertexMode = GL_FALSE;
int selectedVertexIndex = -1;
GLfloat selectedVertexX = 0.0f;
GLfloat selectedVertexY = 0.0f;
GLfloat selectedVertexZ = 0.0f;

const GLfloat step = 1.5f;
const GLfloat angle = 5.0f * M_PI / 180.0f;
const GLfloat scale = 100.0f;


void Desenha(void)
{	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(0.0f,0.0f,0.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
    glScalef(scale, scale, scale);
	drawModel(&meuModelo);
    glPopMatrix();

	glFlush();
	glutSwapBuffers();
 }


void Inicializa (void)
{ 
    GLfloat luzAmbiente[4]={0.2,0.2,0.2,1.0};
	GLfloat luzDifusa[4]={0.7,0.7,0.7,1.0};
	GLfloat luzEspecular[4]={1.0, 1.0, 1.0, 1.0};
	GLfloat posicaoLuz[4]={0.0, 150.0, 150.0, 1.0};

	GLfloat especularidade[4]={1.0,1.0,1.0,1.0}; 
	GLint especMaterial = 60;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT,GL_SPECULAR, especularidade);
	glMateriali(GL_FRONT,GL_SHININESS,especMaterial);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmbiente);
	glLightfv(GL_LIGHT0, GL_AMBIENT, luzAmbiente); 
	glLightfv(GL_LIGHT0, GL_DIFFUSE, luzDifusa );
	glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular );
	glLightfv(GL_LIGHT0, GL_POSITION, posicaoLuz );

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);  
	glEnable(GL_LIGHT0);    
	glEnable(GL_DEPTH_TEST);

    glEnable(GL_RESCALE_NORMAL);
    
    // Load OBJ terrain by default
    if (loadOBJ("scene.obj", "scene.mtl", &meuModelo) == 0) {
        fprintf(stderr, "Erro ao carregar modelo OBJ/MTL\n");
        exit(1);
    }

    eye_x = 200;
    eye_y = 10;
    eye_z = 0;
    center_x = -1000000000;
    center_y = center_z = 0;
    up_x = 0;
    up_y = 1;
    up_z = 0;
}    


void EspecificaParametrosVisualizacao(void)
{
	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

    if (ortho) {
        GLfloat s = 100;
        glOrtho(-s*fAspect, s*fAspect, -s, s, 1, 500);
    } else { 
        gluPerspective(60,fAspect,0.5,500);
    }

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

    gluLookAt(eye_x,eye_y,eye_z, center_x,center_y,center_z, up_x,up_y, up_z);

    puts("");
    puts("DIGITE A TECLA 'w' PARA MOVER PARA FRENTE");
    puts("DIGITE A TECLA 's' PARA MOVER PARA TRÁS");
    puts("DIGITE A TECLA 'a' PARA ROTACIONAR PARA A ESQUERDA");
    puts("DIGITE A TECLA 'd' PARA ROTACIONAR PARA A DIREITA");
    puts("DIGITE A TECLA 't' PARA SUBIR A CAMERA");
    puts("DIGITE A TECLA 'g' PARA DESCER A CAMERA");
    puts("DIGITE A TECLA 'p' PARA ALTERNAR O MODO DE PROJEÇÃO");
    puts("DIGITE A TECLA 'n' PARA ALTERNAR ENTRE TERRENO OBJ E PERLIN NOISE");
    puts("DIGITE A TECLA 'r' PARA RESETAR");
}


void AlteraTamanhoJanela(GLsizei w, GLsizei h)
{
	if ( h == 0 ) h = 1;

    glViewport(0, 0, w, h);

	fAspect = (GLfloat)w/(GLfloat)h;

	EspecificaParametrosVisualizacao();
}


void MoverParaFrente(GLfloat dir)
{
    GLfloat vx = center_x - eye_x;
    GLfloat vy = center_y - eye_y;
    GLfloat vz = center_z - eye_z;

    GLfloat len = sqrt(vx*vx + vy*vy + vz*vz);

    vx /= len; vy /= len; vz /= len;

    eye_x += vx * step * dir;
    eye_y += vy * step * dir;
    eye_z += vz * step * dir;
    center_x += vx * step * dir;
    center_y += vy * step * dir;
    center_z += vz * step * dir;
}


void Rotacionar(GLfloat alfa)
{ 
    GLfloat vx = center_x - eye_x;
    GLfloat vz = center_z - eye_z;

    GLfloat nx = vx * cos(alfa) + vz * sin(alfa);
    GLfloat nz = -vx * sin(alfa) + vz * cos(alfa);

    center_x = eye_x + nx;
    center_z = eye_z + nz;
}


void Teclado (unsigned char key, int x, int y)
{
    switch (key) {

        case 27:
            exit(0);
            break;      
    
        case 'w':
            MoverParaFrente(+1);
            break;
        
        case 's':
            MoverParaFrente(-1);
            break;
        
        case 'a':
            Rotacionar(+angle);
            break;

        case 'd':
            Rotacionar(-angle);
            break;

        case 't':
            eye_y += 1.0f;
            break;

        case 'g':
            eye_y -= 1.0f;
            break;
        
        case 'p':
            ortho = !ortho;
            break;

        case 'n':
            usePerlinTerrain = !usePerlinTerrain;
            if (usePerlinTerrain) {
                // Generate mountainous terrain with dramatic peaks and valleys
                TerrainParams params;
                params.width = 100;          // Good balance of detail and performance
                params.height = 100;         // Good balance of detail and performance
                params.scale = 0.1f;        // Not used in mountainous terrain but kept for compatibility
                params.octaves = 6;         // Not used in mountainous terrain but kept for compatibility
                params.persistence = 0.7f;  // Not used in mountainous terrain but kept for compatibility
                params.lacunarity = 2.5f;   // Not used in mountainous terrain but kept for compatibility
                
                generateMountainousTerrain(&meuModelo, params);
                printf("Terreno montanhoso ativado (picos dramáticos)\n");
            } else {
                // Reload OBJ terrain
                freeObjModel(&meuModelo);
                if (loadOBJ("scene.obj", "scene.mtl", &meuModelo) == 0) {
                    fprintf(stderr, "Erro ao recarregar modelo OBJ/MTL\n");
                    // If OBJ fails, fallback to mountainous terrain
                    TerrainParams params;
                    params.width = 60;
                    params.height = 60;
                    params.scale = 0.1f;
                    params.octaves = 6;
                    params.persistence = 0.7f;
                    params.lacunarity = 2.5f;
                    generateMountainousTerrain(&meuModelo, params);
                    usePerlinTerrain = GL_TRUE;
                } else {
                    printf("Terreno OBJ ativado\n");
                }
            }
            break;

        case 'r':
            eye_x = 200;
            eye_y = 10;
            eye_z = 0;
            center_x = -1000000000; center_y = center_z = 0;
            up_x = 0;
            up_y = 1;
            up_z = 0;
            ortho = GL_FALSE;
            break;
    }

	EspecificaParametrosVisualizacao();
    glutPostRedisplay();
}

 
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(750,750);
	glutCreateWindow("Visualizacao 3D");
	glutDisplayFunc(Desenha);
    glutReshapeFunc(AlteraTamanhoJanela);
    glutKeyboardFunc(Teclado);
	Inicializa();
	glutMainLoop();
    freeObjModel(&meuModelo);
}