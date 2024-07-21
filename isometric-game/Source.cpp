#include <iostream>
#include <string>
#include <assert.h>
#include <windows.h>  

using namespace std;

// DEPENDENCIES
// GLAD
#include <glad/glad.h>
// GLFW
#include <GLFW/glfw3.h>
// GLM
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// STB_IMAGE
#include <stb_image.h>

// COMMON - Códigos comuns entre os projetos
// Classe que gerencia os shaders
#include "Shader.h"

// APP DOMAIN - Códigos específicos desta aplicação
#include "Sprite.h"
#include "Tile.h"
#include <vector>

//enum directions {NONE = -1, LEFT, RIGHT, UP, DOWN, UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT};

// Protótipos das funções de callback 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void resize_callback(GLFWwindow* window, int width, int height);

// Protótipos das funções
GLuint loadTexture(string filePath, int &imgWidth, int &imgHeight);
bool checkCollision(Sprite &one, Sprite &two);
void processInputs();
glm::vec3 computePosOnMap(glm::vec2 iPos, glm::vec2 posIni, glm::vec2 tileSize);
void initializeOpenGl();
glm::vec2 computeTileIndex(glm::vec3 pixelPos, glm::vec2 posIni, glm::vec2 tileSize);
bool isWalkable(directions dir);
bool positionIsWinnable();
void placeObjectOnTilemap(Sprite &object, int row, int col, glm::vec2 posIni, glm::vec2 tileSize);
void createTileObject(Shader *shader);
void createPlayer(Shader *shader);
void createobjectScene(Shader *shader);
void drawObjects();

//Função para fazer a leitura do tilemap do arquivo
void loadMap(string fileName);
void loadWalkableMap(string fileName);
void loadMapObjects(string fileName, Shader *shader);
void drawDiamondMap(Tile &tile);
void doMovement(directions dir);

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;
glm::vec2 viewportSize;
bool resize = false;

//Variável para controle da direção do personagem
directions dir = directions::NONE;

//Variáveis para armazenar as infos do tileset
GLuint tilesetTexID;
glm::vec2 offsetTex; //armazena o deslocamento necessário das coordenadas de textura no tileset
GLuint VAOTile;
int nTiles;
glm::vec2 tileSize;

//Variáveis para armazenar as infos do tilemap
glm::vec2 tilemapSize;
const int MAX_COLUNAS = 15;
const int MAX_LINHAS = 15;
int tilemap[MAX_LINHAS][MAX_COLUNAS]; //este é o mapa de índices para os tiles do tileset
int walkableMap[MAX_LINHAS][MAX_COLUNAS];
glm::vec2 posIni; //pode virar parâmetro 

//Deixando shader de debug global para facilitar acesso nas funções 
Shader *shaderDebug;
GLFWwindow* window;
Tile tile;
Sprite player;
Sprite objectScene;
vector<Sprite> objects;

// Função MAIN
int main() {
	initializeOpenGl();

	// Compilando e buildando o programa de shader
	Shader shader("HelloTriangle.vs","HelloTriangle.fs");
	shaderDebug = new Shader("HelloTriangle.vs","HelloTriangleDebug.fs");
	
	//Leitura do tilemap
	loadMap("./maps/map4.txt");
	loadWalkableMap("./maps/map4_walkable.txt");
	posIni.x = viewportSize.x/2.0;
	posIni.y = tileSize.y/2.0;

	//Criação de um objeto Tile
	createTileObject(&shader);
	//Criação de um objeto Sprite para o personagem 
	createPlayer(&shader);
	createobjectScene(&shader);

	// Carrega objetos do ambiente do mapa
	loadMapObjects("./maps/map4_objects.txt", &shader);
	
	//Habilita o shader que será usado (glUseProgram)
	shader.Use();
	glm::mat4 projection = glm::ortho(0.0, (double) viewportSize.x,(double) viewportSize.y, 0.0, -1.0, 1.0);
	shader.setMat4("projection",glm::value_ptr(projection)); //Enviando para o shader via variável do tipo uniform (glUniform....)

	glActiveTexture(GL_TEXTURE0); //Especificando que o shader usará apenas 1 buffer de tex
	shader.setInt("texBuffer", 0); //Enviando para o shader o ID e nome da var que será o sampler2D 

	//Habilita o shader de debug
	shaderDebug->Use();
	shaderDebug->setMat4("projection",glm::value_ptr(projection));

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		//se houve alteração no tamanho da janela
		if (resize) {
			//Atualizamos a matriz de projeção ortográfica para ficar com relação 1:1 mundo e tela
			shader.Use();
			glm::mat4 projection = glm::ortho(0.0, (double) viewportSize.x,(double) viewportSize.y, 0.0, -1.0, 1.0);
			//Enviando para o shader via variável do tipo uniform (glUniform....)
			shader.setMat4("projection",glm::value_ptr(projection));

			shaderDebug->Use();
			shaderDebug->setMat4("projection",glm::value_ptr(projection));

			posIni.x = viewportSize.x/2.0;
			posIni.y = tileSize.y/2.0;
			resize = false;
		}

		// Verifica flags para movimentação do personagem
		doMovement(dir);

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		// desenha o mapa;
		drawDiamondMap(tile);

		player.desenhar();
		//drawObjects();
		objectScene.desenhar();

		// venceu jogo
		if(positionIsWinnable()) {
			cout << "Você venceu!" << endl;
			break;
		}
		
		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

void createTileObject(Shader *shader) {
	tile.inicializar(tilesetTexID, 1, nTiles, glm::vec3(400.0,300.0,0.0), glm::vec3(tileSize.x,tileSize.y,1.0),0.0,glm::vec3(1.0,1.0,1.0));
	tile.setShader(shader);
	tile.setShaderDebug(shaderDebug);
}

void createPlayer(Shader *shader) {
	int imgWidth, imgHeight;
	GLuint texID = loadTexture("./tex/knight.png", imgWidth, imgHeight);
	glm::vec2 iPos; //posição do indice do personagem no mapa
	iPos.x = 0; //coluna
	iPos.y = 0; //linha
	glm::vec3 playerPos = computePosOnMap(iPos, posIni, tileSize);

	player.inicializar(texID, 1, 1, playerPos, glm::vec3(imgWidth*0.5,imgHeight*0.5,1.0),0.0,glm::vec3(1.0,0.0,1.0));
	player.setShader(shader);
	player.setShaderDebug(shaderDebug);
}

void createobjectScene(Shader *shader) {
	int imgWidth, imgHeight;
	GLuint texID = loadTexture("./tex/trunk.png", imgWidth, imgHeight);
	glm::vec2 iPos; //posição do indice do personagem no mapa
	iPos.x = 4; //coluna
	iPos.y = 5; //linha
	glm::vec3 objectPos = computePosOnMap(iPos, posIni, tileSize);

	objectScene.inicializar(texID, 1, 1, objectPos, glm::vec3(imgWidth*0.8,imgHeight*0.8,1.0),0.0,glm::vec3(1.0,0.0,1.0));
	objectScene.setShader(shader);
	objectScene.setShaderDebug(shaderDebug);
}

void doMovement(directions dir) {
	switch(dir) {
			case LEFT:
				if(isWalkable(directions::LEFT)) {
					player.moveLeft();
				}
				break;
			case RIGHT:
				if(isWalkable(directions::RIGHT)) {
					player.moveRight();
				}
				break;
			case UP:
				if(isWalkable(directions::UP)) {
					player.moveUp();
				}
				break;
			case DOWN:
				if(isWalkable(directions::DOWN)) {
					player.moveDown();
				}
				break;
			case UP_LEFT:
				if(isWalkable(directions::UP_LEFT)) {
					player.moveUpLeft();
				}
				break;
			case  UP_RIGHT:
				if(isWalkable(directions::UP_RIGHT)) {
					player.moveUpRight();
				}
				break;
			case DOWN_LEFT:
				if(isWalkable(directions::DOWN_LEFT)) {
					player.moveDownLeft();
				}
				break;
			case DOWN_RIGHT:
				if(isWalkable(directions::DOWN_RIGHT)) {
					player.moveDownRight();
				}
				break;
		}	
}

bool isWalkable(directions dir) {
  	glm::vec2 playerTileIndex = computeTileIndex(player.nextPos(dir), posIni, tileSize);
		int linha = (int) playerTileIndex.x;
		int coluna = (int) playerTileIndex.y;
		if (linha >= 0 && linha < MAX_LINHAS && coluna >= 0 && coluna < MAX_COLUNAS) {
			return walkableMap[linha][coluna] == 1;
    } else {
			return false;
		}
}

bool positionIsWinnable() {
	glm::vec2 playerTileIndex = computeTileIndex(player.getPos(), posIni, tileSize);
	int linha = (int) playerTileIndex.x;
	int coluna = (int) playerTileIndex.y;
	return linha == MAX_LINHAS-1 && coluna == MAX_COLUNAS-1;
}

glm::vec2 computeTileIndex(glm::vec3 pixelPos, glm::vec2 posIni, glm::vec2 tileSize) {
	// Ajustando a posição para considerar o deslocamento inicial
	glm::vec2 adjustedPos = glm::vec2(pixelPos.x - posIni.x, pixelPos.y - posIni.y);

	// Calculando o índice da coluna e da linha
	int coluna = static_cast<int>(round((adjustedPos.x / (tileSize.x / 2.0f) + adjustedPos.y / (tileSize.y / 2.0f)) / 2.0f));
	int linha = static_cast<int>(round((adjustedPos.y / (tileSize.y / 2.0f) - adjustedPos.x / (tileSize.x / 2.0f)) / 2.0f));

	return glm::ivec2(linha, coluna);
}

void initializeOpenGl() {
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo Isométrico", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowSizeCallback(window, resize_callback);

	// GLAD: carrega todos os ponteiros de funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	viewportSize.x = WIDTH;
	viewportSize.y = HEIGHT;
	glViewport(0, 0, viewportSize.x, viewportSize.y);

	//Habilitando a transparência
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Habilitando o teste de profundidade
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
}

// Função de callback de teclado - chamada sempre que uma tecla for pressionada ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	switch(key) {
		case GLFW_KEY_ESCAPE:
			if(action == GLFW_PRESS) {
				glfwSetWindowShouldClose(window, GL_TRUE);
			}
			break;
		case GLFW_KEY_A:
			dir = LEFT;
			break;
		case GLFW_KEY_D:
			dir = RIGHT;
			break;
		case GLFW_KEY_W:
			dir = UP;
			break;
		case GLFW_KEY_S:
			dir = DOWN;
			break;
		case GLFW_KEY_Q:
			dir = UP_LEFT;
			break;
		case  GLFW_KEY_E:
			dir = UP_RIGHT;
			break;
		case GLFW_KEY_Z:
			dir = DOWN_LEFT;
			break;
		case GLFW_KEY_C:
			dir = DOWN_RIGHT;
			break;
	}

	if (action == GLFW_RELEASE) {
		dir = NONE;
	}

}

GLuint loadTexture(string filePath, int &imgWidth, int &imgHeight)
{
	GLuint texID;

	// Gera o identificador da textura na memória 
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
    	if (nrChannels == 3) //jpg, bmp
    	{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    	}
    	else //png
    	{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    	}
    	glGenerateMipmap(GL_TEXTURE_2D);

		imgWidth = width;
		imgHeight = height;

		stbi_image_free(data);

		glBindTexture(GL_TEXTURE_2D, 0);

	}
	else
	{
    	 std::cout << "Failed to load texture " << filePath << std::endl;
	}
	return texID;
}

bool checkCollision(Sprite &one, Sprite &two) {
    // collision x-axis?
    bool collisionX = one.getPMax().x >= two.getPMin().x &&
        two.getPMax().x >= one.getPMin().x;
    // collision y-axis?
    bool collisionY = one.getPMax().y >= two.getPMin().y &&
        two.getPMax().y >= one.getPMin().y;
    // collision only if on both axes
    return collisionX && collisionY;
}

//Função para fazer a leitura do tilemap do arquivo
void loadMap(string fileName)
{
	ifstream arqEntrada;
	arqEntrada.open(fileName); //abertura do arquivo
	if (arqEntrada)
	{
		///leitura dos dados
		string textureName;
		int width, height;
		//Leitura das informações sobre o tileset
		arqEntrada >> textureName >> nTiles >> tileSize.y >> tileSize.x;
		tilesetTexID = loadTexture("./tex/" + textureName, width, height);
		
		//Leitura das informações sobre o mapa (tilemap)
		arqEntrada >> tilemapSize.y >> tilemapSize.x; //nro de linhas e de colunas do mapa
		for (int i = 0; i < tilemapSize.y; i++) //percorrendo as linhas do mapa
		{
			for (int j = 0; j < tilemapSize.x; j++) //percorrendo as colunas do mapa
			{
				arqEntrada >> tilemap[i][j];
			}
		}
	
	}
	else
	{
		cout << "Houve um problema na leitura de " << fileName << endl;
	}
}

void loadWalkableMap(string fileName) {
	ifstream arqEntrada;
	arqEntrada.open(fileName); 
	if (arqEntrada) {

		string textureName;
		//Leitura das informações sobre o tileset
		arqEntrada >> textureName >> nTiles >> tileSize.y >> tileSize.x;

		//Leitura das informações sobre o mapa (tilemap)
		arqEntrada >> tilemapSize.y >> tilemapSize.x; //nro de linhas e de colunas do mapa
		for (int i = 0; i < tilemapSize.y; i++) //percorrendo as linhas do mapa
		{
			for (int j = 0; j < tilemapSize.x; j++) //percorrendo as colunas do mapa
			{
				arqEntrada >> walkableMap[i][j];
			}
		}
	
	}
	else
	{
		cout << "Houve um problema na leitura de " << fileName << endl;
	}
}


// load objects
void loadMapObjects(string fileName, Shader *shader) {
	ifstream arqEntrada;
	arqEntrada.open(fileName); //abertura do arquivo
	if (arqEntrada) {
		int imgWidth, imgHeight;
		string pathOfTexture;
		glm::vec2 tileIndex;

		while (arqEntrada >> pathOfTexture >> tileIndex.y >> tileIndex.x) {
			// createobjectScene();
			pathOfTexture = "./tex/"+pathOfTexture;
			GLuint objectTexID = loadTexture(pathOfTexture, imgWidth, imgHeight);
			cout << "Texture Path: " << pathOfTexture << " x: " << tileIndex.x << " y: " << tileIndex.y << endl;
			glm::vec3 objPos = computePosOnMap(tileIndex, posIni, tileSize);
			cout << "Computed Position: " << objPos.x << ", " << objPos.y << ", " << objPos.z << endl;
			
			Sprite objectScene;
			objectScene.inicializar(objectTexID, 1, 1, objPos, glm::vec3(imgWidth * 0.8, imgHeight * 0.8, 1.0), 0.0, glm::vec3(1.0, 0.0, 1.0));
			objectScene.setShader(shader);
			objectScene.setShaderDebug(shaderDebug);

			objects.push_back(objectScene);
		}
	} else {
		cout << "Houve um problema na leitura do mapa de objetos." << endl;
	}
}

void drawObjects() {
    for (int x = 0; x < objects.size(); x++) {
        objects[x].desenhar();
    }
}

void drawDiamondMap(Tile &tile) {
	for (int i=0; i < tilemapSize.y; i++) {
		for (int j=0; j < tilemapSize.x; j++) {
			int indiceTile = tilemap[i][j];
			tile.desenharNaPos(i, j, indiceTile, posIni,DIAMOND);
		}	
	}
}

void resize_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	viewportSize.x = width;
	viewportSize.y = height;

	posIni.x = viewportSize.x/2.0;
	posIni.y = 0.0;
	resize = true;	
}

//considerando DIAMOND
glm::vec3 computePosOnMap(glm::vec2 iPos, glm::vec2 posIni, glm::vec2 tileSize) {
	glm::vec3 pos;
	//Encontra a posição no mapa para os índices ij
	pos.x = posIni.x + (iPos.x-iPos.y) * tileSize.x/2.0f;
	pos.y = posIni.y + (iPos.x+iPos.y) * tileSize.y/2.0f;
	return pos;
}



