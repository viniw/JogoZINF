#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"

#define BACKGROUND_COLOR BLACK
#define MENU_COLOR WHITE
#define TITLE_COLOR RAYWHITE

typedef struct {
    const char *options[4];
    int selected;
    int fontSize;
    int spacing;
} Menu;

void InitMenu(Menu *menu) {
    menu->options[0] = "1. Novo Jogo";
    menu->options[1] = "2. Carregar Jogo";
    menu->options[2] = "3. Scoreboard";
    menu->options[3] = "4. Sair";
    menu->selected = 0;
    menu->fontSize = 40;
    menu->spacing = 60;
}

float titleScale = 1.0f;
float titleAnimTime = 0.0f;
bool titleAnimActive = false;

void DrawMenu(Menu *menu, int screenWidth, int screenHeight) {
    const char *title = "ZINF";
    int baseFontSize = 80;
    int titleY = screenHeight / 4;

    float scale = titleScale;
    int scaledFontSize = (int)(baseFontSize * scale);

    int textWidth = MeasureText(title, scaledFontSize);
    int textX = (screenWidth - textWidth) / 2;

    DrawText(title,
             textX,
             titleY,
             scaledFontSize,
             TITLE_COLOR);

    for (int i = 0; i < 4; i++) {
        int optionWidth = MeasureText(menu->options[i], menu->fontSize);
        int optionX = (screenWidth - optionWidth) / 2;
        int optionY = titleY + 150 + i * menu->spacing;

        if (i == menu->selected) {
            DrawRectangleLines(
                optionX - 20,
                optionY - 10,
                optionWidth + 40,
                menu->fontSize + 20,
                RAYWHITE
            );

            DrawText(menu->options[i],
                     optionX,
                     optionY,
                     menu->fontSize,
                     RAYWHITE);
        }
        else {
            DrawText(menu->options[i],
                     optionX,
                     optionY,
                     menu->fontSize,
                     MENU_COLOR);
        }
    }

    const char *footer = "UFRGS - Algoritmos e Programacao 2025";
    DrawText(footer,
             (screenWidth - MeasureText(footer, 20)) / 2,
             screenHeight - 40,
             20,
             GRAY);
}

void UpdateMenu(Menu *menu, int screenWidth, int screenHeight, Sound hoverSound, float deltaTime) {
    static int lastSelected = -1;
    static int lastHovered = -1;
    bool hoveringAny = false;

    if (IsKeyPressed(KEY_DOWN)) {
        menu->selected = (menu->selected + 1) % 4;
        PlaySound(hoverSound);
    }
    else if (IsKeyPressed(KEY_UP)) {
        menu->selected = (menu->selected - 1 + 4) % 4;
        PlaySound(hoverSound);
    }

    if (IsKeyPressed(KEY_ONE)) { menu->selected = 0; PlaySound(hoverSound); }
    if (IsKeyPressed(KEY_TWO)) { menu->selected = 1; PlaySound(hoverSound); }
    if (IsKeyPressed(KEY_THREE)) { menu->selected = 2; PlaySound(hoverSound); }
    if (IsKeyPressed(KEY_FOUR)) { menu->selected = 3; PlaySound(hoverSound); }

    Vector2 mousePos = GetMousePosition();
    int titleY = screenHeight / 4;

    for (int i = 0; i < 4; i++) {
        int optionWidth = MeasureText(menu->options[i], menu->fontSize);
        int optionX = (screenWidth - optionWidth) / 2;
        int optionY = titleY + 150 + i * menu->spacing;

        Rectangle optionBounds = { (float)optionX, (float)optionY, (float)optionWidth, (float)menu->fontSize };

        if (CheckCollisionPointRec(mousePos, optionBounds)) {
            hoveringAny = true;
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

            if (lastHovered != i) {
                PlaySound(hoverSound);
                lastHovered = i;
            }

            menu->selected = i;
        }
    }

    if (!hoveringAny) {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        lastHovered = -1;
    }

    if (menu->selected != lastSelected) {
        titleAnimActive = true;
        titleAnimTime = 0.0f;
        lastSelected = menu->selected;
    }

    if (titleAnimActive) {
        titleAnimTime += deltaTime;
        titleScale = 1.0f + 0.1f * sinf(titleAnimTime * 10.0f);

        if (titleAnimTime > 0.5f) {
            titleAnimActive = false;
            titleScale = 1.0f;
        }
    }
}

// Função para desenhar a textura com efeito blur simples
void DrawBlurredTexture(Texture2D texture, int screenWidth, int screenHeight) {
    Vector2 pos = {0, 0};
    Rectangle sourceRec = {0, 0, (float)texture.width, (float)-texture.height};
    Color tint = (Color){255, 255, 255, 30}; // Transparência baixa para blur

    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            DrawTextureRec(texture,
                           sourceRec,
                           (Vector2){pos.x + x, pos.y + y},
                           tint);
        }
    }
}

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 900;

    InitWindow(screenWidth, screenHeight, "ZINF - Trabalho Final");
    InitAudioDevice();
    SetTargetFPS(60);

    Menu menu;
    InitMenu(&menu);

    Sound hoverSound = LoadSound("resources/hover.wav");

    Texture2D background = LoadTexture("resources/background.png");

    bool shouldClose = false;
    int gameState = 0;

    while (!WindowShouldClose() && !shouldClose) {
        float deltaTime = GetFrameTime();

        UpdateMenu(&menu, screenWidth, screenHeight, hoverSound, deltaTime);

        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            switch (menu.selected) {
                case 0: gameState = 1; shouldClose = true; break;
                case 1: gameState = 2; shouldClose = true; break;
                case 2: gameState = 3; shouldClose = true; break;
                case 3: shouldClose = true; break;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawBlurredTexture(background, screenWidth, screenHeight);

        DrawMenu(&menu, screenWidth, screenHeight);

        EndDrawing();
    }

    UnloadTexture(background);
    UnloadSound(hoverSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
