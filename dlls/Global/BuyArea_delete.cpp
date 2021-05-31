#include "../inc_itemdefs.h"

#include "BuyArea.h"
// Invisible, yet existing areas where you can buy stuff

LINK_ENTITY_TO_CLASS(msarea_buy,CBuyArea);

//1 BuyItem Function
void BuyItem::SpawnNewItem( ) {
	//You know the item this BuyItem represents exists, so this func
	//lets you creates a new one at will
	Item = (CBasePlayerItem *)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING(classname)));
	Item->PreSpawn( );
	Item->pev->effects |= EF_NODRAW;
}
void BuyItem::InitializeNewItem( edict_t *pEdict ) {
	//So you can check if the item exists BEFORE calling this by trying to creating it and 
	//seeing if that fails... then, since it's already created, initialize this BuyItem with 
	//it
	Item = (CBasePlayerItem *)GET_PRIVATE(pEdict);
	Item->PreSpawn( );
	Item->pev->effects |= EF_NODRAW;
}

void CBuyArea::Spawn( ) {
	return;
	//***!!!*** Extract Info like what weapons to sell ***!!!***
	//Master Sword: I learned alot from using brush-based entities:
	//SET_MODEL will ALSO set pev->mins,maxs,absmin,absmax
	//VecBModelOrigin will give you pev->origin
	//KeyValues come in any order but they all come BEFORE Spawn()
	Vector vSave[2];
	SET_MODEL( edict(), STRING(pev->model) );
	pev->origin = VecBModelOrigin( pev );
	vSave[0] = pev->absmin; vSave[1] = pev->absmax;
	UTIL_SetOrigin( pev, pev->origin );
	pev->absmin = vSave[0]; pev->absmax = vSave[1];
	BuyItem *biItem = FirstItemPrice, *NewItem, *OldItem;
	edict_t	*e_newitem;
	while( biItem ) { 
//		ALERT( at_console, "Class: %s\n", biItem->classname );
		e_newitem = CREATE_NAMED_ENTITY(MAKE_STRING(biItem->classname));
		if ( FNullEnt(e_newitem) ) {
			ALERT( at_console, "ERROR: Couldn't create item %s to buy!\n", biItem->classname );
		}
		else {
			NewItem = AddItem( biItem->classname, biItem->Quantity, biItem->Price, &FirstItem );
			NewItem->InitializeNewItem( e_newitem );
		}
		OldItem = biItem;
		biItem = biItem->NextItem; 
		delete OldItem;
	} 

}
void CBuyArea::KeyValue( KeyValueData *pkvd ) {
//	BuyItem *NextItem;
	CHAR pszItemClass[30], *pszPtr;
	int iNameLen, iValue;

	if( !(pszPtr=strstr(pkvd->szKeyName, "_price")) ) 
		if( !(pszPtr=strstr(pkvd->szKeyName, "_quantity")) ) return;

	iValue = atoi(pkvd->szValue);
	if( iValue == -1 ) return;
	iNameLen = (pszPtr - pkvd->szKeyName) + 1;
	strcpy( pszItemClass, "ms_" );
	strncpy( pszItemClass+3, pkvd->szKeyName, (iNameLen-1) );
	strcat(pszItemClass, "" );
	pszItemClass[3+iNameLen-1] = '\0';

	if( pszPtr=strstr(pkvd->szKeyName, "_price") ) {
//		ALERT( at_console, "%s Price: %i\n", pszItemClass, atoi(pkvd->szValue) );
		AddItem( pszItemClass, -2, iValue, &FirstItemPrice );	
		pkvd->fHandled = TRUE;
	}
	else if( pszPtr=strstr(pkvd->szKeyName, "_quantity") ) {
//		ALERT( at_console, "%s Quantity: %i\n", pszItemClass, atoi(pkvd->szValue) );
		AddItem( pszItemClass, iValue, -2, &FirstItemPrice );	
		pkvd->fHandled = TRUE;
	}
	else CBaseEntity::KeyValue( pkvd );
}
BuyItem *CBuyArea::AddItem( char *classname, int Quantity, int Price, BuyItem **FirstItem ) {
	BuyItem *PrevItem = *FirstItem, *NextItem = *FirstItem;
	while( NextItem ) { 
		PrevItem = NextItem;
		if( !strcmp(NextItem->classname, classname) ) break;
		NextItem = NextItem->NextItem; 
	}
	if( !NextItem ) {
		NextItem = new(BuyItem);
		NextItem->NextItem = NULL;
		if( PrevItem ) PrevItem->NextItem = NextItem;
		strcpy( NextItem->classname, classname ); //Initial values
		NextItem->Quantity = 0; NextItem->Price = 0;
	}
	if( !*FirstItem ) *FirstItem = NextItem;
	if( Quantity != -2 ) NextItem->Quantity = Quantity;
	if( Price != -2 ) NextItem->Price = Price;
	return NextItem;
}
CBuyArea::CBuyArea( ) {
	FirstItem = NULL;	FirstItemPrice = NULL;
}
