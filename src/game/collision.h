/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <engine/shared/protocol.h>

#include <list>
#include <vector>

enum
{
	CANTMOVE_LEFT=1<<0,
	CANTMOVE_RIGHT=1<<1,
	CANTMOVE_UP=1<<2,
	CANTMOVE_DOWN=1<<3,
	// F-DDrace
	CANTMOVE_ROOM=1<<4,
	CANTMOVE_DOWN_LASERDOOR=1<<5, // used by prediction
};

vec2 ClampVel(int MoveRestriction, vec2 Vel);

typedef bool (*CALLBACK_SWITCHACTIVE)(int Number, void *pUser);

class CCollision
{
	class CTile* m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers* m_pLayers;

public:
	CCollision();
	~CCollision();
	void Init(class CLayers* pLayers, class CConfig *pConfig);
	bool CheckPoint(float x, float y) { return IsSolid(round_to_int(x), round_to_int(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision);
	int IntersectLineTeleWeapon(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int* pTeleNr);
	int IntersectLineTeleHook(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int* pTeleNr);
	void MovePoint(vec2* pInoutPos, vec2* pInoutVel, float Elasticity, int* pBounces);
	void MoveBox(vec2* pInoutPos, vec2* pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);

	// DDRace

	void Dest();
	void SetCollisionAt(float x, float y, int id);
	void SetDTile(float x, float y, bool State);
	void SetDCollisionAt(float x, float y, int Type, int Flags, int Number);
	int GetDTileIndex(int Index);
	int GetDTileFlags(int Index);
	int GetDTileNumber(int Index);
	int GetFCollisionAt(float x, float y) { return GetFTile(round_to_int(x), round_to_int(y)); }
	int IntersectNoLaser(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision);
	int IntersectNoLaserNW(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision);
	int IntersectAir(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision);
	int GetIndex(int x, int y);
	int GetIndex(vec2 PrevPos, vec2 Pos);
	int GetFIndex(int x, int y);

	struct MoveRestrictionExtra
	{
		bool m_CanEnterRoom;

		MoveRestrictionExtra()
		{
			m_CanEnterRoom = false;
		}
	};

	int GetMoveRestrictions(CALLBACK_SWITCHACTIVE pfnSwitchActive, void *pUser, vec2 Pos, float Distance = 18.0f, int OverrideCenterTileIndex = -1, MoveRestrictionExtra Extra = MoveRestrictionExtra());
	int GetMoveRestrictions(vec2 Pos, float Distance = 18.0f, MoveRestrictionExtra Extra = MoveRestrictionExtra())
	{
		return GetMoveRestrictions(0, 0, Pos, Distance, -1, Extra);
	}

	int GetTile(int x, int y);
	int GetFTile(int x, int y);
	int Entity(int x, int y, int Layer);
	int GetPureMapIndex(float x, float y);
	int GetPureMapIndex(vec2 Pos) { return GetPureMapIndex(Pos.x, Pos.y); }
	std::list<int> GetMapIndices(vec2 PrevPos, vec2 Pos, unsigned MaxIndices = 0);
	int GetMapIndex(vec2 Pos);
	bool TileExists(int Index);
	bool TileExistsNext(int Index);
	vec2 GetPos(int Index);
	int GetTileIndex(int Index);
	int GetFTileIndex(int Index);
	int GetTileFlags(int Index);
	int GetFTileFlags(int Index);
	int IsTeleport(int Index);
	int IsEvilTeleport(int Index);
	int IsCheckTeleport(int Index);
	int IsCheckEvilTeleport(int Index);
	int IsTeleportWeapon(int Index);
	int IsTeleportHook(int Index);
	int IsTCheckpoint(int Index);
	int IsSpeedup(int Index);
	int IsTune(int Index);
	void GetSpeedup(int Index, vec2* Dir, int* Force, int* MaxSpeed);
	int IsSwitch(int Index);
	int GetSwitchNumber(int Index);
	int GetSwitchDelay(int Index);

	int IsSolid(int x, int y);
	bool IsThrough(int x, int y, int xoff, int yoff, vec2 pos0, vec2 pos1);
	bool IsHookBlocker(int x, int y, vec2 pos0, vec2 pos1);
	int IsWallJump(int Index);
	int IsNoLaser(int x, int y);
	int IsFNoLaser(int x, int y);

	int IsCheckpoint(int Index);
	int IsFCheckpoint(int Index);

	int IsMover(int x, int y, int* pFlags);

	vec2 CpSpeed(int index, int Flags = 0);

	class CTeleTile* TeleLayer() { return m_pTele; }
	class CSwitchTile* SwitchLayer() { return m_pSwitch; }
	class CTuneTile* TuneLayer() { return m_pTune; }
	class CLayers* Layers() { return m_pLayers; }
	int m_NumSwitchers;

	// F-DDrace
	class CConfig *m_pConfig;
	int IntersectLinePortalRifleStop(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision);

	// tiles
	bool TileUsed(int Index) { return GetRandomTile(Index) != vec2(-1, -1); }
	vec2 GetRandomTile(int Index);
	std::vector< std::vector<vec2> > m_vTiles;

	int GetTileRaw(int x, int y);
	int GetTileRaw(vec2 Pos) { return GetTileRaw(Pos.x, Pos.y); }
	int GetFTileRaw(int x, int y);
	int GetFTileRaw(vec2 Pos) { return GetFTileRaw(Pos.x, Pos.y); }

	// IntersectLineDoor: returns -1 when intersected with a plot built laserwall, otherwise returns the number of the intersected laser wall
	int IntersectLineDoor(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int Team, bool PlotDoorOnly);
	int GetDoorNumber(vec2 Pos);
	void UnsetDCollisionAt(float x, float y);

	// plots
	bool IsPlotTile(int Index);
	int GetPlotID(int Index);
	int GetSwitchByPlot(int PlotID);
	int GetPlotBySwitch(int SwitchID);
	int m_NumPlots;

private:

	class CTeleTile* m_pTele;
	class CSpeedupTile* m_pSpeedup;
	class CTile* m_pFront;
	class CSwitchTile* m_pSwitch;
	class CTuneTile* m_pTune;
	class CDoorTile* m_pDoor;

	struct SSwitchers
	{
		bool m_Status[MAX_CLIENTS];
		bool m_Initial;
		int m_EndTick[MAX_CLIENTS];
		int m_Type[MAX_CLIENTS];
		// F-DDrace
		int m_ClientID[MAX_CLIENTS];
		int m_StartTick[MAX_CLIENTS];
	};

public:

	// access to plots: PlotID + m_NumSwitchers && PlotID < m_NumPlots + 1
	SSwitchers* m_pSwitchers;
};

void ThroughOffset(vec2 Pos0, vec2 Pos1, int* Ox, int* Oy);
#endif
