#include "Itemlist2.h"

void *Itemlist2 :: GetItem( int index )
{
	listitem_t *TempItem = FirstItem;
	int n = 0;

	if( index < 0 ) return NULL;

	while( n < index ) {
		if( !TempItem )  return NULL;
		TempItem = TempItem->NextItem; n++;
	}
	if( !TempItem ) return NULL;

	return TempItem->ItemData;
}
bool Itemlist2 :: CanAddItem( void *pvNewItem )
{
	if( !pvNewItem ) 
		return false;

	return true;
}
bool Itemlist2 :: AddItem( void *pvNewItem )
{
	if( !CanAddItem(pvNewItem) ) return false;

	listitem_t *TempItem = FirstItem;

	//This Item is already in your pack
	if( ItemExists(pvNewItem) )
		return true;

	if( FirstItem == NULL ) 
		FirstItem = TempItem = new(listitem_t);
	else
	{
		while( TempItem->NextItem )
			TempItem = TempItem->NextItem;
		TempItem = TempItem->NextItem = new(listitem_t);
	}

	TempItem->ItemData = pvNewItem;
	TempItem->NextItem = NULL;
	ItemTotal++;

	return true;
}
bool Itemlist2 :: ItemExists( void *pvItem )
{
	if( !pvItem ) return false;

	listitem_t *TempItem = FirstItem;

	while( TempItem && TempItem->ItemData != pvItem ) TempItem = TempItem->NextItem;
	if( TempItem && TempItem->ItemData == pvItem ) return true;

	return false;
}
bool Itemlist2 :: RemoveItem( void *pvDelItem )
{
	listitem_t *TempItem = FirstItem, *PrevItem = NULL;

	while( TempItem && TempItem->ItemData != pvDelItem ) {
		PrevItem = TempItem;
		TempItem = TempItem->NextItem;
	}
	if( !TempItem ) return false;

	ItemTotal--;

	if( !PrevItem ) FirstItem = TempItem->NextItem;
	else PrevItem->NextItem = TempItem->NextItem;
	delete TempItem;

	return true;
}
bool Itemlist2 :: RemoveAllItems( )
{
	if( !FirstItem ) return false;

	listitem_t *pItem = FirstItem, *PrevItem;

	while( pItem )
	{
		PrevItem = pItem;
		pItem = pItem->NextItem;
		delete PrevItem;
	}
	ItemTotal = 0;
	return true;
}
