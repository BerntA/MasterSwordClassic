#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "Weapons/Weapons.h"
#include "Weapons/GenericItem.h"
#include "MSItemDefs.h"
#include "MSUtils.h"

#ifdef VALVE_DLL
	int g_SaveFilePrg, g_MsgFuncPackPrg;
#else
	extern int g_SaveFilePrg;
	extern int g_MsgFuncPackPrg;
#endif

CGenericItem Itemlist::ItemByNum( int index ) {
	listitem_t *TempItem = FirstItem;
	int n = 0;

	g_MsgFuncPackPrg = 50401;
	if( index < 0 )
	{
		ALERT( at_console, "GetItem: index < 0.\n" );
		return NULL;
	}

	g_SaveFilePrg = 5100;
	g_MsgFuncPackPrg = 50402;
	while( n < index ) {
		if( !TempItem ) { 
#ifdef DG_DEBUG
			ALERT( at_console, "GetItem: Item not found.\n" );
#endif
			return NULL;
		}
		g_SaveFilePrg++;
		g_MsgFuncPackPrg++;
		TempItem = TempItem->NextItem; n++;
	}
	if( !TempItem ) return NULL;

	g_SaveFilePrg = 5800;
	g_MsgFuncPackPrg = 50500;
	return (CGenericItem *)MSInstance(TempItem->peItem);
}
BOOL Itemlist :: CanAddItem( CGenericItem *NewItem )
{
	if( !NewItem ) return false;

	//This Item is already in your pack
	if( ItemExists(NewItem) ) return FALSE;

	//Not enoungh space
	if( NewItem->Volume( ) + FilledVolume( ) > Volume ) {
		return FALSE;
	}

	return TRUE;
}
BOOL Itemlist :: AddItem( CGenericItem *NewItem )
{
	if( !CanAddItem(NewItem) ) return FALSE;

	listitem_t *TempItem = FirstItem;

	if( FirstItem == NULL ) {
		FirstItem = TempItem = new(listitem_t);
		TempItem->peItem = NewItem->edict();
	}
	else {
		while( TempItem->NextItem ) {
/*			Group Items
			CGenericItem *pItem = (CGenericItem *)MSInstance(TempItem->peItem);
			if( pItem && FBitSet( pItem->MSProperties(), ITEM_GROUPABLE ) && pItem->ItemName == NewItem->ItemName )
			{
				break;
			}*/
			TempItem = TempItem->NextItem;
		}
		TempItem = TempItem->NextItem = new(listitem_t);
		TempItem->peItem = NewItem->edict();
	}

	ItemNum++;
	TempItem->NextItem = NULL;

	return TRUE;
}
CGenericItem *Itemlist :: ItemByName( char *pszName ) {
	listitem_t *TempItem = FirstItem;

	while( TempItem && MSInstance(TempItem->peItem) && !strstr(STRING(((CGenericItem *)MSInstance(TempItem->peItem))->ItemName), pszName) ) TempItem = TempItem->NextItem;
	if( !TempItem ) return NULL;

	return (CGenericItem *)MSInstance(TempItem->peItem);
}
BOOL Itemlist :: ItemExists( CGenericItem *pItem ) {
	if( !pItem ) return FALSE;

	listitem_t *TempItem = FirstItem;

	while( TempItem && TempItem->peItem != pItem->edict() ) TempItem = TempItem->NextItem;
	if( TempItem && TempItem->peItem == pItem->edict() ) return TRUE;

	return FALSE;
}
CGenericItem *Itemlist :: ItemByID( ULONG lID ) {
	listitem_t *TempItem = FirstItem;

	while( TempItem && !(MSInstance(TempItem->peItem) && ((CGenericItem *)MSInstance(TempItem->peItem))->m_iId == lID) ) TempItem = TempItem->NextItem;
	if( !TempItem ) return NULL;

	return (CGenericItem *)MSInstance(TempItem->peItem);
}
BOOL Itemlist :: RemoveItem( CGenericItem *DelItem ) {
	listitem_t *TempItem = FirstItem, *PrevItem = NULL;

	while( TempItem && TempItem->peItem != DelItem->edict() ) {
		if( TempItem->NextItem == NULL ) break;
		PrevItem = TempItem;
		TempItem = TempItem->NextItem;
	}
	if( !TempItem || TempItem->peItem != DelItem->edict() ) {
#ifdef DG_DEBUG
		ALERT( at_console, "DelItem: Item not found.\n" );
#endif
		return FALSE;
	}

	ItemNum--;

	if( !PrevItem ) FirstItem = TempItem->NextItem;
	else PrevItem->NextItem = TempItem->NextItem;
	delete TempItem;


	return TRUE;
}
BOOL Itemlist :: RemoveAllItems( ) {
	if( !FirstItem ) return FALSE;
	listitem_t *pItem = FirstItem;

	while( FirstItem ) 
		RemoveItem( (CGenericItem *)MSInstance(FirstItem->peItem) );
	return TRUE;
}
float Itemlist :: FilledVolume( )
{
	//FilledVolume... also known as weight
	float Volume = 0;
	for( int i = 0; i < ItemNum; i++ )
	{
		CGenericItem *pItem = ItemByNum( i );
		if( !pItem ) continue;

		Volume += pItem->Weight( );
	}

	return Volume;
}