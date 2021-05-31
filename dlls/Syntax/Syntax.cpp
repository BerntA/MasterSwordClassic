#include "inc_weapondefs.h"

char ReturnString[4096];
const char *SPEECH::ItemName( CGenericItem *pItem, bool fCapital )
{
	char sz[2][32];
	if( /*FBitSet( pItem->MSProperties(), ITEM_GROUPABLE ) &&*/ pItem->iQuantity > 1 )
	{
		sprintf( sz[0], "%i ", pItem->iQuantity );
		strcpy( sz[1], "s" );
	}
	else {
		if( pItem->DisplayPrefix.len() )
			sprintf( sz[0], "%s ", pItem->DisplayPrefix.c_str() );
		else
			sz[0][0] = 0;
		strcpy( sz[1], "" );
	}
	sprintf( ReturnString, "%s%s%s", sz[0], pItem->DisplayName(), sz[1] );
	if( fCapital ) ReturnString[0] = toupper( ReturnString[0] );
	return &ReturnString[0];
}
const char *SPEECH::NPCName( CMSMonster *pMonster, bool fCapital )
{
	msstring Prefix = pMonster->DisplayPrefix.len() ? (pMonster->DisplayPrefix + " ") : ("");
	sprintf( ReturnString, "%s%s", Prefix.c_str(), pMonster->DisplayName() );
	if( fCapital ) ReturnString[0] = toupper( ReturnString[0] );
	return &ReturnString[0];
}
const char *SPEECH::HandName( int iHand, bool fCapital ) 
{
	if( iHand == 0 ) return fCapital ? "Left" : "left";
	if( iHand == 1 ) return fCapital ? "Right" : "right";
	return "invalid hand";
}

/*
int SPEECH_HandToInt( char *Hand ) {
	if( !stricmp(Hand, "left") ) return 0;
	if( !stricmp(Hand, "right") ) return 1;
	return -1;
}
//SPEECH_GetItemList -- OUTDATED
void SPEECH_GetItemList ( CBasePlayerItem *FirstItem, char *ReturnString ) {
	CHAR *psTemp = new(CHAR[200]);
	CBasePlayerItem *CurrentItem = FirstItem;
	strcpy( ReturnString, "" );
	strcpy( psTemp, "" );
	while( CurrentItem ) {
		if( !CurrentItem->NextItem && CurrentItem != FirstItem)
			strcat( ReturnString, " and" );
		sprintf( psTemp, " %s %s", STRING(CurrentItem->DisplayPrefix), STRING(CurrentItem->DisplayName) );
		strcat( ReturnString, psTemp );
		if( CurrentItem->NextItem ) if( CurrentItem->NextItem->NextItem )
			strcat( ReturnString, "," );
		CurrentItem = CurrentItem->NextItem;
	}
	delete []psTemp;
}*/