#ifndef INC_CLASSES
#define INC_CLASSES

#include "Races.h"

class CClass : public CRace 
{
public:
	//The multiplier is a 0-100 value
	//Any stat experience is multiplied by (multiplier/100)
	CStat *NaturalMulti;
	unsigned char *SkillMulti;

	CClass( );
	~CClass( );
};

void InitializeGlobalClasses( );
CClass *GetClassByID( int m_ID );

#endif //INC_CLASSES