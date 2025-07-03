#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

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

void LoadMapFromFile(char map[ROWS][COLS], const char *filename);
bool RunGame(const char *mapFile, Player *player);
void InitMenu(Menu *menu);
void DrawMenu(const Menu *menu);
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



int main(void) {
    InitWindow(SCREENWIDTH, SCREENHEIGHT, "ZINF - Trabalho Final");
    SetTargetFPS(60);

    Menu menu;
    InitMenu(&menu);

    bool shouldClose = false;

    while (!WindowShouldClose() && !shouldClose) {
        if (IsKeyPressed(KEY_ONE)) {
            Player player = {0};
            player.lives = 3;
            player.score = 0;
            player.level = 1;

            bool gameRunning = true;
            while (gameRunning) {
                char mapFile[32];
                snprintf(mapFile, sizeof(mapFile), "mapa%02d.txt", player.level);

                FILE *file = fopen(mapFile, "r");
                if (!file) {

                    HighScore scores[MAX_SCORES];
                    LoadHighScores(scores, "highscores_kl.bin");

                    int topPos = CheckHighScorePosition(scores, player.score); // verifica posição

                    ShowVictoryScreen(player, topPos); // mostra tela de vitória COM a posição

                    if (topPos != -1) {
                        UpdateHighScores(scores, "highscores_kl.bin", player.score); // só depois, atualiza
                    }

                    break;
                }
                fclose(file);

                gameRunning = RunGame(mapFile, &player);
            }
                } else if (IsKeyPressed(KEY_TWO)) {
                        int slot = ChooseSaveSlot("Escolha um slot para CARREGAR");
                        if (slot != -1) {
                            Player loadedPlayer;
                            if (LoadGameSlot(&loadedPlayer, slot)) {
                                bool gameRunning = true;
                                while (gameRunning) {
                                    char mapFile[32];
                                    snprintf(mapFile, sizeof(mapFile), "mapa%02d.txt", loadedPlayer.level);

                                    FILE *file = fopen(mapFile, "r");
                                    if (!file) {
                                        HighScore scores[MAX_SCORES];
                                        LoadHighScores(scores, "highscores_kl.bin");
                                        int topPos = UpdateHighScores(scores, "highscores_kl.bin", loadedPlayer.score);
                                        ShowVictoryScreen(loadedPlayer, topPos);
                                        break;
                                    }
                                    fclose(file);

                                    gameRunning = RunGame(mapFile, &loadedPlayer);
                                }
                            } else {
                                printf("Falha ao carregar o slot %d!\n", slot);
                            }
                        }
} else if (IsKeyPressed(KEY_THREE)) {
            ShowHighScores("highscores_kl.bin");
        } else if (IsKeyPressed(KEY_FOUR)) {
            shouldClose = true;
        }

        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        DrawMenu(&menu);
        EndDrawing();
    }

    CloseWindow();
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

void DrawMenu(const Menu *menu) {
    const char *title = "ZINF";
    int titleFontSize = 80;
    int titleY = SCREENHEIGHT / 4;

    DrawText(title,
             (SCREENWIDTH - MeasureText(title, titleFontSize)) / 2,
             titleY, titleFontSize, TITLE_COLOR);

    for (int i = 0; i < 4; i++) {
        Color color = (i == menu->selected) ? HIGHLIGHT_COLOR : MENU_COLOR;
        DrawText(menu->options[i],
                 (SCREENWIDTH - MeasureText(menu->options[i], menu->fontSize)) / 2,
                 titleY + 150 + i * menu->spacing,
                 menu->fontSize, color);
    }

    const char *footer = "UFRGS - Algoritmos e Programacao 2025";
    DrawText(footer,
             (SCREENWIDTH - MeasureText(footer, 20)) / 2,
             SCREENHEIGHT - 40, 20, GRAY);
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
            player->level++;
            return true;
        }

        EndDrawing();


        if (IsKeyPressed(KEY_TAB)) {
            bool pauseMenuOpen = true;
            while (pauseMenuOpen && !WindowShouldClose()) {
                BeginDrawing();
                ClearBackground(DARKGRAY);

                DrawText("PAUSE MENU", (SCREENWIDTH - MeasureText("PAUSE MENU", 50)) / 2, 200, 50, YELLOW);
                DrawText("C - Continuar", 400, 300, 40, WHITE);
                DrawText("S - Salvar Jogo", 400, 360, 40, WHITE);
                DrawText("V - Voltar ao menu principal", 400, 420, 40, WHITE);
                DrawText("F - Fechar jogo", 400, 480, 40, WHITE);

                EndDrawing();

                if (IsKeyPressed(KEY_C)) {
                    pauseMenuOpen = false; // Continua jogo
                }

                if (IsKeyPressed(KEY_S)) {
                    int slot = ChooseSaveSlot("Escolha um slot para SALVAR");
                    if (slot != -1) {
                        SaveGameSlot(player, slot);
                    }
                    pauseMenuOpen = false; // Continua o jogo mesmo que o jogador cancele
                }

                if (IsKeyPressed(KEY_V)) {
                    return false; // Volta ao menu principal
                }

                if (IsKeyPressed(KEY_F)) {
                    CloseWindow();
                    exit(0); // Sai do jogo
                }
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) return false;
        if (player->isBlinking && --player->blinkFrames <= 0) player->isBlinking = false;
        if (IsKeyPressed(KEY_J)) PerformSwordAttack(map, player, &attackEffect, &deathManager, &monsterManager);
    }


    return false;
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
}

void DrawMap(char map[ROWS][COLS], const Player *player, int frameCount) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            Rectangle tile = {j * TILE_SIZE, i * TILE_SIZE + HUD_HEIGHT, TILE_SIZE, TILE_SIZE};
            Color color;
            switch (map[i][j]) {
                case 'P': color = GRAY; break;
                case 'J': color = (player->isBlinking && (frameCount / 5) % 2 == 0) ? BLANK : (player->swordActive ? DARKBLUE : BLUE); break;
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

void PerformSwordAttack(char map[ROWS][COLS], Player *player, AttackEffect *effect, MonsterDeathManager *deathManager, MonsterManager *monsterManager) {
    if (!player->swordActive) return;
    effect->active = 1;
    effect->frameCounter = ATTACK_DURATION;
    int idx = 0;
    for (int i = 1; i <= 3; i++) {
        int tr = player->row + i * player->facingRow;
        int tc = player->col + i * player->facingCol;
        if (tr >= 0 && tr < ROWS && tc >= 0 && tc < COLS) {
            if (map[tr][tc] == 'M') {
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
}

void InitializeMonsters(char map[ROWS][COLS], MonsterManager *monsterManager) {
    monsterManager->count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (map[i][j] == 'M') {
                if (monsterManager->count < MAX_MONSTERS) {
                    monsterManager->monsters[monsterManager->count++] = (Monster){i, j, true};
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
    for (int i = 0; i < manager->count; i++) {
        Monster *m = &manager->monsters[i];
        if (m->active && m->row == row && m->col == col) {
            m->active = false;
            return;
        }
    }
}

void LoadHighScores(HighScore scores[MAX_SCORES], const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        // Se não existir, inicializa com pontuações zero
        for (int i = 0; i < MAX_SCORES; i++) {
            strcpy(scores[i].name, "---");
            scores[i].score = 0;
        }
        return;
    }
    fread(scores, sizeof(HighScore), MAX_SCORES, file);
    fclose(file);
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

int UpdateHighScores(HighScore scores[MAX_SCORES], const char *filename, int newScore) {
    int insertPos = -1;
    for (int i = 0; i < MAX_SCORES; i++) {
        if (newScore > scores[i].score) {
            insertPos = i;
            break;
        }
    }

    if (insertPos != -1) {
        // Desloca pontuações para baixo
        for (int i = MAX_SCORES - 1; i > insertPos; i--) {
            scores[i] = scores[i - 1];
        }

        // Pergunta nome do jogador
        char playerName[NAME_LENGTH] = {0};
        printf("Voce conseguiu entrar no TOP 5! Digite seu nome: ");
        fgets(playerName, sizeof(playerName), stdin);

        size_t len = strlen(playerName);
        if (len > 0 && playerName[len - 1] == '\n') playerName[len - 1] = '\0';

        strncpy(scores[insertPos].name, playerName, NAME_LENGTH - 1);
        scores[insertPos].score = newScore;

        SaveHighScores(scores, filename);
    }

    return insertPos;  // Retorna a posição ou -1
}

void ShowHighScores(const char *filename) {
    HighScore scores[MAX_SCORES];
    LoadHighScores(scores, filename);

    bool waiting = true;
    while (!WindowShouldClose() && waiting) {
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

        if (IsKeyPressed(KEY_ESCAPE)) {
            waiting = false;
        }
    }
}

void ShowVictoryScreen(Player player, int topPosition) {
    bool waiting = true;
    while (!WindowShouldClose() && waiting) {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawText("Parabens! Todas as fases concluidas!", 300, SCREENHEIGHT / 2, 40, YELLOW);
        DrawText(TextFormat("Pontuacao final: %d", player.score), 300, SCREENHEIGHT / 2 + 60, 30, WHITE);

        if (topPosition != -1) {
            char msg[100];
            snprintf(msg, sizeof(msg), "Com essa pontuacao voce ficou no top %d!\nDigite seu nome no terminal", topPosition + 1);
            DrawText(msg, 300, SCREENHEIGHT / 2 + 120, 30, GREEN);
        }

        DrawText("Pressione ENTER continuar...", 300, SCREENHEIGHT / 2 + 200, 20, GRAY);

        EndDrawing();

        if (IsKeyPressed(KEY_ENTER)) {
            waiting = false;
        }
    }
}

int CheckHighScorePosition(HighScore scores[MAX_SCORES], int newScore) {
    for (int i = 0; i < MAX_SCORES; i++) {
        if (newScore > scores[i].score) {
            return i;
        }
    }
    return -1;
}

PauseAction ShowPauseMenu() {
    bool waiting = true;
    PauseAction action = PAUSE_CONTINUE;  // padrão: continuar se nada for escolhido

    while (!WindowShouldClose() && waiting) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawText("=== PAUSE MENU ===", 300, 200, 50, RAYWHITE);
        DrawText("C - Continuar", 300, 300, 30, WHITE);
        DrawText("S - Salvar Jogo", 300, 350, 30, WHITE);
        DrawText("V - Voltar ao Menu Principal (sem salvar)", 300, 400, 30, WHITE);
        DrawText("F - Fechar o Jogo (sem salvar)", 300, 450, 30, WHITE);
        EndDrawing();

        if (IsKeyPressed(KEY_C)) {
            action = PAUSE_CONTINUE;
            waiting = false;
        } else if (IsKeyPressed(KEY_S)) {
            action = PAUSE_SAVE;
            waiting = false;
        } else if (IsKeyPressed(KEY_V)) {
            action = PAUSE_RETURN_MENU;
            waiting = false;
        } else if (IsKeyPressed(KEY_F)) {
            action = PAUSE_EXIT_GAME;
            waiting = false;
        }
    }

    return action;
}

void SaveGame(const Player *player, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Erro ao salvar o jogo em %s.\n", filename);
        return;
    }

    fwrite(player, sizeof(Player), 1, file);
    fclose(file);

    printf("Jogo salvo em '%s'.\n", filename);
}

void SaveGameSlot(Player *player, int slot) {
    if (slot < 1 || slot > 5) {
        printf("Slot invalido. Escolha entre 1 e 5.\n");
        return;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "save_slot%d.bin", slot);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Erro ao salvar o jogo no slot %d.\n", slot);
        return;
    }

    fwrite(player, sizeof(Player), 1, file);
    fclose(file);

    printf("Jogo salvo com sucesso no slot %d!\n", slot);
}

bool LoadGameSlot(Player *player, int slot) {
    if (slot < 1 || slot > 5) {
        printf("Slot invalido. Escolha entre 1 e 5.\n");
        return false;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "save_slot%d.bin", slot);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Nenhum jogo salvo no slot %d.\n", slot);
        return false;
    }

    fread(player, sizeof(Player), 1, file);
    fclose(file);

    printf("Jogo carregado do slot %d!\n", slot);
    return true;
}

int ChooseSaveSlot(const char *prompt) {
    int selectedSlot = 1; // Começa no slot 1

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

        if (IsKeyPressed(KEY_DOWN)) {
            if (selectedSlot < 5) selectedSlot++;
        } else if (IsKeyPressed(KEY_UP)) {
            if (selectedSlot > 1) selectedSlot--;
        } else if (IsKeyPressed(KEY_ENTER)) {
            choosing = false;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            return -1; // Cancelado
        }
    }

    return selectedSlot;
}



