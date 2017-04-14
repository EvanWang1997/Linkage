#pragma once

class QuickXMLNode
{
	public:
	QuickXMLNode( bool bElement = true );
	~QuickXMLNode();
	
	bool Parse( const char *pText );
	
	class QuickXMLNode* FindChildByName( const char *pName );
	const char* FindChildDataByName( const char *pName );
	class QuickXMLNode* GetFirstChild( void );
	class QuickXMLNode* GetNextSibling( void );
	bool IsElement( void );
	const CString & GetText( void );
	const CString & GetAttribute( const char *pAttribute );
	
	public:
	const char * Parse( const char *pText, bool bExpectEndTag );
	void Clean( void );
	
	class QuickXMLNodeImplementation * m_pImplementation;
};