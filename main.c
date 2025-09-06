// SUPER JUMP GAME    by Takuya Matsubara
//
// (powered by PVSneslib)
#include <snes.h>
#include "soundbank.h"
extern char SOUNDBANK__;

extern char gfxpsrite,gfxpsrite_end;
extern char palsprite,palsprite_end;
extern char patterns, patterns_end;
extern char palette, palette_end;
extern char map, map_end;
extern char tilfont, palfont;

// サウンド
#define SFX_PITCH  8  // 1=4Khz, 2=8khz, 4=16Khz, 8=32Khz
#define SFX_VOLUME (15<<4)+8  // volume + panning
#define SFX_BANG   0
#define SFX_COIN   1
#define SFX_BELL   2
#define SFX_BUTTON 3
#define SFX_TAP    4

// パレット番号
#define PAL_TXT    1 // テキスト
#define PAL_BACK   0 // 背景
#define PAL_SP     2 // スプライト

// BG番号
#define BGN_TXT    0 // テキストBG
#define BGN_BACK   1 // 背景BG

// スプライト
#define SPRITE_MAX 32      // スプライト最大数
#define PRIO_SP    2       // 優先度
#define ATTR_VFLIP (1<<7)  // V反転アトリビュート
#define ATTR_SP (PRIO_SP<<4)+(PAL_SP<<1)  //アトリビュート

#define VRAM_TXTOFS  0x0100 // TEXTタイル番号オフセット

// VRAM 0x0000-7fff
#define VRAM_TXTMAP  0x1000 // TEXT BG マップ
#define VRAM_BACKMAP 0x0000 // 背景用BG マップ
#define VRAM_BGTPTN  0x2000 // TEXT BG パターン
#define VRAM_TXTPTN  0x3000 // TEXT BG パターン+オフセット
#define VRAM_BACKPTN 0x4000 // 背景用BG パターン
#define VRAM_SPPTN   0x6000 // スプライト パターン

const char strtitle[] = "SUPER JUMP GAME";
const char strhardware[] = "for SUPER FAMICOM";
const char strcopyright[] = "(C)2025 Takuya Matsubara";
const char strpush[]  = "PUSH ANY BUTTON";
const char strgover[] = "GAME OVER";
const char strscore[] = "SCORE";
const char strlevel[] = "LEVEL";

// グローバル変数
short score;            // スコア
unsigned char gameover; // ゲームオーバーカウンタ
signed char jump;       // プレイヤーのジャンプ移動量
unsigned char bgx,bgy;  // BGオフセット座標
unsigned char loopcnt;  // ループカウンタ
unsigned char level;    // 難易度（敵の出現度数）
char tmpstr[6];

// タイル番号テーブル
const u16 tiletbl[]={
    0xA8,   // PLAYER
    0xE8,   // COIN
    0xA0    // MONSTER
};

#define ID_NONE    -1
#define ID_PLAYER  0
#define ID_COIN    1
#define ID_MONSTER 2

// スプライトの座標
s16 spr_x[SPRITE_MAX];
s16 spr_y[SPRITE_MAX];
s16 spr_id[SPRITE_MAX];
s16 spr_gfx[SPRITE_MAX];

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

// スプライト消去
void sp_remove(char i)
{
    spr_id[i] = ID_NONE;
}

// スプライト初期化
void sp_init()
{
    char i;
    for(i=0;i<SPRITE_MAX;i++){
        spr_x[i] = -32;
        spr_y[i] = 224;
        spr_id[i] = ID_NONE;
        spr_gfx[i] = 0;
        oamSet(
            i*4,
            spr_x[i],
            spr_y[i],
            PRIO_SP,  // priority
            0,  // V flip
            0,  // H flip
            spr_gfx[i],  // タイル番号オフセット
            PAL_SP   // palette offset
        );
        oamSetEx(i*4, OBJ_SMALL, OBJ_SHOW); // 表示
    }
}

// スプライト追加(X座標,Y座標,id 0～)
char sp_append(s16 x,s16 y,char id)
{
    char i;
    for(i=0; i<SPRITE_MAX; i++){
        if(spr_id[i] == ID_NONE){
            spr_x[i] = x;
            spr_y[i] = y;
            spr_id[i] = id;
            spr_gfx[i] = tiletbl[id];

//            oamSet(
//                i*4,
//                spr_x[i],
//                spr_y[i],
//                PRIO_SP,  // priority
//                0,  // V flip
//                0,  // H flip
//                spr_gfx[i],  // graphic offset
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
    x -= spr_x[num];
    if(x<0) x = -x;
    if(x >= diff)return(0);
    y -= spr_y[num];
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
void pushbutton(void)
{
    unsigned short newbtn,oldbtn;
    char edge;

    spcEffect(SFX_PITCH, SFX_BELL, SFX_VOLUME);

    consoleDrawText(8,21,(char *)strpush);
    spcProcess();
    WaitForVBlank();      // Vブランク待ち

    oldbtn = 0;
    edge = 0;
    while(edge < 2){
        newbtn = padsCurrent(0);   // コントローラー1入力
        if(oldbtn != newbtn){
            edge++;  // ボタンのエッジ検出
            if(edge == 1){
                spcEffect(SFX_PITCH, SFX_BUTTON, SFX_VOLUME);
            }
        }
        oldbtn = newbtn;
        spcProcess();
        WaitForVBlank();     // Vブランク待ち
    }
    text_clear();     // テキストを全消去
}

// タイトル＆ゲーム開始
void title(void)
{
    sp_init();

    consoleDrawText(7,7,(char *)strtitle); // タイトル
    consoleDrawText(10,10,(char *)strhardware); //
    consoleDrawText(4,24,(char *)strcopyright); //
    pushbutton();    // ボタンの入力待ち

    sp_append(64,120, ID_PLAYER); // プレイヤーを追加
    score = 0;     // スコア
    jump = 0;      // ジャンプ移動量
    gameover = 0;  // ゲームオーバーカウンタ
    bgx = 0;       // BGオフセット座標
    bgy = 0;
    loopcnt = 0;   // ループカウンタ
    level = 50;    // 難易度
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
    pushbutton();    // ボタンの入力待ち
}

// プレイヤー移動処理
void player(char num,s16 x,s16 y)
{
    unsigned short btn;     // ボタン

    if(gameover){
        oamSetAttr(num*4, x, y, spr_gfx[num], ATTR_VFLIP + ATTR_SP);

        if(y < 224) y += 2;
        spr_y[num]=y;
        return;
    }
    btn = padsCurrent(0);   // コントローラー1入力

    if(((btn & KEY_LEFT ) != 0)&&(x > 8  )) x -= 2;  // 左
    if(((btn & KEY_RIGHT) != 0)&&(x < 228)) x += 2;  // 右
    if((y >= 144)&&((btn & KEY_A)!=0)){
        jump = -31;   // ジャンプスタート
        spcEffect(SFX_PITCH, SFX_TAP ,SFX_VOLUME);
    }
    if(jump < 31){
        jump++;                       // 重力による加速
        if((btn & KEY_A)==0)jump+=2;  // Aボタンを押していない場合
    }
    y += (jump/4);
    if(y > 144) y = 144; // 地面のめり込み防止
    spr_x[num]=x;
    spr_y[num]=y;
    if((loopcnt & 31)==num)spr_gfx[num] ^= 2;
    oamSetAttr(num*4, x, y, spr_gfx[num],ATTR_SP);
}

// コイン移動処理
void coin(char num,s16 x,s16 y)
{
    x--;
    if((gameover==0)&&(sp_hitcheck(0,x,y,15)==1)){  // プレイヤーと衝突
        x = -32;  // コインを画面外へ
        spcEffect(SFX_PITCH, SFX_COIN ,SFX_VOLUME);
        score++;      // スコアを加算
    }
    spr_x[num]=x;
    if((loopcnt & 31)==num)spr_gfx[num] ^= 2;
    oamSetAttr(num*4, x, y, spr_gfx[num], ATTR_SP);
}

// オバケ移動処理
void monster(char num,s16 x,s16 y)
{
    x--;
    x-=(num % 3);
    spr_x[num]=x;
    if((gameover==0)&&(sp_hitcheck(0,x,y,10)==1)){  // プレイヤーと衝突
        spcEffect(SFX_PITCH, SFX_BANG, SFX_VOLUME);
        gameover = 1;    // ゲームオーバー
    }
    if((loopcnt & 31)==num)spr_gfx[num] ^= 2;
    oamSetAttr(num*4, x, y, spr_gfx[num], ATTR_SP);
}

// メイン関数
int main(void)
{
    char i,id;
    s16 x,y;

    // サウンド初期化
    spcBoot();
    spcSetBank(&SOUNDBANK__);
    spcStop();
    for (i=0;i<5;i++) {
        spcLoadEffect(i);
    }

    // テキスト画面初期化
    consoleSetTextMapPtr(VRAM_TXTMAP);
    consoleSetTextGfxPtr(VRAM_TXTPTN);
    consoleSetTextOffset(VRAM_TXTOFS);

    // テキスト用BG初期化
    consoleInitText(
        PAL_TXT,  // パレット番号
        32,       // size of palette of text
        &tilfont, // パターンのポインタ
        &palfont  // パレットのポインタ
    );

    bgSetGfxPtr(BGN_TXT, VRAM_BGTPTN);
    bgSetMapPtr(BGN_TXT, VRAM_TXTMAP, SC_32x32);

    // 背景用パターン初期化
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

    // 背景用タイルマップ初期化
    bgInitMapSet(
        BGN_BACK,   // BG番号
        &map,       // タイルマップ
        (&map_end - &map), // size of map
        SC_32x32,     // tile mapping
        VRAM_BACKMAP  // VRAMのアドレス
    );

    // スプライト初期化
    oamInitGfxSet(
        &gfxpsrite,      // パターンのポインタ
        (&gfxpsrite_end - &gfxpsrite),        // パターンのサイズ
        &palsprite,      // パレットのポインタ
        (&palsprite_end - &palsprite),        // パレットサイズ
        PAL_SP,              // パレット番号
        VRAM_SPPTN,          // VRAMのアドレス
        OBJ_SIZE16_L32       // 16x16 のスプライト
    );

    setMode(BG_MODE1,0);
    bgSetDisable(2);
    setScreenOn();

    while (1){
    	title();   // タイトル＆ゲーム開始
        while (gameover < 180){
            for(i=0; i<SPRITE_MAX; i++){
                id=spr_id[i];
                if(id==ID_NONE)continue;

                x = spr_x[i];
                y = spr_y[i];
                if(x < -15){
                    sp_remove(i); // スプライトが画面外にある場合は消去
                    continue;
                }
                switch(id){
                case ID_PLAYER:
                    player(i,x,y);  // プレイヤー移動処理
                    break;
                case ID_COIN:
                    coin(i,x,y);    // コイン移動処理
                    break;
                case ID_MONSTER:
                    monster(i,x,y); // オバケ移動処理
                    break;
                }

            }
            loopcnt++;
            if((loopcnt & 63)==0){
                y = (rand() % 8)*16 + 32;
                sp_append(240,y,ID_COIN); // コイン出現
                if(level < 255)level++;  // 難易度を上げる
            }
            if((loopcnt & 15)==0 ){
                num_to_str(tmpstr,score);   // スコアを文字列化
                consoleDrawText(13,22,tmpstr);  // スコア表示


                if((rand() & 255) <= level){
                    y = (rand() % 17)*8 + 16;
                    sp_append(240,y,ID_MONSTER); // オバケ出現
                }
            }
            if(gameover)gameover++;
            bgx++;  // BGオフセット座標を変更
            spcProcess();
            WaitForVBlank();      // Vブランク待ち

            bgSetScroll(BGN_BACK, bgx, bgy);  // BGスクロール
        }
        result();   // リザルト
    }
    return 0;
}
