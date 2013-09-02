//
// tunnel.h
//

#define	MAX_TUNNEL_SECTIONS		200


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
	long			type;

	long			splineIndex;			// index into spline's point list to where this item is attached
	long			sectionNum;				// which geometry section does that point belong to?

	float			scale;
	OGLVector3D		rot;
	OGLVector3D		positionOffset;			// offset relative to the spline point it's attached to

	u_long			flags;
	u_long			parms[3];

}TunnelItemDefType;


//==============================================================


void InitArea_Tunnels(void);
void PlayArea_Tunnel(void);
void DrawTunnel(OGLSetupOutputType *setupInfo);
void DisposeTunnelData(void);
void CreatePlayerModel_Tunnel(int	tunnelIndex);
void InitTunnelItems(void);
void CalcTunnelCoordFromIndex(float index, OGLPoint3D *coord);

void KillPlayerInTunnel(ObjNode *skip);

