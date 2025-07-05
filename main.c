#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <float.h>
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

GLdouble modelview[16];
GLdouble projection[16];
GLint viewport[4];

// Variáveis globais
GLboolean simulationActive = GL_FALSE;
int* drainagePath = NULL;
int pathLength = 0;
int currentStep = 0;
GLboolean showPath = GL_TRUE;
GLboolean continuousAnimation = GL_TRUE;
int animationSpeed = 100; // ms

// Matrizes para unproject
GLdouble modelview[16];
GLdouble projection[16];
GLint viewport[4];


// Função timer
void Timer(int value) {
    if (simulationActive && continuousAnimation && currentStep < pathLength - 1) {
        currentStep++;
        glutPostRedisplay();
        glutTimerFunc(animationSpeed, Timer, 0);
    }
}

// Função para calcular o caminho de drenagem
void calculateDrainagePath(int startVertex) {
    // Libera caminho anterior
    if (drainagePath != NULL) {
        free(drainagePath);
        drainagePath = NULL;
    }

    pathLength = 0;
    currentStep = 0;
    simulationActive = GL_TRUE;

    int maxPathLength = 1000;
    drainagePath = (int*)malloc(maxPathLength * sizeof(int));
    drainagePath[pathLength++] = startVertex;

    int current = startVertex;
    while (pathLength < maxPathLength) {
        int lowestNeighbor = -1;
        float lowestY = meuModelo.vertices[current].y;

        // Busca o vizinho mais baixo
        for (int i = 0; i < meuModelo.adjacency[current].numNeighbors; i++) {
            int neighbor = meuModelo.adjacency[current].neighbors[i];
            float neighborY = meuModelo.vertices[neighbor].y;
            
            if (neighborY < lowestY) {
                lowestY = neighborY;
                lowestNeighbor = neighbor;
            }
        }

        // Chegou a um mínimo local
        if (lowestNeighbor == -1) break;

        drainagePath[pathLength++] = lowestNeighbor;
        current = lowestNeighbor;
    }

    // Inicia animação
    if (continuousAnimation) {
        glutTimerFunc(animationSpeed, Timer, 0);
    }
}
// Função para encontrar o vértice mais próximo
void findNearestVertexInRadius(GLfloat x, GLfloat y, GLfloat z, float radius) {
    float minDist = FLT_MAX;
    int closestIndex = -1;

    for (int i = 0; i < meuModelo.vertexCount; i++) {
        Vertex v = meuModelo.vertices[i];
        float dx = v.x - x;
        float dy = v.y - y;
        float dz = v.z - z;
        float dist = sqrt(dx*dx + dy*dy + dz*dz);
        
        // Considerar apenas vértices dentro do raio
        if (dist < radius && dist < minDist) {
            minDist = dist;
            closestIndex = i;
        }
    }

    if (closestIndex != -1) {
        selectedVertexIndex = closestIndex;
        selectedVertexX = meuModelo.vertices[closestIndex].x;
        selectedVertexY = meuModelo.vertices[closestIndex].y;
        selectedVertexZ = meuModelo.vertices[closestIndex].z;
        
        printf("Vértice selecionado: %d (%.2f, %.2f, %.2f)\n", 
               closestIndex, selectedVertexX, selectedVertexY, selectedVertexZ);
        
        // Iniciar simulação
        calculateDrainagePath(closestIndex);
    } else {
        printf("Nenhum vértice encontrado dentro do raio de %.2f unidades\n", radius);
    }
}

float calculateSelectionRadius() {
    // Distância aproximada da câmera ao centro da cena
    float camDistance = sqrt(eye_x*eye_x + eye_y*eye_y + eye_z*eye_z);
    
    // Raio base: maior quando a câmera está mais longe
    float baseRadius = camDistance * 0.02f;
    
    // Limitar o raio mínimo e máximo
    if (baseRadius < 0.5f) baseRadius = 0.5f;
    if (baseRadius > 5.0f) baseRadius = 5.0f;
    
    return baseRadius;
}

// Função de clique do mouse
void Mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Obter matrizes atualizadas
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        glGetDoublev(GL_PROJECTION_MATRIX, projection);
        
        GLfloat winX = (GLfloat)x;
        GLfloat winY = (GLfloat)viewport[3] - (GLfloat)y - 1;
        GLfloat winZ;
        
        // Ler profundidade do pixel clicado
        glReadPixels(x, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
        
        // Verificar se a profundidade é válida (não fundo)
        if (winZ > 4.0f) {
            printf("Clicou no fundo (winZ = %.6f), ignorando...\n", winZ);
            return;
        }
        
        GLdouble posX, posY, posZ;
        gluUnProject(winX, winY, winZ,
                     modelview, projection, viewport,
                     &posX, &posY, &posZ);
        
        // Ajustar para coordenadas do modelo
        posX /= scale;
        posY /= scale;
        posZ /= scale;
        
        printf("Clique convertido: X=%.2f, Y=%.2f, Z=%.2f\n", posX, posY, posZ);
        
        // Calcular raio de seleção adaptativo
        float selectionRadius = calculateSelectionRadius();
        printf("Raio de seleção: %.2f unidades\n", selectionRadius);
        
        // Encontrar vértice mais próximo dentro do raio
        findNearestVertexInRadius((GLfloat)posX, (GLfloat)posY, (GLfloat)posZ, selectionRadius);
        
        glutPostRedisplay();
    }
}

void DesenhaCeu() {
    glDisable(GL_LIGHTING);  // Desativa iluminação para o céu
    glDisable(GL_DEPTH_TEST); // Desativa teste de profundidade
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1); // Espaço de coordenadas normalizado
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Desenha gradiente de céu (azul claro para azul escuro)
    glBegin(GL_QUADS);
        // Topo - azul claro
        glColor3f(0.53f, 0.81f, 0.92f); // Azul céu
        glVertex2f(0, 1);
        glVertex2f(1, 1);
        
        // Base - azul mais escuro
        glColor3f(0.0f, 0.35f, 0.5f);   // Azul marinho
        glVertex2f(1, 0);
        glVertex2f(0, 0);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void Desenha(void)
{	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

     // Desenha o céu primeiro (como fundo)
    DesenhaCeu(); 

    glPushMatrix();
    glTranslatef(0.0f,0.0f,0.0f);
	//glColor3f(1.0f, 1.0f, 1.0f);
    glScalef(scale, scale, scale);
	drawModel(&meuModelo);
    glPopMatrix();

    // Desenha o caminho de drenagem
    if (showPath && drainagePath != NULL && pathLength > 0) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= currentStep; i++) {
            Vertex v = meuModelo.vertices[drainagePath[i]];
            glVertex3f(v.x * scale, v.y * scale + 0.1f, v.z * scale);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    }

    // Desenha a bola
    if (simulationActive && drainagePath != NULL && pathLength > 0) {
        Vertex v = meuModelo.vertices[drainagePath[currentStep]];
        glPushMatrix();
        glTranslatef(v.x * scale, v.y * scale + 0.5f, v.z * scale);
        glColor3f(1.0f, 0.0f, 0.0f);
        glutSolidSphere(1.5f, 20, 20);
        glPopMatrix();
    }

    // Desenhar bola no vértice selecionado (mesmo sem simulação)
    if (selectedVertexIndex != -1) {
        glPushMatrix();
        glTranslatef(selectedVertexX * scale, 
                     selectedVertexY * scale + 0.5f, 
                     selectedVertexZ * scale);
        glColor3f(1.0f, 0.0f, 0.0f);
        glutSolidSphere(2.0f, 20, 20);
        glPopMatrix();
    }

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

    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // Mudar para azul mais escuro (para combinar com o gradiente)
    glClearColor(0.0f, 0.35f, 0.5f, 1.0f); // Azul marinho 
    glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT,GL_SPECULAR, especularidade);
	glMateriali(GL_FRONT,GL_SHININESS,especMaterial);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmbiente);
	glLightfv(GL_LIGHT0, GL_AMBIENT, luzAmbiente); 
	glLightfv(GL_LIGHT0, GL_DIFFUSE, luzDifusa );
	glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular );
	glLightfv(GL_LIGHT0, GL_POSITION, posicaoLuz );

	glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glEnable(GL_LIGHTING);  
	glEnable(GL_LIGHT0);    
	glEnable(GL_DEPTH_TEST);

    glEnable(GL_RESCALE_NORMAL);
    
    // Load OBJ terrain by default
    if (loadOBJ("scene.obj", "scene.mtl", &meuModelo) == 0) {
        fprintf(stderr, "Erro ao carregar modelo OBJ/MTL\n");
        exit(1);
    }

    buildAdjacency(&meuModelo);
    
    // Salva matrizes iniciais
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, fAspect, 0.5, 500);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eye_x, eye_y, eye_z, center_x, center_y, center_z, up_x, up_y, up_z);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    // Salvar matrizes iniciais
    //glMatrixMode(GL_PROJECTION);
    //glGetDoublev(GL_PROJECTION_MATRIX, projection);
    //glMatrixMode(GL_MODELVIEW);
    //glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    eye_x = 200;
    eye_y = 10;
    eye_z = 0;
    //center_x = -1000000000;
    center_x = center_y = center_z = 0;
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
        gluPerspective(60,fAspect,0.5,1000);
    }

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

    gluLookAt(eye_x,eye_y,eye_z, center_x,center_y,center_z, up_x,up_y, up_z);

      // Atualizar matrizes após mudança de visualização
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
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


void MoverParaFrente(GLfloat dir) {
    // Calcular vetor direção da câmera para a origem
    GLfloat vx = -eye_x;
    GLfloat vy = -eye_y;
    GLfloat vz = -eye_z;
    
    // Normalizar o vetor
    GLfloat len = sqrt(vx*vx + vy*vy + vz*vz);
    if (len < 0.001f) return; // Evitar divisão por zero
    
    vx /= len; 
    vy /= len; 
    vz /= len;
    
    // Calcular nova distância
    GLfloat newDist = len - dir * step;
    
    // Limitar distância mínima e máxima
    if (newDist < 5.0f) newDist = 5.0f;
    if (newDist > 500.0f) newDist = 500.0f;
    
    // Atualizar posição da câmera
    eye_x = -vx * newDist;
    eye_y = -vy * newDist;
    eye_z = -vz * newDist;
}


void Rotacionar(GLfloat alfa) { 
    // Calcular vetor da câmera para a origem
    GLfloat dx = -eye_x;
    GLfloat dy = -eye_y;
    GLfloat dz = -eye_z;
    
    // Rotacionar o vetor no plano XZ (em torno do eixo Y)
    GLfloat cosA = cos(alfa);
    GLfloat sinA = sin(alfa);
    
    GLfloat newDx = dx * cosA + dz * sinA;
    GLfloat newDz = -dx * sinA + dz * cosA;
    
    // Atualizar posição da câmera
    eye_x = -newDx;
    eye_z = -newDz;
    
    // Manter o centro fixo na origem
    center_x = 0.0f;
    center_y = 0.0f;
    center_z = 0.0f;
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
            Rotacionar(-angle);
            break;

        case 'd':
            Rotacionar(+angle);
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
            
            // Liberar recursos de simulação atuais
            if (drainagePath != NULL) {
                free(drainagePath);
                drainagePath = NULL;
            }
            simulationActive = GL_FALSE;
            selectedVertexIndex = -1;
            
            if (usePerlinTerrain) {
                // Generate mountainous terrain
                TerrainParams params;
                params.width = 100;
                params.height = 100;
                params.scale = 0.1f;
                params.octaves = 6;
                params.persistence = 0.7f;
                params.lacunarity = 2.5f;
                
                generateMountainousTerrain(&meuModelo, params);
                printf("Terreno montanhoso ativado\n");
            } else {
                // Carregar terreno OBJ
                freeObjModel(&meuModelo);
                
                // Inicializar explicitamente
                meuModelo.vertices = NULL;
                meuModelo.texCoords = NULL;
                meuModelo.faces = NULL;
                meuModelo.normals = NULL;
                meuModelo.materials = NULL;
                meuModelo.textures = NULL;
                meuModelo.adjacency = NULL;
                
                if (loadOBJ("scene.obj", "scene.mtl", &meuModelo) == 0) {
                    fprintf(stderr, "Erro ao recarregar modelo OBJ/MTL\n");
                    
                    // Fallback para terreno Perlin
                    TerrainParams params_fallback;
                    params_fallback.width = 60;
                    params_fallback.height = 60;
                    params_fallback.scale = 0.1f;
                    params_fallback.octaves = 6;
                    params_fallback.persistence = 0.7f;
                    params_fallback.lacunarity = 2.5f;
                    
                    generateMountainousTerrain(&meuModelo, params_fallback);
                    usePerlinTerrain = GL_TRUE;
                } else {
                    printf("Terreno OBJ ativado\n");
                    buildAdjacency(&meuModelo);
                }
            }
            break;

        case 'r':
            eye_x = 200;
            eye_y = 10;
            eye_z = 0;
            //center_x = -1000000000; center_y = center_z = 0;
            center_x = center_y = center_z = 0;
            up_x = 0;
            up_y = 1;
            up_z = 0;
            ortho = GL_FALSE;
            break;

        case ' ': // Avançar passo a passo
            if (simulationActive && !continuousAnimation && currentStep < pathLength - 1) {
                currentStep++;
                glutPostRedisplay();
            }
            break;
            
        case 'c': // Alternar animação contínua
            continuousAnimation = !continuousAnimation;
            if (simulationActive && continuousAnimation && currentStep < pathLength - 1) {
                glutTimerFunc(animationSpeed, Timer, 0);
            }
            break;
            
        case 'v': // Alternar visualização do caminho
            showPath = !showPath;
            glutPostRedisplay();
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
    glutMouseFunc(Mouse);
    glutTimerFunc(animationSpeed, Timer, 0);
	Inicializa();
	glutMainLoop();
    freeObjModel(&meuModelo);

    if (drainagePath != NULL) {
        free(drainagePath);
    }

    return 0;
}