#pragma once

#include <cstdint>

#define SCENEID_PREVDECISION -1
#define SCENEID_ENDGAME 32767

#define SEGMENT_BEGINNING 0
#define SEGMENT_DECISION 1

#pragma pack(push, 1)

struct _coord
{
	int16_t     x;
	int16_t     y;
};

struct _actionDef
{
	int32_t     scoreDelta;
	int16_t     nextSceneID;       // will jump to the scene with the name "SCxx", where xx stands for nextSceneID (2 digits at least)
								   // 7FFF (32767) = end game
								   // FFFF (   -1) = go back to the last decision
	int16_t     sceneSegment;      // 0 = scene from beginning, 1 = decision page
	_coord      cHotspotTopLeft;
	_coord      cHotspotBottomRigh;
};

struct _sceneDef
{
	int16_t     numPics;
	int16_t     pictureIndex;
	int16_t     numActions;
	char        szSceneFolder[14]; // Foldername *must* be "SCxx" (case sensitive) where xx stands for a 2 digit ID
	char        szDialogWav[14];
	char        szDecisionBmp[14];
	_actionDef  actions[3];
};

struct _pictureDef
{
	int16_t     duration;          // deciseconds
	char        szBitmapFile[14];
};

struct _gameBinFile
{
	int16_t     unknown1[7];
	int16_t     numScenes;
	int16_t     numPics;
	int16_t     unknown2[2];
	_sceneDef   scenes[100];       // Scenes start at file position 0x0016
	_pictureDef pictures[2000];    // Pictures start at file position 0x2596
};

#pragma pack(pop)