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
typedef enum { INTRO, SHIP, CREW, BOAT, NIGHT, ENDING } Phase;
static Phase phase=INTRO;
static C3D_RenderTarget *top,*bottom;
static int frame=0, area=0, facing=0, food=0, water=0, rope=0, fish=0, day=1, hunger=100, thirst=100, caught=0, hull=100;
static int energy[3]={100,100,100};
static int menuChoice=0, crewChoice=0, salvageSeconds=60, carriedItem=-1;
static bool menuHelp=false;
static int hour=8, room=0, bait=3, plants=0, plantDays=0, lastFishHour=-1, nightEvent=0, nightKind=0, nightChoice=0, weather=0;
static bool gotFood=false,gotWater=false,gotRope=false,gotCompass=false, transferred=false, sleptToday=false, rainCollected=false;
static int itemArea[4]={0,1,2,2};
static bool itemTaken[4]={false,false,false,false};
static bool itemStored[4]={false,false,false,false};
static char message[100]="";
static int messageFrames=0, fishAnim=0;
static float sway=0;
static bool energy_empty(void);
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
static const char *item_name(int i){
    static const char *names[4]={"ESSEN","TRINKWASSER","SEIL","KOMPASS"};
    return names[i<0||i>3?0:i];
}
static const char *item_status(int i){
    if(itemStored[i])return "IM BOOT";
    if(itemTaken[i])return "GETRAGEN";
    return itemArea[i]==0?"AM BUG":(itemArea[i]==1?"IM LAGER":"AUF BRUECKE");
}
static int item_here(int a){
    for(int i=0;i<4;i++)if(itemArea[i]==a&&!itemTaken[i]&&!itemStored[i])return i;
    return -1;
}
static void reset_salvage(void){
    bool atBoat=false;
    for(int i=0;i<4;i++){itemArea[i]=rand()%3;itemTaken[i]=false;itemStored[i]=false;}
    for(int i=0;i<4;i++)if(itemArea[i]==0)atBoat=true;
    if(!atBoat)itemArea[rand()%4]=0;
}
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
static void draw_item_icon(int item,float x,float y,float s){
    /* Large, distinct silhouettes so the pickup is recognizable in the world. */
    if(item==0){
        O(x+10*s,y+12*s,8*s,C(203,72,55,255));
        O(x+17*s,y+12*s,7*s,C(220,91,61,255));
        R(x+12*s,y+1*s,3*s,7*s,C(91,126,66,255));
        R(x+15*s,y+2*s,7*s,3*s,C(104,153,81,255));
    }else if(item==1){
        R(x+8*s,y+6*s,13*s,21*s,C(82,164,198,255));
        R(x+11*s,y+1*s,7*s,7*s,C(82,164,198,255));
        R(x+10*s,y-2*s,9*s,4*s,C(226,191,104,255));
        R(x+10*s,y+15*s,9*s,3*s,C(186,225,229,255));
    }else if(item==2){
        O(x+14*s,y+14*s,10*s,C(189,145,83,255));
        O(x+14*s,y+14*s,5*s,C(35,57,60,255));
        R(x+19*s,y+5*s,8*s,4*s,C(189,145,83,255));
        R(x+24*s,y+5*s,4*s,9*s,C(189,145,83,255));
    }else{
        O(x+14*s,y+14*s,11*s,C(197,177,119,255));
        O(x+14*s,y+14*s,7*s,C(45,77,86,255));
        R(x+13*s,y+4*s,2*s,10*s,C(245,224,164,255));
        R(x+14*s,y+13*s,9*s,2*s,C(245,224,164,255));
        O(x+14*s,y+14*s,2*s,C(230,106,82,255));
    }
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
 if(phase==SHIP){
    int found=item_here(area);
    if(found>=0){
      panel(258,137,132,62);
      draw_item_icon(found,270,151,1.15f);
      text(299,148,.43f,C(255,239,213,255),item_name(found));
      text(299,170,.36f,C(160,226,180,255),"A AUFHEBEN");
    }
  }
  R(196,116,8,2,C(241,231,207,210));R(199,113,2,8,C(241,231,207,210));
}
static void draw_boat_view(void){
 sky(hour<6||hour>=19);
 /* compact dinghy: visible bow, gunwales and one mast from the player's eyes */
 float drift=sinf(sway)*2.2f;
 R(0,150,400,90,C(10,55,70,255));
 for(int i=0;i<14;i++){float x=fmodf(i*41+frame*(.5f+i%3*.12f),420)-10;R(x,154+(i%5)*15,27,2,C(86,149,157,180));}
 R(0,188+drift,400,52,C(93,60,39,255));R(0,181+drift,400,8,C(147,99,60,255));
 R(0,172+drift,74,68,C(117,77,47,255));R(326,172+drift,74,68,C(117,77,47,255));
 R(12,176+drift,376,4,C(193,147,94,255));
 R(195+drift,33,5,151,C(112,87,62,255));R(199+drift,41,2,112,C(195,165,118,255));
 /* sail */
 R(204+drift,43,96,82,C(218,211,182,255));R(207+drift,48,86,70,C(185,192,176,255));
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
 /* room-specific first-person interiors and upper deck */
 if(room==1){
   R(0,0,400,240,C(48,38,34,255));R(0,0,400,18,C(26,31,34,255));
   R(0,24,400,5,C(125,83,55,255));R(22,48,110,70,C(97,143,151,255));R(28,54,98,58,C(29,76,91,255));
   R(178,48,195,88,C(93,63,43,255));R(185,55,181,7,C(158,111,70,255));
   R(52,150,292,16,C(133,87,53,255));R(64,166,12,45,C(77,52,39,255));R(320,166,12,45,C(77,52,39,255));
   R(90,132,60,18,C(174,136,83,255));O(120,127,10,C(187,198,164,255));
   text(135,83,.58f,C(255,231,187,255),"KUECHE");
 }else if(room==2){
   sky(hour<6||hour>=19);
   R(0,145,400,95,C(91,60,42,255));for(int i=0;i<8;i++)R(0,150+i*12,400,2,C(143,101,67,255));
   R(15,82,74,76,C(109,76,49,255));R(21,76,62,10,C(165,121,73,255));
   R(282,111,82,39,C(76,105,60,255));R(291,101,64,16,C(101,142,73,255));
   O(308,94,14,C(68,126,67,255));O(330,88,13,C(76,137,73,255));O(351,96,15,C(58,113,62,255));
   text(112,53,.58f,C(255,231,187,255),"OBERDECK");
 }else if(room==3){
   R(0,0,400,240,C(43,35,40,255));R(0,0,400,18,C(24,29,35,255));
   R(32,40,336,150,C(88,61,54,255));R(44,53,312,122,C(112,80,68,255));
   R(66,118,268,82,C(67,75,91,255));R(66,105,268,29,C(191,183,157,255));
   R(74,109,88,21,C(221,211,185,255));R(0,200,400,40,C(31,28,34,255));
   R(20,65,52,34,C(75,107,123,255));R(25,70,42,24,C(22,55,72,255));
   text(147,76,.62f,C(255,231,187,255),"SCHLAFZIMMER");
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
   sky(true);horizon_ship();panel(18,28,364,184);
   text(43,52,.88f,C(255,230,185,255),"DONT BE AWAKE");
   text(56,83,.88f,C(255,230,185,255),"WITH THE FISCHES");
   text(83,137,.52f,C(220,230,215,255),"EINE REISE AUF OFFENER SEE");
   text(91,174,.50f,C(255,230,185,255),menuHelp?"STEUERUNG UNTEN LESEN":"NEUES SPIEL AUSWAEHLEN");
 }else if(phase==SHIP){
   draw_ship_view();panel(9,8,382,27);
   char timerText[80];snprintf(timerText,sizeof(timerText),"EVAKUIERUNG  %02d SEK.  |  SAMMLE VORRAETE",salvageSeconds);text(20,17,.48f,C(255,239,213,255),timerText);
   if(messageFrames>0){panel(35,72,330,31);text(47,83,.48f,C(255,240,212,255),message);}
   if(area==0){R(286,121,42,26,C(119,79,46,255));R(291,116,31,8,C(175,128,75,255));}
   if(area==1){R(135,112,40,48,C(45,77,85,255));}
   if(area==2){R(151,91,30,23,C(112,169,170,255));}
   panel(92,207,216,24);text(106,214,.43f,C(255,236,205,255),"PFEILE: GEHEN  A: AUFNEHMEN / EINLADEN");
 }else if(phase==CREW){
   sky(true);horizon_ship();panel(24,34,352,172);text(75,55,.78f,C(255,230,185,255),"WAHLE EINEN KAMERADEN");text(50,91,.52f,C(225,231,217,255),crewChoice==0?"> FREDERIK - ANGELN":"  FREDERIK - ANGELN");text(50,119,.52f,C(225,231,217,255),crewChoice==1?"> ROW - REPARATUR":"  ROW - REPARATUR");text(50,147,.52f,C(225,231,217,255),crewChoice==2?"> LAUREL - HEILUNG":"  LAUREL - HEILUNG");text(75,183,.43f,C(255,230,185,255),"A: AUSWAEHLEN");
 }else if(phase==BOAT){
   draw_boat_view();panel(9,8,382,53);
   char s[96];snprintf(s,sizeof(s),"TAG %d  %02d:00   HUNGER %d  DURST %d  RUMPF %d%%",day,hour,hunger,thirst,hull);
   text(18,15,.46f,C(255,239,213,255),s);
   for(int i=0;i<3;i++){int x=20+i*124;text(x,34,.36f,C(255,232,205,255),i==0?"ANGELN":i==1?"ARBEIT":"KAMERAD");R(x,45,100,7,C(39,55,62,255));R(x,45,energy[i],7,energy[i]>40?C(102,190,147,255):C(211,105,91,255));}
   if(messageFrames>0){panel(35,72,330,31);text(47,83,.48f,C(255,240,212,255),message);}
   panel(7,174,298,53);
   button(8,177,72,"DECK",room==0);button(82,177,72,"KUECHE",room==1);button(156,177,72,"OBERDECK",room==2);button(230,177,72,"BETT",room==3);
   text(16,216,.37f,C(255,236,205,255),"A: AKTION  X: REDEN  B: TRINKEN");
 }else if(phase==NIGHT){
   draw_boat_view();panel(25,46,350,145);
   text(73,67,.78f,C(255,227,181,255),"DIE NACHT AUF SEE");
   if(nightKind==0){
     text(55,104,.53f,C(230,233,219,255),"HINTER DIR PLATSCHT ETWAS!");
     text(58,130,.43f,C(210,224,218,255),"Ein Flattern am Heck. Sieh nach?");
   }else if(nightKind==1){
     text(65,104,.55f,C(230,233,219,255),"EINE TRUHE TREIBT VORBEI!");
     text(58,130,.43f,C(210,224,218,255),"Du koenntest sie an Bord ziehen.");
   }else if(nightKind==2){
     text(84,104,.55f,C(230,233,219,255),"EIN STURM ZIEHT AUF!");
     text(58,130,.43f,C(210,224,218,255),"Das Boot wird heftig geschaukelt.");
   }else{
     text(88,104,.55f,C(230,233,219,255),"DIE SEE IST RUHIG.");
     text(58,130,.43f,C(210,224,218,255),"Du kannst dich etwas erholen.");
   }
   if(nightKind<2)text(83,157,.43f,C(255,227,181,255),"UNTEN JA / NEIN AUSWAEHLEN");
   else text(91,157,.43f,C(255,227,181,255),"A DRUECKEN FUER MORGEN");
 }else{
   sky(true);panel(35,48,330,145);text(94,70,.8f,C(255,187,165,255),"REISE BEENDET");
   char s[40];snprintf(s,sizeof(s),"DU HAST TAG %d ERREICHT",day);text(83,124,.55f,C(255,237,211,255),s);
 }
}
static void draw_bottom(void){
 C2D_TargetClear(bottom,C(8,18,28,255));C2D_SceneBegin(bottom);
 if(phase==INTRO){panel(10,10,300,220);
   if(!menuHelp){text(25,23,.65f,C(255,232,187,255),"HAUPTMENUE");button(38,65,244,"NEUES SPIEL",menuChoice==0);button(38,105,244,"STEUERUNG",menuChoice==1);button(38,145,244,"INFO",menuChoice==2);text(25,197,.40f,C(185,213,207,255),"OBEN / UNTEN WAEHLEN - A BESTAETIGEN");}
   else{text(25,23,.65f,C(255,232,187,255),"STEUERUNG");text(25,64,.48f,C(225,231,217,255),"A: AKTION / AUSWAHL");text(25,91,.48f,C(225,231,217,255),"B: ZURUECK / TRINKEN");text(25,118,.48f,C(225,231,217,255),"LINKS / RECHTS: RAUM WECHSELN");text(25,145,.48f,C(225,231,217,255),"TOUCH: BEREICHE AUSWAEHLEN");button(38,178,244,"ZURUECK",true);}}
 else if(phase==SHIP){
   panel(8,8,304,224);text(20,17,.59f,C(255,232,187,255),"DEINE AUFGABE");
   text(20,48,.47f,C(225,231,217,255),"Finde die Dinge auf dem Schiff:");
   {char s[48];snprintf(s,sizeof(s),"ESSEN: %s",item_status(0));text(20,75,.44f,itemStored[0]?C(135,221,155,255):C(255,221,177,255),s);}
   {char s[48];snprintf(s,sizeof(s),"WASSER: %s",item_status(1));text(20,98,.44f,itemStored[1]?C(135,221,155,255):C(255,221,177,255),s);}
   {char s[48];snprintf(s,sizeof(s),"SEIL: %s",item_status(2));text(20,121,.44f,itemStored[2]?C(135,221,155,255):C(255,221,177,255),s);}
   {char s[48];snprintf(s,sizeof(s),"KOMPASS: %s",item_status(3));text(20,144,.44f,itemStored[3]?C(135,221,155,255):C(255,221,177,255),s);}
   text(20,176,.43f,C(185,213,207,255),"ORT WECHSELN: LINKS / RECHTS");
   text(20,197,.43f,C(185,213,207,255),carriedItem>=0?"TRAEGT EINEN GEGENSTAND":"A: AUFNEHMEN / EINLADEN");
   text(20,216,.39f,C(255,232,187,255),"X: EVAKUIEREN (AM BOOT)");
 }else if(phase==CREW){panel(8,8,304,224);text(20,17,.59f,C(255,232,187,255),"KAMERAD AUSWAEHLEN");button(22,61,272,"FREDERIK - ANGELN",crewChoice==0);button(22,101,272,"ROW - REPARATUR",crewChoice==1);button(22,141,272,"LAUREL - HEILUNG",crewChoice==2);text(22,192,.42f,C(185,213,207,255),"LINKS / RECHTS: WAEHLEN  A: BESTAETIGEN");}
 else if(phase==BOAT){
   panel(8,8,304,224);text(20,17,.59f,C(255,232,187,255),"DEIN SEGELBOOT");
   char b[80];snprintf(b,sizeof(b),"ESSEN:%d WASSER:%d FISCHE:%d KOEDER:%d",food,water,fish,bait);text(20,43,.40f,C(225,231,217,255),b);
   text(20,69,.46f,C(255,221,177,255),room==0?"AUSSENDECK - A: ANGELN":room==1?"KUECHE - A: ESSEN / KOCHEN":room==2?"OBERDECK - A: REGENWASSER / PFLANZEN":(energy_empty()||hour>=24?"SCHLAFZIMMER - A: SCHLAFEN":"SCHLAFZIMMER - ENERGIE VERBRAUCHEN"));
   text(20,101,.43f,C(185,213,207,255),"RAUM WAEHLEN: UNTEN ANTIPPEN");
   text(20,126,.43f,C(185,213,207,255),"LINKS / RECHTS: RAUM WECHSELN");
   text(20,151,.43f,C(185,213,207,255),"Aktionen kosten Zeit und Energie.");
   text(20,179,.43f,C(255,232,187,255),"Nachts koennen Ereignisse passieren.");
   text(20,205,.39f,C(185,213,207,255),"Schlafen: alle 3 Balken leer oder Tag vorbei.");
 }else if(phase==NIGHT){
   panel(8,8,304,224);
   text(22,24,.59f,C(255,232,187,255),"WAS MOECHTEST DU TUN?");
   if(nightKind==0){
     text(22,57,.47f,C(225,231,217,255),"Etwas bewegt sich hinter dem Boot.");
     text(22,83,.43f,C(185,213,207,255),"Vielleicht ein Fisch - vielleicht nicht.");
     button(22,127,132,"NACHSEHEN",nightChoice==0);
     button(162,127,132,"IGNORIEREN",nightChoice==1);
   }else if(nightKind==1){
     text(22,57,.47f,C(225,231,217,255),"Eine alte Truhe treibt im Wasser.");
     text(22,83,.43f,C(185,213,207,255),"Sie koennte Vorräte enthalten.");
     button(22,127,132,"BERGEN",nightChoice==0);
     button(162,127,132,"WEITERFAHREN",nightChoice==1);
   }else{
     text(22,70,.47f,C(225,231,217,255),"A druecken, um die Nacht zu beenden.");
   }
   text(22,194,.40f,C(185,213,207,255),"LINKS / RECHTS: AUSWAEHLEN  A: BESTAETIGEN");
 }
 else {panel(12,20,296,200);text(38,43,.68f,C(255,232,187,255),"ENDE DER REISE");button(54,159,212,"ZURUECK ZUM START",true);}
}
static bool tap(touchPosition t,int x,int y,int w,int h){return t.px>=x&&t.px<x+w&&t.py>=y&&t.py<y+h;}
static void interact_ship(void){
 if(carriedItem>=0){
   if(area!=0){say("Bring den Gegenstand zum Rettungsboot am Bug.");return;}
   int id=carriedItem;
   itemStored[id]=true;
   if(id==0){gotFood=true;food=3;}
   else if(id==1){gotWater=true;water=3;}
   else if(id==2){gotRope=true;rope=1;}
   else if(id==3){gotCompass=true;}
   carriedItem=-1;
   char msg[100];snprintf(msg,sizeof(msg),"%s im Rettungsboot verstaut!",item_name(id));say(msg);
   return;
 }
 int found=item_here(area);
 if(found>=0){
   carriedItem=found;itemTaken[found]=true;
   char msg[100];snprintf(msg,sizeof(msg),"%s aufgenommen. Bring es zum Rettungsboot!",item_name(found));say(msg);
   return;
 }
 say(area==0?"Hier liegt nichts mehr. Das Rettungsboot ist bereit.":"In diesem Bereich findest du gerade nichts.");
}
static void start_survival(void){food=gotFood?3:0;water=gotWater?3:0;rope=gotRope?1:0;day=1;hour=8;hunger=100;thirst=100;hull=100;energy[0]=energy[1]=energy[2]=100;lastFishHour=-1;sleptToday=false;room=0;phase=BOAT;say("Die Evakuierung ist geschafft. Ueberlebe auf See!");}
static bool energy_empty(void){return energy[0]<=0 && energy[1]<=0 && energy[2]<=0;}
static void tick_boat(void){
 hour+=2;
 if(hour>24)hour=24;
 hunger=clamp(hunger-2,0,100);
 thirst=clamp(thirst-3,0,100);
 if(hunger<=0||thirst<=0||hull<=0)phase=ENDING;
}
static void do_sleep(void){
 if(sleptToday){say("Du hast heute schon geschlafen.");return;}
 if(!energy_empty() && hour<24){
   say("Du hast noch Energie. Nutze sie erst fuer Aktionen oder warte bis spaeter.");
   return;
 }
 sleptToday=true;
 nightEvent=rand()%100;
 nightKind=nightEvent<36?0:nightEvent<68?1:nightEvent<84?2:3;
 nightChoice=0;
 phase=NIGHT;
 say("Du legst dich schlafen. Hoffentlich bleibt die Nacht ruhig.");
}
int main(int argc,char **argv){
 (void)argc;(void)argv;gfxInitDefault();C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);C2D_Init(12000);C2D_Prepare();
 top=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);bottom=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 srand((unsigned)osGetTime());
 while(aptMainLoop()){
  hidScanInput();u32 down=hidKeysDown();u32 held=hidKeysHeld();touchPosition t;hidTouchRead(&t);bool touched=(down&KEY_TOUCH)!=0;
  if(phase==SHIP && (frame%60)==0 && salvageSeconds>0){salvageSeconds--;if(salvageSeconds<=0){carriedItem=-1;if(area==0){phase=CREW;say("Die Zeit ist um! Du hast das Rettungsboot erreicht.");}else{phase=ENDING;say("Du hast das Rettungsboot nicht rechtzeitig erreicht.");}}}
  if(phase==INTRO){if(down&KEY_UP)menuChoice=(menuChoice+2)%3;if(down&KEY_DOWN)menuChoice=(menuChoice+1)%3;if(down&KEY_B)menuHelp=false;if(down&KEY_A){if(menuHelp)menuHelp=false;else if(menuChoice==0){phase=SHIP;area=0;salvageSeconds=60;carriedItem=-1;gotFood=gotWater=gotRope=gotCompass=false;food=water=rope=0;crewChoice=0;reset_salvage();say("Sammle Vorrraete und bringe sie zum Rettungsboot!");}else if(menuChoice==1)menuHelp=true;else say("Ein Abenteuer auf See. Viel Glueck!");}if(touched){if(tap(t,38,65,244,27)){phase=SHIP;area=0;salvageSeconds=60;carriedItem=-1;gotFood=gotWater=gotRope=gotCompass=false;food=water=rope=0;crewChoice=0;reset_salvage();say("Sammle Vorrraete und bringe sie zum Rettungsboot!");}else if(tap(t,38,105,244,27))menuHelp=true;else if(tap(t,38,145,244,27))say("Fish Night - Nintendo 3DS");else if(menuHelp&&tap(t,38,178,244,27))menuHelp=false;}}
  else if(phase==SHIP){
   if(down&KEY_X && area==0){if(carriedItem>=0)say("Lege den Gegenstand erst im Rettungsboot ab!");else{phase=CREW;say("Waehle einen Kameraden fuer die Reise.");}}
   if(down&KEY_LEFT)area=(area+2)%3;
   if(down&KEY_RIGHT)area=(area+1)%3;
   if(down&KEY_UP)area=2;
   if(down&KEY_DOWN)area=0;
   if(down&KEY_A)interact_ship();
   if(touched){if(tap(t,47,178,225,27)){interact_ship();}}
  }else if(phase==CREW){
   if(down&KEY_LEFT)crewChoice=(crewChoice+2)%3;if(down&KEY_RIGHT)crewChoice=(crewChoice+1)%3;
   if(down&KEY_A)start_survival();
   if(touched){if(tap(t,22,61,272,27))crewChoice=0;else if(tap(t,22,101,272,27))crewChoice=1;else if(tap(t,22,141,272,27))crewChoice=2;else if(tap(t,22,61,272,107))start_survival();}
  }else if(phase==BOAT){
   if(down&KEY_LEFT)room=(room+3)%4;
   if(down&KEY_RIGHT)room=(room+1)%4;
   if(down&KEY_A){
     if(room==0){
       if(energy[0]<35)say("Deine Angel-Energie ist aufgebraucht.");
       else if(lastFishHour==hour)say("Du hast in dieser Stunde schon geangelt.");
       else if(bait<=0)say("Kein Koeder mehr. Du brauchst neue Vorräte.");
       else{energy[0]=clamp(energy[0]-35,0,100);lastFishHour=hour;fishAnim=90;if(rand()%100<52){fish++;caught=1;if(crewChoice==0)say("Frederik hilft: Der Koeder bleibt erhalten!");else{bait--;say("Ein Fisch! Der Fang ist im Vorrat.");}}else{bait--;caught=0;say("Heute beisst nichts an.");}tick_boat();}
     }else if(room==1){
       if(energy[1]<20)say("Deine Arbeits-Energie ist aufgebraucht.");
       else if(food>0&&hunger<90){energy[1]=clamp(energy[1]-20,0,100);food--;hunger=clamp(hunger+(crewChoice==2?60:38),0,100);say(crewChoice==2?"Laurel hilft dir beim Essen. Du erholst dich besser.":"Du isst eine Portion.");tick_boat();}
       else if(fish>0&&hunger<90){energy[1]=clamp(energy[1]-20,0,100);fish--;hunger=clamp(hunger+32,0,100);say("Du bereitest deinen Fisch zu.");tick_boat();}
       else say("Du hast gerade nichts zu essen.");
     }else if(room==2){
       if(energy[1]<25)say("Deine Arbeits-Energie ist aufgebraucht.");
       else if(weather==1&&!rainCollected){energy[1]=clamp(energy[1]-25,0,100);water=clamp(water+2,0,8);rainCollected=true;say("Regen! Du sammelst zwei Portionen Wasser.");tick_boat();}
       else if(hull<100&&rope>0){energy[1]=clamp(energy[1]-(crewChoice==1?15:25),0,100);rope--;hull=clamp(hull+(crewChoice==1?65:40),0,100);say(crewChoice==1?"Row repariert den Rumpf mit dem Seil. Das Boot ist wieder stabil!":"Du reparierst den Rumpf mit dem Seil.");tick_boat();}
       else if(!rainCollected){energy[1]=clamp(energy[1]-15,0,100);rainCollected=true;say("Es regnet nicht. Du stellst einen Topf fuer Regen auf.");tick_boat();}
       else if(plants<2&&food>0){energy[1]=clamp(energy[1]-25,0,100);food--;plants++;plantDays=0;say("Du pflanzt etwas an. Pflege es ueber mehrere Tage.");tick_boat();}
       else say("Hier gibt es gerade nichts zu tun.");
     }else{
       do_sleep();
     }
   }
   if(down&KEY_B){if(water>0&&thirst<90){water--;thirst=clamp(thirst+42,0,100);say("Du trinkst sauberes Wasser.");tick_boat();}else say("Kein Wasser mehr uebrig.");}
   if((down&KEY_X) && phase==BOAT){
     if(energy[2]<25)say("Deine Gespraechs-Energie ist aufgebraucht.");
     else{energy[2]=clamp(energy[2]-25,0,100);tick_boat();
       say(crewChoice==0?"Frederik erzaehlt von seinem letzten Fang.":crewChoice==1?"Row spricht ueber das Boot und seine Reparaturen.":"Laurel erkundigt sich, ob es dir gut geht.");}
   }
   if(touched){
     if(tap(t,8,177,72,45))room=0;
     else if(tap(t,82,177,72,45))room=1;
     else if(tap(t,156,177,72,45))room=2;
     else if(tap(t,230,177,72,45))room=3;
   }
  }else if(phase==NIGHT){
   if(nightKind<2){
     bool tapYes=touched&&tap(t,22,127,132,27);
     bool tapNo=touched&&tap(t,162,127,132,27);
     if(down&KEY_LEFT)nightChoice=0;
     if(down&KEY_RIGHT)nightChoice=1;
     if(tapYes)nightChoice=0;
     if(tapNo)nightChoice=1;
     if((down&KEY_A)||tapYes||tapNo){
       if(nightKind==0){
         if(nightChoice==0){
           if(rand()%100<72){fish++;say("Du siehst einen grossen Fisch! Er landet im Vorrat.");}
           else{energy[0]=clamp(energy[0]-12,0,100);say("Nur ein Schatten. Du bist jetzt hellwach.");}
         }else say("Du drehst dich um und bleibst liegen.");
       }else{
         if(nightChoice==0){
           if(rand()%100<68){
             int reward=rand()%3;
             if(reward==0){food+=2;say("In der Truhe liegen zwei Portionen Essen!");}
             else if(reward==1){water=clamp(water+2,0,8);say("Du findest zwei Flaschen Trinkwasser!");}
             else{bait+=2;say("Du findest neue Angelkoeder in der Truhe!");}
           }else{water=clamp(water-1,0,8);energy[1]=clamp(energy[1]-15,0,100);say("Die Truhe ist schwer. Beim Bergen geht Wasser verloren.");}
         }else say("Du laesst die Truhe vorbeiziehen.");
       }
       day++;hour=8;sleptToday=false;hunger=clamp(hunger-10,0,100);thirst=clamp(thirst-12,0,100);
        energy[0]=energy[1]=energy[2]=100;
        if(plants>0){plantDays++;if(plantDays>=2){food++;plants--;plantDays=0;say("Deine Pflanze ist gewachsen. Du hast Essen!");}}
        rainCollected=false;weather=rand()%3;phase=(hull<=0||hunger<=0||thirst<=0)?ENDING:BOAT;
     }
   }else if((down&KEY_A)||touched){
     if(nightKind==2){water=clamp(water-1,0,8);energy[0]=clamp(energy[0]-20,0,100);energy[1]=clamp(energy[1]-20,0,100);if(crewChoice==1&&rope>0){rope--;hull=clamp(hull-5,0,100);say("Row sichert das Boot mit dem Seil. Der Rumpf haelt dem Sturm stand!");}else{hull=clamp(hull-30,0,100);say("STURM! Der Rumpf wird beschaedigt und Wasser geht verloren.");}}
     else say("Eine ruhige Nacht. Du bist ausgeruht.");
     day++;hour=8;sleptToday=false;hunger=clamp(hunger-10,0,100);thirst=clamp(thirst-12,0,100);
        energy[0]=energy[1]=energy[2]=100;
        if(plants>0){plantDays++;if(plantDays>=2){food++;plants--;plantDays=0;say("Deine Pflanze ist gewachsen. Du hast Essen!");}}
        rainCollected=false;weather=rand()%3;phase=(hull<=0||hunger<=0||thirst<=0)?ENDING:BOAT;
   }
  }
  else if(phase==ENDING){if((down&KEY_A)||touched){phase=INTRO;area=0;food=water=rope=fish=0;gotFood=gotWater=gotRope=gotCompass=transferred=false;carriedItem=-1;salvageSeconds=60;crewChoice=0;day=1;hunger=thirst=100;hull=100;energy[0]=energy[1]=energy[2]=100;menuChoice=0;menuHelp=false;}}
  if(messageFrames>0)messageFrames--;if(fishAnim>0)fishAnim--;sway+=.04f;frame++;
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);draw_top();draw_bottom();C3D_FrameEnd(0);
 }
 C2D_Fini();C3D_Fini();gfxExit();return 0;
}
