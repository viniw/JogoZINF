/*Protótipo 0.1 do Jogo - ZINF*/
/*Uma ideia de base para o jogo*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"

// Definições de constantes
#define MAX_MONSTROS 10
#define MAX_VIDAS_EXTRAS 5
#define LARGURA_TELA 1200
#define ALTURA_TELA 860  // 800 (cenário) + 60 (barra de status)
#define TAMANHO_BLOCO 50
#define LINHAS_MAPA 16
#define COLUNAS_MAPA 24

// Estruturas de dados
typedef struct {
    int x, y;
    int direcao; // 0=N, 1=S, 2=L, 3=O
    int pontos;
    bool ativo;
} Monstro;

typedef struct {
    int x, y;
    bool coletada;
} VidaExtra;

typedef struct {
    int x, y;
    bool coletada;
} Espada;

typedef struct {
    int x, y;
    int direcao; // 0=N, 1=S, 2=L, 3=O
    int vidas;
    int pontos;
    bool temEspada;
    bool espadaAtivada;
    int tempoEspada;
} Jogador;

typedef struct {
    char nome[20];
    int score;
} TIPO_SCORE;

typedef struct {
    char mapa[LINHAS_MAPA][COLUNAS_MAPA];
    int numMonstros;
    int numVidasExtras;
    Monstro monstros[MAX_MONSTROS];
    VidaExtra vidasExtras[MAX_VIDAS_EXTRAS];
    Espada espada;
    Jogador jogador;
} Fase;

// Variáveis globais
Fase fases[4];
int faseAtual = 0;
bool jogoEmAndamento = false;
bool jogoPausado = false;
TIPO_SCORE ranking[5];

// Protótipos de funções
void CarregarRanking();
void SalvarRanking();
void CarregarFase(int numFase);
void InicializarJogo();
void AtualizarJogo();
void DesenharJogo();
void DesenharMenu();
void DesenharGameOver(bool vitoria);
void MoverJogador(int direcao);
void MoverMonstros();
void AtivarEspada();
void VerificarColisoes();
void ProximaFase();
void ReiniciarFase();

// Função principal
int main() {
    // Inicializa a janela do jogo
    InitWindow(LARGURA_TELA, ALTURA_TELA, "ZINF - Zelda INF");
    SetTargetFPS(60);

    // Carrega os rankings
    CarregarRanking();

    // Carrega as fases
    for (int i = 0; i < 4; i++) {
        CarregarFase(i + 1);
    }

    // Loop principal do jogo
    while (!WindowShouldClose()) {
        if (jogoEmAndamento && !jogoPausado) {
            AtualizarJogo();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (jogoEmAndamento) {
            DesenharJogo();
            if (jogoPausado) {
                // Desenha menu de pause sobre o jogo
                DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.5f));
                DrawText("Jogo Pausado", LARGURA_TELA/2 - 100, ALTURA_TELA/2 - 60, 40, WHITE);
                DrawText("C - Continuar", LARGURA_TELA/2 - 80, ALTURA_TELA/2, 20, WHITE);
                DrawText("V - Voltar ao Menu", LARGURA_TELA/2 - 100, ALTURA_TELA/2 + 30, 20, WHITE);
                DrawText("F - Sair", LARGURA_TELA/2 - 50, ALTURA_TELA/2 + 60, 20, WHITE);
            }
        } else {
            DesenharMenu();
        }

        EndDrawing();

        // Verifica teclas para menu de pause
        if (IsKeyPressed(KEY_TAB) {
            if (jogoEmAndamento) {
                jogoPausado = !jogoPausado;
            }
        }

        // Processa opções do menu de pause
        if (jogoPausado) {
            if (IsKeyPressed(KEY_C)) {
                jogoPausado = false;
            } else if (IsKeyPressed(KEY_V)) {
                jogoEmAndamento = false;
                jogoPausado = false;
            } else if (IsKeyPressed(KEY_F)) {
                CloseWindow();
                return 0;
            }
        }
    }

    CloseWindow();
    return 0;
}

// Implementação das funções

void CarregarRanking() {
    // Tenta abrir o arquivo de ranking
    FILE *arquivo = fopen("ranking.bin", "rb");
    if (arquivo != NULL) {
        fread(ranking, sizeof(TIPO_SCORE), 5, arquivo);
        fclose(arquivo);
    } else {
        // Se o arquivo não existir, cria um ranking padrão
        for (int i = 0; i < 5; i++) {
            sprintf(ranking[i].nome, "Jogador %d", i+1);
            ranking[i].score = (5 - i) * 1000;
        }
    }
}

void SalvarRanking() {
    FILE *arquivo = fopen("ranking.bin", "wb");
    if (arquivo != NULL) {
        fwrite(ranking, sizeof(TIPO_SCORE), 5, arquivo);
        fclose(arquivo);
    }
}

void CarregarFase(int numFase) {
    char nomeArquivo[20];
    sprintf(nomeArquivo, "mapa%d.txt", numFase);

    FILE *arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao carregar fase %d\n", numFase);
        exit(1);
    }

    Fase *fase = &fases[numFase - 1];
    fase->numMonstros = 0;
    fase->numVidasExtras = 0;

    for (int i = 0; i < LINHAS_MAPA; i++) {
        for (int j = 0; j < COLUNAS_MAPA; j++) {
            char c = fgetc(arquivo);

            // Ignora quebras de linha e espaços extras
            while (c == '\n' || c == '\r') {
                c = fgetc(arquivo);
            }

            fase->mapa[i][j] = c;

            // Identifica elementos do jogo
            switch (c) { // Acredito que vamos ter que remover os 'break'
                case 'J': // Jogador
                    fase->jogador.x = j;
                    fase->jogador.y = i;
                    fase->jogador.direcao = 0;
                    fase->jogador.vidas = 3;
                    fase->jogador.pontos = 0;
                    fase->jogador.temEspada = false;
                    fase->jogador.espadaAtivada = false;
                    break;
                case 'M': // Monstro
                    if (fase->numMonstros < MAX_MONSTROS) {
                        fase->monstros[fase->numMonstros].x = j;
                        fase->monstros[fase->numMonstros].y = i;
                        fase->monstros[fase->numMonstros].direcao = rand() % 4;
                        fase->monstros[fase->numMonstros].pontos = rand() % 101;
                        fase->monstros[fase->numMonstros].ativo = true;
                        fase->numMonstros++;
                    }
                    break;
                case 'E': // Espada
                    fase->espada.x = j;
                    fase->espada.y = i;
                    fase->espada.coletada = false;
                    break;
                case 'V': // Vida extra
                    if (fase->numVidasExtras < MAX_VIDAS_EXTRAS) {
                        fase->vidasExtras[fase->numVidasExtras].x = j;
                        fase->vidasExtras[fase->numVidasExtras].y = i;
                        fase->vidasExtras[fase->numVidasExtras].coletada = false;
                        fase->numVidasExtras++;
                    }
                    break;
            }
        }
    }

    fclose(arquivo);
}

void InicializarJogo() {
    faseAtual = 0;
    jogoEmAndamento = true;
    jogoPausado = false;

    // Reinicia o jogador para a primeira fase
    fases[0].jogador.vidas = 3;
    fases[0].jogador.pontos = 0;
    fases[0].jogador.temEspada = false;
    fases[0].jogador.espadaAtivada = false;

    // Reinicia os itens coletáveis
    fases[0].espada.coletada = false;
    for (int i = 0; i < fases[0].numVidasExtras; i++) {
        fases[0].vidasExtras[i].coletada = false;
    }

    // Reinicia os monstros
    for (int i = 0; i < fases[0].numMonstros; i++) {
        fases[0].monstros[i].ativo = true;
    }
}

void AtualizarJogo() {
    Fase *fase = &fases[faseAtual];

    // Movimentação do jogador
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        MoverJogador(0); // Norte
    } else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        MoverJogador(1); // Sul
    } else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        MoverJogador(2); // Leste
    } else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        MoverJogador(3); // Oeste
    }

    // Ativação da espada
    if (IsKeyPressed(KEY_J)) {
        AtivarEspada();
    }

    // Movimentação dos monstros
    MoverMonstros();

    // Verifica colisões
    VerificarColisoes();

    // Atualiza tempo da espada ativada
    if (fase->jogador.espadaAtivada) {
        fase->jogador.tempoEspada--;
        if (fase->jogador.tempoEspada <= 0) {
            fase->jogador.espadaAtivada = false;
        }
    }
}

void DesenharJogo() {
    Fase *fase = &fases[faseAtual];

    // Desenha barra de status
    DrawRectangle(0, 0, LARGURA_TELA, 60, DARKGRAY);

    char texto[50];
    sprintf(texto, "Pontos: %d", fase->jogador.pontos);
    DrawText(texto, 20, 20, 20, WHITE);

    sprintf(texto, "Vidas: %d", fase->jogador.vidas);
    DrawText(texto, 200, 20, 20, WHITE);

    sprintf(texto, "Fase: %d/4", faseAtual + 1);
    DrawText(texto, 400, 20, 20, WHITE);

    if (fase->jogador.temEspada) {
        DrawText("Espada: SIM", 600, 20, 20, WHITE);
    } else {
        DrawText("Espada: NAO", 600, 20, 20, WHITE);
    }

    // Desenha o cenário
    for (int i = 0; i < LINHAS_MAPA; i++) {
        for (int j = 0; j < COLUNAS_MAPA; j++) {
            int x = j * TAMANHO_BLOCO;
            int y = i * TAMANHO_BLOCO + 60; // Desloca para baixo da barra de status

            switch (fase->mapa[i][j]) {
                case 'P': // Parede
                    DrawRectangle(x, y, TAMANHO_BLOCO, TAMANHO_BLOCO, BROWN);
                    break;
                case ' ': // Espaço vazio
                    DrawRectangle(x, y, TAMANHO_BLOCO, TAMANHO_BLOCO, GREEN);
                    break;
            }
        }
    }

    // Desenha a espada se não foi coletada
    if (!fase->espada.coletada) {
        int x = fase->espada.x * TAMANHO_BLOCO;
        int y = fase->espada.y * TAMANHO_BLOCO + 60;
        DrawRectangle(x, y, TAMANHO_BLOCO, TAMANHO_BLOCO, GOLD);
        DrawText("E", x + 20, y + 15, 20, BLACK);
    }

    // Desenha vidas extras não coletadas
    for (int i = 0; i < fase->numVidasExtras; i++) {
        if (!fase->vidasExtras[i].coletada) {
            int x = fase->vidasExtras[i].x * TAMANHO_BLOCO;
            int y = fase->vidasExtras[i].y * TAMANHO_BLOCO + 60;
            DrawRectangle(x, y, TAMANHO_BLOCO, TAMANHO_BLOCO, RED);
            DrawText("V", x + 20, y + 15, 20, WHITE);
        }
    }

    // Desenha monstros ativos
    for (int i = 0; i < fase->numMonstros; i++) {
        if (fase->monstros[i].ativo) {
            int x = fase->monstros[i].x * TAMANHO_BLOCO;
            int y = fase->monstros[i].y * TAMANHO_BLOCO + 60;
            DrawRectangle(x, y, TAMANHO_BLOCO, TAMANHO_BLOCO, PURPLE);
            DrawText("M", x + 20, y + 15, 20, WHITE);
        }
    }

    // Desenha jogador
    int x = fase->jogador.x * TAMANHO_BLOCO;
    int y = fase->jogador.y * TAMANHO_BLOCO + 60;
    DrawRectangle(x, y, TAMANHO_BLOCO, TAMANHO_BLOCO, BLUE);
    DrawText("J", x + 20, y + 15, 20, WHITE);

    // Desenha área de ataque da espada se estiver ativada
    if (fase->jogador.espadaAtivada) {
        int dx = 0, dy = 0;
        switch (fase->jogador.direcao) {
            case 0: dy = -1; break; // Norte
            case 1: dy = 1; break;  // Sul
            case 2: dx = 1; break;  // Leste
            case 3: dx = -1; break; // Oeste
        }

        for (int i = 1; i <= 3; i++) {
            int ax = fase->jogador.x + dx * i;
            int ay = fase->jogador.y + dy * i;

            if (ax >= 0 && ax < COLUNAS_MAPA && ay >= 0 && ay < LINHAS_MAPA) {
                DrawRectangle(ax * TAMANHO_BLOCO, ay * TAMANHO_BLOCO + 60,
                              TAMANHO_BLOCO, TAMANHO_BLOCO, Fade(YELLOW, 0.5f));
            }
        }
    }
}

void DesenharMenu() {
    DrawText("ZINF - Zelda INF", LARGURA_TELA/2 - 150, 100, 40, WHITE);

    DrawText("1 - Novo Jogo", LARGURA_TELA/2 - 80, 200, 30, WHITE);
    DrawText("2 - Scoreboard", LARGURA_TELA/2 - 90, 250, 30, WHITE);
    DrawText("3 - Sair", LARGURA_TELA/2 - 50, 300, 30, WHITE);

    // Verifica seleção do menu
    if (IsKeyPressed(KEY_ONE)) {
        InicializarJogo();
    } else if (IsKeyPressed(KEY_TWO)) {
        // Mostra scoreboard (implementação simplificada)
        DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.8f));
        DrawText("TOP 5 JOGADORES", LARGURA_TELA/2 - 120, 100, 30, WHITE);

        for (int i = 0; i < 5; i++) {
            char linha[50];
            sprintf(linha, "%d. %s - %d pontos", i+1, ranking[i].nome, ranking[i].score);
            DrawText(linha, LARGURA_TELA/2 - 150, 150 + i * 40, 20, WHITE);
        }

        DrawText("Pressione qualquer tecla para voltar", LARGURA_TELA/2 - 200, 400, 20, WHITE);
        WaitTime(0.1); // Pequeno delay para evitar leitura múltipla
        while (!WindowShouldClose() && !IsKeyPressed(KEY_ANY)); // Espera tecla
    } else if (IsKeyPressed(KEY_THREE)) {
        CloseWindow();
        exit(0);
    }
}

void DesenharGameOver(bool vitoria) {
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.8f));

    if (vitoria) {
        DrawText("PARABENS! VOCE VENCEU!", LARGURA_TELA/2 - 200, 150, 30, GREEN);
    } else {
        DrawText("GAME OVER", LARGURA_TELA/2 - 100, 150, 40, RED);
    }

    char texto[50];
    sprintf(texto, "Pontuacao final: %d", fases[faseAtual].jogador.pontos);
    DrawText(texto, LARGURA_TELA/2 - 120, 220, 20, WHITE);

    DrawText("V - Voltar ao Menu", LARGURA_TELA/2 - 100, 300, 20, WHITE);
    DrawText("R - Reiniciar Jogo", LARGURA_TELA/2 - 100, 340, 20, WHITE);
    DrawText("F - Sair", LARGURA_TELA/2 - 50, 380, 20, WHITE);

    // Verifica se a pontuação está entre as 5 melhores
    bool novoRecorde = false;
    for (int i = 0; i < 5; i++) {
        if (fases[faseAtual].jogador.pontos > ranking[i].score) {
            novoRecorde = true;
            break;
        }
    }

    if (novoRecorde) {
        DrawText("NOVO RECORDE! Digite seu nome:", LARGURA_TELA/2 - 180, 250, 20, YELLOW);
        // Implementação simplificada - na prática precisaria capturar entrada de texto
    }

    // Processa opções
    if (IsKeyPressed(KEY_V)) {
        jogoEmAndamento = false;
    } else if (IsKeyPressed(KEY_R)) {
        ReiniciarFase();
    } else if (IsKeyPressed(KEY_F)) {
        CloseWindow();
        exit(0);
    }
}

void MoverJogador(int direcao) {
    Fase *fase = &fases[faseAtual];
    fase->jogador.direcao = direcao;

    int novoX = fase->jogador.x;
    int novoY = fase->jogador.y;

    switch (direcao) {
        case 0: novoY--; break; // Norte
        case 1: novoY++; break; // Sul
        case 2: novoX++; break; // Leste
        case 3: novoX--; break; // Oeste
    }

    // Verifica se a nova posição é válida
    if (novoX >= 0 && novoX < COLUNAS_MAPA && novoY >= 0 && novoY < LINHAS_MAPA) {
        if (fase->mapa[novoY][novoX] != 'P') { // Não pode passar por paredes
            fase->jogador.x = novoX;
            fase->jogador.y = novoY;
        }
    }
}

void MoverMonstros() {
    Fase *fase = &fases[faseAtual];

    for (int i = 0; i < fase->numMonstros; i++) {
        if (fase->monstros[i].ativo) {
            // Decide aleatoriamente se vai mudar de direção (1/4 de chance)
            if (rand() % 4 == 0) {
                fase->monstros[i].direcao = rand() % 4;
            }

            int novoX = fase->monstros[i].x;
            int novoY = fase->monstros[i].y;

            switch (fase->monstros[i].direcao) {
                case 0: novoY--; break; // Norte
                case 1: novoY++; break; // Sul
                case 2: novoX++; break; // Leste
                case 3: novoX--; break; // Oeste
            }

            // Verifica se a nova posição é válida
            if (novoX >= 0 && novoX < COLUNAS_MAPA && novoY >= 0 && novoY < LINHAS_MAPA) {
                if (fase->mapa[novoY][novoX] != 'P') { // Não pode passar por paredes
                    fase->monstros[i].x = novoX;
                    fase->monstros[i].y = novoY;
                } else {
                    // Se encontrar parede, muda de direção
                    fase->monstros[i].direcao = rand() % 4;
                }
            } else {
                // Se sair do mapa, muda de direção
                fase->monstros[i].direcao = rand() % 4;
            }
        }
    }
}

void AtivarEspada() {
    Fase *fase = &fases[faseAtual];

    if (fase->jogador.temEspada && !fase->jogador.espadaAtivada) {
        fase->jogador.espadaAtivada = true;
        fase->jogador.tempoEspada = 10; // Dura 10 frames

        int dx = 0, dy = 0;
        switch (fase->jogador.direcao) {
            case 0: dy = -1; break; // Norte
            case 1: dy = 1; break;  // Sul
            case 2: dx = 1; break;  // Leste
            case 3: dx = -1; break; // Oeste
        }

        // Verifica se há monstros nas 3 casas à frente
        for (int i = 1; i <= 3; i++) {
            int ax = fase->jogador.x + dx * i;
            int ay = fase->jogador.y + dy * i;

            if (ax >= 0 && ax < COLUNAS_MAPA && ay >= 0 && ay < LINHAS_MAPA) {
                for (int j = 0; j < fase->numMonstros; j++) {
                    if (fase->monstros[j].ativo && fase->monstros[j].x == ax && fase->monstros[j].y == ay) {
                        fase->monstros[j].ativo = false;
                        fase->jogador.pontos += fase->monstros[j].pontos;
                    }
                }
            }
        }
    }
}

void VerificarColisoes() {
    Fase *fase = &fases[faseAtual];

    // Verifica colisão com espada
    if (!fase->espada.coletada && fase->jogador.x == fase->espada.x && fase->jogador.y == fase->espada.y) {
        fase->espada.coletada = true;
        fase->jogador.temEspada = true;
    }

    // Verifica colisão com vidas extras
    for (int i = 0; i < fase->numVidasExtras; i++) {
        if (!fase->vidasExtras[i].coletada &&
            fase->jogador.x == fase->vidasExtras[i].x &&
            fase->jogador.y == fase->vidasExtras[i].y) {
            fase->vidasExtras[i].coletada = true;
        fase->jogador.vidas++;
            }
    }

    // Verifica colisão com monstros
    for (int i = 0; i < fase->numMonstros; i++) {
        if (fase->monstros[i].ativo &&
            fase->jogador.x == fase->monstros[i].x &&
            fase->jogador.y == fase->monstros[i].y) {
            fase->jogador.vidas--;

        if (fase->jogador.vidas <= 0) {
            DesenharGameOver(false);
            jogoEmAndamento = false;
            return;
        }
            }
    }

    // Verifica se todos os monstros foram derrotados
    bool todosMonstrosDerrotados = true;
    for (int i = 0; i < fase->numMonstros; i++) {
        if (fase->monstros[i].ativo) {
            todosMonstrosDerrotados = false;
            break;
        }
    }

    if (todosMonstrosDerrotados) {
        if (faseAtual < 3) { // Ainda há fases
            ProximaFase();
        } else { // Última fase completada
            DesenharGameOver(true);
            jogoEmAndamento = false;
        }
    }
}

void ProximaFase() {
    faseAtual++;

    // Reinicia o jogador mantendo pontos e vidas
    fases[faseAtual].jogador.vidas = fases[faseAtual - 1].jogador.vidas;
    fases[faseAtual].jogador.pontos = fases[faseAtual - 1].jogador.pontos;
    fases[faseAtual].jogador.temEspada = false;
    fases[faseAtual].jogador.espadaAtivada = false;

    // Reinicia os itens coletáveis
    fases[faseAtual].espada.coletada = false;
    for (int i = 0; i < fases[faseAtual].numVidasExtras; i++) {
        fases[faseAtual].vidasExtras[i].coletada = false;
    }

    // Reinicia os monstros
    for (int i = 0; i < fases[faseAtual].numMonstros; i++) {
        fases[faseAtual].monstros[i].ativo = true;
    }
}

void ReiniciarFase() {
    // Reinicia a fase atual
    fases[faseAtual].jogador.vidas = 3;
    fases[faseAtual].jogador.pontos = 0;
    fases[faseAtual].jogador.temEspada = false;
    fases[faseAtual].jogador.espadaAtivada = false;

    // Reinicia os itens coletáveis
    fases[faseAtual].espada.coletada = false;
    for (int i = 0; i < fases[faseAtual].numVidasExtras; i++) {
        fases[faseAtual].vidasExtras[i].coletada = false;
    }

    // Reinicia os monstros
    for (int i = 0; i < fases[faseAtual].numMonstros; i++) {
        fases[faseAtual].monstros[i].ativo = true;
    }

    // Reposiciona o jogador
    for (int i = 0; i < LINHAS_MAPA; i++) {
        for (int j = 0; j < COLUNAS_MAPA; j++) {
            if (fases[faseAtual].mapa[i][j] == 'J') {
                fases[faseAtual].jogador.x = j;
                fases[faseAtual].jogador.y = i;
                break;
            }
        }
    }
}
