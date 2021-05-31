
//All functions called by the game should call this
//Then I can debug each one, in order
void DbgLog( char *szFmt, ... );

void DbgInputs( const char *Function, const char *File, long Line )
{
	//_asm { int 3 };
	int stop = 0;
	DbgLog( "\r\n[MOD INPUT] %s", Function );
}