#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "memory"
#include "sharedutil.h"
#include "logfile.h"

bool safemem_varbase::m_GInitialized = false;	//If Global mem is initialized
byte *safemem_varbase::m_GData;			//All data is stored here
byte *safemem_varbase::m_GDataBuf;		//Buffer for encrypt/decrypt routines
ulong safemem_varbase::m_GMemSizeKey;	//== m_CurrentKey - <actual global mem size>
ulong safemem_varbase::m_GCurrentKey;	//Current key for this frame
safevarlist safemem_varbase::m_GVars;	//List of all safe vars
bool safevarlist::m_Initialized = false;

#define ENCRYPT_TITLE "Master Sword Memory Encryption"

safemem_varbase::safemem_varbase( )
{ 
	srand( GetTickCount() );
	m_MemSeed = rand();								//Initialize Data random seed
	m_Changed = false;
	m_Registered = false;
	m_GDataBuf = NULL;

	if( !safemem_varbase::m_GInitialized )
	{
		Alloc( sizeof(ulong) );						//Allocate space for the checksum
		safemem_varbase::m_GInitialized = true;
		Update( );										//Change Encrpytion key
		Encrypt( );										//Encrypt mem the first time
		m_GVars.Init( );
		m_GVars.reserve( 1024 );
	}
}

void safemem_varbase::Register( )
{
	startdbg;

	dbg( "Unencrypt" );
	if( m_GData )
	{
		UnEncrypt( );								//Un-encrypt mem
	}

	ulong GSize = GMemSize_Get();					//Get Global mem size
	ulong NewDataSize = m_DataSize;					//Get Data Size
	ulong NewGSize = GSize + NewDataSize;			//Get New Global mem size

	dbg( "AllocAndCopy" );
	if( m_GVars.size() == 79 )
		int stop = 0;
	AllocAndCopy( NewGSize );						//Alloc new Global mem
	memset( &m_GData[GSize], 0, NewDataSize );		//Initialize new data to 0

	MemLoc_Set( GSize );							//Store Data offset
	MemSize_Set( NewDataSize );						//Store Data Size

	m_Registered = true;
	m_GVars.add( this );							//Store the var into the global var list
	m_VarIdx = m_GVars.size() - 1;					//Store the var index locally for fast reference later
	if( m_VarIdx == 0 )
		int stop = 0;

	dbg( "Update" );
	Update( );										//Change validation key

	dbg( "Encrypt" );
	Encrypt( );										//Re-Encrypt mem


	enddbg( "safemem_varbase::Register" );
}
void safemem_varbase::Alloc( ulong NewGSize )
{
	startdbg;

	dbg( "Alloc memory" );
	m_GData = msnew(byte[NewGSize]);				//Alloc new Global mem
	//if( m_GDataBuf ) delete [] m_GDataBuf;			//Delete old buffer
	m_GDataBuf = msnew(byte[NewGSize]);				//Alloc new buffer

	if( !m_GData )
	{
		MessageBox( NULL, "Could not allocate new buffer!", ENCRYPT_TITLE, MB_OK );
		exit( 0 );
		return;
	}

	dbg( "Set mem size" );
	GMemSize_Set( NewGSize );						//Set New Global mem size

	enddbg( "safemem_varbase::Alloc" );
}
void safemem_varbase::AllocAndCopy( ulong NewGSize )
{
	ulong GSize = GMemSize_Get();					//Save Global mem size

	byte *GOldMem = m_GData;						//Save old mem pointer
	Alloc( NewGSize );								//Alloc new Global mem
	if( GSize )
	{
		memcpy( m_GData, GOldMem, GSize );			//Copy old global mem
		delete GOldMem;								//Delete old global memory
	}
}
void safemem_varbase::SwapBufferPointers( )
{
	/*byte *pTemp = m_GData;
	m_GData = m_GDataBuf;
	m_GDataBuf = pTemp;*/
}

void safemem_varbase::Encrypt( )
{
	//return;
	if( !m_GData ) return;
	ulong GSize = GMemSize_Get();					//Get Global mem size
	if( !GSize ) return;							//Global size is zero - should never happen

	*(ulong *)m_GData = m_GCurrentKey;				//Set key within data

	//Start encryption

	for( size_t i = 0; i < GSize; i++ )
	{
		byte addamt = (i * 4) % 128;				//Offset some data
		m_GData[i] = (m_GData[i] + addamt) & 0xFF;	//(The 0xFF is to explicitly convert the result to a byte)
	}

	for( size_t i = 0; i < GSize; i++ )				//Swap some data around
	{
		int swap[2] = { i, (i+7)%GSize };
		byte Temp = m_GData[swap[0]];
		m_GData[swap[0]] = m_GData[swap[1]];
		m_GData[swap[1]] = Temp;
	}

	SwapBufferPointers( );
}

//CopyData = true:  Decrypt to m_GDataBuf
//CopyData = false: Decrypt to m_GData -- Decrypt in-place
void safemem_varbase::UnEncrypt( bool CopyData )
{
	//return;
	startdbg;
	dbg( "Get Size" );
	ulong GSize = GMemSize_Get();				//Get Global mem size
	if( !GSize ) return;

	//Decrypt new data
	byte *pData = m_GData;

	dbg( "Copy" );
	if( CopyData )
	{
		pData = m_GDataBuf;
		memcpy( pData, m_GData, GSize );
	}

	dbg( "UnSwap" );
	for( int i = GSize-1; i >= 0; i-- )			//Unswap the data
	{
		int swap[2] = { i, (i+7)%GSize };
		byte Temp = pData[swap[0]];
		pData[swap[0]] = pData[swap[1]];
		pData[swap[1]] = Temp;
	}

	dbg( "Un-Offset" );
	for( int i = GSize-1; i >= 0; i-- )
		pData[i] -= (i * 4) % 128;			//Un-Offset the data

	dbg( "Validate" );
	if( *(ulong *)pData != m_GCurrentKey )	//Validate Checksum
	{
		#ifdef DEV_BUILD	
			MessageBox( NULL, "Memory inconsistancy detected!", ENCRYPT_TITLE, MB_OK );
		#endif
		Sleep( 3000 );
		exit( 0 );								//Harshly exit, on failure
	}

	SwapBufferPointers( );
	enddbg( "safemem_varbase::UnEncrypt" );
}

void safemem_varbase::UnRegister( ) 
{
	if( !m_Registered )
		return;

	UnEncrypt( );									//Un-encrypt mem

	ulong Ofs = MemLoc_Get( ),						//Get Data Loc/Size
		  Size = MemSize_Get( ),						
		  AfterOfs = Ofs + Size;					//Get Ofs just after the data
	ulong GSize = GMemSize_Get(),					//Save old Global mem size
		  GNewSize = GSize - Size,					//Find new Global mem size
		  GAfterSize = GSize - AfterOfs;			//Get size of the data following the deleted data 
													//(could be none if deleting the last data item)
	byte *GOldMem = m_GData;						//Save old mem pointer

	Alloc( GNewSize );								//Alloc new Global mem

	memcpy( m_GData, GOldMem, Ofs );				//Copy old global mem
	if( GAfterSize )
		memcpy( &m_GData[Ofs], &GOldMem[AfterOfs], GAfterSize );	//Copy old global mem
	delete GOldMem;								//Delete old global memory

	m_GVars.erase( m_VarIdx );						//Delete from the var list
	for( ulong v = m_VarIdx; v < m_GVars.size(); v++ )//Update the other vars' memory positions
	{
		safemem_varbase &Var = *(m_GVars[v]);
		Var.MemLoc_Set( Var.MemLoc_Get() - Size );
		if( Var.m_VarIdx != 0 )
			Var.m_VarIdx--;
		#ifdef DEV_BUILD
			else
				MessageBox( NULL, ENCRYPT_TITLE, "UnRegister Memory error", MB_OK );
		#endif
	}

	Update( );										//Change validation key
	Encrypt( );										//Re-Encrypt mem

	m_Registered = false;
}
void safemem_varbase::ChangeData( void *pData )
{
	UnEncrypt( );									//Un-encrypt mem

	ulong Ofs = MemLoc_Get( ),						//Get Data Loc/Size
		  Size = MemSize_Get( );

	foreach( i, Size )								//Check if any data changed
		if( m_GData[Ofs+i] != ((byte *)pData)[i] )
			{ m_Changed = true; break; }

	memcpy( &m_GData[Ofs], pData, Size );			//Copy in Data
	
	Update( );										//Change validation key
	Encrypt( );										//Re-Encrypt mem
}
void safemem_varbase::GetData( void *pBuffer ) const
{ 
	UnEncrypt( true );								//Un-encrypt mem to temp buffer

	ulong Ofs = MemLoc_Get( ),						//Get Data Loc/Size
		  Size = MemSize_Get( );						
	memcpy( pBuffer, &m_GDataBuf[Ofs], Size );		//Copy out Data
}
void safemem_varbase::Update( )
{ 
	ulong GSize = GMemSize_Get( );					//Get unencrypted data size

	m_GCurrentKey = GetTickCount( );				//Change Encryption key

	GMemSize_Set( GSize );							//Re-encrypt size
}

void safemem_varbase::MemLoc_Set( ulong Loc ) { m_MemLoc = m_MemSeed - Loc; }
void safemem_varbase::MemSize_Set( ulong Size ) { m_MemSize = m_MemSeed - Size; }
ulong safemem_varbase::MemLoc_Get( ) const { return m_MemSeed - m_MemLoc; }
ulong safemem_varbase::MemSize_Get( ) const { return m_MemSeed - m_MemSize; }
