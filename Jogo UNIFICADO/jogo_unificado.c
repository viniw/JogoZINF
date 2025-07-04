#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include "raylib.h"
#include <sys/stat.h>

#define BACKGROUND_COLOR BLACK
#define MENU_COLOR WHITE
#define HIGHLIGHT_COLOR RAYWHITE
#define TEXT_COLOR WHITE
#define TITLE_COLOR RAYWHITE

#define SCREENWIDTH 1200
#define SCREENHEIGHT 900
#define ROWS 16
#define COLS 24
#define TILE_SIZE 50
#define HUD_HEIGHT 60

#define SWORD_SCORE 40
#define LIFE_SCORE 20
#define MONSTER_SCORE 100
#define ATTACK_DURATION 10
#define MAX_DEATH_ANIMATIONS 1000
#define MONSTER_DEATH_DURATION 10
#define MAX_MONSTERS 10
#define MONSTER_MOVE_INTERVAL 30

#define MAX_SCORES 5
#define NAME_LENGTH 20

typedef enum {
    PAUSE_CONTINUE,
    PAUSE_SAVE,
    PAUSE_RETURN_MENU,
    PAUSE_EXIT_GAME
} PauseAction;

typedef struct {
    const char *options[4];
    int selected;
    int fontSize;
    int spacing;
} Menu;

typedef struct {
    int row, col;
    int score, lives, level;
    bool swordActive;
    bool isBlinking;
    int blinkFrames;
    int facingRow, facingCol;
} Player;

typedef struct {
    int active;
    int frameCounter;
    Vector2 tiles[3];
} AttackEffect;

typedef struct {
    int row, col, frameCounter;
} MonsterDeath;

typedef struct {
    MonsterDeath deaths[MAX_DEATH_ANIMATIONS];
    int count;
} MonsterDeathManager;

typedef struct {
    int row, col;
    bool active;
} Monster;

typedef struct {
    Monster monsters[MAX_MONSTERS];
    int count;
} MonsterManager;

typedef struct {
    char name[NAME_LENGTH];
    int score;
} HighScore;

// Protótipos de função
void LoadMapFromFile(char map[ROWS][COLS], const char *filename);
bool RunGame(const char *mapFile, Player *player);
void InitMenu(Menu *menu);
void DrawMenu(Menu *menu, int screenWidth, int screenHeight);
void UpdateMenu(Menu *menu, int screenWidth, int screenHeight, Sound hoverSound, float deltaTime);
void LocatePlayer(char map[ROWS][COLS], Player *player);
void UpdatePlayer(char map[ROWS][COLS], Player *player);
void DrawHUD(const Player *player);
void PerformSwordAttack(char map[ROWS][COLS], Player *player, AttackEffect *effect, MonsterDeathManager *deathManager, MonsterManager *monsterManager);
void DrawMap(char map[ROWS][COLS], const Player *player, int frameCount);
void DrawMonsterDeaths(MonsterDeathManager *deaths);
void InitializeMonsters(char map[ROWS][COLS], MonsterManager *monsterManager);
void UpdateMonsters(char map[ROWS][COLS], MonsterManager *monsterManager, Player *player);
void RemoveMonsterAt(MonsterManager *manager, int row, int col);
void LoadHighScores(HighScore scores[MAX_SCORES], const char *filename);
void SaveHighScores(HighScore scores[MAX_SCORES], const char *filename);
int UpdateHighScores(HighScore scores[MAX_SCORES], const char *filename, int newScore);
void ShowHighScores(const char *filename);
void ShowVictoryScreen(Player player, int topPosition);
int CheckHighScorePosition(HighScore scores[MAX_SCORES], int newScore);
PauseAction ShowPauseMenu();
void SaveGame(const Player *player, const char *filename);
void SaveGameSlot(Player *player, int slot);
bool LoadGameSlot(Player *player, int slot);
int ChooseSaveSlot(const char *prompt);
void DrawBlurredTexture(Texture2D texture, int screenWidth, int screenHeight);
void CreateGameDirectory(const char *path);

int main(void) {
    const int screenWidth = SCREENWIDTH;
    const int screenHeight = SCREENHEIGHT;

    InitWindow(screenWidth, screenHeight, "ZINF - Trabalho Final");
    InitAudioDevice();
    SetTargetFPS(60);

    // Criar diretórios necessários
    CreateGameDirectory("saves");

    Menu menu;
    InitMenu(&menu);

    Sound hoverSound = LoadSound("resources/hover.wav");
    Texture2D background = LoadTexture("resources/background.png");

    bool inGame = false;
    bool shouldClose = false;
    Player player = {0};

    while (!WindowShouldClose() && !shouldClose) {
        float deltaTime = GetFrameTime();

        if (!inGame) {
            UpdateMenu(&menu, screenWidth, screenHeight, hoverSound, deltaTime);

            if (IsKeyPressed(KEY_ENTER)) {
                switch (menu.selected) {
                    case 0:
                        player.lives = 3;
                        player.score = 0;
                        player.level = 1;
                        inGame = true;
                        break;
                    case 1: {
                        int slot = ChooseSaveSlot("Escolha um slot para CARREGAR");
                        if (slot != -1 && LoadGameSlot(&player, slot)) {
                            inGame = true;
                        }
                        break;
                    }
                    case 2:
                        ShowHighScores("highscores_kl.bin");
                        break;
                    case 3:
                        shouldClose = true;
                        break;
                }
            }

            BeginDrawing();
            ClearBackground(BLACK);
            DrawBlurredTexture(background, screenWidth, screenHeight);
            DrawMenu(&menu, screenWidth, screenHeight);
            EndDrawing();
        } else {
            char mapFile[32];
            snprintf(mapFile, sizeof(mapFile), "mapa%02d.txt", player.level);

            FILE *file = fopen(mapFile, "r");
            if (!file) {
                HighScore scores[MAX_SCORES];
                LoadHighScores(scores, "highscores_kl.bin");
                int topPos = CheckHighScorePosition(scores, player.score);
                ShowVictoryScreen(player, topPos);

                if (topPos != -1) {
                    UpdateHighScores(scores, "highscores_kl.bin", player.score);
                }

                inGame = false;
                continue;
            }
            fclose(file);

            bool gameRunning = RunGame(mapFile, &player);
            if (!gameRunning) {
                inGame = false;
            } else {
                player.level++;
            }
        }
    }

    UnloadTexture(background);
    UnloadSound(hoverSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void CreateGameDirectory(const char *path) {
    #if defined(_WIN32)
    _mkdir(path);
    #else
    mkdir(path, 0777);
    #endif
}

void DrawBlurredTexture(Texture2D texture, int screenWidth, int screenHeight) {
    Vector2 pos = {0, 0};
    Rectangle sourceRec = {0, 0, (float)texture.width, (float)-texture.height};
    Color tint = (Color){255, 255, 255, 30};

    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            DrawTextureRec(texture, sourceRec, (Vector2){pos.x + x, pos.y + y}, tint);
        }
    }
}

void InitMenu(Menu *menu) {
    menu->options[0] = "1. Novo Jogo";
    menu->options[1] = "2. Carregar Jogo";
    menu->options[2] = "3. Scoreboard";
    menu->options[3] = "4. Sair";
    menu->selected = 0;
    menu->fontSize = 40;
    menu->spacing = 60;
}

void DrawMenu(Menu *menu, int screenWidth, int screenHeight) {
    const char *title = "ZINF";
    int baseFontSize = 80;
    int titleY = screenHeight / 4;

    float scale = 1.0f;
    int scaledFontSize = (int)(baseFontSize * scale);

    int textWidth = MeasureText(title, scaledFontSize);
    int textX = (screenWidth - textWidth) / 2;

    DrawText(title, textX, titleY, scaledFontSize, TITLE_COLOR);

    for (int i = 0; i < 4; i++) {
        int optionWidth = MeasureText(menu->options[i], menu->fontSize);
        int optionX = (screenWidth - optionWidth) / 2;
        int optionY = titleY + 150 + i * menu->spacing;

        if (i == menu->selected) {
            DrawRectangleLines(optionX - 20, optionY - 10, optionWidth + 40, menu->fontSize + 20, RAYWHITE);
            DrawText(menu->options[i], optionX, optionY, menu->fontSize, RAYWHITE);
        } else {
            DrawText(menu->options[i], optionX, optionY, menu->fontSize, MENU_COLOR);
        }
    }

    const char *footer = "UFRGS - Algoritmos e Programacao 2025";
    DrawText(footer, (screenWidth - MeasureText(footer, 20)) / 2, screenHeight - 40, 20, GRAY);
}

void UpdateMenu(Menu *menu, int screenWidth, int screenHeight, Sound hoverSound, float deltaTime) {
    static int lastHovered = -1;
    bool hoveringAny = false;

    if (IsKeyPressed(KEY_DOWN)) {
        menu->selected = (menu->selected + 1) % 4;
        PlaySound(hoverSound);
    } else if (IsKeyPressed(KEY_UP)) {
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
}

int ChooseSaveSlot(const char *prompt) {
    int selectedSlot = 1;
    bool choosing = true;

    while (choosing && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawText(prompt, (SCREENWIDTH - MeasureText(prompt, 40)) / 2, 150, 40, YELLOW);

        for (int i = 1; i <= 5; i++) {
            Color color = (i == selectedSlot) ? RED : WHITE;
            char slotText[20];
            snprintf(slotText, sizeof(slotText), "Slot %d", i);

            int x = (SCREENWIDTH - 200) / 2;
            int y = 200 + i * 60;
            DrawText(slotText, x, y, 40, color);
        }

        DrawText("Use UP/DOWN para mudar, ENTER para confirmar", 300, SCREENHEIGHT - 100, 20, GRAY);
        EndDrawing();

        if (IsKeyPressed(KEY_DOWN)) selectedSlot = (selectedSlot % 5) + 1;
        else if (IsKeyPressed(KEY_UP)) selectedSlot = (selectedSlot - 2 + 5) % 5 + 1;
        else if (IsKeyPressed(KEY_ENTER)) choosing = false;
        else if (IsKeyPressed(KEY_ESCAPE)) return -1;
    }

    return selectedSlot;
}

bool LoadGameSlot(Player *player, int slot) {
    if (slot < 1 || slot > 5) return false;

    char filename[64];
    snprintf(filename, sizeof(filename), "saves/save_slot%d.bin", slot);

    FILE *file = fopen(filename, "rb");
    if (!file) return false;

    bool success = fread(player, sizeof(Player), 1, file) == 1;
    fclose(file);
    return success;
}

void ShowHighScores(const char *filename) {
    HighScore scores[MAX_SCORES];
    LoadHighScores(scores, filename);

    bool waiting = true;
    while (waiting && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("SCOREBOARD - TOP 5", (SCREENWIDTH - MeasureText("SCOREBOARD - TOP 5", 50)) / 2, 100, 50, BLACK);

        for (int i = 0; i < MAX_SCORES; i++) {
            char entry[100];
            snprintf(entry, sizeof(entry), "%d. %-20s %d", i + 1, scores[i].name, scores[i].score);
            DrawText(entry, (SCREENWIDTH - MeasureText(entry, 30)) / 2, 200 + i * 50, 30, DARKBLUE);
        }

        DrawText("Pressione ESC para voltar...", 300, SCREENHEIGHT - 100, 20, GRAY);
        EndDrawing();

        if (IsKeyPressed(KEY_ESCAPE)) waiting = false;
    }
}

void LoadHighScores(HighScore scores[MAX_SCORES], const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        for (int i = 0; i < MAX_SCORES; i++) {
            strcpy(scores[i].name, "---");
            scores[i].score = 0;
        }
        return;
    }
    fread(scores, sizeof(HighScore), MAX_SCORES, file);
    fclose(file);
}

int CheckHighScorePosition(HighScore scores[MAX_SCORES], int newScore) {
    for (int i = 0; i < MAX_SCORES; i++) {
        if (newScore > scores[i].score) return i;
    }
    return -1;
}

void ShowVictoryScreen(Player player, int topPosition) {
    bool waiting = true;
    while (waiting && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawText("Parabens! Todas as fases concluidas!", 300, SCREENHEIGHT / 2, 40, YELLOW);
        DrawText(TextFormat("Pontuacao final: %d", player.score), 300, SCREENHEIGHT / 2 + 60, 30, WHITE);

        if (topPosition != -1) {
            char msg[100];
            snprintf(msg, sizeof(msg), "Com essa pontuacao voce ficou no top %d!", topPosition + 1);
            DrawText(msg, 300, SCREENHEIGHT / 2 + 120, 30, GREEN);
        }

        DrawText("Pressione ENTER para continuar...", 300, SCREENHEIGHT / 2 + 200, 20, GRAY);
        EndDrawing();

        if (IsKeyPressed(KEY_ENTER)) waiting = false;
    }
}

int UpdateHighScores(HighScore scores[MAX_SCORES], const char *filename, int newScore) {
    int pos = CheckHighScorePosition(scores, newScore);
    if (pos == -1) return -1;

    for (int i = MAX_SCORES - 1; i > pos; i--) {
        scores[i] = scores[i - 1];
    }

    char name[NAME_LENGTH] = {0};
    bool nameEntered = false;
    int letterCount = 0;

    while (!nameEntered && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawText("NOVA PONTUACAO ALTA!", (SCREENWIDTH - MeasureText("NOVA PONTUACAO ALTA!", 40)) / 2, 200, 40, YELLOW);
        DrawText(TextFormat("Posicao: %d", pos + 1), (SCREENWIDTH - MeasureText(TextFormat("Posicao: %d", pos + 1), 30)) / 2, 250, 30, WHITE);
        DrawText("Digite seu nome:", (SCREENWIDTH - MeasureText("Digite seu nome:", 30)) / 2, 300, 30, WHITE);

        // Desenha o retângulo do input
        Rectangle textBox = { (SCREENWIDTH - 300) / 2, 350, 300, 40 };
        DrawRectangleRec(textBox, LIGHTGRAY);
        DrawRectangleLinesEx(textBox, 2, DARKGRAY);
        DrawText(name, textBox.x + 5, textBox.y + 10, 20, MAROON);

        // Verifica entrada de teclado
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (letterCount < NAME_LENGTH - 1)) {
                name[letterCount] = (char)key;
                name[letterCount + 1] = '\0';
                letterCount++;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            letterCount--;
            if (letterCount < 0) letterCount = 0;
            name[letterCount] = '\0';
        }

        if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
            nameEntered = true;
        }

        EndDrawing();
    }

    strncpy(scores[pos].name, name, NAME_LENGTH - 1);
    scores[pos].score = newScore;

    SaveHighScores(scores, filename);
    return pos;
}

void SaveHighScores(HighScore scores[MAX_SCORES], const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Erro ao salvar o arquivo de pontuacoes.\n");
        return;
    }
    fwrite(scores, sizeof(HighScore), MAX_SCORES, file);
    fclose(file);
}

bool RunGame(const char *mapFile, Player *player) {
    char map[ROWS][COLS];
    LoadMapFromFile(map, mapFile);

    LocatePlayer(map, player);

    AttackEffect attackEffect = {0};
    MonsterDeathManager deathManager = {0};
    MonsterManager monsterManager = {0};
    InitializeMonsters(map, &monsterManager);

    int frameCount = 0;
    bool gameRunning = true;
    int monsterMoveCounter = 0;

    while (!WindowShouldClose() && gameRunning) {
        frameCount++;
        monsterMoveCounter++;

        UpdatePlayer(map, player);

        if (monsterMoveCounter >= MONSTER_MOVE_INTERVAL) {
            UpdateMonsters(map, &monsterManager, player);
            monsterMoveCounter = 0;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawHUD(player);
        DrawMap(map, player, frameCount);

        if (attackEffect.active) {
            for (int i = 0; i < 3; i++) {
                int tx = attackEffect.tiles[i].x;
                int ty = attackEffect.tiles[i].y;
                if (tx >= 0 && tx < COLS && ty >= 0 && ty < ROWS) {
                    Rectangle area = {tx * TILE_SIZE, ty * TILE_SIZE + HUD_HEIGHT, TILE_SIZE, TILE_SIZE};
                    DrawRectangleRec(area, Fade(GOLD, 0.5f));
                }
            }
            if (--attackEffect.frameCounter <= 0) attackEffect.active = 0;
        }

        DrawMonsterDeaths(&deathManager);
        DrawText("WASD para mover | J para atacar | TAB para pausar | ESC para sair", 700, SCREENHEIGHT-30, 20, DARKGRAY);

        bool allDefeated = true;
        for (int i = 0; i < monsterManager.count; i++) {
            if (monsterManager.monsters[i].active) {
                allDefeated = false;
                break;
            }
        }

        if (allDefeated) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Fase concluida!", (SCREENWIDTH - MeasureText("Fase concluida!", 60)) / 2, SCREENHEIGHT / 2, 60, GREEN);
            EndDrawing();
            WaitTime(2.0);
            return true;
        }

        EndDrawing();

        if (IsKeyPressed(KEY_TAB)) {
            PauseAction action = ShowPauseMenu();
            switch (action) {
                case PAUSE_CONTINUE:
                    break;
                case PAUSE_SAVE: {
                    int slot = ChooseSaveSlot("Escolha um slot para SALVAR");
                    if (slot != -1) {
                        SaveGameSlot(player, slot);
                    }
                    break;
                }
                case PAUSE_RETURN_MENU:
                    return false;
                case PAUSE_EXIT_GAME:
                    CloseWindow();
                    exit(0);
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) return false;
        if (player->isBlinking && --player->blinkFrames <= 0) player->isBlinking = false;
        if (IsKeyPressed(KEY_J)) PerformSwordAttack(map, player, &attackEffect, &deathManager, &monsterManager);
    }

    return false;
}

void LoadMapFromFile(char map[ROWS][COLS], const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", filename);
        exit(1);
    }

    char line[COLS + 2];
    for (int row = 0; row < ROWS && fgets(line, sizeof(line), file); row++) {
        for (int col = 0; col < COLS; col++) {
            map[row][col] = line[col];
        }
    }
    fclose(file);
}

void LocatePlayer(char map[ROWS][COLS], Player *player) {
    player->swordActive = false;
    player->isBlinking = false;
    player->blinkFrames = 0;
    player->facingRow = 0;
    player->facingCol = 1;

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (map[i][j] == 'J') {
                player->row = i;
                player->col = j;
                return;
            }
        }
    }
}

void UpdatePlayer(char map[ROWS][COLS], Player *player) {
    int dirRow = 0, dirCol = 0;
    if (IsKeyPressed(KEY_W)) { dirRow = -1; player->facingRow = -1; player->facingCol = 0; }
    else if (IsKeyPressed(KEY_S)) { dirRow = 1; player->facingRow = 1; player->facingCol = 0; }
    else if (IsKeyPressed(KEY_A)) { dirCol = -1; player->facingRow = 0; player->facingCol = -1; }
    else if (IsKeyPressed(KEY_D)) { dirCol = 1; player->facingRow = 0; player->facingCol = 1; }

    int newRow = player->row + dirRow;
    int newCol = player->col + dirCol;

    if ((dirRow || dirCol) &&
        newRow >= 0 && newRow < ROWS && newCol >= 0 && newCol < COLS &&
        map[newRow][newCol] != 'P') {

        char target = map[newRow][newCol];

    if (target == 'M') {
        if (!player->isBlinking) {
            player->lives--;
            player->isBlinking = true;
            player->blinkFrames = 30;
        }
        map[player->row][player->col] = ' ';
        map[newRow][newCol] = 'J';
        player->row = newRow;
        player->col = newCol;
        return;
    }

    if (target == 'V') {
        player->lives++;
        player->score += LIFE_SCORE;
    } else if (target == 'E') {
        player->score += SWORD_SCORE;
        player->swordActive = true;
    }

    map[player->row][player->col] = ' ';
    map[newRow][newCol] = 'J';
    player->row = newRow;
    player->col = newCol;
        }
}

void DrawHUD(const Player *player) {
    DrawRectangle(0, 0, SCREENWIDTH, HUD_HEIGHT, DARKGRAY);
    DrawText(TextFormat("Pontuacao: %d", player->score), 20, 20, 20, WHITE);
    DrawText(TextFormat("Vidas: %d", player->lives), 300, 20, 20, WHITE);
    DrawText(TextFormat("Nivel: %d", player->level), 550, 20, 20, WHITE);

    if (player->swordActive) {
        DrawText("ESPADA ATIVA", 800, 20, 20, GOLD);
    }
}

void DrawMap(char map[ROWS][COLS], const Player *player, int frameCount) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            Rectangle tile = {j * TILE_SIZE, i * TILE_SIZE + HUD_HEIGHT, TILE_SIZE, TILE_SIZE};
            Color color;
            switch (map[i][j]) {
                case 'P': color = GRAY; break;
                case 'J': color = (player->isBlinking && (frameCount / 5) % 2 == 0) ? BLANK :
                    (player->swordActive ? DARKBLUE : BLUE); break;
                case 'M': color = RED; break;
                case 'V': color = GREEN; break;
                case 'E': color = GOLD; break;
                case ' ': color = RAYWHITE; break;
                default: color = LIGHTGRAY; break;
            }
            DrawRectangleRec(tile, color);
            DrawRectangleLinesEx(tile, 1, LIGHTGRAY);
        }
    }
}

void DrawMonsterDeaths(MonsterDeathManager *deaths) {
    for (int i = 0; i < deaths->count;) {
        if (--deaths->deaths[i].frameCounter <= 0) {
            for (int j = i; j < deaths->count - 1; j++) deaths->deaths[j] = deaths->deaths[j + 1];
            deaths->count--;
        } else {
            Rectangle deathTile = {deaths->deaths[i].col * TILE_SIZE, deaths->deaths[i].row * TILE_SIZE + HUD_HEIGHT, TILE_SIZE, TILE_SIZE};
            float alpha = deaths->deaths[i].frameCounter / (float)MONSTER_DEATH_DURATION;
            DrawRectangleRec(deathTile, Fade(RED, alpha));
            i++;
        }
    }
}

void PerformSwordAttack(char map[ROWS][COLS], Player *player, AttackEffect *effect,
                        MonsterDeathManager *deathManager, MonsterManager *monsterManager) {
    if (!player->swordActive) return;

    bool hitMonster = false;
    int idx = 0;

    for (int i = 1; i <= 3; i++) {
        int tr = player->row + i * player->facingRow;
        int tc = player->col + i * player->facingCol;
        if (tr >= 0 && tr < ROWS && tc >= 0 && tc < COLS) {
            if (map[tr][tc] == 'M') {
                hitMonster = true;
                if (deathManager->count < MAX_DEATH_ANIMATIONS) {
                    deathManager->deaths[deathManager->count].row = tr;
                    deathManager->deaths[deathManager->count].col = tc;
                    deathManager->deaths[deathManager->count].frameCounter = MONSTER_DEATH_DURATION;
                    deathManager->count++;
                }
                map[tr][tc] = ' ';
                RemoveMonsterAt(monsterManager, tr, tc);
                player->score += MONSTER_SCORE;
            }
            effect->tiles[idx++] = (Vector2){tc, tr};
        }
    }

    if (hitMonster) {
        effect->active = 1;
        effect->frameCounter = ATTACK_DURATION;
    }
                        }

                        void InitializeMonsters(char map[ROWS][COLS], MonsterManager *monsterManager) {
                            monsterManager->count = 0;
                            for (int i = 0; i < ROWS; i++) {
                                for (int j = 0; j < COLS; j++) {
                                    if (map[i][j] == 'M') {
                                        if (monsterManager->count < MAX_MONSTERS) {
                                            monsterManager->monsters[monsterManager->count++] = (Monster){i, j, true};
                                        } else {
                                            printf("Aviso: Número máximo de monstros atingido.\n");
                                        }
                                    }
                                }
                            }
                        }

                        void UpdateMonsters(char map[ROWS][COLS], MonsterManager *monsterManager, Player *player) {
                            for (int i = 0; i < monsterManager->count; i++) {
                                Monster *m = &monsterManager->monsters[i];
                                if (!m->active) continue;

                                int direction = GetRandomValue(0, 3);
                                int dRow = 0, dCol = 0;
                                if (direction == 0) dRow = -1; else if (direction == 1) dRow = 1;
                                else if (direction == 2) dCol = -1; else if (direction == 3) dCol = 1;

                                int newRow = m->row + dRow;
                                int newCol = m->col + dCol;

                                if (newRow >= 0 && newRow < ROWS && newCol >= 0 && newCol < COLS) {
                                    char targetCell = map[newRow][newCol];
                                    if (targetCell == ' ') {
                                        map[m->row][m->col] = ' ';
                                        map[newRow][newCol] = 'M';
                                        m->row = newRow;
                                        m->col = newCol;
                                    } else if (targetCell == 'J') {
                                        if (!player->isBlinking) {
                                            player->lives--;
                                            player->isBlinking = true;
                                            player->blinkFrames = 30;
                                        }
                                    }
                                }
                            }
                        }

                        void RemoveMonsterAt(MonsterManager *manager, int row, int col) {
                            for (int i = 0; i < manager->count;) {
                                Monster *m = &manager->monsters[i];
                                if (m->active && m->row == row && m->col == col) {
                                    m->active = false;
                                    // Remove o monstro inativo do array
                                    for (int j = i; j < manager->count - 1; j++) {
                                        manager->monsters[j] = manager->monsters[j + 1];
                                    }
                                    manager->count--;
                                } else {
                                    i++;
                                }
                            }
                        }

                        PauseAction ShowPauseMenu() {
                            bool waiting = true;
                            PauseAction action = PAUSE_CONTINUE;
                            int selected = 0;
                            const char *options[] = {
                                "Continuar",
                                "Salvar Jogo",
                                "Voltar ao Menu Principal",
                                "Fechar o Jogo"
                            };

                            while (waiting && !WindowShouldClose()) {
                                BeginDrawing();
                                ClearBackground(Fade(DARKGRAY, 0.8f));

                                DrawText("=== PAUSE MENU ===", (SCREENWIDTH - MeasureText("=== PAUSE MENU ===", 50)) / 2, 200, 50, RAYWHITE);

                                for (int i = 0; i < 4; i++) {
                                    Color color = (i == selected) ? RED : WHITE;
                                    DrawText(options[i], (SCREENWIDTH - MeasureText(options[i], 30)) / 2, 300 + i * 50, 30, color);
                                }

                                EndDrawing();

                                if (IsKeyPressed(KEY_DOWN)) selected = (selected + 1) % 4;
                                else if (IsKeyPressed(KEY_UP)) selected = (selected - 1 + 4) % 4;
                                else if (IsKeyPressed(KEY_ENTER)) {
                                    switch (selected) {
                                        case 0: action = PAUSE_CONTINUE; waiting = false; break;
                                        case 1: action = PAUSE_SAVE; waiting = false; break;
                                        case 2: action = PAUSE_RETURN_MENU; waiting = false; break;
                                        case 3: action = PAUSE_EXIT_GAME; waiting = false; break;
                                    }
                                }
                            }

                            return action;
                        }

                        void SaveGameSlot(Player *player, int slot) {
                            if (slot < 1 || slot > 5) {
                                printf("Slot invalido. Escolha entre 1 e 5.\n");
                                return;
                            }

                            char filename[64];
                            snprintf(filename, sizeof(filename), "saves/save_slot%d.bin", slot);

                            FILE *file = fopen(filename, "wb");
                            if (!file) {
                                fprintf(stderr, "Erro ao salvar o jogo no slot %d.\n", slot);
                                return;
                            }

                            fwrite(player, sizeof(Player), 1, file);
                            fclose(file);

                            // Mostra mensagem de confirmação
                            BeginDrawing();
                            ClearBackground(DARKGRAY);
                            DrawText(TextFormat("Jogo salvo no slot %d!", slot),
                                     (SCREENWIDTH - MeasureText(TextFormat("Jogo salvo no slot %d!", slot), 30)) / 2,
                                     SCREENHEIGHT / 2, 30, GREEN);
                            EndDrawing();
                            WaitTime(1.0);
                        }
