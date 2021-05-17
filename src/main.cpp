//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   TRABALHO FINAL - Carlos Santiago e Gabriel Martins
//

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

#define M_PI 3.14159265358979323846
int door1open = 0;
int door2open = 0;
int lever1act = 0;
int lever2act = 0;
int lever3act = 0;
int lever4act = 0;
int lever5act = 0;
int lever6act = 0;
int lever7act = 0;
int woodenChairRotation = 0;
int woodenZ1Rotation = 3;
int woodenZ2Rotation = 5;
int woodenZ3Rotation = 4;

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char *filename, const char *basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4 &M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel *);                         // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel *model);                                        // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles();                                                 // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char *filename);                                 // Função que carrega imagens de textura
void DrawVirtualObject(const char *object_name);                             // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char *filename);                              // Carrega um vertex shader
GLuint LoadShader_Fragment(const char *filename);                            // Carrega um fragment shader
void LoadShader(const char *filename, GLuint shader_id);                     // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel *);                                          // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow *window);
float TextRendering_CharWidth(GLFWwindow *window);
void TextRendering_PrintString(GLFWwindow *window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow *window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow *window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void PrintFinalMessage(GLFWwindow *window, std::string text, float scale);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow *window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow *window);
void TextRendering_ShowProjection(GLFWwindow *window);
void TextRendering_ShowFramesPerSecond(GLFWwindow *window);
void TextRendering_ShowControls(GLFWwindow *window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

bool collisionTest(glm::vec4 position);
glm::mat4 bezierTipCurve();

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string name;              // Nome do objeto
    size_t first_index;            // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t num_indices;            // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum rendering_mode;         // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3 bbox_min;            // Axis-Aligned Bounding Box do objeto
    glm::vec3 bbox_max;
};

float p_seconds = (float)glfwGetTime();
float seconds;
float ellapsed_s;

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4> g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = true;

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f;     // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;       // Ângulo em relação ao eixo Y
float g_CameraDistance = 50.0f; // Distância da câmera para a origem
float g_camX = 0.0f;            //Distancia X da camera
float g_camY = 1.5f;            // Distancia Y da camera
float g_camZ = 0.0f;            //Distancia Z da camera

glm::vec4 cameraPosition_c_g = glm::vec4(g_camX, g_camY, g_camZ, 1.0f);
glm::vec4 cameraLookAt_l_g = glm::vec4(4.0f, 0.0f, 1.0f, 1.0f);
glm::vec4 cameraViewVector_g = cameraLookAt_l_g - cameraPosition_c_g;
glm::vec4 cameraUpVector_g = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
glm::vec4 cameraOrtoVector_g = cameraUpVector_g * (-cameraViewVector_g);

// Variável que controla qual câmera usaremos: look-at ou free-cam.
bool g_lookAt = false;

// Variavel que controla se exibiremos o cursor ou não
bool isCursorEnabled = false;

bool showControlMessage = false;

bool endGame = false;

// Variaveis de movimentação da camera Free Look
bool g_KeyPressedW = false;
bool g_KeyPressedS = false;
bool g_KeyPressedA = false;
bool g_KeyPressedD = false;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

bool isDoor1Open()
{
    return !lever1act && lever2act && !lever3act && lever4act && lever5act && !lever6act && !lever7act;
}

bool isDoor2Open()
{
    return woodenChairRotation % 4 == 1 && woodenZ1Rotation % 10 == 5 && woodenZ2Rotation % 10 == 0 && woodenZ3Rotation % 10 == 5;
}

float bezierAux = 0.0f;
int bezierAux2 = 0;

int main(int argc, char *argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow *window;
    window = glfwCreateWindow(800, 600, "INF01047 - Trabalho Final 2020/2 - Carlos Santiago & Gabriel Martins", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *glversion = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 176-196 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/tc-earth_daymap_surface.jpg");      // TextureImage0
    LoadTextureImage("../../data/tc-earth_nightmap_citylights.gif"); // TextureImage1
    LoadTextureImage("../../data/wall.jpg");                         // WallTexture
    LoadTextureImage("../../data/floor.jpg");                        // FloorTexture
    LoadTextureImage("../../data/oak-wood.png");                     // OakWoodTexture
    LoadTextureImage("../../data/tip1.png");                         // tip1Texture
    LoadTextureImage("../../data/tip2.png");                         // tip2Texture
    LoadTextureImage("../../data/goldTexture.jpg");                  //GoldTexture
    LoadTextureImage("../../data/silverTexture.jpg");                //SilverTexture

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel wall("../../data/wall.obj");
    ComputeNormals(&wall);
    BuildTrianglesAndAddToVirtualScene(&wall);

    ObjModel spiderModel("../../data/spider.obj");
    ComputeNormals(&spiderModel);
    BuildTrianglesAndAddToVirtualScene(&spiderModel);

    ObjModel doorModel("../../data/door.obj");
    ComputeNormals(&doorModel);
    BuildTrianglesAndAddToVirtualScene(&doorModel);

    ObjModel leverModel("../../data/lever.obj");
    ComputeNormals(&leverModel);
    BuildTrianglesAndAddToVirtualScene(&leverModel);

    ObjModel woodChair("../../data/woodChair.obj");
    ComputeNormals(&woodChair);
    BuildTrianglesAndAddToVirtualScene(&woodChair);

    ObjModel woodTable("../../data/woodTable.obj");
    ComputeNormals(&woodTable);
    BuildTrianglesAndAddToVirtualScene(&woodTable);

    ObjModel woodZ("../../data/woodZ.obj");
    ComputeNormals(&woodZ);
    BuildTrianglesAndAddToVirtualScene(&woodZ);

    ObjModel oscar("../../data/oscar.obj");
    ComputeNormals(&oscar);
    BuildTrianglesAndAddToVirtualScene(&oscar);

    ObjModel trophy("../../data/trophy.obj");
    ComputeNormals(&trophy);
    BuildTrianglesAndAddToVirtualScene(&trophy);

    if (argc > 1)
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    glm::mat4 m = Matrix_Identity();

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl".
    GLint model_uniform = glGetUniformLocation(program_id, "model");                     // Variável da matriz "model"
    GLint view_uniform = glGetUniformLocation(program_id, "view");                       // Variável da matriz "view" em shader_vertex.glsl
    GLint projection_uniform = glGetUniformLocation(program_id, "projection");           // Variável da matriz "projection" em shader_vertex.glsl
    GLint render_as_black_uniform = glGetUniformLocation(program_id, "render_as_black"); // Variável booleana em shader_vertex.glsl

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);

        glm::vec4 cameraPosition_c;
        glm::vec4 cameraLookAt_l;
        glm::vec4 cameraViewVector;
        glm::vec4 cameraUpVector;
        glm::vec4 cameraPosition_c_x;
        glm::vec4 cameraLookAt_l_x;
        glm::vec4 cameraUpVector_x;

        float r = 2.0f;
        float y = r * sin(g_CameraPhi);
        float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
        float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);
        float step_size = 2.0f;
        float g_camX_temp = g_camX; //Distancia X da camera
        float g_camY_temp = g_camY; // Distancia Y da camera
        float g_camZ_temp = g_camZ; //Distancia Z da camera
        glm::vec4 new_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        seconds = (float)glfwGetTime();
        ellapsed_s = seconds - p_seconds;

        if (g_lookAt)
        {
            cameraPosition_c_x = glm::vec4(2.0, 3.0, 0.0, 1.0f); // camera da parede apontada fixamente.
            cameraLookAt_l_x = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            cameraUpVector_x = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // vetor "up" sendo colocado fixamente sempre olhando para o "céu" (eixo y global).
        }
        else
        {
            if (g_KeyPressedS)
            {
                g_camX_temp -= ellapsed_s * step_size * sin(g_CameraTheta);
                g_camZ_temp -= ellapsed_s * step_size * cos(g_CameraTheta);
            }
            else if (g_KeyPressedD)
            {
                g_camX_temp -= ellapsed_s * step_size * cos(g_CameraPhi) * cos(g_CameraTheta);
                g_camZ_temp += ellapsed_s * step_size * cos(g_CameraPhi) * sin(g_CameraTheta);
            }
            else if (g_KeyPressedW)
            {
                g_camX_temp += ellapsed_s * step_size * cos(g_CameraPhi) * sin(g_CameraTheta);
                g_camZ_temp += ellapsed_s * step_size * cos(g_CameraPhi) * cos(g_CameraTheta);
            }
            else if (g_KeyPressedA)
            {
                g_camX_temp += ellapsed_s * step_size * cos(g_CameraPhi) * cos(g_CameraTheta);
                g_camZ_temp -= ellapsed_s * step_size * cos(g_CameraPhi) * sin(g_CameraTheta);
            }

            new_pos = glm::vec4(g_camX_temp, g_camY_temp, g_camZ_temp, 1.0f);
            if (!collisionTest(new_pos))
            {
                g_camX = g_camX_temp;
                g_camY = g_camY_temp;
                g_camZ = g_camZ_temp;
            }

            cameraPosition_c_x = glm::vec4(g_camX, g_camY, g_camZ, 1.0f);
            cameraLookAt_l_x = cameraLookAt_l_g;
            cameraUpVector_x = cameraUpVector_g;
        }

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        cameraPosition_c = cameraPosition_c_x; // Ponto "c", centro da câmera
        cameraPosition_c_g = cameraPosition_c_x;
        cameraLookAt_l = cameraLookAt_l_x;                    // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        cameraViewVector = cameraLookAt_l - cameraPosition_c; // Vetor "view", sentido para onde a câmera está virada
        cameraUpVector = cameraUpVector_x;                    // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(cameraPosition_c, cameraViewVector, cameraUpVector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        p_seconds = seconds;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;     // Posição do "near plane"
        float farplane = -10000.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = M_PI / 2.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f * g_CameraDistance / 2.5f;
            float b = -t;
            float r = t * g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

#define SPHERE 0
#define BUNNY 1
#define WALL 2
#define FLOOR 6
#define FLOOR2 11
#define FLOOR3 16
#define DOOR1 17
#define DOOR2 18
#define MAP 19
#define LEVER1 20
#define LEVER2 21
#define LEVER3 22
#define LEVER4 23
#define LEVER5 24
#define LEVER6 25
#define LEVER7 26
#define WOODTABLE 27
#define WOODCHAIR 28
#define WOODZ1 29
#define WOODZ2 30
#define WOODZ3 31
#define TIPBOARD1 32
#define TIPBOARD2 33
#define OSCAR 34
#define SPIDER1 35
#define SPIDER2 36
#define TROPHY 37
#define ROOF1 38
#define ROOF2 39
#define ROOF3 40
#define TIPSPHERE 41

        if (isDoor1Open())
        {
            door1open = true;
        }
        if (isDoor2Open())
        {
            door2open = true;
        }
        // Desenhamos o modelo da esfera
        model = Matrix_Translate(0.0f, 0.9f, -2.0f) * Matrix_Rotate_Z(0.6f) * Matrix_Rotate_X(0.2f) * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f) * Matrix_Scale(0.3f, 0.3f, 0.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPHERE);
        DrawVirtualObject("sphere");

        // Desenhamos a sphera com dica
        model = bezierTipCurve() * Matrix_Scale(0.1f, 0.1f, 0.1f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, TIPSPHERE);
        if (g_lookAt)
            DrawVirtualObject("sphere");

        //desenhar parede 1
        model = Matrix_Translate(2.5f, 1.3f, 0.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 2
        model = Matrix_Translate(-2.5f, 1.3f, 0.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 3
        model = Matrix_Translate(0.0f, 1.3f, 2.5f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 4
        model = Matrix_Translate(-1.0f, 1.3f, -2.5f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI) * Matrix_Scale(2.0f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar chao
        model = Matrix_Translate(0.0f, 0.0f, 0.0f) * Matrix_Scale(2.5f, 1.0f, 2.5f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, FLOOR);
        DrawVirtualObject("plane");

        // desenhar teto1
        model = Matrix_Translate(0.0f, 3.6f, 0.0f) * Matrix_Scale(2.5f, 1.0f, 2.5f) * Matrix_Rotate_Z(M_PI);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, ROOF1);
        DrawVirtualObject("plane");

        // desenhar porta1
        model = Matrix_Translate(1.85f, 1.0f, -2.5f) * Matrix_Rotate_Y(-M_PI / 2) * Matrix_Scale(0.2f, 0.7f, 0.15f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, DOOR1);
        if (!door1open)
        {
            DrawVirtualObject("door");
        }

        // desenhar parede 5
        model = Matrix_Translate(2.5f, 1.3f, -5.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 6
        model = Matrix_Translate(-2.5f, 1.3f, -5.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 7
        model = Matrix_Translate(-1.0f, 1.3f, -2.5f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Scale(2.0f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 8
        model = Matrix_Translate(1.35f, 1.3f, -7.5f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI) * Matrix_Scale(2.0f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar chao2
        model = Matrix_Translate(0.0f, 0.0f, -5.0f) * Matrix_Scale(2.5f, 1.0f, 2.5f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, FLOOR2);
        DrawVirtualObject("plane");

        // desenhar teto2
        model = Matrix_Translate(0.0f, 3.6f, -5.0f) * Matrix_Scale(2.5f, 1.0f, 2.5f) * Matrix_Rotate_Z(M_PI);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, ROOF2);
        DrawVirtualObject("plane");

        // desenhar porta2
        model = Matrix_Translate(-1.5f, 1.0f, -7.5f) * Matrix_Rotate_Y(-M_PI / 2) * Matrix_Scale(0.2f, 0.7f, 0.15f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, DOOR2);
        if (!door2open)
        {
            DrawVirtualObject("door");
        }

        // desenhar parede 9
        model = Matrix_Translate(2.5f, 1.3f, -10.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 10
        model = Matrix_Translate(-2.5f, 1.3f, -10.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 11
        model = Matrix_Translate(1.35f, 1.3f, -7.5f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Scale(2.0f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar parede 12
        model = Matrix_Translate(0.0f, 1.3f, -12.5f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI) * Matrix_Scale(2.5f, 2.5f, 2.3f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WALL);
        DrawVirtualObject("plane");

        // desenhar chao3
        model = Matrix_Translate(0.0f, 0.0f, -10.0f) * Matrix_Scale(2.5f, 1.0f, 2.5f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, FLOOR2);
        DrawVirtualObject("plane");

        // desenhar teto3
        model = Matrix_Translate(0.0f, 3.6f, -10.0f) * Matrix_Scale(2.5f, 1.0f, 2.5f) * Matrix_Rotate_Z(M_PI);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, ROOF3);
        DrawVirtualObject("plane");

        // desenhar map
        model = Matrix_Translate(-2.4f, 1.3f, 0.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Rotate_Y(M_PI) * Matrix_Scale(2.2f, 1.0f, 1.0f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, MAP);
        DrawVirtualObject("plane");

        // desenhar lever1
        model = Matrix_Translate(-2.4f, 1.9f, 1.3f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever1act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER1);
        DrawVirtualObject("lever");

        // desenhar lever2
        model = Matrix_Translate(-2.4f, 1.0f, -1.70f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever2act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER2);
        DrawVirtualObject("lever");

        // desenhar lever3
        model = Matrix_Translate(-2.4f, 1.95f, -1.0f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever3act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER3);
        DrawVirtualObject("lever");

        // desenhar lever4
        model = Matrix_Translate(-2.4f, 1.5f, -0.95f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever4act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER4);
        DrawVirtualObject("lever");

        // desenhar lever5
        model = Matrix_Translate(-2.4f, 1.2f, 0.55f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever5act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER5);
        DrawVirtualObject("lever");

        // desenhar 6
        model = Matrix_Translate(-2.4f, 1.5f, -0.5f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever6act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER6);
        DrawVirtualObject("lever");

        // desenhar lever7
        model = Matrix_Translate(-2.4f, 1.8f, -0.2f) * Matrix_Rotate_Z(-M_PI / 2) * Matrix_Scale(0.075f, 0.075f, 0.075f);
        if (lever7act)
        {
            model = model * Matrix_Rotate_Y(M_PI);
        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, LEVER7);
        DrawVirtualObject("lever");

        // desenhar TIPBOARD1
        model = Matrix_Translate(0.0f, 1.3f, 2.49f) * Matrix_Rotate_X(M_PI / 2) * Matrix_Rotate_Z(M_PI) * Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, TIPBOARD1);
        DrawVirtualObject("plane");

        // desenhar WOODTABLE
        model = Matrix_Translate(-1.0f, 0.3f, -4.0f) * Matrix_Scale(0.175f, 0.175f, 0.175f) * Matrix_Rotate_Y(M_PI / 2);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WOODTABLE);
        DrawVirtualObject("woodTable");

        // desenhar WOODTABLE2 mesa em baixo do globo
        model = Matrix_Translate(0.0f, 0.2f, -2.4f) * Matrix_Scale(0.1f, 0.1f, 0.1f) * Matrix_Rotate_Y(M_PI / 2);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WOODTABLE);
        DrawVirtualObject("woodTable");

        // desenhar WOODCHAIR
        model = Matrix_Translate(-1.0f, 0.0f, -4.0f) * Matrix_Scale(0.135f, 0.135f, 0.135f) * Matrix_Rotate_Y(woodenChairRotation * -M_PI / 2);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WOODCHAIR);
        DrawVirtualObject("woodChair");

        // desenhar WOODZ1
        model = Matrix_Translate(-2.4f, 1.8f, -5.2f) * Matrix_Scale(1.0f, 1.0f, 1.0f) * Matrix_Rotate_X(woodenZ1Rotation * M_PI / 5);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WOODZ1);
        DrawVirtualObject("woodZ");

        // desenhar WOODZ2
        model = Matrix_Translate(-2.4f, 1.5f, -5.4f) * Matrix_Scale(1.0f, 1.0f, 1.0f) * Matrix_Rotate_X(woodenZ2Rotation * M_PI / 5);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WOODZ2);
        DrawVirtualObject("woodZ");

        // desenhar WOODZ3
        model = Matrix_Translate(-2.4f, 1.8f, -5.6f) * Matrix_Scale(1.0f, 1.0f, 1.0f) * Matrix_Rotate_X(woodenZ3Rotation * M_PI / 5);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, WOODZ3);
        DrawVirtualObject("woodZ");

        // desenhar TIPBOARD2
        model = Matrix_Translate(2.49f, 1.3f, -5.0f) * Matrix_Rotate_X(-M_PI / 2) * Matrix_Rotate_Z(M_PI / 2) * Matrix_Rotate_Y(M_PI) * Matrix_Scale(1.5f, 0.75f, 0.75f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, TIPBOARD2);
        DrawVirtualObject("plane");

        // desenhar OSCAR
        model = Matrix_Translate(0.0f, 0.0f, -12.0f) * Matrix_Scale(2.5f, 2.5f, 2.5f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, OSCAR);
        DrawVirtualObject("oscar");

        // desenhar Spider1
        model = Matrix_Translate(1.0f, 0.0f, -11.5f) * Matrix_Scale(0.50f, 0.50f, 0.50f) * Matrix_Rotate_Y(-M_PI / 5);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPIDER1);
        DrawVirtualObject("spider");

        // desenhar Spider2
        model = Matrix_Translate(-1.0f, 0.0f, -11.5f) * Matrix_Scale(0.50f, 0.50f, 0.50f) * Matrix_Rotate_Y(M_PI / 5);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPIDER2);
        DrawVirtualObject("spider");

        // desenhar TROPHY
        model = Matrix_Translate(0.0f, 0.0f, -11.0f) * Matrix_Scale(0.25f, 0.25f, 0.25f) * Matrix_Rotate_Y(M_PI / 2);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, TROPHY);
        DrawVirtualObject("trophy");

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);
        while (!glfwWindowShouldClose(window) && showControlMessage)
        {
            //Pintamos tudo de branco e reiniciamos o Z-BUFFER
            //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(program_id);

            PrintFinalMessage(window, "CONTROLS: \n WASD: MOVE CHARACTER\n F: TOGGLE TIP_CAM\n 1234567: TOGGLE LEVERS ON FIRST ROOM \n 2345: CHANGE CHAIR AND WALL PUZZLE POSITION ON SECOND ROOM \n ESC: QUIT GAME", 2.0f);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        if (endGame)
            while (!glfwWindowShouldClose(window))
            {
                //Pintamos tudo de branco e reiniciamos o Z-BUFFER
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glUseProgram(program_id);

                PrintFinalMessage(window, "CONGRATULATIONS! \n YOU'VE FINISHED THE GAME\n Press ESC to close this window", 2.0f);

                glfwSwapBuffers(window);
                glfwPollEvents();
            }

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

void PrintFinalMessage(GLFWwindow *window, std::string text, float scale)
{
    float pad = TextRendering_LineHeight(window) * scale;
    float charWidth = TextRendering_CharWidth(window) * scale;

    char buffer[200];
    int lineCount = 0;
    std::string::size_type lastTokenPos = 0;
    std::string::size_type nextTokenPos = 0;
    std::vector<std::string> lines = {};
    while (nextTokenPos != std::string::npos)
    {
        lineCount += 1;
        lastTokenPos = nextTokenPos;
        nextTokenPos = text.find_first_of("\n", lastTokenPos + 1);

        lines.push_back(text.substr(lastTokenPos, nextTokenPos - lastTokenPos));
    }

    int totalLines = lines.size();
    int currLine = 0;
    while (lines.size() > 0)
    {
        std::string text = lines[lines.size() - 1];
        snprintf(buffer, 200, text.c_str());
        TextRendering_PrintString(window, buffer, text.size() * 0.5 * charWidth * -1, currLine * pad - totalLines * 0.5 * pad, scale);

        currLine += 1;
        lines.pop_back();
    }
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char *filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if (data == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char *object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void *)(g_VirtualScene[object_name].first_index * sizeof(GLuint)));

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 176-196 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if (program_id != 0)
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform = glGetUniformLocation(program_id, "model");           // Variável da matriz "model"
    view_uniform = glGetUniformLocation(program_id, "view");             // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform = glGetUniformLocation(program_id, "object_id");   // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform = glGetUniformLocation(program_id, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "WallTexture"), 2);
    glUniform1i(glGetUniformLocation(program_id, "FloorTexture"), 3);
    glUniform1i(glGetUniformLocation(program_id, "OakWoodTexture"), 4);
    glUniform1i(glGetUniformLocation(program_id, "Tip1Texture"), 5);
    glUniform1i(glGetUniformLocation(program_id, "Tip2Texture"), 6);
    glUniform1i(glGetUniformLocation(program_id, "GoldTexture"), 7);
    glUniform1i(glGetUniformLocation(program_id, "SilverTexture"), 8);

    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4 &M)
{
    if (g_MatrixStack.empty())
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel *model)
{
    if (!model->attrib.normals.empty())
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4 vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx, vy, vz, 1.0);
            }

            const glm::vec4 a = vertices[0];
            const glm::vec4 b = vertices[1];
            const glm::vec4 c = vertices[2];

            const glm::vec4 n = crossproduct(b - a, c - a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize(3 * num_vertices);

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3 * i + 0] = n.x;
        model->attrib.normals[3 * i + 1] = n.y;
        model->attrib.normals[3 * i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel *model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float> model_coefficients;
    std::vector<float> normal_coefficients;
    std::vector<float> texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
        glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

                indices.push_back(first_index + 3 * triangle + vertex);

                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back(vx);   // X
                model_coefficients.push_back(vy);   // Y
                model_coefficients.push_back(vz);   // Z
                model_coefficients.push_back(1.0f); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if (idx.normal_index != -1)
                {
                    const float nx = model->attrib.normals[3 * idx.normal_index + 0];
                    const float ny = model->attrib.normals[3 * idx.normal_index + 1];
                    const float nz = model->attrib.normals[3 * idx.normal_index + 2];
                    normal_coefficients.push_back(nx);   // X
                    normal_coefficients.push_back(ny);   // Y
                    normal_coefficients.push_back(nz);   // Z
                    normal_coefficients.push_back(0.0f); // W
                }

                if (idx.texcoord_index != -1)
                {
                    const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
                    texture_coefficients.push_back(u);
                    texture_coefficients.push_back(v);
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name = model->shapes[shape].name;
        theobject.first_index = first_index;                  // Primeiro índice
        theobject.num_indices = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;              // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!normal_coefficients.empty())
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!texture_coefficients.empty())
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

bool collisionTest(glm::vec4 position)
{
    if (position.x >= 2.35f || position.x <= -2.35f)
    {
        return true;
    }
    if (position.z >= 2.35 || position.z <= -12.35)
    {
        return true;
    }
    if (!door1open)
    {
        if (position.z <= -2.35f)
        {
            return true;
        }
    }
    else if (!door2open)
    {
        if (position.z <= -7.35f)
        {
            return true;
        }
    }
    else
    {
        if (position.x <= 0.5f && position.x >= -0.5f && position.z >= -11.5f && position.z <= -10.5f)
        {
            endGame = true;
        }
    }
    return false;
}

glm::mat4 bezierTipCurve() // Usado apenas para as duas curvas da esfera de dica da primeira sala
{
    glm::vec3 p0 = glm::vec3(-2.39f, 1.0f, -1.7f);

    glm::vec3 p1 = glm::vec3(-2.39f, 1.0f, -1.5f);
    glm::vec3 p2 = glm::vec3(-2.39f, 1.0f, -0.95f);

    glm::vec3 p3 = glm::vec3(-2.39f, 1.5f, -0.95f);

    glm::vec3 p4 = glm::vec3(-2.39f, 2.3f, -0.95f);
    glm::vec3 p5 = glm::vec3(-2.39f, 1.2f, 0.35f);

    glm::vec3 p6 = glm::vec3(-2.39f, 1.2f, 0.55f);

    if (bezierAux >= 2)
        bezierAux2 = 1;
    if (bezierAux <= 0)
        bezierAux2 = 0;

    if (bezierAux2 == 1)
        bezierAux -= ellapsed_s * 0.125;
    if (bezierAux2 == 0)
        bezierAux += ellapsed_s * 0.125;

    //curva 1
    glm::vec3 c_01 = p0 + bezierAux * (p1 - p0);
    glm::vec3 c_12 = p1 + bezierAux * (p2 - p1);
    glm::vec3 c_23 = p2 + bezierAux * (p3 - p2);
    glm::vec3 c_01_12 = c_01 + bezierAux * (c_12 - c_01);
    glm::vec3 c_12_23 = c_12 + bezierAux * (c_23 - c_12);
    glm::vec3 c_01_12_23 = c_01_12 + bezierAux * (c_12_23 - c_01_12);

    //curva 1
    glm::vec3 c_34 = p3 + (bezierAux - 1) * (p4 - p3);
    glm::vec3 c_45 = p4 + (bezierAux - 1) * (p5 - p4);
    glm::vec3 c_56 = p5 + (bezierAux - 1) * (p6 - p5);
    glm::vec3 c_34_45 = c_34 + (bezierAux - 1) * (c_45 - c_34);
    glm::vec3 c_45_56 = c_45 + (bezierAux - 1) * (c_56 - c_45);
    glm::vec3 c_34_45_56 = c_34_45 + (bezierAux - 1) * (c_45_56 - c_34_45);

    if (bezierAux <= 1.000)
        return Matrix_Translate(c_01_12_23.x, c_01_12_23.y, c_01_12_23.z);
    else
        return Matrix_Translate(c_34_45_56.x, c_34_45_56.y, c_34_45_56.z);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char *filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar *shader_string = str.c_str();
    const GLint shader_string_length = static_cast<GLint>(str.length());

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar *log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if (log_length != 0)
    {
        std::string output;

        if (!compiled_ok)
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete[] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar *log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete[] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = !g_LeftMouseButtonPressed;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (!g_LeftMouseButtonPressed)
        return;

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.01f * dx;
    g_CameraPhi += 0.01f * dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = M_PI / 2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;

    float r = g_CameraDistance;
    float y = r * sin(g_CameraPhi);
    float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
    float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);

    cameraLookAt_l_g = glm::vec4(x, -y, z, 1.0f);
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f * yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
    // ==============
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ==============

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = M_PI / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }

    //Tecla F alterna entre os tipos de câmera (look-at previamente implementada no código original e free-cam implementada através das modificações nesse arquivo main).
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        g_lookAt = !g_lookAt;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever1act = !lever1act;
        }
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever5act = !lever5act;
        }
        else if (!door2open)
        {
            woodenChairRotation += 1;
        }
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever7act = !lever7act;
        }
        else if (!door2open)
        {
            woodenZ1Rotation += 1;
        }
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever6act = !lever6act;
        }
        else if (!door2open)
        {
            woodenZ2Rotation += 1;
        }
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever4act = !lever4act;
        }
        else if (!door2open)
        {
            woodenZ3Rotation += 1;
        }
    }
    if (key == GLFW_KEY_6 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever3act = !lever3act;
        }
    }
    if (key == GLFW_KEY_7 && action == GLFW_PRESS)
    {
        if (!door1open)
        {
            lever2act = !lever2act;
        }
    }

    //movimentação em primeira pessoa para a frente.
    if (key == GLFW_KEY_W && !g_lookAt)
    {

        if (action == GLFW_PRESS)
        {
            g_KeyPressedW = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_KeyPressedW = false;
        }
    }

    //movimentação em primeira pessoa para a direita.
    if (key == GLFW_KEY_D && !g_lookAt)
    {

        if (action == GLFW_PRESS)
        {
            g_KeyPressedD = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_KeyPressedD = false;
        }
    }

    //movimentação em primeira pessoa para trás.
    if (key == GLFW_KEY_S && !g_lookAt)
    {

        if (action == GLFW_PRESS)
        {
            g_KeyPressedS = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_KeyPressedS = false;
        }
    }

    //movimentação em primeira pessoa para a esquerda.
    if (key == GLFW_KEY_A && !g_lookAt)
    {

        if (action == GLFW_PRESS)
        {
            g_KeyPressedA = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_KeyPressedA = false;
        }
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        isCursorEnabled = !isCursorEnabled;
        showControlMessage = !showControlMessage;
        if (isCursorEnabled)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char *description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow *window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model)
{
    if (!g_ShowInfoText)
        return;

    glm::vec4 p_world = model * p_model;
    glm::vec4 p_camera = view * p_world;
    glm::vec4 p_clip = projection * p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f - pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f - 2 * pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 6 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 7 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 8 * pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f - 9 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f - 10 * pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 14 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 15 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 16 * pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f - 17 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f - 18 * pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2(0, 0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x) / (b.x - a.x), 0.0f, 0.0f, (b.x * p.x - a.x * q.x) / (b.x - a.x),
        0.0f, (q.y - p.y) / (b.y - a.y), 0.0f, (b.y * p.y - a.y * q.y) / (b.y - a.y),
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f - 22 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f - 23 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f - 24 * pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f - 25 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f - 26 * pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, "Press Space to Open Controls Window", -1.0f + pad / 10, -1.0f + 2 * pad / 10, 1.0f);
}
// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if (g_UsePerspectiveProjection)
        TextRendering_PrintString(window, "Perspective", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int ellapsed_frames = 0;
    static char buffer[20] = "?? fps";
    static int numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if (ellapsed_seconds > 1.0f)
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1) * charwidth, 1.0f - lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel *model)
{
    const tinyobj::attrib_t &attrib = model->attrib;
    const std::vector<tinyobj::shape_t> &shapes = model->shapes;
    const std::vector<tinyobj::material_t> &materials = model->materials;

    printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
    printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
    printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
    printf("# of shapes    : %d\n", (int)shapes.size());
    printf("# of materials : %d\n", (int)materials.size());

    for (size_t v = 0; v < attrib.vertices.size() / 3; v++)
    {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.vertices[3 * v + 0]),
               static_cast<const double>(attrib.vertices[3 * v + 1]),
               static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++)
    {
        printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.normals[3 * v + 0]),
               static_cast<const double>(attrib.normals[3 * v + 1]),
               static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++)
    {
        printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.texcoords[2 * v + 0]),
               static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

    // For each shape
    for (size_t i = 0; i < shapes.size(); i++)
    {
        printf("shape[%ld].name = %s\n", static_cast<long>(i),
               shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.indices.size()));

        size_t index_offset = 0;

        assert(shapes[i].mesh.num_face_vertices.size() ==
               shapes[i].mesh.material_ids.size());

        printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

        // For each face
        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
        {
            size_t fnum = shapes[i].mesh.num_face_vertices[f];

            printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
                   static_cast<unsigned long>(fnum));

            // For each vertex in the face
            for (size_t v = 0; v < fnum; v++)
            {
                tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
                printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
                       static_cast<long>(v), idx.vertex_index, idx.normal_index,
                       idx.texcoord_index);
            }

            printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
                   shapes[i].mesh.material_ids[f]);

            index_offset += fnum;
        }

        printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.tags.size()));
        for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++)
        {
            printf("  tag[%ld] = %s ", static_cast<long>(t),
                   shapes[i].mesh.tags[t].name.c_str());
            printf(" ints: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
            {
                printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
                if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" floats: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
            {
                printf("%f", static_cast<const double>(
                                 shapes[i].mesh.tags[t].floatValues[j]));
                if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" strings: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
            {
                printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
                if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");
            printf("\n");
        }
    }

    for (size_t i = 0; i < materials.size(); i++)
    {
        printf("material[%ld].name = %s\n", static_cast<long>(i),
               materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].ambient[0]),
               static_cast<const double>(materials[i].ambient[1]),
               static_cast<const double>(materials[i].ambient[2]));
        printf("  material.Kd = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].diffuse[0]),
               static_cast<const double>(materials[i].diffuse[1]),
               static_cast<const double>(materials[i].diffuse[2]));
        printf("  material.Ks = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].specular[0]),
               static_cast<const double>(materials[i].specular[1]),
               static_cast<const double>(materials[i].specular[2]));
        printf("  material.Tr = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].transmittance[0]),
               static_cast<const double>(materials[i].transmittance[1]),
               static_cast<const double>(materials[i].transmittance[2]));
        printf("  material.Ke = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].emission[0]),
               static_cast<const double>(materials[i].emission[1]),
               static_cast<const double>(materials[i].emission[2]));
        printf("  material.Ns = %f\n",
               static_cast<const double>(materials[i].shininess));
        printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
        printf("  material.dissolve = %f\n",
               static_cast<const double>(materials[i].dissolve));
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n",
               materials[i].specular_highlight_texname.c_str());
        printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
        printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
        printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
        printf("  <<PBR>>\n");
        printf("  material.Pr     = %f\n", materials[i].roughness);
        printf("  material.Pm     = %f\n", materials[i].metallic);
        printf("  material.Ps     = %f\n", materials[i].sheen);
        printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
        printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
        printf("  material.aniso  = %f\n", materials[i].anisotropy);
        printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
        printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
        printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
        printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
        printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
        printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(
            materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(
            materials[i].unknown_parameter.end());

        for (; it != itEnd; it++)
        {
            printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");
    }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
