#include "logfile.h"
#include <string.h>     // Needed for strlen().
#include "md5.h"        // MD5 message digest classes.
#include <fstream>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include "../MSShared/stackstring.h"
void Print( char *szFmt, ... );
using namespace std;

bool FileCheckSumMatch( const char *FileName, unsigned char *CheckSum )
{
	ifstream file( FileName, ios_base::in );
	if( !file.is_open( ) ) return false;

	file.seekg( 0, ios::end );
	long FileSize = file.tellg( );
	
	byte *pFileData = new byte[FileSize];
	file.seekg( 0, ios::beg );
	file.read( (char *)pFileData, FileSize );
	file.close();

	md5	hash;      // Create a MD5 object.
	md5Digest digest;

	hash.update( pFileData, FileSize );     // Process data 0 string.
	hash.report( digest );
	for( int i = 0; i < MD5_DIGEST_LENGTH; i++ )
		if( CheckSum[i] != digest[i] )
		{
				msstring CRCText;
				char tmpBuf[32];
				for( int i = 0; i < MD5_DIGEST_LENGTH; i++ )
				{
					sprintf( tmpBuf, "\\x%x", digest[i] );
					CRCText += tmpBuf;
				}

				logfile << "Map CRC Failed.  CRC is:" << endl;
				logfile << CRCText << endl;
				logfile << "Length: " << FileSize << endl;
			return false;
		}
	

	return true;
}
