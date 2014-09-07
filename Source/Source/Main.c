#include <shinobi.h>
#include <kamui2.h>
#include <sn_fcntl.h>
#include <usrsnasm.h>
#include <sg_syCbl.h>
#include <sg_Chain.h>

extern Uint8 *_BSG_END;
#define P1AREA 0x80000000
#define WORK_END ( ( ( Uint32 ) _BSG_END ) & 0xE0000000 | 0x0D000000 )
#define HEAP_AREA \
	( ( void * )( ( ( ( Uint32 ) _BSG_END | P1AREA ) & 0xFFFFFFE0 ) + 0x20 ) )
#define HEAP_SIZE ( WORK_END - ( Uint32 )HEAP_AREA )

#define TEXTURE_MEMORY_SIZE ( 0x400000 )
#define MAX_TEXTURES ( 4096 )
#define MAX_SMALLVQ ( 0 )
#define VERTEX_BUFFER_SIZE ( 0x40000 )
KMSTRIPHEAD	g_StripHead01;
KMSTRIPCONTEXT g_DefaultContext =
{
	0x90,
	{ KM_OPAQUE_POLYGON, KM_USERCLIP_DISABLE, KM_NORMAL_POLYGON,
		KM_FLOATINGCOLOR, KM_FALSE, KM_TRUE },
	{ KM_GREATER, KM_CULLCCW, KM_FALSE, KM_FALSE, 0 },
	{ 0, 0 },
	{ KM_ONE, KM_ZERO, KM_FALSE, KM_FALSE, KM_NOFOG, KM_FALSE, KM_FALSE,
		KM_FALSE, KM_NOFLIP, KM_NOCLAMP, KM_BILINEAR, KM_FALSE,
		KM_MIPMAP_D_ADJUST_1_00, KM_MODULATE, 0, 0 }
};

#define MAPLE_BUFFER_SIZE ( 1024 * 24 * 2 + 32 )
Uint8 g_MapleRecvBuffer[ MAPLE_BUFFER_SIZE ];
Uint8 g_MapleSendBuffer[ MAPLE_BUFFER_SIZE ];

void *( *MallocPtr )( unsigned long ) = syMalloc;
void ( *FreePtr )( void * ) = syFree;

#define Align32Malloc ( *MallocPtr )
#define Align32Free ( *FreePtr )

#define AlignByte32( Address ) \
	( ( ( (  long ) Address ) + 0x1F ) & 0xFFFFFFE0 )
#define SH4_P2NonCachedMemory( Address ) \
	( ( ( ( long ) Address ) & 0x0FFFFFFF ) | 0xA0000000 )

#pragma aligndata32( g_pTextureWorkArea )
KMDWORD g_pTextureWorkArea[ MAX_TEXTURES * 24 / 4 + MAX_SMALLVQ * 76 / 4 ];

void *_Align32Malloc( unsigned long p_Size );
void _Align32Free( void *p_pAddress );
void Initialise32Malloc( void );

void DebugOut( const char *p_pString );

#define VSYNC_PRIORITY ( 0x90 )
KMVOID VSyncCallback( PKMVOID p_CallbackArgs );
KMVOID EORCallback( PKMVOID p_pCallbackArgs );

volatile KMDWORD g_VSync = KM_TRUE;

int main( void )
{
	int DisplayMode = 0;
	int Running = 1;
	const PDS_PERIPHERAL	*pControllerA;
	const PDS_PERIPHERALINFO	*pControllerAInfo;
	KMSYSTEMCONFIGSTRUCT	KamuiSystemConfig;
	PKMSURFACEDESC			FramebufferSurfaces[ 2 ];
	KMSURFACEDESC			FrontBuffer;
	KMSURFACEDESC			BackBuffer;
	KMSURFACEDESC			TextureSurface;
	KMVERTEXBUFFDESC		VertexBufferDesc;
	PKMDWORD				pVertexBuffer;
	SYCHAIN					UserVSyncChainID;

	/* Initialise hardware */
	set_imask( 15 );
	syHwInit( );
	syMallocInit( HEAP_AREA, HEAP_SIZE );
	syStartGlobalConstructor( );
	kmInitDevice( KM_DREAMCAST );

	switch( syCblCheck( ) )
	{
	case SYE_CBL_NTSC:
		DisplayMode = KM_DSPMODE_NTSCNI640x480;
		break;
	case SYE_CBL_PAL:
		DisplayMode = KM_DSPMODE_PALNI640x480EXT;
		break;
	case SYE_CBL_VGA:
		DisplayMode = KM_DSPMODE_VGA;
		break;
	default:
		syBtExit( );
	}
	kmSetDisplayMode( DisplayMode, KM_DSPBPP_RGB565, TRUE, FALSE );
	kmSetWaitVsyncCount( 1 );
	syHwInit2( );
	pdInitPeripheral( PDD_PLOGIC_ACTIVE, g_MapleRecvBuffer,
		g_MapleSendBuffer );
	syRtcInit( );
	set_imask( 0 );

	/* Set up MallocPtr and FreePtr to allocate memory aligned to a 32-byte
	 * boundary */
	Initialise32Malloc( );

	/* Testing output */
	DebugOut( "Anniversary" );

	/* Control what happens when V-Sync is triggered */
	UserVSyncChainID = syChainAddHandler( SYD_CHAIN_EVENT_IML6_VBLANK,
		VSyncCallback, VSYNC_PRIORITY, NULL );

	kmSetEORCallback( EORCallback, NULL );

	/* Set up Kamui 2 */
	KamuiSystemConfig.dwSize = sizeof( KamuiSystemConfig );

	/* Rendering flags */
	KamuiSystemConfig.flags = KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER |
		KM_CONFIGFLAG_NOWAITVSYNC | KM_CONFIGFLAG_ENABLE_2V_LATENCY;

	/* Double-buffer setup */
	FramebufferSurfaces[ 0 ] = &FrontBuffer;
	FramebufferSurfaces[ 1 ] = &BackBuffer;

	KamuiSystemConfig.ppSurfaceDescArray = FramebufferSurfaces;
	KamuiSystemConfig.fb.nNumOfFrameBuffer = 2;
	KamuiSystemConfig.fb.nStripBufferHeight = 32;

	/* Texture memory configuration */
	KamuiSystemConfig.nTextureMemorySize = TEXTURE_MEMORY_SIZE;
	KamuiSystemConfig.nNumOfTextureStruct = MAX_TEXTURES;
	KamuiSystemConfig.nNumOfSmallVQStruct = MAX_SMALLVQ;
	KamuiSystemConfig.pTextureWork = g_pTextureWorkArea;

	/* Vertex buffer allocation */
	pVertexBuffer = ( PKMDWORD )Align32Malloc( VERTEX_BUFFER_SIZE );
	KamuiSystemConfig.pVertexBuffer =
		( PKMDWORD )SH4_P2NonCachedMemory( pVertexBuffer );
	
	KamuiSystemConfig.pBufferDesc = &VertexBufferDesc;
	KamuiSystemConfig.nVertexBufferSize = VERTEX_BUFFER_SIZE;
	/* 2V latency only requires one vertex bank, 3V needs it double-buffered */
	KamuiSystemConfig.nNumOfVertexBank = 1;

	/* Only one rendering pass is used */
	KamuiSystemConfig.nPassDepth = 1;
	/* Autosort translucent polygons for rendering */
	KamuiSystemConfig.Pass[ 0 ].dwRegionArrayFlag = KM_PASSINFO_AUTOSORT;

	/* 2V allows one buffer to be directly transferred, 0 (Opaque polygons)
	 * are the most common, so send them directly */
	KamuiSystemConfig.Pass[ 0 ].nDirectTransferList = KM_OPAQUE_POLYGON;

	/* As the opaque polygons are already being sent directly, set their usage
	 * to zero
	 * All of these percentages must add up to 100.0f */
	KamuiSystemConfig.Pass[ 0 ].fBufferSize[ 0 ] = 0.0f; /* Opaque polygon*/
	KamuiSystemConfig.Pass[ 0 ].fBufferSize[ 1 ] = 0.0f; /* Opaque modifier */
	KamuiSystemConfig.Pass[ 0 ].fBufferSize[ 2 ] = 50.0f; /* Translucent */
	KamuiSystemConfig.Pass[ 0 ].fBufferSize[ 3 ] = 0.0f; /* Translucent
															modifier */
	KamuiSystemConfig.Pass[ 0 ].fBufferSize[ 4 ] = 50.0f; /* Punchthrough */

	/* Finally, set the Kamui 2 configuration */
	kmSetSystemConfiguration( &KamuiSystemConfig );

	/* Create a strip head for type 01 */
	memset( &g_StripHead01, 0, sizeof( g_StripHead01 ) );
	kmGenerateStripHead01( &g_StripHead01, &g_DefaultContext );

	/* Make an initial call to the EOR function to set the background plane */
	EORCallback( NULL );

	while( Running )
	{
		/* Get the latest data from controller port A and quit if the Start
		 * button is pressed */
		pControllerA = pdGetPeripheral( PDD_PORT_A0 );

		if( pControllerA->press & PDD_DGT_ST )
		{
			Running = 0;
		}

		/* Pump the renderer */
		kmBeginScene( &KamuiSystemConfig );
		kmBeginPass( &VertexBufferDesc );
		kmEndPass( &VertexBufferDesc );
		kmRender( KM_RENDER_FLIP );
		kmEndScene( &KamuiSystemConfig );

		/* Update input */
		pdExecPeripheralServer( );
	}

	/* Uninstall the V-Sync function */
	syChainDeleteHandler( UserVSyncChainID );

	/* Clean up the hardware */
	syRtcFinish( );
	pdExitPeripheral( );
	kmUnloadDevice( );
	syStartGlobalDestructor( );
	syMallocFinish( );
	syHwFinish( );
	set_imask( 15 );

	/* Reboot into the firmware */
	syBtExit( );
	
	/* This will never be reached */
	return 0;
}

/* Helper function to display text to the debug log in Codescape
 * Returns: None */
void DebugOut( const char *p_pString )
{
	if( p_pString != NULL )
	{
		debug_write( SNASM_STDOUT, p_pString, strlen( p_pString ) );
	}
}

/* Create a pointer to a 32-byte aligned address
 * Parameters:
 *   p_Size: Amount of memory to allocate
 * Returns: Pointer to a 32-byte aligned address */
void *_Align32Malloc( unsigned long p_Size )
{
	void *pAddress, *pAlignedAddress;

	/* Adjust the size to a multiple of 32 bytes */
	p_Size = ( p_Size + 0x1F ) & 0xFFFFFFE0;

	/* Add 32-bytes of padding to store a byte determining the real address to
	 * free */
	pAddress = syMalloc( p_Size + 32 );

	/* Align to 32-bytes, adding 32 bytes if it's already aligned */
	pAlignedAddress =
		( void * ) ( ( ( ( long ) pAddress ) + 0x20 ) & 0xFFFFFFE0 );
	
	/* Store the amount of bytes that have been added just one byte before the
	 * aligned address (used when freeing later) */
	*( ( char * ) pAlignedAddress - 1 ) =
		( char )( ( long ) pAlignedAddress - ( long ) pAddress );
	
	return ( pAlignedAddress );
}

/* Frees 32-byte aligned memory
 * Parameters:
 *   p_pAddress: Pointer to a 32-byte aligned address
 * Returns: None */
void _Align32Free( void *p_pAddress )
{
	char Difference;

	/* Read the byte stored before the 32-byte aligned address, which is the
	 * original, unaligned address */
	Difference = *( ( char * ) p_pAddress - 1 );
	/* Convert the 32-byte aligned address to the actual address originally
	 * used before aligning to a 32-bytes boundary */
	p_pAddress = ( void * )( ( long ) p_pAddress - Difference );

	syFree( p_pAddress );
}

/* Sets up 32-byte malloc and free functions
 * Parameters: None
 * Returns: None */
void Initialise32Malloc( void )
{
	char *pAddress1, *pAddress2;

	MallocPtr = syMalloc;
	FreePtr = syFree;

	/* Alocate two contiguous bytes, which should be aligned to a 32-byte
	 * boundary if a 32-byte memory allocation scheme is used */
	pAddress1 = syMalloc( 1 );
	pAddress2 = syMalloc( 1 );

	/* Test the addresses to see if they are 32-byte aligned */
	if( ( ( long ) pAddress1 & 0x1F ) || ( ( long ) pAddress2 & 0x1F ) )
	{
		/* Set up the 32-byte aligned versions of malloc and free */
		MallocPtr = _Align32Malloc;
		FreePtr = _Align32Free;
	}

	syFree( pAddress1 );
	syFree( pAddress2 );
}

/* Vertical Sync callback function
 * Parameters:
 *   p_CallbackArgs: Pointer to arguments for the V-Sync
 * Returns: None */
KMVOID VSyncCallback( PKMVOID p_CallbackArgs )
{
	g_VSync = KM_TRUE;
}

/* End of render callback
 * Parameters:
 *   p_CallbackArgs: Pointer to arguments for the end-of-render call
 * Returns: None */
KMVOID EORCallback( PKMVOID p_pCallbackArgs )
{
	static float Red = ( 96.0f / 255.0f );
	KMVERTEX_01 Background[ 3 ];

	/* When V-Sync has been encountered, clear the screen */
	if( g_VSync == KM_TRUE )
	{
		Background[ 0 ].ParamControlWord	= KM_VERTEXPARAM_NORMAL;
		Background[ 0 ].fX					= 0.0f;
		Background[ 0 ].fY					= 479.0f;
		Background[ 0 ].u.fZ				= 0.2f;
		Background[ 0 ].fBaseAlpha			= 1.0f;
		Background[ 0 ].fBaseRed			= Red;
		Background[ 0 ].fBaseGreen			= 0.0f;
		Background[ 0 ].fBaseBlue			= 0.0f;

		Background[ 1 ].ParamControlWord	= KM_VERTEXPARAM_NORMAL;
		Background[ 1 ].fX					= 0.0f;
		Background[ 1 ].fY					= 0.0f;
		Background[ 1 ].u.fZ				= 0.2f;
		Background[ 1 ].fBaseAlpha			= 1.0f;
		Background[ 1 ].fBaseRed			= Red;
		Background[ 1 ].fBaseGreen			= 0.0f;
		Background[ 1 ].fBaseBlue			= 0.0f;

		Background[ 2 ].ParamControlWord	= KM_VERTEXPARAM_ENDOFSTRIP;
		Background[ 2 ].fX					= 639.0f;
		Background[ 2 ].fY					= 479.0f;
		Background[ 2 ].u.fZ				= 0.2f;
		Background[ 2 ].fBaseAlpha			= 1.0f;
		Background[ 2 ].fBaseRed			= Red;
		Background[ 2 ].fBaseGreen			= 0.0f;
		Background[ 2 ].fBaseBlue			= 0.0f;

		kmSetBackGround( &g_StripHead01, KM_VERTEXTYPE_01,
			&Background[ 0 ], &Background[ 1 ], &Background[ 2 ] );

		g_VSync = KM_FALSE;
	}
}

