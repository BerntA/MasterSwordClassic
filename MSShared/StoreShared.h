struct containeritem_t  : public genericitem_t
{
	containeritem_t( ) { }
	containeritem_t( genericitem_t&Item );
	containeritem_t( class CGenericItem *pItem );
	void init( class CGenericItem *pItem );

	string_i SpriteName;
	msstring FullName;
	msstring DebugName;					//Remove
	bool Disabled;
	int Value;
};

struct storeitem_t : public containeritem_t
{
	storeitem_t( ) { }
	storeitem_t( class CGenericItem *pItem );
	int iCost, iBundleAmt;
	float flSellRatio;
};

#define STORE_BUY	(1<<0)
#define STORE_SELL	(1<<1)
#define STORE_INV	(1<<2)