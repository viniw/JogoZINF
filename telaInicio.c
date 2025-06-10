#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

// Definições de cores
#define BACKGROUND_COLOR BLACK
#define MENU_COLOR WHITE
#define HIGHLIGHT_COLOR RAYWHITE
#define TEXT_COLOR WHITE
#define TITLE_COLOR RAYWHITE

// Estrutura para o personagem
typedef struct {
    Vector2 position;
    Texture2D texture;
    float jumpHeight;
    bool isJumping;
    float jumpVelocity;
    float gravity;
} Character;

// Estrutura para o menu
typedef struct {
    const char *options[4];
    int selected;
    int fontSize;
    int spacing;
} Menu;

// Inicializa o personagem
void InitCharacter(Character *character) {
    character->position = (Vector2){100, 100};
    // Carrega uma textura para o personagem (substitua pelo seu arquivo)
    character->texture = LoadTexture("resources/character.png");
    character->jumpHeight = 0;
    character->isJumping = false;
    character->jumpVelocity = 0;
    character->gravity = 0.5f;
}

// Atualiza o estado do personagem
void UpdateCharacter(Character *character) {
    // Lógica do pulo
    if (character->isJumping) {
        character->position.y -= character->jumpVelocity;
        character->jumpVelocity -= character->gravity;

        // Verifica se o personagem voltou ao chão
        if (character->position.y >= 100) {
            character->position.y = 100;
            character->isJumping = false;
            character->jumpVelocity = 0;
        }
    }
}

// Desenha o personagem com animação de pulo
void DrawCharacter(Character *character) {
    // Desenha o personagem com deslocamento vertical do pulo
    DrawTexture(character->texture,
                character->position.x,
                character->position.y - character->jumpHeight,
                WHITE);
}

// Inicializa o menu
void InitMenu(Menu *menu) {
    menu->options[0] = "1. Novo Jogo";
    menu->options[1] = "2. Carregar Jogo";
    menu->options[2] = "3. Scoreboard";
    menu->options[3] = "4. Sair";
    menu->selected = 0;
    menu->fontSize = 40;
    menu->spacing = 60;
}

// Desenha o menu
void DrawMenu(Menu *menu, int screenWidth, int screenHeight) {
    // Desenha o título do jogo
    const char *title = "ZINF";
    int titleFontSize = 80;
    int titleY = screenHeight / 4;

    DrawText(title,
             (screenWidth - MeasureText(title, titleFontSize)) / 2,
             titleY,
             titleFontSize,
             TITLE_COLOR);

    // Desenha as opções do menu
    for (int i = 0; i < 4; i++) {
        Color color = (i == menu->selected) ? HIGHLIGHT_COLOR : MENU_COLOR;

        DrawText(menu->options[i],
                 (screenWidth - MeasureText(menu->options[i], menu->fontSize)) / 2,
                 titleY + 150 + i * menu->spacing,
                 menu->fontSize,
                 color);
    }

    // Desenha informações adicionais
    const char *footer = "UFRGS - Algoritmos e Programacao 2025";
    DrawText(footer,
             (screenWidth - MeasureText(footer, 20)) / 2,
             screenHeight - 40,
             20,
             GRAY);
}

// Atualiza o menu com base na entrada do usuário
void UpdateMenu(Menu *menu, Character *character) {
    if (IsKeyPressed(KEY_DOWN)) {
        menu->selected = (menu->selected + 1) % 4;
        character->isJumping = true;
        character->jumpVelocity = 10.0f;
    }
    else if (IsKeyPressed(KEY_UP)) {
        menu->selected = (menu->selected - 1 + 4) % 4;
        character->isJumping = true;
        character->jumpVelocity = 10.0f;
    }
    // Pulo também com as teclas direcionais
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT)) {
        character->isJumping = true;
        character->jumpVelocity = 10.0f;
    }
}

int main(void) {
    // Configuração inicial da janela
    const int screenWidth = 1200;
    const int screenHeight = 900;

    InitWindow(screenWidth, screenHeight, "ZINF - Trabalho Final");
    SetTargetFPS(60);

    // Inicializa o menu e o personagem
    Menu menu;
    InitMenu(&menu);

    Character character;
    InitCharacter(&character);

    // Variável para controlar o loop principal
    bool shouldClose = false;
    int gameState = 0;

    // Loop principal do jogo
    while (!WindowShouldClose() && !shouldClose) {
        // Atualizações
        UpdateMenu(&menu, &character);
        UpdateCharacter(&character);

        // Verifica se o usuário selecionou uma opção
        if (IsKeyPressed(KEY_ENTER)) {
            if (menu.selected == 0) {
                gameState = 1;
                shouldClose = true;
            }
            else if (menu.selected == 1) {
                gameState = 2;
                shouldClose = true;
            }
            else if (menu.selected == 2) {
                gameState = 3;
                shouldClose = true;
            }
            else if (menu.selected == 3) {
                shouldClose = true;
            }
        }

        // Desenho
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        DrawMenu(&menu, screenWidth, screenHeight);
        DrawCharacter(&character);
        EndDrawing();
    }

    // Libera recursos
    UnloadTexture(character.texture);
    CloseWindow();

    return 0;
}
