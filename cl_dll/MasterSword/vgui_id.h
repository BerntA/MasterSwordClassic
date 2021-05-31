//
//  This should only be included by vgui_HUD.cpp
//

class VGUI_ID : public Panel
{
public:
	VGUI_FadeText *m_Label[2];
	entinfo_t *m_LastID;

	VGUI_ID( Panel *pParent, int x, int y ) : Panel( x, y, XRES(200), YRES(36) )
	{
		setParent( pParent );
		setBgColor( 0, 0, 0, 255 );

		 for (int i = 0; i < 2; i++) 
		{
			m_Label[i] = new VGUI_FadeText( this, 1, "", 0, 0 + i * g_FontID->getTall(), MSLabel::a_west );
			m_Label[i]->setFont( g_FontID );
		}
		m_Label[0]->SetFGColorRGB( Color_Text_White );
	}

	void Update( entinfo_t *pEntInfo )
	{
		if( pEntInfo == m_LastID )
			return;

		if( pEntInfo )
		{
			entinfo_t &EntInfo = *pEntInfo;
			m_Label[0]->setText( EntInfo.Name );


			msstring String;
			COLOR DifficultyColor = COLOR( 0, 255, 0, 0 );
			if( EntInfo.Type == ENT_FRIENDLY ) {
				String = "Friendly";
				DifficultyColor = COLOR( 0, 255, 0, 0 );
			}
			else if( EntInfo.Type == ENT_WARY ) {
				String = "Wary";
				DifficultyColor = COLOR( 255, 255, 0, 0 );
			}
			else if( EntInfo.Type == ENT_HOSTILE ) {
				String = "Hostile";
				DifficultyColor = COLOR( 255, 0, 0, 0 );
			}
			else if( EntInfo.Type == ENT_DEADLY ) {
				String = "Deadly";
				DifficultyColor = COLOR( 255, 0, 0, 0 );
			}
			else if( EntInfo.Type == ENT_BOSS ) {
				String = "Elite";
				DifficultyColor = COLOR( 255, 128, 0, 0 );
			}
			m_Label[1]->setText( String );
			m_Label[1]->SetFGColorRGB( DifficultyColor );
			m_Label[0]->StartFade( false );
			m_Label[1]->StartFade( false );
		}
		else
		{
			 for (int i = 0; i < 2; i++) 
			{
				float PrevDelta = gpGlobals->time - m_Label[i]->m_StartTime;
				PrevDelta = min(PrevDelta,m_Label[i]->m_FadeDuration);
				m_Label[i]->StartFade( true );
				m_Label[i]->m_StartTime = gpGlobals->time - (m_Label[i]->m_FadeDuration - PrevDelta);
			}
		}
		
		m_LastID = pEntInfo;
		for (int i = 0; i < 2; i++) m_Label[i]->Update( );
	}
	void Update( )
	{
		for (int i = 0; i < 2; i++) m_Label[i]->Update();
		setVisible( ShowHUD( ) );
	}
	void NewLevel( )
	{
		 for (int i = 0; i < 2; i++) 
		{
			m_Label[i]->m_StartTime = -1000;	//Ensure ID doesn't show up after a level change
			m_Label[i]->setText( "" );
		}
	}
};