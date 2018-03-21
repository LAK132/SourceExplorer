//D:\Google Drive\Five Nights at Freddys\Discord\Clickteam Dumps\FiveNightsatFreddys1.exe

#ifndef DEFINES_H
#define DEFINES_H
// Executable Signature
#define WIN_EXE_SIG 0x5A4D
// Position of Pointer to PE Header
#define WIN_EXE_PNT 0x3C
// PE Signature
#define WIN_PE_SIG 0x00004550

//
// Game Headers
//
#define HEADER_PACK 0x1247874977777777
// PAME
#define HEADER_UNIC 0x554d4150
// PAMU
#define HEADER_GAME 0x454D4150
#define CNCV1VER 0x207

//
// Product code
//
#define PROD_MMF1 0x0300
#define PROD_MMF15 0x0301
#define PROD_MMF2 0x0302

//
// Old Data Chunks
//
#define CHUNK_OLD_HEADER 0x2223
#define CHUNK_OLD_FRAMEITEMS 0x2229
// Dummy in some games (PA2)
#define CHUNK_OLD_FRAMEITEMS2 0x223F
#define CHUNK_OLD_FRAME 0x3333
#define CHUNK_OLD_FRAMEHEADER 0x3334
// Object Instances
#define CHUNK_OLD_OBJINST 0x3338
// Frame Events
#define CHUNK_OLD_FRAMEEVENTS 0x333D
// Object Properties
#define CHUNK_OLD_OBJPROP 0x4446

//
// New Data Chunks
//
#define CHUNK_ENTRY 0x0302
// Vitalise Chunks (0x11XX)
// Vitalise Preview
#define CHUNK_VITAPREV 0x1122
// App Chunks (0x22XX)
#define CHUNK_HEADER 0x2223
#define CHUNK_TITLE 0x2224
#define CHUNK_AUTHOR 0x2225
#define CHUNK_MENU 0x2226
#define CHUNK_EXTPATH 0x2227
// Deprecated // Not in Anaconda
#define CHUNK_EXTENS 0x2228
#define CHUNK_OBJECTBANK 0x2229
// Deprecated // Not in Anaconda
#define CHUNK_GLOBALEVENTS 0x222A
#define CHUNK_FRAMEHANDLES 0x222B
#define CHUNK_EXTDATA 0x222C
// Deprecated // Not in Anaconda
#define CHUNK_ADDEXTNS 0x222D
#define CHUNK_PROJPATH 0x222E
#define CHUNK_OUTPATH 0x222F
#define CHUNK_APPDOC 0x22230
#define CHUNK_OTHEREXT 0x2231
#define CHUNK_GLOBALVALS 0x2232
#define CHUNK_GLOBALSTRS 0x2233
#define CHUNK_EXTNLIST 0x2234
#define CHUNK_ICON 0x2235
// Not in Anaconda 
#define CHUNK_DEMOVER 0x2236
#define CHUNK_SECNUM 0x2237
#define CHUNK_BINFILES 0x2238
// Not in Anaconda
#define CHUNK_MENUIMAGES 0x2239
#define CHUNK_ABOUT 0x223A
#define CHUNK_COPYRIGHT 0x223B
// Not in Anaconda
#define CHUNK_GLOBALVALNAMES 0x223C
// Not in Anaconda
#define CHUNK_GLOBALSTRNAMES 0x223D
// Movement Extensions
#define CHUNK_MODEMNTEXTNS 0x223E
#define CHUNK_UNKNOWN8 0x223F
#define CHUNK_EXEONLY 0x2240
// 0x2241
#define CHUNK_PROTECTION 0x2242
#define CHUNK_SHADERS 0x2243
// 0x2244
#define CHUNK_EXTDHEADER 0x2245
#define CHUNK_SPACER 0x2246
// Means FRAMEHANDLES might be broken
#define CHUNK_FRAMEHANDLESERROR 0x224D
#define CHUNK_TITLE2 0x2251
// Frame Chunks (0x33XX)
#define CHUNK_FRAME 0x3333
#define CHUNK_FRAMEHEADER 0x3334
#define CHUNK_FRAMENAME 0x3335
#define CHUNK_FRAMEPASSWORD 0x3336
#define CHUNK_FRAMEPALETTE 0x3337
// "WTF"?
#define CHUNK_OBJNAME2 0x3337
#define CHUNK_OBJINST 0x3338
// Frame Fade In Frame // Not in Anaconda
#define CHUNK_FRAMEFADEIF 0x3339
// Frame Fade Out Frame // Not in Anaconda
#define CHUNK_FRAMEFADEOF 0x333A
// Frame Fade In
#define CHUNK_FRAMEFADEI 0x333B
// Frame Fade Out
#define CHUNK_FRAMEFADEO 0x333C
// Frame Events
#define CHUNK_FRAMEEVENTS 0x333D
// Frame Play Header // Not in Anaconda
#define CHUNK_FRAMEPLYHEAD 0x333E
// Frame Additional Item // Not in Anaconda
#define CHUNK_FRAMEADDITEM 0x333F
// Frame Additional Item Instance // Not in Anaconda
#define CHUNK_FRAMEADDITEMISNT 0x3340
#define CHUNK_FRAMELAYERS 0x3341
#define CHUNK_FRAMEVIRTSIZE 0x3342
#define CHUNK_DEMOFILEPATH 0x3343
// Not in Anaconda
#define CHUNK_RANDOMSEED 0x3344
#define CHUNK_FRAMELAYEREFFECT 0x3345
// Frame BluRay Options // Not in Anaconda
#define CHUNK_FRAMEBLURAY 0x3346
// Movement Timer Base
#define CHUNK_MOVETIMEBASE 0x3347
// Mosaic Image Table // Not in Anaconda
#define CHUNK_MOSAICIMGTABLE 0x3348
#define CHUNK_FRAMEEFFECTS 0x3349
// iPhone Options // Not in Anaconda
#define CHUNK_FRAMEIPHONEOPTS 0x334A
// Object Chunks (0x44XX)
// Not a data chunk
#define CHUNK_PAERROR 0x4150
// Object Header
#define CHUNK_OBJHEAD 0x4444
// Object Name
#define CHUNK_OBJNAME 0x4445
// Object Properties
#define CHUNK_OBJPROP 0x4446
// Object Unknown // Not in Anaconda
#define CHUNK_OBJUNKN 0x4447
// Object Effect
#define CHUNK_OBJEFCT 0x4448
// Offset Chunks (0x55XX)
#define CHUNK_ENDIMAGE 0x5555
#define CHUNK_ENDFONT 0x5556
#define CHUNK_ENDSOUND 0x5557
#define CHUNK_ENDMUSIC 0x5558
// Bank Chunks (0x66XX)
#define CHUNK_IMAGEBANK 0x6666
#define CHUNK_FONTBANK 0x6667
#define CHUNK_SOUNDBANK 0x6668
#define CHUNK_MUSICBANK 0x6669
// Last Chunk
#define CHUNK_LAST 0x7F7F

//
// States
//
#define STATE_DEFAULT 0x00
#define STATE_VITA 0x11
#define STATE_NEW 0x22
#define STATE_OLD 0x20
#define STATE_IMAGE 0x66
#define STATE_FONT 0x67
#define STATE_SOUND 0x68
#define STATE_MUSIC 0x69

//
// Modes
//
// Uncompressed
#define MODE_0 0x0000
#define MODE_2 0x0002
// Compressed
#define MODE_1 0x0001
#define MODE_3 0x0003
// Unknown
#define MODE_4 0x0004
#define MODE_FLAG_COMPRESSED 0x1
#define MODE_DEFAULT MODE_0

//
// Frame Header Data Flags
//
#define FRAME_HEADER_DISPLAYNAME 0x0001
#define FRAME_HEADER_GRABDESKTOP 0x0002
#define FRAME_HEADER_KEEPDISPLAY 0x0004
#define FRAME_HEADER_FADEIN 0x0008
#define FRAME_HEADER_FADEOUT 0x0010
#define FRAME_HEADER_TOTALCOLLISIONMASK 0x0020
#define FRAME_HEADER_PASSWORD 0x0040
#define FRAME_HEADER_RESIZEATSTART 0x0080
#define FRAME_HEADER_DONOTCENTER 0x0100
#define FRAME_HEADER_FORCELOADONCALL 0x0200
#define FRAME_HEADER_NOSURFACE 0x0400
#define FRAME_HEADER_RESERVED1 0x0800
#define FRAME_HEADER_RESERVED2 0x1000
#define FRAME_HEADER_RECORDDEMO 0x2000
// 0x4000 - N/A
#define FRAME_HEADER_TIMEDMOVEMENTS 0x8000

//
// Object Property Flags
//
#define OBJECT_PROP_PLAYER -7
#define OBJECT_PROP_KEYBOARD -6
#define OBJECT_PROP_CREATE -5
#define OBJECT_PROP_TIME -4
#define OBJECT_PROP_GAME -3
#define OBJECT_PROP_SPEAKER -2
#define OBJECT_PROP_SYSTEM -1
#define OBJECT_PROP_QUICKBACKDROP 0
#define OBJECT_PROP_BACKDROP 1
#define OBJECT_PROP_ACTIVE 2
#define OBJECT_PROP_TEXT 3
#define OBJECT_PROP_QUESTION 4
#define OBJECT_PROP_SCORE 5
#define OBJECT_PROP_LIVES 6
#define OBJECT_PROP_COUNTER 7
#define OBJECT_PROP_RTF 8
#define OBJECT_PROP_SUBAPP 9

#endif