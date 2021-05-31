class BuyItem {
public:
	CBasePlayerItem *Item;
	int Quantity, Price;
	BuyItem *NextItem;
	char classname[20];
	void SpawnNewItem( );
	void InitializeNewItem( edict_t *pEdict );
};

class CBuyArea : public CBaseEntity
{
public:
	void Spawn( );
	void KeyValue( KeyValueData *pkvd );
	BuyItem *AddItem( char *classname, int Quantity, int Price, BuyItem **FirstItem );
	int AvailableItems; 
	BuyItem *FirstItem, *FirstItemPrice;
	CBuyArea( );
};
