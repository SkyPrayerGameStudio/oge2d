/*
-----------------------------------------------------------------------------
This source file is part of Open Game Engine 2D.
It is licensed under the terms of the MIT license.
For the latest info, see http://oge2d.sourceforge.net

Copyright (c) 2010-2012 Lin Jia Jun (Joe Lam)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef __OGE_H_INCLUDED__
#define __OGE_H_INCLUDED__

#include "ogeEngine.h"

//int OGE_Init(int iWidth, int iHeight, int iBPP, bool bFullScreen = false);

int OGE_Init(int argc, char** argv);
void OGE_Fini();
void OGE_Run();

int OGE_GetParamCount();
char** OGE_GetParams();

void OGE_ResetStdOutput();

const std::string& OGE_GetMainDir();
std::string OGE_GetMainPath();

const std::string& OGE_GetGameDir();
std::string OGE_GetGamePath();

const std::string& OGE_GetDebugServerAddr();

int OGE_GetWindowID();

CogeEngine* OGE_GetEngine();

CogeVideo* OGE_GetVideo();

CogeImage* OGE_GetScreen();


// api for script ...

void OGE_Print(const std::string& sInfo);

std::string OGE_IntToStr(int iValue);
int  OGE_StrToInt(const std::string& sValue);
int  OGE_HexToInt(std::string& sValue);

std::string OGE_FloatToStr(double fValue);
double  OGE_StrToFloat(const std::string& sValue);

int OGE_StrToUnicode(const std::string& sText, const std::string& sCharsetName, int iBufferId);
std::string OGE_UnicodeToStr(int iUnicodeBufferId, int iBufferSize, const std::string& sCharsetName);

int OGE_StrLen(const std::string& sText);
int OGE_FindStr(const std::string& sText, const std::string& sSubText, int iStartPos);
std::string OGE_CopyStr(const std::string& sText, int iStartPos, int iCount);
std::string OGE_DeleteStr(const std::string& sText, int iStartPos, int iCount);
std::string OGE_ReplaceStr(const std::string& sText, const std::string& sOldSubText, const std::string& sNewSubText);
std::string OGE_TrimStr(const std::string& sText);

int  OGE_GetTicks();

int  OGE_GetOS();

std::string OGE_GetDateTimeStr();
bool OGE_GetDateTime(int& year, int& month, int& day, int& hour, int& min, int& sec);

void OGE_Delay(int iMilliSeconds);

int  OGE_Random(int iMax);
double OGE_RandomFloat();

bool OGE_LibraryExists(const std::string& sLibraryName);
bool OGE_ServiceExists(const std::string& sLibraryName, const std::string& sServiceName);
int OGE_CallService(const std::string& sLibraryName, const std::string& sServiceName, int iParam);

const std::string& OGE_GetGameMainDir();

int OGE_MakeColor(int iRed, int iGreen, int iBlue);

int OGE_GetImage(const std::string& sName);

int OGE_LoadImage(const std::string& sName, int iWidth, int iHeight, int iColorKeyRGB, const std::string& sFileName);

const std::string& OGE_GetImageName(int iImageId);

int  OGE_GetColorKey(int iImageId);
void OGE_SetColorKey(int iImageId, int iColor);

int  OGE_GetPenColor(int iImageId);
void OGE_SetPenColor(int iImageId, int iColor);

int OGE_GetFont(const std::string& sName, int iFontSize);
const std::string& OGE_GetFontName(int iFontId);
int OGE_GetFontSize(int iFontId);
int OGE_GetDefaultFont();
void OGE_SetDefaultFont(int iFontId);
int OGE_GetImageFont(int iImageId);
void OGE_SetImageFont(int iImageId, int iFontId);

const std::string& OGE_GetDefaultCharset();
void OGE_SetDefaultCharset(const std::string& sCharsetName);

const std::string& OGE_GetSystemCharset();
void OGE_SetSystemCharset(const std::string& sCharsetName);

int OGE_GetImageWidth(int iImageId);
int OGE_GetImageHeight(int iImageId);

int OGE_GetImageRect(int iImageId);
int OGE_GetRect(int x, int y, int w, int h);

//void OGE_DrawImage(int iDstImageId, int x, int y, int iSrcImageId);
//void OGE_DrawImageWithEffect(int iDstImageId, int x, int y, int iSrcImageId, int iEffectType, double fEffectValue);

void OGE_Draw(int iDstImageId, int x, int y, int iSrcImageId, int iRectId);
void OGE_DrawWithEffect(int iDstImageId, int x, int y, int iSrcImageId, int iRectId, int iEffectType, double fEffectValue);

void OGE_FillColor(int iImageId, int iColor);
void OGE_FillRect(int iImageId, int iRGBColor, int iRectId);
void OGE_FillRectAlpha(int iImageId, int iRGBColor, int iRectId, int iAlpha);

void OGE_DrawText(int iImageId, const std::string& sText, int x, int y);
void OGE_DrawTextWithDotFont(int iImageId, const std::string& sText, int x, int y);
int  OGE_GetTextWidth(int iImageId, const std::string& sText);
bool OGE_GetTextSize(int iImageId, const std::string& sText, int& w, int& h);

bool OGE_IsInputUnicode();

void OGE_DrawBufferText(int iImageId, int iBufferId, int iBufferSize, int x, int y, const std::string& sCharsetName);
int  OGE_GetBufferTextWidth(int iImageId, int iBufferId, int iBufferSize, const std::string& sCharsetName);
bool OGE_GetBufferTextSize(int iImageId, int iBufferId, int iBufferSize, const std::string& sCharsetName, int& w, int& h);

void OGE_DrawLine(int iImageId, int iStartX, int iStartY, int iEndX, int iEndY);
void OGE_DrawCircle(int iImageId, int iCenterX, int iCenterY, int iRadius);

int OGE_GetClipboardB();
int OGE_GetClipboardC();

void OGE_Screenshot();
int OGE_GetLastScreenshot();

int OGE_GetScreenImage();
void OGE_ImageRGB(int iImageId, int iRectId, int iRed, int iGreen, int iBlue);
//void OGE_ScreenBltColor(int iColor, int iAlpha);
void OGE_ImageLightness(int iImageId, int iRectId, int iAmount);
void OGE_ImageGrayscale(int iImageId, int iRectId, int iAmount);
void OGE_ImageBlur(int iImageId, int iRectId, int iAmount);

void OGE_StretchBltToScreen(int iSrcImageId, int iRectId);

void OGE_SaveImageAsBMP(int iImageId, const std::string& sBmpFileName);


void OGE_ShowFPS(bool bShow);
//void OGE_HideFPS();

void OGE_LockFPS(int iFPS);
//void OGE_UnlockFPS();
//bool OGE_IsFPSLocked();
int  OGE_GetFPS();
int  OGE_GetLockedFPS();

void OGE_LockCPS(int iCPS);
//void OGE_UnlockCPS();
//bool OGE_IsCPSLocked();
int  OGE_GetCPS();
int  OGE_GetLockedCPS();

void OGE_ShowMousePos(bool bShow);
//void OGE_HideMousePos();

void OGE_SetDirtyRectMode(bool bValue);

void OGE_Scroll(int iIncX, int iIncY);
void OGE_SetViewPos(int iLeft, int iTop);
int  OGE_GetViewPosX();
int  OGE_GetViewPosY();
int  OGE_GetViewWidth();
int  OGE_GetViewHeight();

void OGE_StopAutoScroll();
void OGE_StartAutoScroll(int iStepX, int iStepY, int iInterval, bool bLoop);
void OGE_SetAutoScrollSpeed(int iStepX, int iStepY, int iInterval);
void OGE_SetScrollTarget(int iTargetX, int iTargetY);

int  OGE_SetSceneMap(int iSceneId, const std::string& sNewMap, int iNewLeft, int iNewTop);

int  OGE_GetMap();
int  OGE_GetMapByName(const std::string& sMapName);
const std::string& OGE_GetMapName(int iMapId);
int  OGE_GetMapWidth(int iMapId);
int  OGE_GetMapHeight(int iMapId);
int  OGE_GetMapImage(int iMapId);
bool OGE_GetEightDirectionMode(int iMapId);
void OGE_SetEightDirectionMode(int iMapId, bool value);
int  OGE_GetColumnCount(int iMapId);
int  OGE_GetRowCount(int iMapId);
int  OGE_GetTileValue(int iMapId, int iTileX, int iTileY);
int  OGE_SetTileValue(int iMapId, int iTileX, int iTileY, int iValue);
void OGE_ReloadTileValues(int iMapId);
void OGE_ResetGameMap(int iMapId);
int  OGE_GetTileData(int iMapId, int iTileX, int iTileY);
int  OGE_SetTileData(int iMapId, int iTileX, int iTileY, int iValue);
void OGE_InitTileData(int iMapId, int iInitValue);
void OGE_InitTileValues(int iMapId, int iInitValue);
bool OGE_PixelToTile(int iMapId, int iPixelX, int iPixelY, int& iTileX, int& iTileY);
bool OGE_TileToPixel(int iMapId, int iTileX, int iTileY, int& iPixelX, int& iPixelY);
bool OGE_AlignPixel(int iMapId, int iInPixelX, int iInPixelY, int& iOutPixelX, int& iOutPixelY);
int  OGE_FindWay(int iMapId, int iStartX, int iStartY, int iEndX, int iEndY, int iTileListId);
int  OGE_FindRange(int iMapId, int iPosX, int iPosY, int iMovementPoints, int iTileListId);

int  OGE_GetTileListSize(int iTileListId);
int  OGE_GetTileXFromList(int iTileListId, int iIndex);
int  OGE_GetTileYFromList(int iTileListId, int iIndex);
int  OGE_GetTileIndexInList(int iTileListId, int iTileX, int iTileY);
int  OGE_AddTileToList(int iTileListId, int iTileX, int iTileY);
int  OGE_RemoveTileFromList(int iTileListId, int iIndex);
void OGE_ClearTileList(int iTileListId);

int  OGE_GetKey();
void OGE_HoldKeyEvent();
void OGE_SkipMouseEvent();

int  OGE_GetScanCode();

void OGE_ShowKeyboard(bool bShow);
bool OGE_IsKeyboardShown();

void OGE_SetKeyDown(int iKey);
void OGE_SetKeyUp(int iKey);

bool OGE_IsKeyDown(int iKey);
bool OGE_IsKeyDownTime(int iKey, int iInterval);

int  OGE_GetJoystickCount();
bool OGE_IsJoyKeyDown(int iJoyId, int iKey);
bool OGE_IsJoyKeyDownTime(int iJoyId, int iKey, int iInterval);

bool OGE_IsMouseLeftDown();
bool OGE_IsMouseRightDown();

bool OGE_IsMouseLeftUp();
bool OGE_IsMouseRightUp();

int  OGE_GetMouseX();
int  OGE_GetMouseY();

int OGE_GetIntVar(int iDataId, int iIndex);
void OGE_SetIntVar(int iDataId, int iIndex, int iValue);
double OGE_GetFloatVar(int iDataId, int iIndex);
void OGE_SetFloatVar(int iDataId, int iIndex, double fValue);
const std::string& OGE_GetStrVar(int iDataId, int iIndex);
void OGE_SetStrVar(int iDataId, int iIndex, const std::string& sValue);
int OGE_GetBufVar(int iDataId, int iIndex);
void OGE_SetBufVar(int iDataId, int iIndex, const std::string& sValue);

int  OGE_GetIntVarCount(int iDataId);
void OGE_SetIntVarCount(int iDataId, int iCount);
int  OGE_GetFloatVarCount(int iDataId);
void OGE_SetFloatVarCount(int iDataId, int iCount);
int  OGE_GetStrVarCount(int iDataId);
void OGE_SetStrVarCount(int iDataId, int iCount);
int  OGE_GetBufVarCount(int iDataId);
void OGE_SetBufVarCount(int iDataId, int iCount);

void OGE_SortIntVar(int iDataId);
void OGE_SortFloatVar(int iDataId);
void OGE_SortStrVar(int iDataId);

int OGE_FindIntVar(int iDataId, int iValue);
int OGE_FindFloatVar(int iDataId, double fValue);
int OGE_FindStrVar(int iDataId, const std::string& sValue);

int OGE_SplitStrIntoData(const std::string& sStr, const std::string& sDelim, int iDataId);

//int  OGE_GetGameCurrentScene();
//int  OGE_GetGameCurrentSprite();

int  OGE_GetSprCurrentScene(int iSprId);
int  OGE_GetSprCurrentGroup(int iSprId);

int  OGE_GetSpr();
//ogeScriptString OGE_GetCurrentSprName();
const std::string& OGE_GetSprName(int iSprId);

void OGE_CallBaseEvent();

int  OGE_FindGroupSpr(int iGroupId, const std::string& sSprName);
// get spr in current scene, iState: 0=all, 1=active, -1=inactive
//int  OGE_FindSpr(const std::string& sSprName, int iState);
int  OGE_FindSceneSpr(int iSceneId, const std::string& sSprName);
// get spr in whole game, sSprName must be a full name
//int  OGE_FindGameSpr(const std::string& sSprName);

int  OGE_GetFocusSpr();
void OGE_SetFocusSpr(int iSprId);

int  OGE_GetModalSpr();
void OGE_SetModalSpr(int iSprId);

int  OGE_GetMouseSpr();
void OGE_SetMouseSpr(int iSprId);

int  OGE_GetFirstSpr();
void OGE_SetFirstSpr(int iSprId);
int  OGE_GetLastSpr();
void OGE_SetLastSpr(int iSprId);
int  OGE_GetProxySpr();
void OGE_SetProxySpr(int iSprId);

void OGE_FreezeSprites(bool bValue);

//void OGE_HideMouseCursor();
void OGE_ShowMouseCursor(bool bShow);

int  OGE_GetTopZ();
int  OGE_BringSprToTop(int iSprId);

//int  OGE_GetScenePlayerSpr(int iPlayerId);
//void OGE_SetScenePlayerSpr(int iSprId, int iPlayerId);
int  OGE_GetSceneSpr(int idx);
//void OGE_SetSceneSpr(int iSprId, int idx);

int  OGE_GetDefaultPlayerSpr();
void OGE_SetDefaultPlayerSpr(int iSprId);

int  OGE_GetSceneLightMap(int iSceneId);
int  OGE_SetSceneLightMap(int iSceneId, int iImageId, int iLightMode);

int  OGE_GetPlayerSpr(int iPlayerId);
void OGE_SetPlayerSpr(int iSprId, int iPlayerId);
int  OGE_GetUserSpr(int idx);
void OGE_SetUserSpr(int iSprId, int idx);

int  OGE_GetUserGroup(int idx);
void OGE_SetUserGroup(int iSprGroupId, int idx);

int  OGE_GetUserPath(int idx);
void OGE_SetUserPath(int iSprPathId, int idx);

int  OGE_GetUserImage(int idx);
void OGE_SetUserImage(int iImageId, int idx);

int  OGE_GetUserData(int idx);
void OGE_SetUserData(int iDataId, int idx);

int  OGE_GetUserObject(int idx);
void OGE_SetUserObject(int iObjId, int idx);

int  OGE_GetCaretPos(int iSprId);
void OGE_ResetCaretPos(int iSprId);
int  OGE_GetInputTextBuffer(int iSprId);
void OGE_ClearInputTextBuffer(int iSprId);
int  OGE_GetInputCharCount(int iSprId);
//int  OGE_SetInputCharCount(int iSprId, int iSize);
std::string OGE_GetInputText(int iSprId);
int  OGE_SetInputText(int iSprId, const std::string& sText);
void OGE_SetInputWinPos(int iSprId, int iPosX, int iPosY);

bool OGE_GetSprActive(int iSprId);
void OGE_SetSprActive(int iSprId, bool bValue);

bool OGE_GetSprVisible(int iSprId);
void OGE_SetSprVisible(int iSprId, bool bValue);

bool OGE_GetSprInput(int iSprId);
void OGE_SetSprInput(int iSprId, bool bValue);

bool OGE_GetSprDrag(int iSprId);
void OGE_SetSprDrag(int iSprId, bool bValue);

bool OGE_GetSprCollide(int iSprId);
void OGE_SetSprCollide(int iSprId, bool bValue);

bool OGE_GetSprBusy(int iSprId);
void OGE_SetSprBusy(int iSprId, bool bValue);


bool OGE_GetSprDefaultDraw(int iSprId);
void OGE_SetSprDefaultDraw(int iSprId, bool bValue);

int  OGE_GetSprLightMap(int iSprId);
void OGE_SetSprLightMap(int iSprId, int iImageId);

int OGE_GetCollidedSpr(int iSprId);

int OGE_GetSprPlot(int iSprId);
void OGE_SetSprPlot(int iSprId, int iPlotSprId);
int OGE_GetSprMaster(int iSprId);
void OGE_SetSprMaster(int iSprId, int iMasterSprId);
int OGE_GetReplacedSpr(int iSprId);
void OGE_SetReplacedSpr(int iSprId, int iReplacedSprId);

int OGE_GetSprParent(int iSprId);
int OGE_SetSprParent(int iSprId, int iParentId, int iClientX, int iClientY);

int  OGE_GetRelatedSpr(int iSprId, int idx);
void OGE_SetRelatedSpr(int iSprId, int iRelatedSprId, int idx);

int  OGE_GetRelatedGroup(int iSprId, int idx);
void OGE_SetRelatedGroup(int iSprId, int iRelatedGroupId, int idx);

int  OGE_GetSprTileList(int iSprId);

void OGE_PushEvent(int iSprId, int iCustomEventId);

int  OGE_GetSprCurrentImage(int iSprId);
int  OGE_SetSprImage(int iSprId, int iImageId);
int  OGE_GetSprAnimationImg(int iSprId, int iDir, int iAct);
bool OGE_SetSprAnimationImg(int iSprId, int iDir, int iAct, int iImageId);

void OGE_MoveX(int iSprId, int iIncX);
void OGE_MoveY(int iSprId, int iIncY);

int  OGE_SetDirection(int iSprId, int iDir);
int  OGE_SetAction(int iSprId, int iAct);
int  OGE_SetAnimation(int iSprId, int iDir, int iAct);
void OGE_ResetAnimation(int iSprId);

void OGE_DrawSpr(int iSprId);

int  OGE_GetPosX(int iSprId);
int  OGE_GetPosY(int iSprId);
int  OGE_GetPosZ(int iSprId);
void OGE_SetPosXYZ(int iSprId, int iPosX, int iPosY, int iPosZ);
void OGE_SetPos(int iSprId, int iPosX, int iPosY);
void OGE_SetPosZ(int iSprId, int iPosZ);
void OGE_SetRelPos(int iSprId, int iRelativeX, int iRelativeY);
int  OGE_GetRelPosX(int iSprId);
int  OGE_GetRelPosY(int iSprId);
bool OGE_GetRelMode(int iSprId);
void OGE_SetRelMode(int iSprId, bool bEnable);

int  OGE_GetSprRootX(int iSprId);
int  OGE_GetSprRootY(int iSprId);
int  OGE_GetSprWidth(int iSprId);
int  OGE_GetSprHeight(int iSprId);

void OGE_SetSprDefaultRoot(int iSprId, int iRootX, int iRootY);
void OGE_SetSprDefaultSize(int iSprId, int iWidth, int iHeight);

int  OGE_GetDirection(int iSprId);
int  OGE_GetAction(int iSprId);
int  OGE_GetSprType(int iSprId);

int  OGE_GetSprClassTag(int iSprId);
int  OGE_GetSprTag(int iSprId);
void OGE_SetSprTag(int iSprId, int iTag);

int  OGE_GetSprChildCount(int iSprId);
int  OGE_GetSprChildByIndex(int iSprId, int iIndex);

int  OGE_AddEffect(int iSprId, int iEffectType, double fEffectValue);
int  OGE_AddEffectEx(int iSprId, int iEffectType, double fStart, double fEnd, double fIncrement, int iStepInterval, int iRepeat);
int  OGE_RemoveEffect(int iSprId, int iEffectType);
bool OGE_HasEffect(int iSprId, int iEffectType);
bool OGE_IsEffectCompleted(int iSprId, int iEffectType);
double OGE_GetEffectValue(int iSprId, int iEffectType);
double OGE_GetEffectIncrement(int iSprId, int iEffectType);

int  OGE_GetCurrentPath(int iSprId);
int  OGE_GetLocalPath(int iSprId);

int  OGE_UsePath(int iSprId, int iPathId, int iStepLength, bool bAutoStepping);
int  OGE_UseLocalPath(int iSprId, int iStepLength, bool bAutoStepping);
int  OGE_ResetPath(int iSprId, int iStepLength, bool bAutoStepping);
int  OGE_NextPathStep(int iSprId);
void OGE_AbortPath(int iSprId);
int  OGE_GetPathNodeIndex(int iSprId);
int  OGE_GetPathStepLength(int iSprId);
void OGE_SetPathStepLength(int iSprId, int iStepLength);
int  OGE_GetPathStepInterval(int iSprId);
void OGE_SetPathStepInterval(int iSprId, int iStepInterval);
int  OGE_GetValidPathNodeCount(int iSprId);
void OGE_SetValidPathNodeCount(int iSprId, int iStepCount);
int  OGE_GetPathDirection(int iSprId);
//int  OGE_GetPathAvgDirection(int iSprId);

int  OGE_GetPath(const std::string& sPathName);
int  OGE_GetPathByCode(int iCode);
void OGE_PathGenLine(int iPathId, int x1, int y1, int x2, int y2);
void OGE_PathGenExtLine(int iPathId, int iSrcX, int iSrcY, int iDstX, int iDstY, int iExtraWidth, int iExtraHeight);
int  OGE_GetPathNodeCount(int iPathId);

bool OGE_OpenTimer(int iSprId, int iInterval);
void OGE_CloseTimer(int iSprId);
int  OGE_GetTimerInterval(int iSprId);
bool OGE_IsTimerWaiting(int iSprId);

bool OGE_OpenSceneTimer(int iInterval);
void OGE_CloseSceneTimer();
int  OGE_GetSceneTimerInterval();
bool OGE_IsSceneTimerWaiting();

int  OGE_GetScenePlot();
void OGE_SetScenePlot(int iSprId);

void OGE_PlayPlot(int iSprId, int iLoopTimes);
void OGE_OpenPlot(int iSprId);
void OGE_ClosePlot(int iSprId);
void OGE_PausePlot(int iSprId);
void OGE_ResumePlot(int iSprId);
bool OGE_IsPlayingPlot(int iSprId);
int OGE_GetPlotTriggerFlag(int iSprId, int iEventCode);
void OGE_SetPlotTriggerFlag(int iSprId, int iEventCode, int iFlag);

void OGE_PauseGame();
void OGE_ResumeGame();
bool OGE_IsPlayingGame();
int OGE_QuitGame();

int  OGE_GetSprGameData(int iSprId);
//void OGE_SetSprGameData(int iSprId, const std::string& sGameDataName);
int  OGE_GetSprCustomData(int iSprId);
void OGE_SetSprCustomData(int iSprId, int iGameDataId, bool bOwnIt);

int  OGE_GetAppCustomData();
void OGE_SetAppCustomData(int iGameDataId);

int  OGE_GetSceneCustomData(int iSceneId);
void OGE_SetSceneCustomData(int iSceneId, int iGameDataId, bool bOwnIt);

int  OGE_FindGameData(const std::string& sGameDataName);
int  OGE_GetGameData(const std::string& sGameDataName, const std::string& sTemplateName);
const std::string& OGE_GetGameDataName(int iGameDataId);
int  OGE_GetCustomGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount);
int  OGE_GetAppGameData();

void OGE_CopyGameData(int iFromGameDataId, int iToGameDataId);

//int  OGE_UseMusic(std::string& sMusicName);
//int  OGE_PlayMusic();
//int  OGE_StopMusic();
//int  OGE_StopAudio();
//int  OGE_PlaySound(std::string& sSoundName);
//int  OGE_PlaySomeMusic(std::string& sMusicName);

int  OGE_PlayBgMusic(int iLoopTimes);
int  OGE_PlayMusic(int iLoopTimes);
int  OGE_StopMusic();
int  OGE_PauseMusic();
int  OGE_ResumeMusic();
int  OGE_PlayMusicByName(const std::string& sMusicName, int iLoopTimes);
int  OGE_FadeInMusic(const std::string& sMusicName, int iLoopTimes, int iFadingTime);
int  OGE_FadeOutMusic(int iFadingTime);
int  OGE_GetSound(const std::string& sSoundName);
int  OGE_PlaySound(int iSoundCode);
int  OGE_PlaySoundById(int iSoundId, int iLoopTimes);
int  OGE_PlaySoundByCode(int iSoundCode, int iLoopTimes);
int  OGE_PlaySoundByName(const std::string& sSoundName, int iLoopTimes);
int  OGE_StopSoundById(int iSoundId);
int  OGE_StopSoundByCode(int iSoundCode);
int  OGE_StopSoundByName(const std::string& sSoundName);

int  OGE_StopAudio();

int  OGE_PlayMovie(const std::string& sMovieName);
int  OGE_StopMovie();
bool OGE_IsPlayingMovie();

int OGE_GetSoundVolume();
int OGE_GetMusicVolume();
void OGE_SetSoundVolume(int iValue);
void OGE_SetMusicVolume(int iValue);

bool OGE_GetFullScreen();
void OGE_SetFullScreen(bool bValue);

int  OGE_GetScene();
int  OGE_GetActiveScene();
int  OGE_GetLastActiveScene();
int  OGE_GetNextActiveScene();
int  OGE_GetSceneByName(const std::string& sSceneName);
int  OGE_FindScene(const std::string& sSceneName);
const std::string& OGE_GetSceneName(int iSceneId);
const std::string& OGE_GetSceneChapter(int iSceneId);
void OGE_SetSceneChapter(int iSceneId, const std::string& sChapterName);
const std::string& OGE_GetSceneTag(int iSceneId);
void OGE_SetSceneTag(int iSceneId, const std::string& sTagName);
int  OGE_GetSceneGameData(int iSceneId);

int OGE_SetSceneBgMusic(int iSceneId, const std::string& sMusicName);
const std::string& OGE_GetSceneBgMusic(int iSceneId);
int OGE_GetSceneTime();

int  OGE_CallScene(const std::string& sSceneName);

//void OGE_FadeIn (int iFadeType);
//void OGE_FadeOut(int iFadeType);

//int OGE_GetSceneFadeInSpeed(int iSceneId);
//int OGE_GetSceneFadeOutSpeed(int iSceneId);
//int OGE_SetSceneFadeInSpeed(int iSceneId, int iSpeed);
//int OGE_SetSceneFadeOutSpeed(int iSceneId, int iSpeed);
//int OGE_GetSceneFadeInType(int iSceneId);
//int OGE_GetSceneFadeOutType(int iSceneId);
//int OGE_SetSceneFadeInType(int iSceneId, int iType);
//int OGE_SetSceneFadeOutType(int iSceneId, int iType);

int OGE_SetSceneFadeIn(int iSceneId, int iType, int iSpeed);
int OGE_SetSceneFadeOut(int iSceneId, int iType, int iSpeed);

int OGE_SetSceneFadeMask(int iSceneId, int iImageId);

int  OGE_GetGroup(const std::string& sGroupName);
int  OGE_GetGroupByCode(int iCode);
const std::string& OGE_GetGroupName(int iSprGroupId);
//void OGE_BindSprGroupToScene(int iSprGroupId, int iSceneId);
void OGE_OpenGroup(int iSprGroupId);
void OGE_CloseGroup(int iSprGroupId);
int  OGE_GroupGenSpr(int iSprGroupId, const std::string& sTempletSprName, int iCount);
int  OGE_GetRootSprFromGroup(int iSprGroupId);
int  OGE_GetFreeSprFromGroup(int iSprGroupId);
//int  OGE_GetFreePlotFromGroup(int iSprGroupId);
//int  OGE_GetFreeTimerFromGroup(int iSprGroupId);
int  OGE_GetFreeSprWithType(int iSprGroupId, int iSprType);
int  OGE_GetFreeSprWithClassTag(int iSprGroupId, int iSprClassTag);
int  OGE_GetFreeSprWithTag(int iSprGroupId, int iSprTag);

int  OGE_GetGroupSprCount(int iSprGroupId);
int  OGE_GetGroupSprByIndex(int iGroupId, int iIndex);

int  OGE_GetGroupSpr(int iGroupId, int idx);

// get spr in current scene, iState: 0=all, 1=active, -1=inactive
int  OGE_GetSceneSprCount(int iSceneId, int iState);
int  OGE_GetSceneSprByIndex(int iSceneId, int iIndex, int iState);

int  OGE_GetGroupCount();
int  OGE_GetGroupByIndex(int iIndex);

int  OGE_GetSceneCount();
int  OGE_GetSceneByIndex(int iIndex);

int OGE_GetSprForScene(int iSceneId, const std::string& sSprName, const std::string& sClassName);
int OGE_GetSprForGroup(int iGroupId, const std::string& sSprName, const std::string& sClassName);

int OGE_RemoveSprFromScene(int iSceneId, int iSprId);
int OGE_RemoveSprFromGroup(int iGroupId, int iSprId);

void OGE_RemoveScene(const std::string& sName);
void OGE_RemoveGroup(const std::string& sName);
void OGE_RemoveGameData(const std::string& sName);
void OGE_RemoveGameMap(const std::string& sName);
void OGE_RemoveImage(const std::string& sName);
void OGE_RemoveMusic(const std::string& sName);
void OGE_RemoveMovie(const std::string& sName);
void OGE_RemoveFont(const std::string& sName, int iFontSize);

int OGE_GetStaticBuf(int iIndex);
int OGE_ClearBuf(int iBufferId);
int OGE_CopyBuf(int iFromId, int iToId, int iStart, int iLen);

//int OGE_OpenStreamBuf(int iBufferId);
//int OGE_CloseStreamBuf(int iStreamBufId);
//int OGE_GetBufReadPos(int iStreamBufId);
//int OGE_SetBufReadPos(int iStreamBufId, int iPos);
//int OGE_GetBufWritePos(int iStreamBufId);
//int OGE_SetBufWritePos(int iStreamBufId, int iPos);

//char OGE_GetBufByte(int iStreamBufId);
//int OGE_GetBufInt(int iStreamBufId);
//double OGE_GetBufFloat(int iStreamBufId);
//ogeScriptString OGE_GetBufStr(int iStreamBufId);

//int OGE_PutBufByte(int iStreamBufId, char value);
//int OGE_PutBufInt(int iStreamBufId, int value);
//int OGE_PutBufFloat(int iStreamBufId, double value);
//int OGE_PutBufStr(int iStreamBufId, const std::string& value);

char OGE_GetBufByte(int iBufId, int pos, int& next);
int OGE_GetBufInt(int iBufId, int pos, int& next);
double OGE_GetBufFloat(int iBufId, int pos, int& next);
std::string OGE_GetBufStr(int iBufId, int pos, int len, int& next);

int OGE_PutBufByte(int iBufId, int pos, char value);
int OGE_PutBufInt(int iBufId, int pos, int value);
int OGE_PutBufFloat(int iBufId, int pos, double value);
int OGE_PutBufStr(int iBufId, int pos, const std::string& value);

int OGE_OpenLocalServer(int iPort);
int OGE_CloseLocalServer();
int OGE_ConnectServer(const std::string& sAddr);
int OGE_DisconnectServer(const std::string& sAddr);
int OGE_DisconnectClient(const std::string& sAddr);

const std::string& OGE_GetSocketAddr(int iSockId);

//int OGE_RefreshClientList();
//int OGE_RefreshServerList();
//int OGE_GetClientByIndex(int idx);
//int OGE_GetServerByIndex(int idx);

int OGE_GetCurrentSocket();

int OGE_GetUserSocket(int idx);
void OGE_SetUserSocket(int iSockId, int idx);

int OGE_GetRecvDataSize();
int OGE_GetRecvDataType();

char OGE_GetRecvByte(int pos, int& next);
int OGE_GetRecvInt(int pos, int& next);
double OGE_GetRecvFloat(int pos, int& next);
int OGE_GetRecvBuf(int pos, int len, int buf, int& next);
std::string OGE_GetRecvStr(int pos, int len, int& next);

//ogeScriptString OGE_GetRecvMsg();
//int OGE_SendMsg(const std::string& sMsg);

int OGE_PutSendingByte(int iSockId, int pos, char value);
int OGE_PutSendingInt(int iSockId, int pos, int value);
int OGE_PutSendingFloat(int iSockId, int pos, double value);
int OGE_PutSendingBuf(int iSockId, int pos, int buf, int len);
int OGE_PutSendingStr(int iSockId, int pos, const std::string& value);

int OGE_SendData(int iSockId, int iDataSize);

const std::string& OGE_GetLocalIP();
int OGE_FindClient(const std::string& sAddr);
int OGE_FindServer(const std::string& sAddr);
int OGE_GetLocalServer();

//ogeScriptByteArray OGE_GetRecvData();
//int OGE_SendData(ogeScriptByteArray data);
//ogeScriptByteArray OGE_TestGetByteArray();
//int OGE_TestSetByteArray(ogeScriptByteArray arr);
//int OGE_GetIntData(ogeScriptByteArray data, int idx);
//ogeScriptString OGE_GetStrData(ogeScriptByteArray data, int idx, int len);
//int OGE_SetIntData(ogeScriptByteArray data, int idx, int value);
//int OGE_SetStrData(ogeScriptByteArray data, int idx, std::string& value);

int OGE_FileSize(const std::string& sFileName);
bool OGE_FileExists(const std::string& sFileName);
bool OGE_CreateEmptyFile(const std::string& sFileName);
bool OGE_RemoveFile(const std::string& sFileName);
int OGE_OpenFile(const std::string& sFileName, bool bTryCreate);
int OGE_CloseFile(int iFileId);
int OGE_SaveFile(int iFileId);
bool OGE_GetFileEof(int iFileId);
int OGE_GetFileReadPos(int iFileId);
int OGE_SetFileReadPos(int iFileId, int iPos);
int OGE_SetFileReadPosFromEnd(int iFileId, int iPos);
int OGE_GetFileWritePos(int iFileId);
int OGE_SetFileWritePos(int iFileId, int iPos);
int OGE_SetFileWritePosFromEnd(int iFileId, int iPos);

char OGE_ReadFileByte(int iFileId);
int OGE_ReadFileInt(int iFileId);
double OGE_ReadFileFloat(int iFileId);
int OGE_ReadFileBuf(int iFileId, int iLen, int iBufId);
std::string OGE_ReadFileStr(int iFileId, int iLen);

int OGE_WriteFileByte(int iFileId, char value);
int OGE_WriteFileInt(int iFileId, int value);
int OGE_WriteFileFloat(int iFileId, double value);
int OGE_WriteFileBuf(int iFileId, int buf, int len);
int OGE_WriteFileStr(int iFileId, const std::string& value);

// custom config file (game.ini file) ...

int OGE_OpenCustomConfig(const std::string& sConfigFile);
int OGE_CloseCustomConfig(int iConfigId);

int OGE_GetDefaultConfig();
int OGE_GetAppConfig();

std::string OGE_ReadConfigStr(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
//ogeScriptString OGE_ReadConfigPath(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
int OGE_ReadConfigInt(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, int iDefault);
double OGE_ReadConfigFloat(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, double fDefault);

bool OGE_WriteConfigStr(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, const std::string& sValue);
bool OGE_WriteConfigInt(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, int iValue);
bool OGE_WriteConfigFloat(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, double fValue);

bool OGE_SaveConfig(int iConfigId);

int OGE_SaveDataToConfig(int iDataId, int iConfigId);
int OGE_LoadDataFromConfig(int iDataId, int iConfigId);

int OGE_SaveDataToDB(int iDataId, int iSessionId);
int OGE_LoadDataFromDB(int iDataId, int iSessionId);

// database ...

bool OGE_IsValidDBType(int iDbType);

int OGE_OpenDefaultDB();

int OGE_OpenDB(int iDbType, const std::string& sCnnStr);
void OGE_CloseDB(int iSessionId);
int OGE_RunSql(int iSessionId, const std::string& sSql);
int OGE_OpenQuery(int iSessionId, const std::string& sQuerySql);
void OGE_CloseQuery(int iQueryId);
int OGE_FirstRecord(int iQueryId);
int OGE_NextRecord(int iQueryId);
bool OGE_GetBoolFieldValue(int iQueryId, const std::string& sFieldName);
int OGE_GetIntFieldValue(int iQueryId, const std::string& sFieldName);
double OGE_GetFloatFieldValue(int iQueryId, const std::string& sFieldName);
std::string OGE_GetStrFieldValue(int iQueryId, const std::string& sFieldName);
std::string OGE_GetTimeFieldValue(int iQueryId, const std::string& sFieldName);



#endif // __OGE_H_INCLUDED__
