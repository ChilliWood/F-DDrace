/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include <engine/config.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include "dragger.h"
#include "character.h"

CDragger::CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW,
		int CaughtTeam, int Layer, int Number) :
		CEntity(pGameWorld, CGameWorld::ENTTYPE_DRAGGER, Pos)
{
	m_Layer = Layer;
	m_Number = Number;
	m_Pos = Pos;
	m_Strength = Strength;
	m_EvalTick = Server()->Tick();
	m_NW = NW;
	m_CaughtTeam = CaughtTeam;
	GameWorld()->InsertEntity(this);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		m_SoloIDs[i] = -1;
	}
}

void CDragger::Move()
{
	if (m_Target && (!m_Target->IsAlive() || (m_Target->IsAlive()
			&& (m_Target->m_Super || m_Target->IsPaused()
					|| (m_Layer == LAYER_SWITCH && m_Number
							&& !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[m_Target->Team()])))))
		m_Target = 0;

	mem_zero(m_SoloEnts, sizeof(m_SoloEnts));
	CCharacter *TempEnts[MAX_CLIENTS];

	int Num = GameWorld()->FindEntities(m_Pos, Config()->m_SvDraggerRange,
			(CEntity**) m_SoloEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
	mem_copy(TempEnts, m_SoloEnts, sizeof(TempEnts));

	int Id = -1;
	int MinLen = 0;
	CCharacter *Temp;
	for (int i = 0; i < Num; i++)
	{
		Temp = m_SoloEnts[i];
		if (Temp->Team() != m_CaughtTeam)
		{
			m_SoloEnts[i] = 0;
			continue;
		}
		if (m_Layer == LAYER_SWITCH && m_Number
				&& !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Temp->Team()])
		{
			m_SoloEnts[i] = 0;
			continue;
		}
		int Res =
				m_NW ? GameServer()->Collision()->IntersectNoLaserNW(m_Pos,
								Temp->GetPos(), 0, 0) :
						GameServer()->Collision()->IntersectNoLaser(m_Pos,
								Temp->GetPos(), 0, 0);

		if (Res == 0)
		{
			int Len = length(Temp->GetPos() - m_Pos);
			if (MinLen == 0 || MinLen > Len)
			{
				MinLen = Len;
				Id = i;
			}

			if (!Temp->Teams()->m_Core.GetSolo(Temp->GetPlayer()->GetCID()))
				m_SoloEnts[i] = 0;
		}
		else
		{
			m_SoloEnts[i] = 0;
		}
	}

	if (!m_Target)
		m_Target = Id != -1 ? TempEnts[Id] : 0;

	if (m_Target)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_SoloEnts[i] == m_Target)
				m_SoloEnts[i] = 0;
		}
	}
}

void CDragger::Drag()
{
	if (m_Target)
	{
		CCharacter *Target = m_Target;

		for (int i = -1; i < MAX_CLIENTS; i++)
		{
			if (i >= 0)
				Target = m_SoloEnts[i];

			if (!Target)
				continue;

			int Res = 0;
			if (!m_NW)
				Res = GameServer()->Collision()->IntersectNoLaser(m_Pos,
						Target->GetPos(), 0, 0);
			else
				Res = GameServer()->Collision()->IntersectNoLaserNW(m_Pos,
						Target->GetPos(), 0, 0);
			if (Res || length(m_Pos - Target->GetPos()) > Config()->m_SvDraggerRange)
			{
				Target = 0;
				if (i == -1)
					m_Target = 0;
				else
					m_SoloEnts[i] = 0;
			}
			else if (length(m_Pos - Target->GetPos()) > 28)
			{
				vec2 Temp = Target->Core()->m_Vel + (normalize(GetPos() - Target->GetPos()) * m_Strength);
				Target->Core()->m_Vel = ClampVel(Target->m_MoveRestrictions, Temp);

				// F-DDrace
				Target->SetLastTouchedSwitcher(m_Number);
			}
		}
	}
}

void CDragger::Reset()
{

}

void CDragger::Tick()
{
	if (((CGameControllerDDRace*) GameServer()->m_pController)->m_Teams.GetTeamState(
			m_CaughtTeam) == CGameTeams::TEAMSTATE_EMPTY)
		return;
	if (Server()->Tick() % int(Server()->TickSpeed() * 0.15f) == 0)
	{
		int Flags;
		m_EvalTick = Server()->Tick();
		int index = GameServer()->Collision()->IsMover(m_Pos.x, m_Pos.y,
				&Flags);
		if (index)
		{
			m_Core = GameServer()->Collision()->CpSpeed(index, Flags);
		}
		m_Pos += m_Core;
		Move();
	}
	Drag();
	return;

}

void CDragger::Snap(int SnappingClient)
{
	if (((CGameControllerDDRace*) GameServer()->m_pController)->m_Teams.GetTeamState(
			m_CaughtTeam) == CGameTeams::TEAMSTATE_EMPTY)
		return;

	CCharacter *Target = m_Target;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_SoloIDs[i] == -1)
			break;

		Server()->SnapFreeID(m_SoloIDs[i]);
		m_SoloIDs[i] = -1;
	}

	int pos = 0;

	for (int i = -1; i < MAX_CLIENTS; i++)
	{
		if (i >= 0)
		{
			Target = m_SoloEnts[i];

			if (!Target)
				continue;
		}

		if (Target)
		{
			if (NetworkClipped(SnappingClient, m_Pos)
					&& NetworkClipped(SnappingClient, Target->GetPos()))
				continue;
		}
		else if (NetworkClipped(SnappingClient, m_Pos))
			continue;

		CCharacter * Char = GameServer()->GetPlayerChar(SnappingClient);

		if(SnappingClient > -1 && (GameServer()->m_apPlayers[SnappingClient]->GetTeam() == -1
					|| GameServer()->m_apPlayers[SnappingClient]->IsPaused())
				&& GameServer()->m_apPlayers[SnappingClient]->GetSpectatorID() != -1)
			Char = GameServer()->GetPlayerChar(GameServer()->m_apPlayers[SnappingClient]->GetSpectatorID());

		int Tick = (Server()->Tick() % Server()->TickSpeed()) % 11;
		if (Char && Char->IsAlive()
				&& (m_Layer == LAYER_SWITCH && m_Number
						&& !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Char->Team()]
						&& (!Tick)))
			continue;
		if (Char && Char->IsAlive())
		{
			if (Char->Team() != m_CaughtTeam)
				continue;
		}
		else
		{
			// send to spectators only active draggers and some inactive from team 0
			if (!((Target && Target->IsAlive()) || m_CaughtTeam == 0))
				continue;
		}

		if (Char && Char->IsAlive() && Target && Target->IsAlive() && Target->GetPlayer()->GetCID() != Char->GetPlayer()->GetCID() && !Char->GetPlayer()->m_ShowOthers &&
			(Char->Teams()->m_Core.GetSolo(SnappingClient) || Char->Teams()->m_Core.GetSolo(Target->GetPlayer()->GetCID())))
		{
			continue;
		}

		CNetObj_Laser *obj;

		if (i == -1)
		{
			obj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(
					NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
		}
		else
		{
			m_SoloIDs[pos] = Server()->SnapNewID();
			obj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem( // TODO: Have to free IDs again?
					NETOBJTYPE_LASER, m_SoloIDs[pos], sizeof(CNetObj_Laser)));
			pos++;
		}

		if (!obj)
			continue;
		obj->m_X = (int)m_Pos.x;
		obj->m_Y = (int)m_Pos.y;
		if (Target)
		{
			obj->m_FromX = (int)Target->GetPos().x;
			obj->m_FromY = (int)Target->GetPos().y;
		}
		else
		{
			obj->m_FromX = (int)m_Pos.x;
			obj->m_FromY = (int)m_Pos.y;
		}

		int StartTick = m_EvalTick;
		if (StartTick < Server()->Tick() - 4)
			StartTick = Server()->Tick() - 4;
		else if (StartTick > Server()->Tick())
			StartTick = Server()->Tick();
		obj->m_StartTick = StartTick;
	}
}

CDraggerTeam::CDraggerTeam(CGameWorld *pGameWorld, vec2 Pos, float Strength,
		bool NW, int Layer, int Number)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		m_Draggers[i] = new CDragger(pGameWorld, Pos, Strength, NW, i, Layer, Number);
	}
}

//CDraggerTeam::~CDraggerTeam()
//{
//	for (int i = 0; i < MAX_CLIENTS; ++i)
//	{
//		delete m_Draggers[i];
//	}
//}
