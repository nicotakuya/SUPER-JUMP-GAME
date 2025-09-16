// SUPER JUMP GAME    by Takuya Matsubara
//
// (powered by PVSnesLib)
#include <snes.h>
#include "soundbank.h"
extern char SOUNDBANK__;

extern char gfxpsrite,gfxpsrite_end;
extern char palsprite,palsprite_end;
extern char patterns, patterns_end;
extern char palette, palette_end;
extern char map, map_end;
extern char patterns2, patterns2_end;
extern char palette2, palette2_end;
extern char map2, map2_end;
extern char tilfont, palfont;
extern char tilfont_end, palfont_end;

// サウンド
#define SFX_PITCH  8  // 1=4Khz, 2=8khz, 4=16Khz, 8=32Khz
#define SFX_VOLUME (15<<4)+8  // volume + panning
#define SFX_BANG   0
#define SFX_COIN   1
#define SFX_BELL   2
#define SFX_BUTTON 3
#define SFX_TAP    4
#define SFX_MAX    5

// パレット番号
#define PAL_TXT    3  // テキスト
#define PAL_SP     1  // スプライト
#define PAL_BACK   2  // 背景（手前）
#define PAL_BACK2  0  // 背景（奥）

// BG番号
#define BGN_TXT    0  // テキスト
#define BGN_BACK   1  // 背景（手前）
#define BGN_BACK2  2  // 背景（奥）

// スプライト
#define SPRITE_MAX 32      // スプライト最大数
#define PRIO_SP    2       // 優先度
#define ATTR_VFLIP  (1<<7)+(PRIO_SP<<4)+(PAL_SP<<1)  // V反転アトリビュート
#define ATTR_NORMAL (PRIO_SP<<4)+(PAL_SP<<1)  //アトリビュート

#define VRAM_TXTOFS  0x0000 // テキスト タイル番号オフセット

// VRAM 0x0000-7fff
#define VRAM_SPPTN    0x0000 // スプライト パターン
#define VRAM_BGTPTN   0x2000 // テキストのパターン
#define VRAM_TXTPTN   0x2000 // テキストのパターン+オフセット
#define VRAM_TXTMAP   0x2800 // テキストタイルマップ
#define VRAM_BACKMAP  0x3000 // 背景(手前)用BG タイルマップ
#define VRAM_BACKPTN  0x4000 // 背景(手前)用BG パターン
#define VRAM_BACK2MAP 0x5000 // 背景(奥)用BG タイルマップ
#define VRAM_BACK2PTN 0x6000 // 背景(奥)用BG パターン

const char strversion[]= "Ver.1.10";
const char strtitle[] = "SUPER JUMP GAME";
const char strhardware[] = "for SUPER FAMICOM";
const char strcopyright[] = "(C)2025 Takuya Matsubara.";
const char strpush[]  = "PUSH ANY BUTTON";
const char strgover[] = "GAME OVER";
const char strscore[] = "SCORE";
const char strlevel[] = "LEVEL";
const char strpause[] = "PAUSE";
const char strcursor1[] = "=>";
const char strcursor2[] = "  ";
const char strmenu[4][12]={
    "GAME MODE 1",
    "GAME MODE 2",
    "GAME MODE 3",
    "ABOUT THIS"
};
const char strabout[6][24] = {
    "-- About this game --",
    "There is no story.",
    "Just control the player",
    "and collect coins.",
    "This game was developed",
    "using PVSnesLib."
};

// グローバル変数
short score,score_old;            // スコア
unsigned char gamemode;
unsigned char gameover; // ゲームオーバーカウンタ
unsigned char bgx,bgy;  // BGオフセット座標
unsigned char bg2x,bg2y;  // BGオフセット座標
unsigned char loopcnt;  // ループカウンタ
unsigned char level;    // 難易度（敵の出現度数）
char tmpstr[6];

// タイル番号テーブル
const u16 tiletbl[]={
    0xA8,   // 0:PLAYER
    0xE8,   // 1:COIN
    0xA0,   // 2:MONSTER
    0xE2,   // 3:SMOKE
    0x80,   // 4:EXPLOSION
    0x44,   // 5:BOMB
    0x20,   // 6:FIRE_L
    0x24,   // 7:FIRE_S
    0x2C,   // 8:BALL
    0xCC,   // 9:TOGE
    0xCE,   // 10:ROCK
    0x4C,   // 11:THUNDER
    0x60,   // 12:ROBOT_A
};

// アニメパターン数テーブル
const u16 animetbl[]={
    2,   // 0:PLAYER
    4,   // 1:COIN
    2,   // 2:MONSTER
    4,   // 3:SMOKE
    4,   // 4:EXPLOSION
    2,   // 5:BOMB
    2,   // 6:FIRE_L
    2,   // 7:FIRE_S
    2,   // 8:BALL
    1,   // 9:TOGE
    1,   // 10:ROCK
    2,   // 11:THUNDER
    4, // 12:ROBOT_A
};

#define ID_NONE      255
#define ID_PLAYER    0
#define ID_COIN      1
#define ID_MONSTER   2
#define ID_SMOKE     3
#define ID_EXPLOSION 4
#define ID_BOMB      5
#define ID_FIRE_L    6
#define ID_FIRE_S    7
#define ID_BALL      8
#define ID_TOGE      9
#define ID_ROCK      10
#define ID_THUNDER   11
#define ID_ROBOT_A   12

typedef struct {
    u8 id;
    u8 count;
    s16 x;
    s16 y;
    u16 gfx;
    u16 attr;
    s8 x1;
    s8 y1;
} ST_OBJ;

ST_OBJ sprobj[SPRITE_MAX];

// 10進数5ケタ変換(格納先ポインタ,数値0～65535)
void num_to_str(char *str,unsigned short num)
{
    char i=5;
    str += 5;
    *str = 0; // NULL
    str--;
    while(i--){
        *str = (num % 10)+'0';
        num /= 10;
        str--;
    }
}

// ゲーム開始サブ
void game_start()
{
    char i;
    ST_OBJ *p_obj = &sprobj[0];

    for(i=0;i<SPRITE_MAX;i++,p_obj++){
        p_obj->x = -32;
        p_obj->y = 224;
        p_obj->id = ID_NONE;
        p_obj->gfx = 0;
        oamSet(
            i*4,
            p_obj->x,
            p_obj->y,
            PRIO_SP,  // priority
            0,  // V flip
            0,  // H flip
            0,  // タイル番号オフセット
            PAL_SP   // palette offset
        );
        oamSetEx(i*4, OBJ_SMALL, OBJ_SHOW); // 表示
    }

    bgx = 0;       // BGオフセット座標
    bgy = 0;
    bg2x = 0;      // BGオフセット座標
    bg2y = 0;
    bgSetScroll(BGN_BACK, bgx, bgy);
    bgSetScroll(BGN_BACK2, bg2x, bg2y);

    score = 0;      // スコア
    score_old = -1; // スコア
    gameover = 0;   // ゲームオーバーカウンタ
    loopcnt = 0;    // ループカウンタ
    level = 50;     // 難易度
}

// スプライト追加(X座標,Y座標,id 0～)
char sp_append(s16 x,s16 y,char id)
{
    char i;
    ST_OBJ *p_obj = &sprobj[0];

    for(i=0; i<SPRITE_MAX; i++,p_obj++){
        if(p_obj->id == ID_NONE){
            p_obj->x = x;
            p_obj->y = y;
            p_obj->id = id;
            p_obj->gfx = tiletbl[id];
            p_obj->attr = ATTR_NORMAL;
            p_obj->count = 0;

//            oamSet(
//                i*4,
//                p_obj->x,
//                p_obj->y,
//                PRIO_SP,  // priority
//                0,  // V flip
//                0,  // H flip
//                p_obj->gfx,  // graphic offset
//                PAL_SP   // palette offset
//            );
            break;
        }
    }
    return(i);
}

// スプライト衝突判定(スプライト番号,X座標,Y座標,しきい値)
char sp_hitcheck(char num,s16 x,s16 y,s16 diff)
{
    ST_OBJ *p_obj;
    p_obj = &sprobj[num];
    x -= p_obj->x;
    if(x<0) x = -x;
    if(x >= diff)return(0);
    y -= p_obj->y;
    if(y<0) y = -y;
    if(y >= diff)return(0);
    return(1);  // 戻り値=1の場合、衝突あり
}

// テキスト全消去
void text_clear()
{
    char tmpx,tmpy,i;
    for(i=0; i<32*28/16; i++){
        tmpx=(i % 2)*16;
        tmpy=(i / 2);
        consoleDrawText(tmpx, tmpy, "                ");
        spcProcess();
        WaitForVBlank();
    }
}

// ボタンの入力待ち
u16 pushbutton(void)
{
    u16 btn;
    while(1){
        rand();
        if(padsCurrent(0)==0)break;
        spcProcess();
        WaitForVBlank();      // Vブランク待ち
    }
    while(1){
        rand();
        btn = padsCurrent(0);
        if(btn != 0)break;
        spcProcess();
        WaitForVBlank();     // Vブランク待ち
    }
    spcEffect(SFX_PITCH, SFX_BUTTON, SFX_VOLUME);
    return(btn);
}

// このゲームについて
void about(void)
{
    char i;

    for(i=0; i<6; i++){
        consoleDrawText(
            4,4+(i*2),(char *)strabout[i]
        ); //
    }
    pushbutton();    // ボタンの入力待ち
    return;
}

// タイトル＆ゲーム開始
void title(void)
{
#define MENU_X 6
#define MENU_Y 10

    char i;
    u16 btn;

    game_start();  // ゲーム開始サブ
    text_clear();     // テキストを全消去
    consoleDrawText(23, 1,(char *)strversion);   //
    consoleDrawText( 7, 5,(char *)strtitle);     // タイトル
    consoleDrawText(10, 7,(char *)strhardware);  //
    consoleDrawText( 4,24,(char *)strcopyright); //

    spcEffect(SFX_PITCH, SFX_BELL, SFX_VOLUME);
    for(i=0; i<4; i++){
        consoleDrawText(
            MENU_X+2,MENU_Y+(i*2),(char *)strmenu[i]
        );
    }
    while(1){ // メニュー
        gamemode &= 3;
        consoleDrawText(
            MENU_X,MENU_Y+(gamemode*2),(char *)strcursor1
        ); //
        btn = pushbutton();    // ボタンの入力待ち
        if(btn & (KEY_START | KEY_A | KEY_B))break;
        consoleDrawText(
            MENU_X,MENU_Y+(gamemode*2),(char *)strcursor2
        );
        if(btn & KEY_UP   ) gamemode--;
        if(btn & KEY_DOWN ) gamemode++;
    }
    text_clear();
}

// リザルト画面
void result(void)
{
    text_clear();    // テキストを全消去
    consoleDrawText(11,6,(char *)strgover); // GAME OVER
    consoleDrawText(10,10,(char *)strscore); // SCORE表示
    consoleDrawText(10,14,(char *)strlevel); // 難易度表示
    num_to_str(tmpstr,score);   // スコアを文字列化
    consoleDrawText(16,10,tmpstr);  // スコア表示
    num_to_str(tmpstr,level);   // 難易度を文字列化
    consoleDrawText(16,14,tmpstr);  // 難易度表示
    consoleDrawText(8,21,(char *)strpush);
    spcEffect(SFX_PITCH, SFX_BELL, SFX_VOLUME);
    pushbutton();    // ボタンの入力待ち
}

// プレイヤー移動処理
void player(ST_OBJ *p_obj)
{
    unsigned short btn;     // ボタン
    s16 x,y;

    x = p_obj->x;
    y = p_obj->y;
    if(gameover){
        if(y < 224) y += 2;
        p_obj->y = y;
        p_obj->attr = ATTR_VFLIP;
        return;
    }
    btn = padsCurrent(0);   // コントローラー1入力

    if(((btn & KEY_LEFT ) != 0)&&(x > 8  )) x -= 2;  // 左
    if(((btn & KEY_RIGHT) != 0)&&(x < 228)) x += 2;  // 右
    if((y >= 144)&&((btn & KEY_A)!=0)){
        p_obj->y1 = -31;   // ジャンプスタート
        sp_append(x,y+8, ID_SMOKE); // 煙
        spcEffect(SFX_PITCH, SFX_TAP ,SFX_VOLUME);
    }
    if(p_obj->y1 < 31){
        p_obj->y1++;                       // 重力による加速
        if((btn & KEY_A)==0) p_obj->y1 += 2;  // Aボタンを押していない場合
    }
    y += (p_obj->y1 / 4);
    if(y > 144){
        y = 144; // 地面のめり込み防止
    }
    p_obj->x = x;
    p_obj->y = y;
}

// コイン移動処理
void coin(ST_OBJ *p_obj)
{
    s16 x,y;

    x = p_obj->x - 1;
    y = p_obj->y;
    if((gameover==0)&&(sp_hitcheck(0,x,y,15)==1)){  // プレイヤーと衝突
        p_obj->x = -32;  // コインを画面外へ
        spcEffect(SFX_PITCH, SFX_COIN ,SFX_VOLUME);
        score++;      // スコアを加算
        return;
    }
    p_obj->x = x;
}

// オバケ移動処理
void monster(ST_OBJ *p_obj)
{
    s16 x,y;
    char i;

    x = p_obj->x;
    y = p_obj->y;

    if(p_obj->count == 0){ // 速度を決定
        p_obj->x1 = -((rand() % 3)+1);
    }
    x += p_obj->x1;
    if((gameover==0)&&(sp_hitcheck(0,x,y,10)==1)){  // プレイヤーと衝突
        gameover = 1;    // ゲームオーバー
    }
    p_obj->x = x;
}

// 煙
void smoke(ST_OBJ *p_obj){
    p_obj->x--;
    if(p_obj->count >= 32){
        p_obj->x = -16;
    }
}

// 爆発
void explosion(ST_OBJ *p_obj){
    if(p_obj->count == 0){
        p_obj->x1 = ((rand() % 5)-2);
        p_obj->y1 = ((rand() % 5)-2);
    }else{
        if(p_obj->count >= 32){
            p_obj->x = -16;
            return;
        }
    }
    p_obj->x += p_obj->x1;
    p_obj->y += p_obj->y1;
}

// ボール移動処理
void ball(ST_OBJ *p_obj)
{
    s16 x,y;
    char i;

    if(p_obj->count == 0){ // 速度を決定
        p_obj->x1 = -((rand() % 3)+3);
        p_obj->y1 = 0;
    }
    if(p_obj->count & 1)return;

    x = p_obj->x;
    y = p_obj->y;
    x += p_obj->x1;
    y += (p_obj->y1 / 2);

    p_obj->y1 += 1;
    if(p_obj->y > 144){
        y = 144;
        p_obj->y1 = -18;
    }

    if((gameover==0)&&(sp_hitcheck(0,x,y,10)==1)){  // プレイヤーと衝突
        gameover = 1;    // ゲームオーバー
    }
    p_obj->x = x;
    p_obj->y = y;
}
// ロボットA移動処理
void robot_a(ST_OBJ *p_obj)
{
    s16 x,y;
    char i;

    x = p_obj->x;
    y = p_obj->y;
    if(p_obj->count == 0){ // 速度を決定
        p_obj->x1 = ((rand() % 3)+1);
        p_obj->y1 = ((rand() % 3)-1);
    }
    x -= p_obj->x1;
    if((p_obj->count & 3)== 0){
    y += p_obj->y1;
    }
    if((gameover==0)&&(sp_hitcheck(0,x,y,10)==1)){  // プレイヤーと衝突
        spcEffect(SFX_PITCH, SFX_BANG, SFX_VOLUME);
        gameover = 1;    // ゲームオーバー
    }
    p_obj->x = x;
    p_obj->y = y;
}

// 画面初期化
void screen_init()
{
    char i;

    // サウンド初期化
    spcBoot();
    spcSetBank(&SOUNDBANK__);
    spcStop();
    for (i=0;i< SFX_MAX;i++) {
        spcLoadEffect(i);
    }

    // テキスト初期化
    consoleInit();
    consoleSetTextMapPtr(VRAM_TXTMAP); // タイルマップ
    consoleSetTextGfxPtr(VRAM_TXTPTN); // パターン
    consoleSetTextOffset(VRAM_TXTOFS); // TEXTタイル番号オフセット

    // テキスト用パターン＆パレット設定
    consoleInitText(
        PAL_TXT,  // パレット番号
        (&palfont_end - &palfont),    // パレットのサイズ
        &tilfont, // パターンのデータ
        &palfont  // パレットのデータ
    );

    // テキスト用 BG初期化
    bgSetGfxPtr(BGN_TXT, VRAM_BGTPTN);
    bgSetMapPtr(BGN_TXT, VRAM_TXTMAP, SC_32x32);

    // 背景(手前)用パターン初期化
    bgInitTileSet(
        BGN_BACK,  // BG番号
        &patterns, // パターンのポインタ
        &palette,  // パレットのポインタ
        PAL_BACK,  // パレット番号
        (&patterns_end - &patterns),   // パターンのサイズ
        (&palette_end - &palette),     // パレットのサイズ
        BG_16COLORS,   // used for correct palette entry
        VRAM_BACKPTN   // VRAMのアドレス
    );

    // 背景(手前)用タイルマップ初期化
    bgInitMapSet(
        BGN_BACK,   // BG番号
        &map,       // タイルマップ
        (&map_end - &map), // size of map
        SC_32x32,     // tile mapping
        VRAM_BACKMAP  // VRAMのアドレス
    );

    // 背景（奥）用パターン初期化
    bgInitTileSet(
        BGN_BACK2,     // BG番号
        &patterns2,    // パターンのポインタ
        &palette2,     // パレットのポインタ
        PAL_BACK2,     // パレット番号
        (&patterns2_end - &patterns2),   // パターンのサイズ
        (&palette2_end - &palette2),     // パレットのサイズ
        BG_4COLORS0,   // used for correct palette entry
        VRAM_BACK2PTN  // VRAMのアドレス
    );

    // 背景（奥）用タイルマップ初期化
    bgInitMapSet(
        BGN_BACK2,   // BG番号
        &map2,       // タイルマップ
        (&map2_end - &map2), // size of map
        SC_32x32,     // tile mapping
        VRAM_BACK2MAP  // VRAMのアドレス
    );

    // パレット0を変更
    u8 *ptr = (u8 *)&palette2;
    for(i=0; i<4; i++){
        u16 color;  // 15bit カラー
        color = (u16)*ptr++;
        color += (u16)*ptr++ << 8;
        setPaletteColor(i,color);
    }

    // スプライト初期化
    oamInitGfxSet(
        &gfxpsrite,      // パターンのポインタ
        (&gfxpsrite_end - &gfxpsrite),        // パターンのサイズ
        &palsprite,      // パレットのポインタ
        (&palsprite_end - &palsprite),        // パレットサイズ
        PAL_SP,          // パレット番号
        VRAM_SPPTN,      // VRAMのアドレス
        OBJ_SIZE16_L32   // 16x16 のスプライト
    );

    setMode(BG_MODE1, 0);
    WaitForVBlank();
    setScreenOn();
    gamemode = 0;
}

// メイン関数
int main(void)
{
    char i,id;
    s16 y;
    ST_OBJ *p_obj;

    screen_init();

    while (1){
    	title();   // タイトル＆ゲーム開始
        if(gamemode==3){
            about();
            continue;
        }
        sp_append(64,120, ID_PLAYER); // プレイヤー追加
        while (1){
            p_obj = &sprobj[0];
            for(i=0; i<SPRITE_MAX; i++,p_obj++){
                id = p_obj->id;
                if(id == ID_NONE)continue;
                switch(id){
                case ID_PLAYER:
                    player(p_obj);  // プレイヤー
                    break;
                case ID_COIN:
                    coin(p_obj);    // コイン
                    break;
                case ID_MONSTER:
                    monster(p_obj); // オバケ
                    break;
                case ID_SMOKE:
                    smoke(p_obj); // 煙
                    break;
                case ID_EXPLOSION:
                    explosion(p_obj); // 爆発
                    break;
                case ID_BALL:
                    ball(p_obj); // ボール
                    break;
                case ID_ROBOT_A:
                    robot_a(p_obj); // ロボットA
                    break;
                }
                oamSetAttr(i*4, p_obj->x, p_obj->y, p_obj->gfx, p_obj->attr);

                if(p_obj->x < -15){
                    // スプライトが画面外にある場合は消去
                    p_obj->id = ID_NONE;
                    continue;
                }
                p_obj->count++; // 汎用カウンタ
                if((p_obj->count & 7)==0){     // アニメ
                    u16 mask,anime;
                    mask = animetbl[id]*2 -1;
                    anime = (p_obj->gfx + 2) & mask;
                    anime += (p_obj->gfx & ~mask);
                    p_obj->gfx = anime;
                }
            }
            loopcnt++;
            if((loopcnt & 63)==0){       // コイン出現
                y = (rand() % 8)*16 + 32;
                sp_append(255,y,ID_COIN);
                if(level < 255)level++;  // 難易度を上げる
            }
            if(score != score_old){           // スコア表示
                num_to_str(tmpstr,score);
                consoleDrawText(13,22,tmpstr);
                score_old = score;
            }
            if((loopcnt & 15)==0 ){           // 敵の出現
                if((rand() & 255) <= level){
                    y = (rand() % 17)*8 + 16;
                    switch(gamemode){
                    case 0:
                        sp_append(255,y,ID_MONSTER);
                        break;
                    case 1:
                        sp_append(255,y,ID_BALL);
                        break;
                    case 2:
                        sp_append(255,y,ID_ROBOT_A);
                        break;
                    }
                }
            }
            if(loopcnt & 1){                 // BGスクロール
                bgx+=2;
                bgSetScroll(BGN_BACK, bgx, bgy);
//                bg2x++;
//                bgSetScroll(BGN_BACK2, bg2x, bg2y);
            }
            if(gameover){ // ゲームオーバー処理
                if(gameover==1){
                    spcEffect(SFX_PITCH, SFX_BANG, SFX_VOLUME);
                }
                if(gameover <= 12){
                    sp_append(sprobj[0].x,sprobj[0].y, ID_EXPLOSION);
                }
                gameover++;
                if(gameover >= 180)break;
            }
            spcProcess();
            WaitForVBlank();      // Vブランク待ち
        }
        result();   // リザルト
    }
    return 0;
}
