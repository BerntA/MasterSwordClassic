#include "msdebug.h"
#include "cpplibraries.h"

/*
bool TokenizeString( const char *pszString, vector<msstring> &Tokens )
{
	char cTemp[MSSTRING_SIZE-1];
	int i = 0;
	bool AnyFound = false;
	while( sscanf( &pszString[i], "%[^;]", cTemp ) > 0 )
	{
		i += strlen(cTemp);
		Tokens.push_back( cTemp );
		AnyFound = true;

		if( pszString[i] ) i++;	//Hit a semi-colon, continue
	}
	return AnyFound;
}	
*/