#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define WALL1 2
#define WALL2 3
#define WALL3 4
#define WALL4 5
#define FLOOR1 6
#define WALL5 7
#define WALL6 8
#define WALL7 9
#define WALL8 10
#define FLOOR2 11
#define WALL9 12
#define WALL10 13
#define WALL11 14
#define WALL12 15
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
#define TIPBOARD2 32

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D WallTexture;
uniform sampler2D FloorTexture;
uniform sampler2D OakWoodTexture;
uniform sampler2D Tip1Texture;
uniform sampler2D Tip2Texture;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;
    vec4 lightPosition = vec4(0.0f, 2.5f, 0.0f, 1.0f);
    vec4 lightPosition2 = vec4(10.0f, -0.8f, 0.0f, 1.0f);
    vec4 lightDirection = lightPosition - p;
    vec4 lightDirection2 = normalize(lightPosition2 - p);
    lightDirection = normalize(lightDirection);

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    float q; // Expoente especular para o modelo de iluminação de Blinn-Phong

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente

    vec3 I = vec3(0.9,0.9,0.9); //espectro da fonte de iluminacao
    vec3 Ia = vec3(0.25,0.25,0.3); // espectro da luz ambiente

    float lambert = max(0,dot(n,l));

    if ( object_id == SPHERE )
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        vec4 p_vec = normalize(position_model - bbox_center);

        float theta = atan(p_vec.x, p_vec.z);
        float phi = asin(p_vec.y);

        U = (theta + M_PI)/(2*M_PI);
        V = (phi + M_PI_2)/M_PI;

    //Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    //adicionando segunda textura
    vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;

    color = Kd0 * (lambert + 0.01);
    ////transição
    if(object_id == SPHERE)
        color += Kd1 / ((lambert+0.02) * 50);
    }
    else if ( object_id == BUNNY || object_id == DOOR1 || object_id == DOOR2)
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - bbox_min.x) / (bbox_max.x - bbox_min.x);
        V = (position_model.y - bbox_min.y) / (bbox_max.y - bbox_min.y);

    //Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    //adicionando segunda textura
    vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;

    color = Kd0 * (lambert + 0.01);
    }
    else if (object_id == FLOOR1 || object_id == FLOOR2 || object_id == FLOOR3)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 kd0 = texture(FloorTexture, vec2(U, V)).rgb;
        float lambert = max(0, dot(n, lightDirection));

        color = kd0 + (lambert *0.01);
    }
    else if(object_id == TIPBOARD1) {
        U = texcoords.x;
        V = texcoords.y;

        vec3 kd0 = texture(Tip1Texture, vec2(U, V)).rgb;
        float lambert = max(0, dot(n, lightDirection));

        color = kd0 + (lambert *0.01);
    }
        else if(object_id == TIPBOARD2) {
        U = texcoords.x;
        V = texcoords.y;

        vec3 kd0 = texture(Tip2Texture, vec2(U, V)).rgb;
        float lambert = max(0, dot(n, lightDirection));

        color = kd0 + (lambert *0.01);
    }
    else if (object_id == WALL1 || object_id == WALL2 || object_id == WALL3 || object_id == WALL4 ||object_id == WALL5 || object_id == WALL6 || object_id == WALL7 || object_id == WALL8 || object_id == WALL9 || object_id == WALL10 || object_id == WALL11 || object_id == WALL12)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 kd0 = texture(WallTexture, vec2(U, V)).rgb;
        float lambert = max(0, dot(n, lightDirection));

        color = kd0 + (lambert *0.01);
    }
        else if (object_id == MAP)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 kd0 = texture(TextureImage0, vec2(U, V)).rgb;
        float lambert = max(0, dot(n, lightDirection));

        color = kd0 + (lambert *0.01);
    }
        else if (object_id == WOODCHAIR || object_id == WOODTABLE || object_id == WOODZ1 || object_id == WOODZ2 || object_id == WOODZ3)
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - bbox_min.x) / (bbox_max.x - bbox_min.x);
        V = (position_model.y - bbox_min.y) / (bbox_max.y - bbox_min.y);

    //Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd0 = texture(OakWoodTexture, vec2(U,V)).rgb;

    color = Kd0 * (lambert + 0.01);
    }
        else if ( object_id == LEVER1 || object_id == LEVER2 || object_id == LEVER3 || object_id == LEVER4
             || object_id == LEVER5 || object_id == LEVER6 || object_id == LEVER7 )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - bbox_min.x) / (bbox_max.x - bbox_min.x);
        V = (position_model.y - bbox_min.y) / (bbox_max.y - bbox_min.y);

    vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        float lambert = max(0, dot(n, lightDirection));
    color = Kd0 * (lambert + 0.01);
    }


    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec3(1.0,1.0,1.0)/2.2);
}

