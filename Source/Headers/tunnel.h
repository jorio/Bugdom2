//
// tunnel.h
//

#pragma once

// Plumbing level has 126 tunnel sections
// Gutter level has 137 tunnel sections
#define	MAX_TUNNEL_SECTIONS		140


typedef struct
{
	OGLPoint3D	point;
	OGLVector3D	up;
}TunnelSplinePointType;

typedef struct
{
	OGLPoint3D	point;
	OGLVector3D	up;
}TunnelSplineNubType;


typedef struct
{
	int32_t			type;

	int32_t			splineIndex;			// index into spline's point list to where this item is attached
	int32_t			sectionNum;				// which geometry section does that point belong to?

	float			scale;
	OGLVector3D		rot;
	OGLVector3D		positionOffset;			// offset relative to the spline point it's attached to

	uint32_t		flags;
	uint32_t		parms[3];
}TunnelItemDefType;


//==============================================================

Boolean IsTunnelLevel(void);

void InitArea_Tunnels(void);
void PlayArea_Tunnel(void);
void DrawTunnel(void);
void DisposeTunnelData(void);
void CreatePlayerModel_Tunnel(int	tunnelIndex);
void InitTunnelItems(void);
void CalcTunnelCoordFromIndex(float index, OGLPoint3D *coord);

void KillPlayerInTunnel(ObjNode *skip);

