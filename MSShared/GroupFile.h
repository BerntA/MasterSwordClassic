//Groupfile... its just like a pakfile

#include "stackstring.h"
typedef unsigned long ulong;

struct groupheader_t
{
	msstring FileName;
	ulong DataOfs, DataSize;
};

struct cachedentry_t : groupheader_t
{
	byte *Data;
};

class CGroupFile
{

protected:

	char m_FileName[MAX_PATH];
	//unsigned long FindHeader( const char *pszName, groupheader_t &GroupHeader );
	bool DeleteEntry( const char *pszName );
	bool m_IsOpen;

public:

	void Open( char *pszFileName );
	~CGroupFile( ) { Close( ); }
	void Close( );
	bool IsOpen( ) { return m_IsOpen; }
	bool WriteEntry( const char *pszName, byte *pData, unsigned long DataSize );
	
	//Call Read() with pBuffer == NULL to just get the size
	bool ReadEntry( const char *pszName, byte *pBuffer, unsigned long &DataSize );
	void Flush( );


	mslist<cachedentry_t> m_EntryList;
};

/*Format:

  [DWORD] Number of Headers
  [groupheader_t * X] X Amount of Headers
  [DATA] All data
 */
