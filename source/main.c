#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define W 400
#define H 240
#define BW 320
#define BH 240
#define clamp(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
typedef enum { INTRO, SHIP, BOAT, NIGHT, ENDING } Phase;
static Phase phase=INTRO;
static C3D_RenderTarget *top,*bottom;
static int frame=0, area=0, facing=0, food=0, water=0, rope=0, fish=0, day=1, hunger=100, thirst=100, stamina=100, caught=0;
static int hour=8, room=0, bait=3, plants=0, plantDays=0, lastFishHour=-1, nightEvent=0, weather=0;
static bool gotFood=false,gotWater=false,gotRope=false,gotCompass=false, transferred=false, sleptToday=false, rainCollected=false;
static char message[100]="";
static int messageFrames=0, fishAnim=0;
static float sway=0;
static u32 C(int r,int g,int b,int a){return C2D_Color32(r,g,b,a);}
static void R(float x,float y,float w,float h,u32 c){C2D_DrawRectSolid(x,y,0.5f,w,h,c);}
static void O(float x,float y,float r,u32 c){C2D_DrawCircleSolid(x,y,0.5f,r,c);}
static void rect(float x,float y,float z,float w,float h,u32 c){(void)z;R(x,y,w,h,c);}
static const unsigned char font5x7[36][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
 {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
};
static const unsigned char *glyph(char ch){
    static const unsigned char blank[7]={0,0,0,0,0,0,0};
    static const unsigned char dash[7]={0,0,0,31,0,0,0};
    static const unsigned char dot[7]={0,0,0,0,0,12,12};
    static const unsigned char colon[7]={0,12,12,0,12,12,0};
    static const unsigned char bang[7]={4,4,4,4,4,0,4};
    static const unsigned char question[7]={14,17,1,2,4,0,4};
    static const unsigned char plus[7]={0,4,4,31,4,4,0};
    static const unsigned char slash[7]={1,2,2,4,8,8,16};
    if(ch>='a'&&ch<='z')ch=(char)(ch-'a'+'A');
    if(ch>='A'&&ch<='Z')return font5x7[ch-'A'];
    if(ch>='0'&&ch<='9')return font5x7[26+ch-'0'];
    switch(ch){case '-':case '_':return dash;case '.':return dot;case ':':return colon;case '!':return bang;case '?':return question;case '+':return plus;case '/':return slash;case ' ':return blank;default:return blank;}
}
static void text(float x,float y,float s,u32 color,const char *str){
    float cell=s*2.4f;
    if(cell<1.0f)cell=1.0f;
    for(;*str;str++,x+=cell*6.0f){
        const unsigned char *rows=glyph(*str);
        for(int row=0;row<7;row++){
            int col=0;
            while(col<5){
                if(!(rows[row]&(1u<<(4-col)))){col++;continue;}
                int start=col;
                while(col<5&&(rows[row]&(1u<<(4-col))))col++;
                rect(x+start*cell,y+row*cell,0.55f,(col-start)*cell,cell*0.88f,color);
            }
        }
    }
}

static void say(const char *s){snprintf(message,sizeof(message),"%s",s);messageFrames=150;}
static void panel(float x,float y,float w,float h){R(x+2,y+3,w,h,C(0,0,0,120));R(x,y,w,h,C(9,23,34,242));R(x+2,y+2,w-4,2,C(106,171,174,255));}
static void button(float x,float y,float w,const char *s,bool active){R(x,y,w,27,active?C(45,110,119,255):C(27,52,64,255));text(x+8,y+8,0.48f,C(250,240,217,255),s);}
static void sky(bool night){R(0,0,W,240,night?C(7,17,35,255):C(49,112,142,255));R(0,0,W,95,night?C(11,26,50,255):C(112,171,188,255));O(326,night?44:36,night?13:19,night?C(211,220,213,255):C(255,218,151,255));R(0,102,W,138,night?C(5,37,53,255):C(20,91,111,255));for(int i=0;i<18;i++){float x=fmodf(i*53+frame*(0.35f+i%3*.12f),430)-15;R(x,108+(i%6)*21,25+i%3*12,2,C(97,161,169,180));}}
static void horizon_ship(void){
 sky(false);
 /* distant silhouette of the enormous ship */
 R(18,104,364,15,C(39,51,57,255));R(34,119,330,38,C(75,65,55,255));R(49,116,300,6,C(158,117,75,255));
 R(65,78,88,39,C(75,81,76,255));R(73,86,72,20,C(34,57,66,255));R(210,83,68,34,C(78,78,68,255));
 R(244,34,4,83,C(103,84,65,255));R(248,41,65,3,C(179,147,104,255));R(248,44,2,32,C(179,147,104,255));
 /* first-person deck perspective */
 R(0,145,400,95,C(91,60,42,255));
 for(int i=0;i<8;i++){int y=148+i*13;R(0,y,400,2,C(143,101,67,255));}
 for(int i=0;i<9;i++){int x=200+(i-4)*((i-4)*(i-4)+18);R(x,145,2,95,C(65,47,37,255));}
 R(0,128,400,6,C(118,82,54,255));R(0,131,400,2,C(197,149,92,255));
 /* railings recede to vanishing point */
 R(0,111,4,39,C(104,77,55,255));R(396,111,4,39,C(104,77,55,255));
 R(0,119,400,3,C(175,137,93,255));
}
static void draw_ship_view(void){
 horizon_ship();
 if(area==0){
   /* open deck, supplies near the bow */
   R(285,123,42,26,C(106,71,43,255));R(291,117,30,9,C(150,106,66,255));
   O(305,111,7,C(174,191,167,255));
   text(250,84,0.42f,C(255,239,194,255),"VORRATSKISTE");
   R(38,128,34,17,C(116,84,52,255));R(42,124,26,7,C(168,129,80,255));
   text(27,101,0.42f,C(255,239,194,255),"BUG");
   R(294,109,16,23,C(171,198,192,255));R(297,105,10,5,C(190,161,116,255));
 }else if(area==1){
   R(105,76,190,98,C(83,61,47,255));R(115,83,170,9,C(156,111,69,255));
   R(129,104,54,60,C(57,72,70,255));R(136,111,40,46,C(34,48,48,255));
   R(220,108,53,56,C(130,92,53,255));R(226,101,41,9,C(182,138,79,255));
   R(246,119,12,26,C(149,191,192,255));R(249,114,6,6,C(195,167,120,255));
   text(141,53,0.48f,C(255,239,194,255),"LAGER / KUECHE");
 }else if(area==2){
   R(126,65,145,78,C(73,82,82,255));R(138,78,121,55,C(22,48,60,255));
   R(151,91,30,23,C(113,165,167,255));R(193,91,30,23,C(113,165,167,255));
   R(164,27,4,70,C(103,84,65,255));R(168,34,57,3,C(179,147,104,255));
   text(155,48,0.45f,C(255,239,194,255),"BRUECKE");
 }
 /* vignette and crosshair make the viewpoint unmistakably first-person */
 R(0,0,400,8,C(0,0,0,55));R(0,232,400,8,C(0,0,0,55));
 R(196,116,8,2,C(241,231,207,210));R(199,113,2,8,C(241,231,207,210));
}
static void draw_boat_view(void){
 sky(hour<6||hour>=19);
 /* compact dinghy: visible bow, gunwales and one mast from the player's eyes */
 R(0,150,400,90,C(10,55,70,255));
 for(int i=0;i<14;i++){float x=fmodf(i*41+frame*(.5f+i%3*.12f),420)-10;R(x,154+(i%5)*15,27,2,C(86,149,157,180));}
 R(0,188,400,52,C(93,60,39,255));R(0,181,400,8,C(147,99,60,255));
 R(0,172,74,68,C(117,77,47,255));R(326,172,74,68,C(117,77,47,255));
 R(12,176,376,4,C(193,147,94,255));
 R(195,33,5,151,C(112,87,62,255));R(199,41,2,112,C(195,165,118,255));
 /* sail */
 R(204,43,96,82,C(218,211,182,255));R(207,48,86,70,C(185,192,176,255));
 for(int i=0;i<4;i++)R(212+i*20,52,1,59,C(143,151,140,255));
 /* clearly visible first-person fishing rod, line and bobber */
 R(301,153,4,48,C(105,69,43,255));R(292,153,12,3,C(185,148,102,255));
 R(278,124,2,37,C(211,198,167,220));
 float bobberY=158.0f+sinf(frame*.13f)*3.0f;
 O(278,bobberY,5,C(244,117,83,255));R(276,bobberY-1,4,2,C(246,232,208,255));
 for(int k=0;k<3;k++)R(245+k*9,183+(k%2)*3,6,2,C(104,173,184,190));
 if(fishAnim>0){
   float jump=(90-fishAnim)/90.0f;
   float fx=245.0f+jump*38.0f;
   float fy=174.0f-fabsf(sinf(jump*3.14159f))*43.0f;
   O(fx,fy,7,C(180,205,181,255));R(fx-9,fy-2,18,4,C(180,205,181,255));
   O(fx+4,fy-3,1.5f,C(20,38,43,255));
   for(int k=0;k<3;k++)O(238+k*8,183+k%2*4,2,C(159,211,215,210));
 }
 /* small supply crate */
 R(160,176,75,35,C(113,74,43,255));R(166,170,63,8,C(166,120,69,255));
 R(194,180,4,25,C(68,47,35,255));
 if(caught && fishAnim==0){O(187,166,8,C(177,202,180,255));R(180,164,15,4,C(177,202,180,255));}
 R(196,116,8,2,C(241,231,207,210));R(199,113,2,8,C(241,231,207,210));
}
static void draw_top(void){
 C2D_TargetClear(top,C(5,14,25,255));C2D_SceneBegin(top);
 if(phase==INTRO){
   sky(false);horizon_ship();panel(25,31,350,177);
   text(48,48,.82f,C(255,230,185,255),"DONT BE AWAKE");
   text(48,77,.82f,C(255,230,185,255),"WITH THE FISCHES");
   text(59,130,.5f,C(220,230,215,255),"DEIN SCHIFF VERLAESST DEN HAFEN");
   text(79,171,.5f,C(255,230,185,255),"A DRUECKEN ZUM START");
 }else if(phase==SHIP){
   draw_ship_view();panel(9,8,382,27);
   text(20,17,.48f,C(255,239,213,255),"DAS GROSSE SCHIFF  |  SAMMLE DEINE AUSRUESTUNG");
   if(messageFrames>0){panel(35,72,330,31);text(47,83,.48f,C(255,240,212,255),message);}
   if(area==0){R(286,121,42,26,C(119,79,46,255));R(291,116,31,8,C(175,128,75,255));}
   if(area==1){R(135,112,40,48,C(45,77,85,255));}
   if(area==2){R(151,91,30,23,C(112,169,170,255));}
   panel(92,207,216,24);text(106,214,.43f,C(255,236,205,255),"PFEILE: UMSCHAUEN / GEHEN   A: NEHMEN");
 }else if(phase==BOAT){
   draw_boat_view();panel(9,8,382,28);
   char s[80];snprintf(s,sizeof(s),"TAG %d  %02d:00   HUNGER %d  DURST %d",day,hour,hunger,thirst);
   text(20,17,.5f,C(255,239,213,255),s);
   if(messageFrames>0){panel(35,72,330,31);text(47,83,.48f,C(255,240,212,255),message);}
   panel(7,174,298,53);
   button(8,177,72,"DECK",room==0);button(82,177,72,"KUECHE",room==1);button(156,177,72,"OBERDECK",room==2);button(230,177,72,"BETT",room==3);
   text(16,216,.37f,C(255,236,205,255),"A: AKTION   B: TRINKEN");
 }else if(phase==NIGHT){
   draw_boat_view();panel(25,46,350,145);
   text(73,67,.78f,C(255,227,181,255),"DIE NACHT AUF SEE");
   text(56,112,.5f,C(230,233,219,255),nightEvent<50?"Eine ruhige Nacht auf See.":nightEvent<78?"Ein Sturm zieht ueber das Boot!":"Du entdeckst Treibgut im Mondlicht.");
   text(76,153,.5f,C(255,227,181,255),"A DRUECKEN FUER MORGEN");
 }else{
   sky(true);panel(35,48,330,145);text(94,70,.8f,C(255,187,165,255),"REISE BEENDET");
   char s[40];snprintf(s,sizeof(s),"DU HAST TAG %d ERREICHT",day);text(83,124,.55f,C(255,237,211,255),s);
 }
}
static void draw_bottom(void){
 C2D_TargetClear(bottom,C(8,18,28,255));C2D_SceneBegin(bottom);
 if(phase==INTRO){panel(13,16,294,208);text(28,30,.62f,C(255,232,187,255),"DEIN ABENTEUER");text(28,68,.47f,C(214,225,216,255),"Du bist auf einem grossen Schiff.");text(28,91,.47f,C(214,225,216,255),"Es bleibt nicht dein Zuhause.");text(28,121,.47f,C(214,225,216,255),"Sammle Wasser, Essen und ein Seil.");text(28,144,.47f,C(214,225,216,255),"Dann erreichst du das kleine Segelboot.");button(47,178,225,"LOS GEHTS",true);}
 else if(phase==SHIP){
   panel(8,8,304,224);text(20,17,.59f,C(255,232,187,255),"DEINE AUFGABE");
   text(20,48,.47f,C(225,231,217,255),"Finde die Dinge auf dem Schiff:");
   text(20,75,.48f,gotFood?C(135,221,155,255):C(255,221,177,255),gotFood?"[X] ESSEN":"[ ] ESSEN");
   text(20,98,.48f,gotWater?C(135,221,155,255):C(255,221,177,255),gotWater?"[X] TRINKWASSER":"[ ] TRINKWASSER");
   text(20,121,.48f,gotRope?C(135,221,155,255):C(255,221,177,255),gotRope?"[X] SEIL":"[ ] SEIL");
   text(20,144,.48f,gotCompass?C(135,221,155,255):C(255,221,177,255),gotCompass?"[X] KOMPASS":"[ ] KOMPASS");
   text(20,176,.43f,C(185,213,207,255),"ORT WECHSELN: LINKS / RECHTS");
   text(20,197,.43f,C(185,213,207,255),"A: GEGENSTAND NEHMEN");
   text(20,216,.39f,C(255,232,187,255),"BUG = KLEINES SEGELBOOT");
 }else if(phase==BOAT){
   panel(8,8,304,224);text(20,17,.59f,C(255,232,187,255),"DEIN SEGELBOOT");
   char b[80];snprintf(b,sizeof(b),"ESSEN:%d WASSER:%d FISCHE:%d KOEDER:%d",food,water,fish,bait);text(20,43,.40f,C(225,231,217,255),b);
   text(20,69,.46f,C(255,221,177,255),room==0?"AUSSENDECK - A: ANGELN":room==1?"KUECHE - A: ESSEN / KOCHEN":room==2?"OBERDECK - A: REGENWASSER / PFLANZEN":"SCHLAFZIMMER - A: SCHLAFEN");
   text(20,101,.43f,C(185,213,207,255),"RAUM WAEHLEN: UNTEN ANTIPPEN");
   text(20,126,.43f,C(185,213,207,255),"LINKS / RECHTS: RAUM WECHSELN");
   text(20,151,.43f,C(185,213,207,255),"ZEIT VERGEHT AUCH OHNE AKTIONEN.");
   text(20,179,.43f,C(255,232,187,255),"Nachts koennen Ereignisse passieren.");
   text(20,205,.39f,C(185,213,207,255),"Pflanzen brauchen mehrere Tage.");
 }else if(phase==NIGHT){panel(8,8,304,224);text(22,40,.55f,C(255,232,187,255),"MORGEN KOMMT");text(22,90,.48f,C(214,225,216,255),"Deine Vorräte sind wichtig.");text(22,126,.48f,C(214,225,216,255),"A druecken, um weiterzuspielen.");}
 else {panel(12,20,296,200);text(38,43,.68f,C(255,232,187,255),"ENDE DER REISE");button(54,159,212,"ZURUECK ZUM START",true);}
}
static bool tap(touchPosition t,int x,int y,int w,int h){return t.px>=x&&t.px<x+w&&t.py>=y&&t.py<y+h;}
static void interact_ship(void){
 if(area==0){
   if(!gotFood){gotFood=true;food=3;say("Essen eingepackt. Noch Wasser, Seil und Kompass!");}
   else if(gotCompass){transferred=true;phase=BOAT;day=1;hunger=100;thirst=100;say("Du bist im kleinen Segelboot. Jetzt beginnt die Reise!");}
   else say("Hier ist das Bug. Sammle erst alle vier Dinge!");
 }else if(area==1){
   if(!gotWater){gotWater=true;water=3;say("Trinkwasser gefunden! Suche weiter.");}
   else say("Die Vorratskammer ist leer.");
 }else if(area==2){
   if(!gotRope){gotRope=true;rope=1;say("Ein stabiles Seil. Es koennte nuetzlich sein.");}
   else if(!gotCompass){gotCompass=true;say("Kompass gefunden! Geh zum Bug und steig ins Segelboot.");}
   else say("Auf der Bruecke ist nichts mehr fuer dich.");
 }
}
static void tick_boat(void){
 hour++;
 hunger=clamp(hunger-3,0,100);
 thirst=clamp(thirst-4,0,100);
 if(hour>=24){hour=6;day++;sleptToday=false;rainCollected=false;weather=rand()%3;if(plants>0){plantDays++;if(plantDays>=2){food++;plants--;plantDays=0;say("Deine Pflanze ist gewachsen. Du hast Essen!");}}}
 if(hunger<25||thirst<25)stamina=clamp(stamina-5,0,100);
 if(hunger<=0||thirst<=0||stamina<=0)phase=ENDING;
}
int main(int argc,char **argv){
 (void)argc;(void)argv;gfxInitDefault();C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);C2D_Init(12000);C2D_Prepare();
 top=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);bottom=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 srand((unsigned)osGetTime());
 while(aptMainLoop()){
  hidScanInput();u32 down=hidKeysDown();u32 held=hidKeysHeld();touchPosition t;hidTouchRead(&t);bool touched=(down&KEY_TOUCH)!=0;
  if(phase==INTRO){if((down&KEY_A)||touched){phase=SHIP;area=0;say("Erkunde das grosse Schiff. Sammle vier wichtige Dinge.");}}
  else if(phase==SHIP){
   if(down&KEY_LEFT)area=(area+2)%3;
   if(down&KEY_RIGHT)area=(area+1)%3;
   if(down&KEY_UP)area=2;
   if(down&KEY_DOWN)area=0;
   if(down&KEY_A)interact_ship();
   if(touched){if(tap(t,47,178,225,27)){interact_ship();}}
  }else if(phase==BOAT){
   if(down&KEY_LEFT)room=(room+3)%4;
   if(down&KEY_RIGHT)room=(room+1)%4;
   if(down&KEY_A){
     if(room==0){
       if(lastFishHour==hour)say("Du hast in dieser Stunde schon geangelt.");
       else if(bait<=0)say("Kein Koeder mehr. Du brauchst neue Vorräte.");
       else{lastFishHour=hour;bait--;fishAnim=90;if(rand()%100<52){fish++;caught=1;say("Ein Fisch! Der Fang ist im Vorrat.");}else{caught=0;say("Heute beisst nichts an.");}}
     }else if(room==1){
       if(food>0&&hunger<90){food--;hunger=clamp(hunger+38,0,100);say("Du isst eine Portion.");}
       else if(fish>0&&hunger<90){fish--;hunger=clamp(hunger+32,0,100);say("Du bereitest deinen Fisch zu.");}
       else say("Du hast gerade nichts zu essen.");
     }else if(room==2){
       if(weather==1&&!rainCollected){water=clamp(water+2,0,8);rainCollected=true;say("Regen! Du sammelst zwei Portionen Wasser.");}
       else if(!rainCollected){rainCollected=true;say("Es regnet nicht. Du stellst einen Topf fuer Regen auf.");}
       else if(plants<2&&food>0){food--;plants++;plantDays=0;say("Du pflanzt etwas an. Pflege es ueber mehrere Tage.");}
       else say("Hier gibt es gerade nichts zu tun.");
     }else{
       if(hour<19)say("Es ist noch zu frueh. Schlafen kannst du abends.");
       else if(sleptToday)say("Du hast heute schon geschlafen.");
       else{sleptToday=true;nightEvent=rand()%100;phase=NIGHT;}
     }
   }
   if(down&KEY_B){if(water>0&&thirst<90){water--;thirst=clamp(thirst+42,0,100);say("Du trinkst sauberes Wasser.");}else say("Kein Wasser mehr uebrig.");}
   if((frame%300)==0)tick_boat();
   if(touched){
     if(tap(t,8,177,72,45))room=0;
     else if(tap(t,82,177,72,45))room=1;
     else if(tap(t,156,177,72,45))room=2;
     else if(tap(t,230,177,72,45))room=3;
   }
  }else if(phase==NIGHT){if((down&KEY_A)||touched){
   day++;hour=6;hunger=clamp(hunger-10,0,100);thirst=clamp(thirst-12,0,100);stamina=clamp(stamina+30,0,100);
   if(nightEvent<50)say("Eine ruhige Nacht. Du bist ausgeruht.");
   else if(nightEvent<78){water=clamp(water-1,0,8);stamina=clamp(stamina-20,0,100);say("STURM! Wasser ging verloren und du bist erschoepft.");}
   else{food++;say("GUTES ZEICHEN: Treibgut mit Essen ist am Boot!");}
   rainCollected=false;weather=rand()%3;phase=BOAT;
 }}
  else if(phase==ENDING){if((down&KEY_A)||touched){phase=INTRO;area=0;food=water=rope=fish=0;gotFood=gotWater=gotRope=gotCompass=transferred=false;day=1;hunger=thirst=stamina=100;}}
  if(messageFrames>0)messageFrames--;if(fishAnim>0)fishAnim--;sway+=.04f;frame++;
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);draw_top();draw_bottom();C3D_FrameEnd(0);
 }
 C2D_Fini();C3D_Fini();gfxExit();return 0;
}
