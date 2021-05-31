#include "buildcontrol.h"
#include <pshpack1.h>
typedef unsigned char byte;
typedef unsigned long ulong;

class safemem_varbase
{
public:
	safemem_varbase( );
	
	ulong m_Key;			// == random seed (From GetTickCount())
	ulong m_EncrypredLoc;	// == m_Key - <Location of Data>
	ulong m_EncrypredSize;	// == m_Key - <actual data size>
	ulong m_CheckSum;		// == Checksum stored by Encrypt().  Checked in Decrypt()
	byte *m_DataBuf;		//Buffer for encrypt/decrypt routines

	bool m_Changed;			// Marked as true each time this variable is changed.  Any code that
							// constantly checks this variable for changes should use this instead of
							// the slow get accessors.  Saves an UnEncrypt().  The code must set this to false

	void DataLoc_Set( ulong Loc );
	void DataSize_Set( ulong Size );
	byte *DataLoc_Get( ) const;
	ulong DataSize_Get( ) const;

	void Alloc( );			//Allocates memory, stores the location
	void Decrypt( ) const;	//Decrypts memory to buffer, Checks result against m_CheckSum
	void Encrypt( );		//Encrypts memory from buffer, Updates m_CheckSum, Updates m_Key, Loc and Size

	void Register( );
	void UnRegister( );
	void ChangeData( void *pData );	//Set Data
	void GetData( void *pBuffer ) const;	//Get Data
	//static void Deactivate( );		//Clear memory

	//static void SwapBufferPointers( );	//Swaps the main buffer with the temporary one (For encrypt/decrypt)

	//static bool m_GInitialized;	//If Global mem is initialized
	//static safevarlist m_GVars;	//List of all safe vars
};




template<class itemtype_y>
class safemem_var : public safemem_varbase
{
public:
	safemem_var( )
	{
		init( );
	}
	safemem_var( const itemtype_y &Item )
	{
		init( );
		Set( Item );
	}
	void init( )
	{
		DataSize_Set( sizeof(itemtype_y) );
		Register( );
	}

	inline ulong MemSize( )
	{
		sizeof(itemtype_y) + ;
	}

	~safemem_var( )
	{
		UnRegister( );
	}

	inline operator itemtype_y ( )
	{
		return Get( );
	}

	itemtype_y operator = ( const itemtype_y &Item )
	{
		Set( Item );

		return Item;
	}

	itemtype_y operator += ( const int Value )
	{
		itemtype_y Data = Get( );
		Data += Value;
		Set( Data );

		return Data;
	}

	itemtype_y operator -= ( const int Value )
	{
		return operator += ( -Value );
	}

	inline itemtype_y operator ++ ( )
	{
		return operator += ( 1 );
	}
	inline itemtype_y operator ++ ( int )
	{
		return operator += ( 1 );
	}

	inline itemtype_y operator -- ( )
	{
		return operator += ( -1 );
	}

	itemtype_y operator *= ( const float Value )
	{
		itemtype_y Data = Get( );
		Data *= Value;
		Set( Data );

		return Data;
	}

	itemtype_y operator /= ( const float Value )
	{
		itemtype_y Data = Get( );
		Data /= Value;
		Set( Data );

		return Data;
	}

	inline void Set( const itemtype_y &Item )
	{
		ChangeData( (void *)&Item );
	}
	inline itemtype_y Get( ) const
	{
		itemtype_y Data;
		GetData( &Data );
		return Data;
	}

};

#ifdef MEM_ENCRYPT
	#define safevar( type, name ) safemem_var<type> name
#else
	#define safevar( type, name ) type name
#endif

#include <poppack.h>
