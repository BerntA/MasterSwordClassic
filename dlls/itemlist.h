class CGenericItem;

struct listitem_t {
	edict_t *peItem;
	listitem_t *NextItem;
};

class Itemlist {
public:
	listitem_t *FirstItem;
	int Volume, ItemNum;

	Itemlist( ) { Spawn(); }
	void Spawn( ) { FirstItem = NULL; ItemNum = 0; }
	BOOL Itemlist :: CanAddItem( CGenericItem *NewItem );
	BOOL AddItem( CGenericItem *NewItem );
	BOOL ItemExists( CGenericItem *pItem );
	CGenericItem *ItemByName( char *pszName );
	CGenericItem *ItemByNum( int index );
	CGenericItem *ItemByID( ULONG lID );
	BOOL RemoveItem( CGenericItem *DelItem );
	BOOL RemoveAllItems( );
	float FilledVolume( );
};