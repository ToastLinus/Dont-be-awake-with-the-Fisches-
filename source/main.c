#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define W 400
#define H 240
#define ITEMS 6
#define EVENTS 9

typedef enum { PHASE_PREP, PHASE_NIGHT, PHASE_EVENT, PHASE_END } Phase;
typedef struct { float x,y; int kind; bool taken; const char *name; } Loot;
typedef struct {
 int hunger, thirst, fatigue, fear, food, water, wood, beacons;
 int night, seconds, prep_left, event_left, event_id, collected;
 bool has_key, lantern, event_done;
 char message[100];
} Game;

static Game g;
static Phase phase=PHASE_PREP;
static Loot loot[ITEMS]={
 {78,143,0,false,"Konservendose"},
 {142,170,1,false,"Trinkwasser"},
 {220,130,2,false,"Bretter"},
 {305,158,3,false,"Signallicht"},
 {255,83,4,false,"Schluessel"},
 {110,92,5,false,"alte Laterne"}
};
static float px=190,py=145,walk=0,sea=0,flash=0;
static int frame=0, second_tick=0;
static u64 last_second=0;
static C2D_TextBuf textbuf;
static C3D_RenderTarget *top,*bottom;

static u32 col(int r,int gg,int b,int a){return C2D_Color32(r,gg,b,a);}
static void msg(const char *s){snprintf(g.message,sizeof(g.message),"%s",s);}
static void txt(float x,float y,float scale,u32 tint,const char *s){
 C2D_Text t; C2D_TextParse(&t,textbuf,s); C2D_TextOptimize(&t);
 C2D_DrawText(&t,C2D_WithColor,x,y,0.5f,scale,scale,tint);
}
static void rect(float x,float y,float z,float w,float h,u32 c){C2D_DrawRectSolid(x,y,z,w,h,c);}
static void bar(float x,float y,float w,int v,u32 c){
 if(v<0)v=0;if(v>100)v=100; rect(x,y,1,w,8,col(26,32,40,255));
 rect(x+1,y+1,1.1f,(w-2)*v/100.0f,6,c);
}
static void init(void){
 memset(&g,0,sizeof(g));g.hunger=94;g.thirst=94;g.fatigue=12;g.fear=3;
 g.prep_left=60;g.night=1;g.food=0;g.water=0;g.beacons=0;
 msg("60 Sekunden! Sammle Vorräte, bevor die Nacht beginnt.");
 last_second=osGetTime();
}
static void start_night(void){
 phase=PHASE_NIGHT;g.seconds=0;g.night++;
 msg("Die Sonne versinkt. Bleib wachsam und teile deine Vorräte ein.");
}
static void take_item(void){
 for(int i=0;i<ITEMS;i++){
  if(!loot[i].taken && fabsf(px-loot[i].x)<27 && fabsf(py-loot[i].y)<27){
   loot[i].taken=true;g.collected++;
   switch(loot[i].kind){
    case 0:g.food+=2;msg("Konserven gefunden: 2 Rationen.");break;
    case 1:g.water+=2;msg("Trinkwasser gefunden: 2 Flaschen.");break;
    case 2:g.wood+=2;msg("Trockene Bretter: Du kannst etwas reparieren.");break;
    case 3:g.beacons++;msg("Notsender gefunden. Fuer spaeter aufheben!");break;
    case 4:g.has_key=true;msg("Ein kleiner Messingschluessel. Wozu gehoert er?");break;
    case 5:g.lantern=true;msg("Die alte Laterne funktioniert noch.");break;
   }return;
  }
 }
 msg("Hier liegt nichts, das du mitnehmen kannst.");
}
static const char *events[EVENTS]={
 "DREI SCHLAEGE an der Bordwand. Dann Stille.",
 "Ein Licht wandert weit draussen ueber das Wasser.",
 "Etwas zieht unter dem Boot entlang. Das Deck bebt.",
 "Ein Funkgeraet knistert: eine Stimme sagt deinen Namen.",
 "Nebel verschluckt den Horizont. Du hoerst Schritte.",
 "Ein Sturm zieht auf. Lose Dinge rutschen ueber Deck.",
 "Ein Schatten steht am Heck - und ist ploetzlich weg.",
 "Die Laterne flackert. Im Wasser leuchten zwei Punkte.",
 "Ein Signal blinkt am Horizont. Vielleicht ein Schiff."
};
static void trigger_event(void){
 g.event_id=rand()%EVENTS;g.event_left=7;phase=PHASE_EVENT;g.event_done=false;
 g.fear+=8;if(g.fear>100)g.fear=100;msg(events[g.event_id]);
}
static void sleep_or_wait(void){
 if(phase==PHASE_PREP){msg("Noch ist Vorbereitungszeit. Sammle alles ein!");return;}
 if(g.fatigue>55){g.fatigue-=35;if(g.fatigue<0)g.fatigue=0;g.hunger-=5;g.thirst-=7;
  if(g.hunger<0)g.hunger=0;if(g.thirst<0)g.thirst=0;msg("Du ruhst kurz. Ein Geraeusch weckt dich wieder.");
 }else msg("Du bist noch nicht muede genug zum Schlafen.");
}
static void use_supply(void){
 if(g.food>0 && g.hunger<90){g.food--;g.hunger+=28;if(g.hunger>100)g.hunger=100;msg("Du isst eine Ration. Das hilft.");}
 else if(g.water>0 && g.thirst<90){g.water--;g.thirst+=32;if(g.thirst>100)g.thirst=100;msg("Ein Schluck Wasser. Du kannst weiter.");}
 else if(g.beacons>0 && phase!=PHASE_PREP){g.beacons--;g.fear-=18;if(g.fear<0)g.fear=0;msg("Der Notsender blinkt auf dem Deck. Fuer einen Moment ist alles still.");}
 else msg("Nichts Passendes im Inventar. A sammelt Gegenstaende ein.");
}
static void update_second(void){
 if(phase==PHASE_PREP){
  if(g.prep_left>0)g.prep_left--;
  if(g.prep_left==0)start_night();
  return;
 }
 if(phase==PHASE_EVENT){
  if(g.event_left>0)g.event_left--;
  if(g.event_left==0){phase=PHASE_NIGHT;msg("Das Meer ist wieder still. Aber nicht friedlich.");}
 }
 g.seconds++;
 if(g.seconds%12==0 && g.hunger>0)g.hunger--;
 if(g.seconds%9==0 && g.thirst>0)g.thirst--;
 if(g.seconds%16==0 && g.fatigue<100)g.fatigue++;
 if(g.seconds%7==0 && g.fear>0)g.fear--;
 if(g.seconds%25==0 && phase==PHASE_NIGHT)trigger_event();
 if(g.hunger==0 || g.thirst==0 || g.fatigue>=100 || g.fear>=100){
  phase=PHASE_END;msg("Du kannst nicht mehr weitermachen. Die Nacht hat gewonnen.");
 }
}
static void draw_sky(void){
 rect(0,0,0,W,H,col(5,13,27,255));
 rect(0,0,0.1f,W,95,col(11,28,46,255));
 for(int i=0;i<34;i++){
  float x=(float)((i*83+17)%400),y=(float)((i*47+11)%88);
  C2D_DrawCircleSolid(x,y,0.3f,(i%4==0)?1.7f:0.8f,col(170,190,201,180));
 }
 C2D_DrawCircleSolid(324,48,0.4f,22,col(207,213,194,255));
 C2D_DrawCircleSolid(333,42,0.5f,21,col(11,28,46,255));
 for(int i=0;i<7;i++){
  float x=fmodf((float)(i*73)+sea*(i%2?1.2f:0.7f),460.0f)-30;
  rect(x,103+(i%3)*7,0.6f,38,2,col(75,111,126,130));
 }
 rect(0,130,0.7f,W,110,col(7,29,43,255));
 for(int i=0;i<10;i++){
  float x=fmodf((float)(i*49)+sea*(1.0f+i%3),430.0f)-25;
  rect(x,147+(i%4)*18,0.8f,26+(i%3)*9,2,col(39,83,99,190));
 }
}
static void draw_boat(void){
 /* Deck boards, raised railings, cabin, mast and rope details. */
 rect(27,100,1.0f,347,122,col(61,48,40,255));
 rect(34,105,1.1f,332,7,col(128,96,66,255));
 rect(34,211,1.1f,332,8,col(35,31,29,255));
 for(int i=0;i<9;i++)rect(40+i*39,111,1.2f,2,96,col(36,34,32,255));
 rect(49,117,1.3f,94,54,col(44,48,47,255));
 rect(55,123,1.4f,82,39,col(20,35,42,255));
 rect(60,128,1.5f,27,24,col(105,140,143,150));
 rect(94,128,1.5f,35,24,col(105,140,143,150));
 rect(258,125,1.3f,70,42,col(104,76,52,255));
 rect(266,117,1.4f,54,9,col(146,108,70,255));
 rect(180,60,1.3f,4,45,col(137,122,100,255));
 rect(184,65,1.3f,72,3,col(171,153,119,255));
 rect(188,68,1.4f,2,31,col(171,153,119,255));
 for(int i=0;i<6;i++)rect(32+i*66,99,1.6f,3,12,col(156,126,89,255));
 /* animated lantern glow */
 float glow=8.0f+3.0f*sinf((float)frame*0.09f);
 C2D_DrawCircleSolid(82,145,2.0f,glow,col(237,171,85,35));
 C2D_DrawCircleSolid(82,145,2.1f,4,col(255,203,112,255));
}
static void draw_player(void){
 float bob=sinf(walk)*2.2f;
 C2D_DrawCircleSolid(px,py+7+bob,2.0f,8,col(0,0,0,85));
 rect(px-5,py-5+bob,2.1f,10,14,col(49,68,76,255));
 C2D_DrawCircleSolid(px,py-10+bob,2.2f,5.5f,col(202,169,132,255));
 rect(px-6,py-16+bob,2.3f,12,4,col(38,35,32,255));
 rect(px-7,py+7+bob,2.2f,4,7,col(42,42,44,255));
 rect(px+3,py+7+bob,2.2f,4,7,col(42,42,44,255));
}
static void draw_item(Loot *it){
 if(it->taken)return;
 u32 c=col(220,181,111,255);
 if(it->kind==1)c=col(92,175,213,255);
 if(it->kind==2)c=col(147,105,72,255);
 if(it->kind==3)c=col(240,90,68,255);
 if(it->kind==4)c=col(214,194,111,255);
 if(it->kind==5)c=col(238,179,86,255);
 C2D_DrawCircleSolid(it->x,it->y,2.0f,9,c);
 rect(it->x-4,it->y-3,2.1f,8,6,col(29,34,36,255));
 C2D_DrawCircleSolid(it->x,it->y,2.2f,2.2f,c);
}
static void draw_top(void){
 C2D_SceneBegin(top);
 draw_sky();draw_boat();
 for(int i=0;i<ITEMS;i++)draw_item(&loot[i]);
 draw_player();
 txt(12,9,0.55f,col(242,232,211,255),"DONT BE AWAKE WITH THE FISCHES");
 if(phase==PHASE_PREP){
  char b[64];snprintf(b,sizeof(b),"VORBEREITUNG: %02d SEKUNDEN",g.prep_left);
  rect(84,28,4,232,27,col(4,10,17,220));txt(96,35,0.62f,col(255,214,139,255),b);
 }else{
  char b[48];snprintf(b,sizeof(b),"NACHT %d  |  %02d:%02d",g.night,g.seconds/60,g.seconds%60);
  txt(12,27,0.46f,col(206,219,224,255),b);
 }
 if(phase==PHASE_EVENT){
  rect(35,38,5,330,49,col(5,8,13,230));
  txt(48,47,0.58f,col(255,213,171,255),"UNGEWOEHNLICHES EREIGNIS");
  txt(48,68,0.42f,col(235,235,225,255),events[g.event_id]);
 }
 if(phase==PHASE_END){
  rect(38,67,5,324,90,col(5,8,13,235));
  txt(72,82,0.75f,col(235,103,92,255),"DIE NACHT HAT GEWONNEN");
  txt(70,112,0.45f,col(236,230,213,255),"START druecken fuer Neustart");
 }
}
static void draw_bottom(void){
 C2D_SceneBegin(bottom);
 rect(0,0,0,320,240,col(13,19,27,255));
 txt(12,9,0.57f,col(239,228,204,255),"JACKSTONE - UEBERLEBEN");
 txt(12,35,0.42f,col(181,197,205,255),"HUNGER");bar(105,37,195,g.hunger,col(196,139,76,255));
 txt(12,56,0.42f,col(181,197,205,255),"DURST");bar(105,58,195,g.thirst,col(72,157,203,255));
 txt(12,77,0.42f,col(181,197,205,255),"MUEDIGKEIT");bar(105,79,195,g.fatigue,col(145,126,190,255));
 txt(12,98,0.42f,col(181,197,205,255),"ANGST");bar(105,100,195,g.fear,col(190,76,76,255));
 txt(12,124,0.48f,col(239,216,174,255),"VORRAETE");
 char b[80];snprintf(b,sizeof(b),"Essen %d   Wasser %d   Holz %d",g.food,g.water,g.wood);
 txt(12,143,0.43f,col(218,221,213,255),b);
 snprintf(b,sizeof(b),"Notsender %d   Schluessel %s",g.beacons,g.has_key?"JA":"NEIN");
 txt(12,160,0.43f,col(218,221,213,255),b);
 txt(12,184,0.39f,col(229,190,139,255),g.message);
 txt(12,211,0.38f,col(153,172,183,255),"A Sammeln  Y Nutzen  X Ruhen  B Karte");
}
int main(int argc,char **argv){
 (void)argc;(void)argv;
 gfxInitDefault();C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);C2D_Init(C2D_DEFAULT_MAX_OBJECTS);C2D_Prepare();
 top=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);bottom=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 textbuf=C2D_TextBufNew(4096);srand((unsigned int)osGetTime());init();
 while(aptMainLoop()){
  hidScanInput();u32 down=hidKeysDown();u32 held=hidKeysHeld();circlePosition cp;hidCircleRead(&cp);
  if(down&KEY_START){if(phase==PHASE_END){init();phase=PHASE_PREP;px=190;py=145;for(int i=0;i<ITEMS;i++)loot[i].taken=false;}else break;}
  if(down&KEY_A)take_item();
  if(down&KEY_Y)use_supply();
  if(down&KEY_X)sleep_or_wait();
  if(down&KEY_B)msg("Deckplan: Heck links, Kabine mittig, Vorratskisten rechts.");
  if(phase!=PHASE_END){
   float dx=cp.dx/850.0f,dy=-cp.dy/850.0f;
   if(held&KEY_DLEFT)dx-=2.4f;if(held&KEY_DRIGHT)dx+=2.4f;
   if(held&KEY_DUP)dy-=2.4f;if(held&KEY_DDOWN)dy+=2.4f;
   if(fabsf(dx)+fabsf(dy)>0.1f){px+=dx;py+=dy;walk+=0.23f;}
   if(px<40)px=40;if(px>360)px=360;if(py<78)py=78;if(py>205)py=205;
   sea+=0.35f;flash+=0.07f;frame++;
   u64 now=osGetTime();if(now-last_second>=1000){last_second+=1000;update_second();}
  }
  C2D_TextBufClear(textbuf);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  draw_top();draw_bottom();
  C3D_FrameEnd(0);
 }
 C2D_TextBufDelete(textbuf);C2D_Fini();C3D_Fini();gfxExit();return 0;
}
