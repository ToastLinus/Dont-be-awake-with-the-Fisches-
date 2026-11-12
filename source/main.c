#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TOP_W 400
#define BOT_W 320
#define H 240
#define MAX_TEXT 128

typedef enum { SCREEN_TITLE, SCREEN_DIFFICULTY, SCREEN_CHARACTER, SCREEN_GAME, SCREEN_EVENT, SCREEN_END } Screen;
typedef enum { ROOM_DECK, ROOM_KITCHEN, ROOM_ROOF, ROOM_BEDROOM, ROOM_COUNT } Room;
typedef enum { DIFF_EASY, DIFF_NORMAL, DIFF_HARD } Difficulty;

typedef struct {
    int day, hunger, thirst, energy, health, fear;
    int fish, meals, water, seeds, crops, rain;
    int wood, bait, plants;
    int eventId, eventKind;
    int character, name;
    Difficulty difficulty;
    Room room;
    bool hasLantern;
    bool firstRun;
    char message[MAX_TEXT];
} Game;

static Game g;
static Screen screen = SCREEN_TITLE;
static C3D_RenderTarget *topTarget, *bottomTarget;
static C2D_TextBuf textBuffer;
static int frameNo = 0;
static float wave = 0.0f;
static int menuChoice = 0;
static int characterChoice = 0;
static int nameChoice = 0;
static const char *names[] = {"JACK", "ALEX", "MORGAN", "SAM"};
static const char *roomNames[] = {"DECK", "KUECHE", "OBERDECK", "SCHLAFZIMMER"};
static const char *events[] = {
    "Drei Klopfer an der Bordwand.",
    "Ein Licht blinkt weit draussen.",
    "Der Funkempfaenger erwacht.",
    "Etwas Grosses zieht vorbei.",
    "Ein warmer Regen zieht auf.",
    "Eine Kiste treibt ans Boot.",
    "Die Laterne leuchtet ploetzlich heller.",
    "Im Nebel erklingt eine Glocke.",
    "Ein Schwarm Fische folgt dem Boot.",
    "Der Wind reisst an den Segeln.",
    "Ein Schatten huscht ueber das Dach.",
    "Eine freundliche Stimme funkt Hilfe."
};

static u32 C(int r,int g,int b,int a){ return C2D_Color32(r,g,b,a); }
static void rect(float x,float y,float z,float w,float h,u32 c){ C2D_DrawRectSolid(x,y,z,w,h,c); }
static void circle(float x,float y,float z,float r,u32 c){ C2D_DrawCircleSolid(x,y,z,r,c); }
static void text(float x,float y,float s,u32 color,const char *str){
    C2D_Text t;
    C2D_TextParse(&t,textBuffer,str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t,C2D_WithColor,x,y,0.5f,s,s,color);
}
static void panel(float x,float y,float w,float h){
    rect(x+2,y+3,0.8f,w,h,C(0,0,0,90));
    rect(x,y,0.9f,w,h,C(14,27,39,245));
    rect(x+2,y+2,1.0f,w-4,2,C(104,177,184,255));
    rect(x+2,y+h-4,1.0f,w-4,2,C(46,79,91,255));
}
static void button(float x,float y,float w,float h,const char *label,bool selected){
    rect(x,y+2,1.0f,w,h,C(0,0,0,100));
    rect(x,y,1.1f,w,h,selected?C(50,117,124,255):C(34,57,70,255));
    rect(x+2,y+2,1.2f,w-4,2,selected?C(180,224,190,255):C(83,119,128,255));
    text(x+9,y+8,0.52f,C(250,242,220,255),label);
}
static void say(const char *s){ snprintf(g.message,sizeof(g.message),"%s",s); }
static int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }

static void reset_game(void){
    Difficulty selectedDifficulty=g.difficulty;
    memset(&g,0,sizeof(g));
    g.difficulty=selectedDifficulty;
    g.day=1; g.hunger=85; g.thirst=85; g.energy=90; g.health=100; g.fear=5;
    g.meals=2; g.water=2; g.seeds=2; g.wood=2; g.bait=2;
    g.hasLantern=true; g.room=ROOM_DECK; g.firstRun=true;
    if(g.difficulty==DIFF_EASY){g.hunger=100;g.thirst=100;g.meals=4;g.water=4;g.health=100;}
    if(g.difficulty==DIFF_HARD){g.hunger=72;g.thirst=72;g.meals=1;g.water=1;g.health=85;}
    say("Dein erster Tag auf See. Erkunde das Boot!");
}
static void begin_game(void){ reset_game(); screen=SCREEN_GAME; }
static void change_room(int dir){
    int r=(int)g.room+dir;
    if(r<0)r=ROOM_COUNT-1;if(r>=ROOM_COUNT)r=0;
    g.room=(Room)r;
    if(g.room==ROOM_DECK)say("Frische Seeluft. Zeit zum Angeln!");
    if(g.room==ROOM_KITCHEN)say("In der Kueche kannst du Vorrate zubereiten.");
    if(g.room==ROOM_ROOF)say("Hier wachsen deine kleinen Dachgarten-Pflanzen.");
    if(g.room==ROOM_BEDROOM)say("Im Bett kannst du bis zum naechsten Tag schlafen.");
}
static void do_fish(void){
    if(g.bait>0)g.bait--;
    int chance=(g.difficulty==DIFF_HARD)?55:75;
    if(rand()%100<chance){g.fish++;say("Petri Heil! Ein Fisch ist im Korb.");}
    else {g.fear=clampi(g.fear+3,0,100);say("Heute beisst nichts. Etwas bewegt sich im Wasser.");}
    g.energy=clampi(g.energy-7,0,100);
}
static void do_kitchen(void){
    if(g.fish>0){g.fish--;g.hunger=clampi(g.hunger+28,0,100);g.health=clampi(g.health+3,0,100);say("Frischer Fisch! Du fuehlst dich gestaerkt.");}
    else if(g.crops>0){g.crops--;g.hunger=clampi(g.hunger+20,0,100);say("Du bereitest eine Mahlzeit aus deinem Garten zu.");}
    else if(g.meals>0){g.meals--;g.hunger=clampi(g.hunger+24,0,100);say("Eine Konservenmahlzeit. Nicht edel, aber gut.");}
    else say("Die Vorratskiste ist leer. Geh angeln oder pflanze etwas an.");
}
static void do_roof(void){
    if(g.rain>0){g.water=clampi(g.water+g.rain,0,99);g.rain=0;say("Regenwasser in die Tanks gefuellt.");}
    else if(g.seeds>0){g.seeds--;g.plants++;say("Samen eingepflanzt. Mit etwas Wasser wachsen sie.");}
    else if(g.plants>0 && g.water>0){g.plants--;g.water--;g.crops+=2;say("Deine Pflanzen sind gewachsen! Zwei Ernten gesammelt.");}
    else say("Hier ist gerade nichts zu tun. Sammle Regen oder bring Samen mit.");
}
static void do_drink(void){
    if(g.water>0){g.water--;g.thirst=clampi(g.thirst+32,0,100);say("Ein Schluck klares Regenwasser.");}
    else say("Kein Trinkwasser mehr. Auf dem Oberdeck kannst du Regen sammeln.");
}
static void do_sleep(void){
    if(g.energy<100)g.energy=clampi(g.energy+55,0,100);
    g.day++;
    g.hunger=clampi(g.hunger-7,0,100);
    g.thirst=clampi(g.thirst-9,0,100);
    if(g.rain==0 && rand()%100<35)g.rain=1+rand()%2;
    int roll=rand()%100;
    if(roll<22){
        g.eventKind=1;g.eventId=rand()%12;screen=SCREEN_EVENT;
        g.fear=clampi(g.fear+8,0,100);
    }else if(roll<47){
        g.eventKind=2;g.eventId=rand()%12;screen=SCREEN_EVENT;
    }else{
        g.eventKind=0;g.eventId=-1;screen=SCREEN_EVENT;
    }
}
static void resolve_event(void){
    if(g.eventKind==1){
        int damage=(g.difficulty==DIFF_HARD)?15:9;
        g.health=clampi(g.health-damage,0,100);
        g.fear=clampi(g.fear+8,0,100);
        say("Eine unruhige Nacht. Du verlierst etwas Kraft.");
    }else if(g.eventKind==2){
        int gift=rand()%3;
        if(gift==0){g.fish+=2;say("Gute Nachricht: Fische springen an Bord! +2 Fische.");}
        else if(gift==1){g.water+=2;say("Gute Nachricht: Der Regen hat deine Tanks gefuellt! +2 Wasser.");}
        else {g.meals++;say("Gute Nachricht: Eine Treibkiste enthaelt eine Mahlzeit!");}
        g.fear=clampi(g.fear-5,0,100);
    }else say("Eine ruhige Nacht. Das Meer war heute freundlich.");
    if(g.hunger<=0 || g.thirst<=0 || g.health<=0){screen=SCREEN_END;}
    else {screen=SCREEN_GAME;g.room=ROOM_BEDROOM;}
}
static void interact(void){
    if(screen!=SCREEN_GAME)return;
    switch(g.room){
        case ROOM_DECK: do_fish();break;
        case ROOM_KITCHEN: do_kitchen();break;
        case ROOM_ROOF: do_roof();break;
        case ROOM_BEDROOM: do_sleep();break;
        default:break;
    }
    if(g.hunger<=0 || g.thirst<=0 || g.health<=0)screen=SCREEN_END;
}
static void update_stats(void){
    if(g.hunger>0)g.hunger=clampi(g.hunger-1,0,100);
    if(g.thirst>0)g.thirst=clampi(g.thirst-1,0,100);
    if(g.hunger<20 || g.thirst<20)g.health=clampi(g.health-1,0,100);
    if(g.health<=0 || g.hunger<=0 || g.thirst<=0)screen=SCREEN_END;
}
static void draw_sea(void){
    rect(0,0,0,TOP_W,240,C(9,24,43,255));
    rect(0,0,0.1f,TOP_W,82,C(20,47,69,255));
    circle(325,42,0.2f,20,C(222,218,181,255));
    circle(333,35,0.3f,18,C(20,47,69,255));
    for(int i=0;i<35;i++){
        float x=(float)((i*79+frameNo*0.11f*(i%3+1))%410);
        float y=(float)((i*37+13)%78);
        circle(x,y,0.4f,(i%5==0)?1.3f:0.7f,C(204,218,213,180));
    }
    rect(0,94,0.5f,TOP_W,146,C(8,45,62,255));
    for(int i=0;i<18;i++){
        float x=fmodf((float)i*43.0f+wave*(0.8f+(i%4)*0.25f),450.0f)-25.0f;
        float y=102+(i%6)*22;
        rect(x,y,0.7f,22+(i%3)*10,2,C(55,113,129,170));
        rect(x+7,y+4,0.7f,12,1,C(107,156,158,110));
    }
}
static void draw_boat(void){
    /* layered hull, deck planks, railings, cabin and warm portholes */
    rect(26,145,1.0f,348,62,C(28,34,42,255));
    rect(34,135,1.1f,332,22,C(126,79,49,255));
    rect(40,139,1.2f,320,5,C(184,128,76,255));
    for(int i=0;i<10;i++){
        rect(43+i*34,148,1.3f,2,52,C(78,50,38,255));
        rect(43+i*34,158,1.3f,27,1,C(160,109,69,150));
    }
    rect(70,93,1.4f,115,47,C(54,67,68,255));
    rect(78,101,1.5f,99,31,C(18,43,54,255));
    rect(85,106,1.6f,31,20,C(104,163,164,255));
    rect(127,106,1.6f,39,20,C(104,163,164,255));
    rect(275,108,1.4f,66,31,C(111,72,46,255));
    rect(267,101,1.5f,82,9,C(181,124,72,255));
    rect(193,44,1.4f,4,91,C(157,139,106,255));
    rect(197,53,1.4f,65,3,C(192,170,126,255));
    rect(198,56,1.5f,2,29,C(192,170,126,255));
    for(int i=0;i<6;i++){rect(38+i*65,128,1.6f,3,16,C(210,173,113,255));}
    circle(112,118,1.8f,5,C(255,203,111,255));
    circle(148,118,1.8f,5,C(255,203,111,255));
    circle(112,118,1.9f,2,C(255,239,177,255));
    circle(148,118,1.9f,2,C(255,239,177,255));
}
static void draw_captain(float x,float y){
    u32 coat= g.character==1 ? C(76,116,139,255) : (g.character==2 ? C(91,111,76,255) : C(61,78,91,255));
    circle(x,y+15,2.0f,7,C(0,0,0,85));
    rect(x-6,y-7,2.1f,12,18,coat);
    circle(x,y-13,2.2f,6,C(218,177,139,255));
    rect(x-7,y-20,2.3f,14,5,C(45,43,39,255));
    rect(x-5,y+10,2.2f,4,9,C(41,43,49,255));
    rect(x+2,y+10,2.2f,4,9,C(41,43,49,255));
}
static void draw_room_scene(void){
    draw_sea();
    if(g.room==ROOM_DECK){
        draw_boat();
        text(18,12,0.65f,C(255,239,205,255),"AUSSENDECK");
        /* fishing rod, bucket and rope details */
        rect(285,91,1.8f,3,47,C(93,65,43,255));
        rect(287,91,1.8f,36,2,C(197,171,126,255));
        circle(310,177,1.8f,12,C(48,91,99,255));
        circle(310,177,1.9f,8,C(18,49,61,255));
        draw_captain(205,157);
        text(20,211,0.48f,C(244,223,186,255),"Angeln und Vorrate sammeln");
    }else if(g.room==ROOM_KITCHEN){
        rect(0,0,0.8f,TOP_W,240,C(32,47,55,255));
        rect(0,178,0.9f,TOP_W,62,C(89,63,47,255));
        rect(30,76,1.0f,340,105,C(112,76,53,255));
        rect(39,84,1.1f,322,5,C(191,143,91,255));
        rect(53,106,1.2f,105,67,C(58,69,68,255));
        rect(63,116,1.3f,85,44,C(28,43,48,255));
        rect(211,105,1.2f,137,69,C(68,76,70,255));
        rect(224,116,1.3f,111,12,C(183,138,86,255));
        for(int i=0;i<3;i++)circle(244+i*29,145,1.5f,8,C(201,91+i*25,63,255));
        rect(79,50,1.4f,68,24,C(22,37,43,255));
        circle(95,62,1.5f,6,C(255,191,98,255));
        circle(111,62,1.5f,6,C(255,191,98,255));
        text(18,12,0.65f,C(255,239,205,255),"KUECHE");
        draw_captain(185,160);
        text(24,211,0.48f,C(244,223,186,255),"Fisch, Pflanzen oder Konserven essen");
    }else if(g.room==ROOM_ROOF){
        rect(0,0,0.8f,TOP_W,240,C(16,51,65,255));
        rect(0,0,0.9f,TOP_W,85,C(36,75,91,255));
        for(int i=0;i<24;i++){
            float x=(i*53+frameNo*0.5f)%410;
            rect(x,(i*31)%82,1.0f,2,11,C(133,194,205,130));
        }
        rect(22,133,1.1f,355,68,C(100,66,46,255));
        rect(37,121,1.2f,326,18,C(177,128,76,255));
        for(int i=0;i<5;i++){
            rect(48+i*65,147,1.3f,52,35,C(67,48,39,255));
            rect(52+i*65,151,1.4f,44,27,C(50,91,55,255));
            for(int j=0;j<3;j++){
                rect(59+i*65+j*12,155,1.5f,5,15,C(88,157,78,255));
                circle(61+i*65+j*12,155,1.6f,4,C(117,185,91,255));
            }
        }
        text(18,12,0.65f,C(255,239,205,255),"OBERDECK-GARTEN");
        draw_captain(206,165);
        text(24,211,0.48f,C(244,223,186,255),"Regen sammeln und Pflanzen anbauen");
    }else{
        rect(0,0,0.8f,TOP_W,240,C(27,35,48,255));
        rect(0,178,0.9f,TOP_W,62,C(71,50,47,255));
        rect(44,93,1.0f,312,91,C(104,72,58,255));
        rect(54,102,1.1f,292,73,C(44,53,62,255));
        rect(82,131,1.2f,230,57,C(126,78,69,255));
        rect(82,122,1.3f,230,15,C(203,158,123,255));
        rect(93,137,1.4f,207,40,C(222,188,151,255));
        rect(73,69,1.3f,71,55,C(23,47,59,255));
        rect(79,75,1.4f,59,43,C(89,142,148,255));
        circle(329,62,1.5f,12,C(255,193,108,70));
        circle(329,62,1.6f,5,C(255,204,120,255));
        text(18,12,0.65f,C(255,239,205,255),"SCHLAFZIMMER");
        draw_captain(183,166);
        text(24,211,0.48f,C(244,223,186,255),"Ausruhen und den Tag ueberspringen");
    }
}
static void draw_top(void){
    C2D_TargetClear(topTarget,C(5,13,24,255));
    C2D_SceneBegin(topTarget);
    if(screen==SCREEN_TITLE || screen==SCREEN_DIFFICULTY || screen==SCREEN_CHARACTER){
        draw_sea();draw_boat();
        rect(0,0,4,TOP_W,240,C(0,0,0,65));
        panel(37,30,326,178);
        text(58,46,0.78f,C(255,232,187,255),"DONT BE AWAKE");
        text(58,72,0.78f,C(255,232,187,255),"WITH THE FISCHES");
        text(60,112,0.5f,C(183,215,207,255),"EIN ABENTEUER AUF OFFENER SEE");
        if(screen==SCREEN_TITLE)text(84,166,0.55f,C(255,236,194,255),"A DRUECKEN ODER TOUCH START");
        if(screen==SCREEN_DIFFICULTY)text(82,164,0.55f,C(255,236,194,255),"WAEHLE DEINE HERAUSFORDERUNG");
        if(screen==SCREEN_CHARACTER){
            text(77,133,0.55f,C(255,236,194,255),"KAPITAEN: ");
            text(170,133,0.55f,C(255,236,194,255),names[nameChoice]);
            text(77,158,0.55f,C(255,236,194,255),"LOOK: ");
            text(170,158,0.55f,C(255,236,194,255),characterChoice==0?"KLASSISCH":(characterChoice==1?"REGENJACKE":"SEEMANN"));
        }
    }else if(screen==SCREEN_GAME){
        draw_room_scene();
        panel(10,34,380,34);
        char b[80];snprintf(b,sizeof(b),"TAG %d   HUNGER %d   DURST %d   KRAFT %d",g.day,g.hunger,g.thirst,g.energy);
        text(20,45,0.48f,C(255,245,224,255),b);
        if(g.room==ROOM_DECK){
            circle(178,183,2.0f,6,C(196,158,118,255));
            circle(185,179,2.0f,5,C(199,174,139,255));
        }
    }else if(screen==SCREEN_EVENT){
        draw_room_scene();
        panel(25,45,350,148);
        if(g.eventKind==0)text(54,65,0.72f,C(255,234,185,255),"RUHIGE NACHT");
        else if(g.eventKind==1)text(54,65,0.72f,C(255,165,145,255),"UNRUHIGE NACHT");
        else text(54,65,0.72f,C(168,230,184,255),"GUTE NACHRICHT");
        text(52,108,0.52f,C(240,239,221,255),g.eventKind==0?"Das Meer bleibt still.":events[g.eventId]);
        text(52,144,0.47f,C(203,216,214,255),"A DRUECKEN ZUM WEITERMACHEN");
    }else{
        draw_sea();draw_boat();
        panel(36,55,328,130);
        text(82,75,0.78f,C(255,174,155,255),"REISE BEENDET");
        text(76,119,0.55f,C(246,235,215,255),"Du hast bis Tag ueberlebt:");
        char b[32];snprintf(b,sizeof(b),"TAG %d",g.day);text(162,145,0.62f,C(255,232,187,255),b);
    }
    C2D_SceneEnd();
}
static void statbar(float x,float y,const char *label,int v,u32 fill){
    text(x,y,0.43f,C(220,229,220,255),label);
    rect(x+83,y+2,1.0f,112,8,C(37,48,56,255));
    rect(x+85,y+4,1.1f,108.0f*v/100.0f,4,fill);
}
static void small_room_button(float x,const char *label,bool selected){
    rect(x,202,1.0f,72,25,C(0,0,0,100));
    rect(x,200,1.1f,72,25,selected?C(53,119,124,255):C(34,57,70,255));
    text(x+5,207,0.39f,C(250,242,220,255),label);
}
static void draw_bottom(void){
    C2D_TargetClear(bottomTarget,C(9,18,28,255));
    C2D_SceneBegin(bottomTarget);
    if(screen==SCREEN_TITLE){
        panel(14,20,292,198);
        text(34,39,0.66f,C(255,232,187,255),"WILLKOMMEN AN BORD");
        text(34,79,0.5f,C(207,220,214,255),"Ein Boot. Ein endloses Meer.");
        text(34,101,0.5f,C(207,220,214,255),"Und Naechte voller Geheimnisse.");
        button(48,151,224,34,"NEUES SPIEL",true);
        text(60,198,0.42f,C(160,184,188,255),"TOUCH ODER A ZUM STARTEN");
    }else if(screen==SCREEN_DIFFICULTY){
        panel(14,18,292,204);text(34,31,0.62f,C(255,232,187,255),"SCHWIERIGKEIT");
        button(40,75,240,30,"EINFACH",menuChoice==0);
        button(40,116,240,30,"NORMAL",menuChoice==1);
        button(40,157,240,30,"SCHWER",menuChoice==2);
        text(36,197,0.4f,C(165,190,194,255),"LINKS/RECHTS + A ODER TOUCH");
    }else if(screen==SCREEN_CHARACTER){
        panel(12,14,296,210);text(30,27,0.62f,C(255,232,187,255),"DEIN KAPITAEN");
        text(28,65,0.48f,C(210,223,215,255),"NAME");
        button(28,83,264,29,names[nameChoice],false);
        text(28,124,0.48f,C(210,223,215,255),"KLEIDUNG");
        button(28,142,264,29,characterChoice==0?"KLASSISCH":(characterChoice==1?"REGENJACKE":"SEEMANN"),false);
        button(65,184,190,29,"AB AUFS MEER!",true);
    }else if(screen==SCREEN_GAME){
        panel(7,7,306,226);
        text(18,14,0.55f,C(255,232,187,255),"AN BORD  /  TAG");
        char b[50];snprintf(b,sizeof(b),"%d",g.day);text(272,14,0.55f,C(255,232,187,255),b);
        statbar(18,43,"HUNGER",g.hunger,C(222,157,82,255));
        statbar(18,63,"DURST",g.thirst,C(88,178,214,255));
        statbar(18,83,"KRAFT",g.energy,C(155,151,218,255));
        statbar(18,103,"GESUNDHEIT",g.health,C(101,190,132,255));
        text(18,128,0.48f,C(255,224,176,255),"VORRAETE");
        snprintf(b,sizeof(b),"Fisch %d  Essen %d  Wasser %d",g.fish,g.meals,g.water);text(18,147,0.43f,C(218,226,219,255),b);
        snprintf(b,sizeof(b),"Samen %d  Ernte %d  Regen %d",g.seeds,g.crops,g.rain);text(18,164,0.43f,C(218,226,219,255),b);
        text(18,183,0.42f,C(184,207,207,255),g.message);
        small_room_button(8,"DECK",g.room==ROOM_DECK);
        small_room_button(86,"KUECHE",g.room==ROOM_KITCHEN);
        small_room_button(164,"GARTEN",g.room==ROOM_ROOF);
        small_room_button(242,"BETT",g.room==ROOM_BEDROOM);
    }else if(screen==SCREEN_EVENT){
        panel(10,15,300,210);
        text(28,30,0.62f,C(255,232,187,255),"DIE NACHT");
        text(28,73,0.48f,C(210,223,215,255),g.eventKind==0?"Alles ist ruhig.":(g.eventKind==1?"Etwas ist passiert!":"Das Glueck ist auf deiner Seite."));
        text(28,112,0.45f,C(190,212,210,255),"Druecke A, um den Morgen zu sehen.");
        button(65,173,190,32,"WEITER",true);
    }else{
        panel(12,20,296,200);
        text(48,42,0.72f,C(255,185,165,255),"ENDE DER REISE");
        text(39,91,0.5f,C(215,226,218,255),"Du hast Tag fuer Tag gekaempft.");
        button(52,158,216,34,"NOCH EINMAL",true);
    }
    C2D_SceneEnd();
}
static bool touch_in(touchPosition t,int x,int y,int w,int h){return t.px>=x&&t.px<x+w&&t.py>=y&&t.py<y+h;}
int main(int argc,char **argv){
    (void)argc;(void)argv;
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    topTarget=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);
    bottomTarget=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
    textBuffer=C2D_TextBufNew(4096);
    srand((unsigned int)osGetTime());
    g.difficulty=DIFF_NORMAL;
    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        touchPosition touch;hidTouchRead(&touch);
        bool tapped=(down&KEY_TOUCH)!=0;
        if(screen==SCREEN_TITLE){
            if((down&KEY_A)||tapped){screen=SCREEN_DIFFICULTY;menuChoice=1;}
        }else if(screen==SCREEN_DIFFICULTY){
            if(down&KEY_UP)menuChoice=(menuChoice+2)%3;
            if(down&KEY_DOWN)menuChoice=(menuChoice+1)%3;
            if(tapped){
                if(touch_in(touch,40,75,240,30))menuChoice=0;
                if(touch_in(touch,40,116,240,30))menuChoice=1;
                if(touch_in(touch,40,157,240,30))menuChoice=2;
                if(touch_in(touch,40,75,240,112)){g.difficulty=(Difficulty)menuChoice;screen=SCREEN_CHARACTER;}
            }
            if(down&KEY_A){g.difficulty=(Difficulty)menuChoice;screen=SCREEN_CHARACTER;}
            if(down&KEY_B)screen=SCREEN_TITLE;
        }else if(screen==SCREEN_CHARACTER){
            if(down&KEY_LEFT)nameChoice=(nameChoice+3)%4;
            if(down&KEY_RIGHT)nameChoice=(nameChoice+1)%4;
            if(down&KEY_UP)characterChoice=(characterChoice+2)%3;
            if(down&KEY_DOWN)characterChoice=(characterChoice+1)%3;
            if(tapped){
                if(touch_in(touch,28,83,264,29))nameChoice=(nameChoice+1)%4;
                else if(touch_in(touch,28,142,264,29))characterChoice=(characterChoice+1)%3;
                else if(touch_in(touch,65,184,190,29))begin_game();
            }
            if(down&KEY_A)begin_game();
            if(down&KEY_B)screen=SCREEN_DIFFICULTY;
        }else if(screen==SCREEN_GAME){
            if(down&KEY_LEFT)change_room(-1);
            if(down&KEY_RIGHT)change_room(1);
            if(down&KEY_A)interact();
            if(down&KEY_B)do_drink();
            if(tapped){
                if(touch_in(touch,8,200,72,27))g.room=ROOM_DECK;
                else if(touch_in(touch,86,200,72,27))g.room=ROOM_KITCHEN;
                else if(touch_in(touch,164,200,72,27))g.room=ROOM_ROOF;
                else if(touch_in(touch,242,200,72,27))g.room=ROOM_BEDROOM;
            }
            if(down&KEY_X){if(g.room==ROOM_BEDROOM)do_sleep();else say("Zum Schlafen ins Schlafzimmer gehen.");}
            if((frameNo%180)==0)update_stats();
        }else if(screen==SCREEN_EVENT){
            if((down&KEY_A)||tapped)resolve_event();
        }else if(screen==SCREEN_END){
            if((down&KEY_A)||tapped){screen=SCREEN_TITLE;g.difficulty=DIFF_NORMAL;}
        }
        wave+=0.35f;frameNo++;
        C2D_TextBufClear(textBuffer);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        draw_top();draw_bottom();
        C3D_FrameEnd(0);
    }
    C2D_TextBufDelete(textBuffer);
    C2D_Fini();C3D_Fini();gfxExit();
    return 0;
}
