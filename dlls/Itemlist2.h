#define NULL 0

class Itemlist2 {
public:
	struct listitem_t {
		void *ItemData;
		listitem_t *NextItem;
	};

	listitem_t *FirstItem;
	int Volume, ItemTotal;

	Itemlist2( ) { Spawn(); }
	void Spawn( ) { FirstItem = 0; ItemTotal = 0; }
	bool CanAddItem( void *pvNewItem );
	bool AddItem( void *vNewItem );
	bool ItemExists( void *pItem );
	void *GetItem( int index );
	bool RemoveItem( void *vDelItem );
	bool RemoveAllItems( );
};