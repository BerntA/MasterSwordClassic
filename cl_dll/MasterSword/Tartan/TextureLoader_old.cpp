// This program is free software; you can redistribute it and/or modify it
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <iostream>
using namespace std;

#include <string.h>

#include "texturestruct.h"
#include "tgaloader.h"
//#include "pcxloader.h"

#include "TextureLoader.h"

#ifndef _WIN32
void strlwr( char *string )
{
	char *p = string;
	while( *p != 0 )
	{
		*p = tolower( *p );
		p++;
	}
}
#endif

namespace Tartan
{

//! Loads sFilePath as a texture file into OpenGL, returns OpenGL texture id in iTextureID
//!
//! Loads sFilePath as a texture file into OpenGL, returns OpenGL texture id in iTextureID
//! Returns true on success
bool LoadTextureFile( int &iTextureID, const char *sFilePath )
{
	if( strlen( sFilePath ) <= 4 )
	{
		cout << "Tartan::LoadTextureFile() Error: File " << sFilePath << " has an invalid filename extension" << endl;
	}

	char filetypestring[5];
	sprintf( filetypestring, "%s", sFilePath + strlen( sFilePath ) - 4 );
	cout << "filetypestring " << filetypestring << endl; // DEBUG

	strlwr( filetypestring );
	if( strcmp( filetypestring, ".tga" ) == 0 )
	{
		return LoadTextureTGA( iTextureID, sFilePath );
	}
	/*else if( strcmp( filetypestring, ".pcx" ) == 0 )
	{
		return LoadTexturePCX( iTextureID, sFilePath );
	}*/
	else if( strcmp( filetypestring, ".bmp" ) == 0 )
	{
		return LoadTextureBMP( iTextureID, sFilePath );
	}
	else
	{
		cout << "Tartan::LoadTextureFile() Error: file type for file " << sFilePath << " unknown." << endl;
		return false;
	}
}

//! Input:
//!
//! Returns: True on success.
//!
//! Description: Loads the "missing texture" placeholder
//!
//! Thread safety: Thread-safe if the OpenGL library is
//!
//! History: 20050415 Mark Wagner - Created
bool LoadMissingTexture( int &iTextureID )
{
	unsigned char Pixel[4] = {'\255', '\255','\255'};	// A white pixel
	GLuint Texture;
	glGenTextures(1, &Texture );
	glBindTexture(GL_TEXTURE_2D, Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &Pixel);
	iTextureID = Texture;
	return true;
}

//! Input:
//!
//! Returns: True on success, false otherwise
//!
//! Description: Loads a PCX image into an OpenGL texture.
//! This function based on NeHe texture loading tutorial (http://nehe.gamedev.net)
//!
//! Thread safety: Unknown
//!
/*bool LoadTexturePCX( int &iReturnedTextureID, const char *sFilepath )
{
	//DEBUG(  "Texture file is at: " << FilePath.GetFullPath().mb_str() ); // DEBUG

	int width, height;
	unsigned char *pImage = NULL;
	GLuint iTextureID;
	bool PCXLoadingResult = LoadPCX( sFilepath, width, height, &pImage );
	//DEBUG(  "pcxloading result: " << PCXLoadingResult ); // DEBUG
	if( !PCXLoadingResult )
	{
		cout <<  "error loading pcx file " << sFilepath << endl;
		return false;
	}

	glGenTextures(1, &iTextureID);				// Create The Texture ( CHANGE )
	glBindTexture(GL_TEXTURE_2D, iTextureID);
	
	cout << "textureid: " << iTextureID << " width " << width << " height " << height << endl;
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pImage);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	if (pImage != NULL)						// If Texture Image Exists ( CHANGE )
	{
		free(pImage);					// Free The Texture Image Memory ( CHANGE )
	}

	iReturnedTextureID = iTextureID;

	return true;
}*/

//! Input:
//!
//! Returns: True on success, false otherwise
//!
//! Description: Loads a TGA image into an OpenGL texture.
//! This function based on NeHe texture loading tutorial (http://nehe.gamedev.net)
//!
//! Thread safety: Unknown
//!
bool LoadTextureTGA( int &iReturnedTextureID, const char *sFilepath )
{
	Texture Texture;
	
//	DEBUG(  "Loading TGA file " << FilePath.GetFullPath().mb_str() << " ... " ); // DEBUG

	int LoadTGAResult = LoadTGA(&Texture, sFilepath );
	//DEBUG(  "Check result of TGA loading..." ); // DEBUG
	if( !LoadTGAResult )
	{
		cout <<  "Couldn't find or load file " << sFilepath << endl;
		return false;
	}
	//DEBUG(  "File found, loading..." ); // DEBUG

	// Typical Texture Generation Using Data From The TGA ( CHANGE )
	glGenTextures(1, &Texture.texID);				// Create The Texture ( CHANGE )
	glBindTexture(GL_TEXTURE_2D, Texture.texID);

	GLint format;
	//DEBUG(" *** DESIRED format : " << (Texture.bpp / 8) << " width: " << Texture.width << " height : " << Texture.height);
	glTexImage2D(GL_PROXY_TEXTURE_2D, 0, Texture.bpp / 8, Texture.width, Texture.height, 0, Texture.type, GL_UNSIGNED_BYTE, NULL);
	glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	//DEBUG(" ***** Load texture: " << format);

	glTexImage2D(GL_TEXTURE_2D, 0, Texture.bpp / 8, Texture.width, Texture.height, 0, Texture.type, GL_UNSIGNED_BYTE, Texture.imageData);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

	GLenum err = glGetError();
	cout << "** glGetError from LoadTextureTGA : " << err << endl;

	if (Texture.imageData)						// If Texture Image Exists ( CHANGE )
	{
		free(Texture.imageData);					// Free The Texture Image Memory ( CHANGE )
	}

	iReturnedTextureID = Texture.texID;

	return true;
}

}
