#include <pshpack1.h>
typedef unsigned char byte;
typedef unsigned long ulong;

#define MEM_ENCRYPT

class safevarlist : public mslist<class safemem_varbase *>
{
public:
	safevarlist( ) : mslist<class safemem_varbase *>( true )
	{ 
		Init( );
	}
	void Init( ) 
	{ 
		if( !m_Initialized ) clear();
		m_Initialized = true; 
	}
	static bool m_Initialized;
};

class safemem_varbase
{
public:
	safemem_varbase( );
	
	ulong m_MemLoc;			// == RAND( MemSeed ) - <actual mem location>
	ulong m_MemSize;		// == RAND( MemSeed ) - <actual mem size>
	ulong m_MemSeed;		// == random seed
	ulong m_VarIdx;			// == My index into m_GVars

	bool m_Changed;			// Marked as true each time this variable is changed.  Any code that
							// constantly checks this variable for changes should use this instead
							// the slow get accessors.  Saves an UnEncrypt().  The code must set this to false
	bool m_Registered;

	void MemLoc_Set( ulong Loc );
	void MemSize_Set( ulong Size );
	ulong MemLoc_Get( ) const;
	ulong MemSize_Get( ) const;

	ulong m_DataSize;		//Set in child class safemem_var::init()
	//UpdateKey( ulong NewKey );

	void Register( );
	void UnRegister( );
	void ChangeData( void *pData );	//Set Data
	void GetData( void *pBuffer ) const;	//Get Data
	static void Update( );			//Generates a new key every frame
	static void Deactivate( );		//Clear memory

	static void Alloc( ulong NewSize );	//Allocates memory, sets the the m_GData pointer to it and stores the size
	static void AllocAndCopy( ulong NewGSize );		//Allocates the memory, sets the pointers, and copies the old memory
	static void UnEncrypt( bool CopyData = false );	//Unencrypts m_GData and stores the result back in m_GData
	static void Encrypt( );			//Encrypts m_GData and stores the result back in m_GData
	static void SwapBufferPointers( );	//Swaps the main buffer with the temporary one (For encrypt/decrypt)

	static bool m_GInitialized;	//If Global mem is initialized
	static byte *m_GData;		//All data is stored here
	static byte *m_GDataBuf;	//Buffer for encrypt/decrypt routines
	static ulong m_GMemSizeKey;	//== m_CurrentKey - <actual global mem size>
	static ulong m_GCurrentKey;	//Current key for this frame
								//Every GetData() operation checks this to make sure memory
								//hasn't been tampered
	static safevarlist m_GVars;	//List of all safe vars
	inline static void GMemSize_Set( ulong Size ) { m_GMemSizeKey = m_GCurrentKey - Size; }
	inline static ulong GMemSize_Get( ) { return m_GCurrentKey - m_GMemSizeKey; }
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
		m_DataSize = sizeof(itemtype_y);
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
