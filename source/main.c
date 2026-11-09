#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOP_W 400.0f
#define TOP_H 240.0f
#define BOTTOM_W 320.0f
#define BOTTOM_H 240.0f

typedef enum {
    SCENE_BOAT,
    SCENE_DECK,
    SCENE_SLEEP,
    SCENE_EVENT
} Scene;

typedef struct {
    int hunger;
    int thirst;
    int fatigue;
    int fear;
    int supplies;
    int night;
    int minutes;
    int collected;
    bool has_key;
    bool event_active;
    char message[96];
} Game;

static Game game;
static Scene scene = SCENE_BOAT;
static float player_x = 200.0f;
static float player_y = 150.0f;
static int event_timer = 0;
static C2D_TextBuf text_buffer;
static C2D_Text text_cache[8];

static u32 color(u8 r, u8 g, u8 b, u8 a) {
    return C2D_Color32(r, g, b, a);
}

static void set_message(const char *message) {
    snprintf(game.message, sizeof(game.message), "%s", message);
}

static void draw_text(float x, float y, float scale, u32 tint, const char *value) {
    C2D_Text text;
    C2D_TextParse(&text, text_buffer, value);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.5f, scale, scale, tint);
}

static void draw_bar(float x, float y, float width, int value, u32 fill) {
    C2D_DrawRectSolid(x, y, 0.5f, width, 9.0f, color(28, 34, 42, 255));
    C2D_DrawRectSolid(x + 1, y + 1, 0.6f, (width - 2) * value / 100.0f, 7.0f, fill);
}

static void init_game(void) {
    memset(&game, 0, sizeof(game));
    game.hunger = 88;
    game.thirst = 92;
    game.fatigue = 18;
    game.fear = 5;
    game.supplies = 0;
    game.night = 1;
    set_message("Jackstone wacht auf. Das Boot treibt im Nebel.");
}

static void update_needs(void) {
    game.minutes++;
    if (game.minutes % 18 == 0 && game.hunger > 0) game.hunger--;
    if (game.minutes % 13 == 0 && game.thirst > 0) game.thirst--;
    if (game.minutes % 22 == 0 && game.fatigue < 100) game.fatigue++;
    if (game.hunger < 25 || game.thirst < 25) game.fear++;
    if (game.fear > 100) game.fear = 100;
}

static void interact(void) {
    if (scene == SCENE_BOAT) {
        game.collected++;
        game.supplies++;
        if (game.collected == 1) set_message("Du findest eine Konservendose. Vorrat +1.");
        else if (game.collected == 2) set_message("Eine Flasche Trinkwasser. Vorrat +1.");
        else if (game.collected == 3) {
            game.has_key = true;
            set_message("Ein rostiger Schluessel liegt unter dem Seil.");
        } else set_message("Nur nasses Holz und altes Tau.");
    } else if (scene == SCENE_DECK) {
        set_message("Unter dem Boot bewegt sich etwas im schwarzen Wasser.");
        game.fear += 7;
    } else if (scene == SCENE_EVENT) {
        game.event_active = false;
        scene = SCENE_DECK;
        set_message("Das Geraeusch entfernt sich. Vorerst.");
    }
}

static void sleep_through_night(void) {
    scene = SCENE_SLEEP;
    game.night++;
    game.fatigue = 8;
    game.hunger = game.hunger > 12 ? game.hunger - 12 : 0;
    game.thirst = game.thirst > 16 ? game.thirst - 16 : 0;
    int roll = rand() % 5;
    if (roll == 0) {
        scene = SCENE_EVENT;
        game.event_active = true;
        event_timer = 0;
        game.fear += 22;
        set_message("Ein dumpfer Schlag reisst dich aus dem Schlaf.");
    } else if (roll == 1) {
        scene = SCENE_EVENT;
        game.event_active = true;
        event_timer = 0;
        game.fear += 12;
        set_message("Etwas kratzt langsam an der Bordwand.");
    } else {
        scene = SCENE_DECK;
        set_message("Die Nacht vergeht ruhig. Der Morgen ist grau.");
    }
    if (game.fear > 100) game.fear = 100;
}

static void draw_boat_scene(void) {
    C2D_DrawRectSolid(0, 0, 0.1f, TOP_W, TOP_H, color(7, 20, 34, 255));
    C2D_DrawRectSolid(0, 0, 0.2f, TOP_W, 74, color(18, 42, 59, 255));
    for (int i = 0; i < 12; i++) {
        float x = (float)((i * 47 + 19) % 400);
        float y = 80.0f + (float)((i * 29) % 125);
        C2D_DrawRectSolid(x, y, 0.3f, 22.0f, 2.0f, color(65, 104, 121, 150));
    }
    C2D_DrawRectSolid(35, 118, 0.4f, 330, 92, color(77, 65, 54, 255));
    C2D_DrawRectSolid(52, 105, 0.5f, 290, 20, color(139, 112, 83, 255));
    C2D_DrawRectSolid(75, 132, 0.6f, 85, 49, color(47, 53, 53, 255));
    C2D_DrawRectSolid(270, 135, 0.6f, 55, 38, color(133, 91, 55, 255));
    C2D_DrawRectSolid(170, 88, 0.6f, 5, 35, color(170, 163, 140, 255));
    C2D_DrawRectSolid(174, 91, 0.6f, 60, 3, color(170, 163, 140, 255));
    C2D_DrawCircleSolid(player_x, player_y, 0.8f, 7, color(210, 183, 145, 255));
    draw_text(12, 10, 0.55f, color(235, 240, 241, 255), "DAS BOOT - TAG 1");
}

static void draw_deck_scene(void) {
    C2D_DrawRectSolid(0, 0, 0.1f, TOP_W, TOP_H, color(3, 11, 21, 255));
    C2D_DrawRectSolid(0, 0, 0.2f, TOP_W, 95, color(10, 25, 39, 255));
    C2D_DrawRectSolid(0, 145, 0.3f, TOP_W, 95, color(5, 23, 37, 255));
    for (int i = 0; i < 9; i++) {
        float x = (float)(i * 51);
        C2D_DrawRectSolid(x, 151 + (float)((i % 3) * 5), 0.4f, 36, 2, color(34, 76, 92, 255));
    }
    C2D_DrawRectSolid(65, 120, 0.5f, 270, 90, color(54, 48, 43, 255));
    C2D_DrawRectSolid(82, 108, 0.6f, 235, 16, color(112, 86, 63, 255));
    C2D_DrawCircleSolid(310, 176, 0.7f, 19, color(14, 23, 31, 255));
    C2D_DrawCircleSolid(302, 171, 2.0f, 3, color(195, 207, 185, 255));
    C2D_DrawCircleSolid(318, 171, 2.0f, 3, color(195, 207, 185, 255));
    C2D_DrawCircleSolid(player_x, player_y, 0.8f, 7, color(210, 183, 145, 255));
    draw_text(12, 10, 0.55f, color(225, 230, 233, 255), "OFFENES MEER");
}

static void draw_bottom_screen(void) {
    C2D_SceneBegin(GFX_BOTTOM);
    C2D_DrawRectSolid(0, 0, 0.1f, BOTTOM_W, BOTTOM_H, color(12, 17, 24, 255));
    draw_text(12, 10, 0.55f, color(226, 225, 213, 255), "JACKSTONE - STATUS");
    draw_text(12, 37, 0.42f, color(185, 197, 203, 255), "HUNGER");
    draw_bar(105, 39, 195, game.hunger, color(188, 139, 75, 255));
    draw_text(12, 58, 0.42f, color(185, 197, 203, 255), "DURST");
    draw_bar(105, 60, 195, game.thirst, color(73, 151, 190, 255));
    draw_text(12, 79, 0.42f, color(185, 197, 203, 255), "MUEDIGKEIT");
    draw_bar(105, 81, 195, game.fatigue, color(135, 119, 177, 255));
    draw_text(12, 100, 0.42f, color(185, 197, 203, 255), "ANGST");
    draw_bar(105, 102, 195, game.fear, color(171, 76, 76, 255));
    draw_text(12, 128, 0.48f, color(231, 218, 185, 255), "INVENTAR");
    char inventory[64];
    snprintf(inventory, sizeof(inventory), "Vorrat: %d   Schluessel: %s", game.supplies, game.has_key ? "Ja" : "Nein");
    draw_text(12, 148, 0.42f, color(203, 210, 210, 255), inventory);
    draw_text(12, 178, 0.39f, color(224, 190, 137, 255), game.message);
    draw_text(12, 211, 0.37f, color(147, 164, 174, 255), "A Interagieren   X Schlafen");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    text_buffer = C2D_TextBufNew(4096);
    srand((unsigned int)osGetTime());
    init_game();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        circlePosition circle;
        hidCircleRead(&circle);

        if (kDown & KEY_START) break;
        if (kDown & KEY_A) interact();
        if ((kDown & KEY_X) && (scene == SCENE_BOAT || scene == SCENE_DECK)) sleep_through_night();
        if (kDown & KEY_B) {
            if (scene == SCENE_BOAT) {
                scene = SCENE_DECK;
                set_message("Du gehst an Deck. Das Meer ist unnatuerlich still.");
            } else if (scene == SCENE_DECK) {
                scene = SCENE_BOAT;
                set_message("Zurueck auf dem Boot. Du solltest Vorrat suchen.");
            }
        }

        float dx = 0.0f, dy = 0.0f;
        if (kDown & KEY_DLEFT) dx -= 5.0f;
        if (kDown & KEY_DRIGHT) dx += 5.0f;
        if (kDown & KEY_DUP) dy -= 5.0f;
        if (kDown & KEY_DDOWN) dy += 5.0f;
        player_x += dx + circle.dx / 900.0f;
        player_y += dy - circle.dy / 900.0f;
        if (player_x < 18) player_x = 18;
        if (player_x > 382) player_x = 382;
        if (player_y < 92) player_y = 92;
        if (player_y > 220) player_y = 220;

        if (scene == SCENE_EVENT && ++event_timer > 420) {
            scene = SCENE_DECK;
            game.event_active = false;
            set_message("Es ist still. Du bist wieder allein.");
        }
        update_needs();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_SceneBegin(GFX_TOP);
        if (scene == SCENE_BOAT || scene == SCENE_SLEEP) draw_boat_scene();
        else draw_deck_scene();
        if (scene == SCENE_EVENT) {
            C2D_DrawRectSolid(34, 34, 5.0f, 332, 55, color(5, 8, 13, 225));
            draw_text(48, 49, 0.62f, color(244, 224, 196, 255), "ETWAS IST DRAUSSEN...");
        }
        draw_bottom_screen();
        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(text_buffer);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
