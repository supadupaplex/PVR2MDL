/*
=====================================================================
Copyright (c) 2018, Alexey Leushin
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:
- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of the copyright holders nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
=====================================================================
*/

//
// This file contains all definitions and declarations
//

#pragma once

////////// Includes //////////
#include <stdio.h>		// puts(), printf(), sscanf(), snprintf()
#include <conio.h>		// _getch()
#include <direct.h>		// _mkdir()
#include <string.h>		// strcpy(), strcat(), strlen(), strtok(), strncpy()
#include <malloc.h>		// malloc(), free()
#include <stdlib.h>		// exit()
#include <math.h>		// round()
#include <ctype.h>		// tolower()
#include <sys\stat.h>	// stat()
#include <windows.h>	// CreateDitectoryA()

////////// Definitions //////////
#define PROG_VERSION "0.93"
#define _8BIT_PLTE_SZ 256
#define BMP_PLTE_ENTRY_SZ 4
#define MDL_PLTE_ENTRY_SZ 3
#define NOTEXTURES_MODEL 0
#define NORMAL_MODEL 1
#define SEQ_MODEL 2
#define DUMMY_MODEL 3
#define UNKNOWN_MODEL -1

////////// Typedefs //////////
typedef unsigned short int ushort;
typedef unsigned long int ulong;
typedef unsigned int uint;
typedef unsigned char uchar;

////////// Functions //////////
ulong FileSize(FILE **ptrFile);																			// Get size of file
void FileReadBlock(FILE **ptrSrcFile, void * DstBuff, ulong Addr, ulong Size);							// Read block from file to buffer
void FileWriteBlock(FILE **ptrDstFile, void * SrcBuff, ulong Addr, ulong Size);							// Write data from buffer to file
void FileWriteBlock(FILE **ptrDstFile, void * SrcBuff, ulong Size);										// Write data from buffer to file
void SafeFileOpen(FILE **ptrFile, const char * FileName, char * Mode);									// Try to open file, if problem oocur then exit
void FileGetExtension(const char * Path, char * OutputBuffer, uint OutputBufferSize);					// Get file extension
void FileGetName(const char * Path, char * OutputBuffer, uint OutputBufferSize, bool WithExtension);	// Get name of file with or without extension
void FileGetFullName(const char * Path, char * OutputBuffer, uint OutputBufferSize);					// Get full file name (with folders) without extension
void FileGetPath(const char * Path, char * OutputBuffer, uint OutputBufferSize);						// Get file path
bool CheckFile(char * FileName);																		// Check existance of file
void GenerateFolders(char * cPath);																		// Make sure, that all folders in path are existing
void PatchSlashes(char * cPathBuff, ulong BuffSize, bool SlashToBackslash);								// Patch slashes when transitioning between PAK and Windows file names
bool CheckDir(const char * Path);																		// Check if path is directory
void NewDir(const char * DirName);																		// Create directory
void FileSafeRename(char * OldName, char * NewName);													// Raname file

////////// Structures //////////

// MDL model header
#pragma pack(1)					// Eliminate unwanted 0x00 bytes
struct sModelHeader
{
	char Signature[4];			// "IDST"
	ulong Version;				// 0xA - GoldSrc model
	char Name[64];				// Internal model name
	ulong FileSize;				// Model file size
	char SomeData1[96];			// Data that is not important for conversion
	ulong SubmodelCount;		// How many submodels
	ulong SubmodelTableOffset;	// Location of submodel table 
	ulong TextureCount;			// How many textures
	ulong TextureTableOffset;	// Texture table location
	ulong TextureDataOffset;	// Texture data location
	ulong SkinCount;			// How many skins
	ulong SkinEntrySize;		// Size of entry in skin table (measured in shorts)
	ulong SkinTableOffset;		// Location of skin table
	ulong SubmeshCount;			// How many submeshes
	ulong SubmeshTableOffset;	// Location of submesh table
	char SomeData2[32];			// Data that is not important for conversion

	void UpdateFromFile(FILE ** ptrFile)	// Update header from file
	{
		FileReadBlock(ptrFile, this, 0, sizeof(sModelHeader));

		// I found some models that have long non null terminated internal name string
		// that was causing creepy beeping during printf(), so there is a fix for that
		Name[63] = '\0';
	}

	int CheckModel()						// Check model type
	{
		if (this->Signature[0] == 'I' && this->Signature[1] == 'D' && this->Signature[2] == 'S' && this->Version == 0xA)
		{
			if (this->Signature[3] == 'T')
			{
				if (this->TextureCount > 0)
					return NORMAL_MODEL;
				else
					return NOTEXTURES_MODEL;
			}
			else if (this->Signature[3] == 'Q')
			{
				return SEQ_MODEL;
			}
		}

		return UNKNOWN_MODEL;
	}

	void Rename(const char * NewName)
	{
		// Clear Name[] from garbage and leftovers
		memset(this->Name, 0x00, sizeof(this->Name));

		strcpy(this->Name, NewName);
	}
};

// MDL texture table entry
#pragma pack(1)					// Eliminate unwanted 0x00 bytes
struct sModelTextureEntry
{
	char Name[68];				// Texture name
	ulong Width;				// Texture width
	ulong Height;				// Texture height
	ulong Offset;				// Texture offset (in bytes)

	void UpdateFromFile(FILE ** ptrFile, ulong TextureTableOffset, ulong TextureTableEntryNumber)		// Update texture entry from model file
	{
		FileReadBlock(ptrFile, this, TextureTableOffset + TextureTableEntryNumber * sizeof(sModelTextureEntry), sizeof(sModelTextureEntry));
	}

	void Update(const char * NewName, ulong NewWidth, ulong NewHeight, ulong NewOffset)			// Update texture entry with new data
	{
		// Clear memory from garbage
		memset(this, 0x00, sizeof(sModelTextureEntry));

		// Update fields
		strcpy(this->Name, NewName);
		this->Width = NewWidth;
		this->Height = NewHeight;
		this->Offset = NewOffset;
	}
};

// 8-bit *.bmp header
#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sBMPHeader
{
	char Signature1[2];		// "BM" Signature
	ulong FileSize;			// Total file size (in bytes)
	ulong Signature2;		// 0x00000000
	ulong Offset;			// Offset
	ulong StructSize;		// BMP version
	ulong Width;			// Picture Width (in pixels)
	ulong Height;			// Picture Height (in pixels)
	ushort Signature3;		// 
	ushort BitsPerPixel;	// How many bits per 1 pixel
	ulong Compression;		// Compression type
	ulong PixelDataSize;	// Size of bitmap
	ulong HorizontalPPM;	// Horizontal pixels per meter value
	ulong VerticalPPM;		// Vertical pixels per meter value
	ulong ColorTabSize;		// How many colors are present in color table
	ulong ColorTabAlloc;	// How many colors are actually used in color table

	void Update(unsigned long int Width, unsigned long int Height)		// Update all fields of BMP header
	{
		this->Signature1[0] = 'B';				// Add 'BM' signature
		this->Signature1[1] = 'M';				//
		this->Signature2 = 0x00000000;			// Should be 0x00000000
		this->Offset = 0x00000436;				// Bitmap offset for version 3 BMP
		this->StructSize = 0x00000028;			// Version 3 BMP
		this->Signature3 = 0x0001;				// Should be 0x0001 for BMP
		this->BitsPerPixel = 0x0008;			// 8 bit palletized bitmap
		this->Compression = 0x00000000;			// RGB bitmap (no compression)
		this->HorizontalPPM = 0x00000000;		// Horizontal pixels per meter value (not used)
		this->VerticalPPM = 0x00000000;			// Vertical pixels per meter value (not used)
		this->ColorTabSize = 0x00000100;		// How many colors in color table
		this->ColorTabAlloc = 0x00000100;		// How many colors actually used in color table
		this->Width = Width;					// Picture Width (in pixels)
		this->Height = Height;					// Picture Height (in pixels)
		this->PixelDataSize = Height * Width;					// For this case it = Height * Width
		this->FileSize = this->PixelDataSize + this->Offset;	// For this case it = PixelDataSize + Offset
	}
};

// PVR headers
#define PVR_TWIDDLE	0x01
#define PVR_VQ		0x03
#define PVR_RECT	0x09
#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sPVRGlobalHeader
{
	ulong Signature;					// "GBIX" (0x58494247 in little endian)
	ulong ImageHeaderOffset;			// Offset to next header
	unsigned long long GlobalIndex;		// ???
};

struct sPVRImageHeader
{
	ulong Signature;					// "PVRT" signature (0x54525650 in little endian)
	ulong Size;							// Size of a rest of the file
	uchar ColorFormat;					// For DC HL should be 0x01 - RGB565
	uchar ImageFormat;					// I found 3 supported formats for DC HL: 0x01 - square twiddled, 0x03 - VQ, 0x09 - rectangle
	ushort Zeroes;						// Filled with zeroes
	ushort Width;						// Width
	ushort Height;						// Height
};

// Model texture data
#pragma pack(1)					// Eliminate unwanted 0x00 bytes
struct sTexture
{
	char Name[64];				// Texture name
	ulong Width;				// Texture width (in pixels)
	ulong Height;				// Texture height (in pixels)
	uchar * Palette;			// Texture palette
	ulong PaletteSize;			// Texture palette size
	uchar * Bitmap;				// Pointer to bitmap
	
	void Initialize()			// Initialize structure
	{
		strcpy(this->Name, "New_Texture");
		this->Width = 0;
		this->Height = 0;
		this->Palette = NULL;
		this->PaletteSize = 0;
		this->Bitmap = NULL;
	}

	void UpdateFromFile(FILE ** ptrFile, ulong FileBitmapOffset, ulong FileBitmapSize, ulong FilePaletteOffset, ulong FilePaletteSize, const char * NewName, ulong NewWidth, ulong NewHeight)	// Update from file
	{
		// Destroy old palette and bitmap
		free(Palette);
		free(Bitmap);

		// Allocate memory for new ones
		Palette = (uchar *) malloc(FilePaletteSize);
		Bitmap = (uchar *) malloc(FileBitmapSize);
		if (Palette == NULL || Bitmap == NULL)
		{
			puts("Unable to allocate memory ...");
			_getch();
			exit(EXIT_FAILURE);
		}

		// Copy data from file to memory
		FileReadBlock(ptrFile, (char *) Palette, FilePaletteOffset, FilePaletteSize);
		FileReadBlock(ptrFile, (char *) Bitmap, FileBitmapOffset, FileBitmapSize);

		// Update other fields
		strcpy_s(this->Name, sizeof(Name), NewName);
		this->Width = NewWidth;
		this->Height = NewHeight;
		this->PaletteSize = FilePaletteSize;
	}

	void FlipBitmap()		// Flip bitmap vertically. Needed for DOL\MDL to BMP conversion and vice versa.
	{
		char * NewBitmap;

		// Allocate memory for new bitmap
		NewBitmap = (char *)malloc(this->Width * this->Height);
		if (NewBitmap == NULL)
		{
			puts("Unable to allocate memory!");
			_getch();
			exit(EXIT_FAILURE);
		}

		// Copy flipped bitmap to new place
		for (int y = 0; y < this->Height; y++)
			for (int x = 0; x < this->Width; x++)
				NewBitmap[(this->Width * y) + x] = this->Bitmap[(this->Width * ((this->Height - 1) - y)) + x];

		// Destroy old bitmap
		free(this->Bitmap);

		// Save pointer to new bitmap
		this->Bitmap = (uchar *) NewBitmap;
	}

	void PaletteAddSpacers(char Spacer)		// Convert palette to DOL\BMP format
	{
		char * NewPalette;
		ulong NewPaletteSize = _8BIT_PLTE_SZ * BMP_PLTE_ENTRY_SZ;

		if (this->PaletteSize == _8BIT_PLTE_SZ * MDL_PLTE_ENTRY_SZ)
		{
			// Allocate memory for new palette
			NewPalette = (char *)malloc(NewPaletteSize);
			if (NewPalette == NULL)
			{
				puts("Unable to allocate memory!");
				_getch();
				exit(EXIT_FAILURE);
			}

			// Copy palette with spacers to new place
			int ByteCounterOld = 0;
			int ByteCounterNew = 0;
			for (ByteCounterNew = 0; ByteCounterNew < NewPaletteSize; ByteCounterNew++)	// Copy 3 bytes from old palette + Spacer to new one
			{
				if ((ByteCounterNew + 1) % 4 != 0)
				{
					NewPalette[ByteCounterNew] = this->Palette[ByteCounterOld];
					ByteCounterOld++;
				}
				else
				{
					NewPalette[ByteCounterNew] = Spacer;
				}
			}

			// Destroy old palette
			free(this->Palette);

			// Save pointer to new palette
			this->Palette = (uchar *) NewPalette;

			// Update size
			this->PaletteSize = NewPaletteSize;
		}
	}

	void PaletteSwapRedAndGreen(int ElementSize)		// Needed for MDL/DOL to BMP conversion and vice versa.
	{
		char Temp;

		for (int Element = 0; Element < _8BIT_PLTE_SZ; Element++)	// Swap 1-st and 3-rd bytes in element
		{
			Temp = this->Palette[Element * ElementSize];
			this->Palette[Element * ElementSize] = this->Palette[Element * ElementSize + 2];
			this->Palette[Element * ElementSize + 2] = Temp;
		}
	}

	bool UpdateFromPVR(FILE ** ptrFile, ulong FileOffset, const char * NewName)
	{
		ulong Offset = FileOffset;
		sPVRGlobalHeader PVRGlobalHeader;
		sPVRImageHeader PVRImageHeader;
		ulong DirectImageSz = 0;
		ushort * DirectImage;

		puts("Analyzing PVR headers ...");

		// Load first header and check
		FileReadBlock(ptrFile, &PVRGlobalHeader, Offset, sizeof(PVRGlobalHeader));
		if (PVRGlobalHeader.Signature != 0x58494247)
		{
			puts("Can't recognise global header ...");
			return false;
		}

		// Load second header and check
		Offset += sizeof(PVRGlobalHeader.Signature) + sizeof(PVRGlobalHeader.ImageHeaderOffset) + PVRGlobalHeader.ImageHeaderOffset;
		FileReadBlock(ptrFile, &PVRImageHeader, Offset, sizeof(PVRImageHeader));
		if (PVRImageHeader.Signature != 0x54525650)
		{
			puts("Can't recognise image header ...");
			return false;
		}

		// Output some info
		printf("PVR image:\n Width: %d, Height: %d\n Color type: 0x%X, Image type: 0x%X\n",
			PVRImageHeader.Width,
			PVRImageHeader.Height,
			PVRImageHeader.ColorFormat,
			PVRImageHeader.ImageFormat);

		if (PVRImageHeader.ColorFormat != 0x01)
		{
			puts("Unsupported color format ...");
			return false;
		}

		if (PVRImageHeader.ImageFormat != PVR_TWIDDLE &&
			PVRImageHeader.ImageFormat != PVR_VQ &&
			PVRImageHeader.ImageFormat != PVR_RECT)
		{
			puts("Unsupported image format ...");
			return false;
		}

		// Allocate space for 16-bit direct color image
		DirectImageSz = PVRImageHeader.Width * PVRImageHeader.Height * 2;
		DirectImage = (ushort *)malloc(DirectImageSz);

		puts("Loading PVR image ...");

		// Read 16-bit direct color image
		Offset += sizeof(sPVRImageHeader);
		if (PVRImageHeader.ImageFormat == PVR_RECT)
		{
			// Normal image
			FileReadBlock(ptrFile, DirectImage, Offset, DirectImageSz);
		}
		else if (PVRImageHeader.ImageFormat == PVR_TWIDDLE)
		{
			// Twiddled image
			ushort * TwiddledBitmap;

			// Allocate memory
			TwiddledBitmap = (ushort *)malloc(DirectImageSz);
			if (TwiddledBitmap == NULL)
			{
				puts("Memory allocation faiure!");
				return false;
			}

			// Read image
			FileReadBlock(ptrFile, TwiddledBitmap, Offset, DirectImageSz);

			// Untwiddle
			for (ushort Y = 0; Y < PVRImageHeader.Height; Y++)
				for (ushort X = 0; X < PVRImageHeader.Width; X++)
					DirectImage[Y * PVRImageHeader.Width + X] = TwiddledBitmap[TwiddleToLinear(X, Y)];

			// Free memory
			free(TwiddledBitmap);
		}
		else if (PVRImageHeader.ImageFormat == PVR_VQ)
		{
			// VQ image
			uchar * Codebook = NULL;
			ushort CodebookSz = 0x800;
			uchar * VQBitmap = NULL;
			ushort VQWidth = PVRImageHeader.Width >> 1;
			ushort VQHieght = PVRImageHeader.Height >> 1;

			// Allocate memory
			Codebook = (uchar *)malloc(CodebookSz);
			VQBitmap = (uchar *)malloc(VQWidth * VQHieght);
			if (Codebook == NULL || VQBitmap == NULL)
			{
				puts("Memory allocation faiure!");
				return false;
			}

			// Read codebook and VQ bitmap
			FileReadBlock(ptrFile, Codebook, Offset, CodebookSz);
			Offset += CodebookSz;
			FileReadBlock(ptrFile, VQBitmap, Offset, VQWidth * VQHieght);

			// Reconstruct full 16-bit bitmap
			ushort * CodebookEntry;
			uchar CodeBookEntrySz = 0x08;
			for (uint VY = 0; VY < VQHieght; VY++)
			{
				for (uint VX = 0; VX < VQWidth; VX++)
				{
					// Get index from the next pixel
					uchar VQIndex = VQBitmap[TwiddleToLinear(VX, VY)];

					// Set pointer to codebook entry (texel)
					CodebookEntry = (ushort *)&Codebook[VQIndex * CodeBookEntrySz];

					// Write texel from codebook to full bitmap
					DirectImage[(VY << 1) * PVRImageHeader.Width + (VX << 1)] = *(CodebookEntry + 0);				// Upper left
					DirectImage[(VY << 1) * PVRImageHeader.Width + (VX << 1) + 1] = *(CodebookEntry + 2);			// Uper right
					DirectImage[((VY << 1) + 1) * PVRImageHeader.Width + (VX << 1)] = *(CodebookEntry + 1);			// Bottom left
					DirectImage[((VY << 1) + 1) * PVRImageHeader.Width + (VX << 1) + 1] = *(CodebookEntry + 3);		// Bototm right
				}
			}

			// Free memory
			free(Codebook);
			free(VQBitmap);
		}

		// Destroy old palette and bitmap
		if (Palette != NULL)
			free(Palette);
		if (Bitmap != NULL)
			free(Bitmap);

		// Update properties
		strcpy(this->Name, NewName);
		this->Height = PVRImageHeader.Height;
		this->Width = PVRImageHeader.Width;
		this->PaletteSize = _8BIT_PLTE_SZ * MDL_PLTE_ENTRY_SZ;

		// Allocate new palette and bitmap
		this->Palette = (uchar *)malloc(this->PaletteSize);
		this->Bitmap = (uchar *)malloc(this->Width * this->Height);
		if (this->Palette == NULL || this->Bitmap == NULL)
		{
			puts("Memory allocation failure!");
			return false;
		}

		puts("Converting to 8-bit indexed format ...");

		// Fetch colors
		ushort Palette16[256];	// Temporary 16-bit palette
		uchar ShrinkTier = 0;
		ushort ShrinkMasks[9] = {
			0xFFFF,	// RGB565 (Full color set)
			0xFFDF,	// RGB555
			0xFFDE,	// RGB554
			0xF7DE,	// RGB454
			0xF79E,	// RGB444
			0xF79C,	// RGB443
			0xE79C,	// RGB343
			0xE71C,	// RGB333
			0xE718	// RGB332 (Forced 8-bit color set)
		};
		ushort ColorCount = 0;
		bool Complete = false;
		while (Complete == false)
		{
			// This flag would be unset if image has too many colors
			Complete = true;

			// Clear palettes
			memset(this->Palette, 0x00, this->PaletteSize);
			memset(Palette16, 0x00, sizeof(Palette16));
			ColorCount = 0;

			// Get next color shrink mask
			ushort ShrinkMask = ShrinkMasks[ShrinkTier];

			// Process image
			for (uint Y = 0; Y < this->Height; Y++)
			{
				// Take next line
				ulong LineOffset = Y * this->Width;

				for (uint X = 0; X < this->Width; X++)
				{
					// Get color of the next pixel
					ulong CurrentPixel = LineOffset + X;
					ushort CurrentColor = DirectImage[CurrentPixel];

					// Apply mask
					if (ShrinkTier != 0)
						CurrentColor &= ShrinkMask;

					// Check if color is present in the palette
					bool IsPresent = false;
					ushort ColorIndex;
					for (ColorIndex = 0; ColorIndex < ColorCount; ColorIndex++)
					{
						if (CurrentColor == Palette16[ColorIndex])
						{
							IsPresent = true;
							break;
						}
					}

					// Add color if it isn't present in the palette
					if (IsPresent == false)
					{
						if (ColorCount < 256)
						{
							// Add color to 16-bit palette
							Palette16[ColorCount] = CurrentColor;

							// Add color to 8-bit palette
							// Get components
							uchar R = CurrentColor >> 11;
							uchar G = (CurrentColor >> 5) & 0x003F;
							uchar B = CurrentColor & 0x001F;
							// Convert to 24-bit format
							R = R << 3;
							G = G << 2;
							B = B << 3;
							// Write color to palette
							this->Palette[ColorCount * MDL_PLTE_ENTRY_SZ + 0] = R;
							this->Palette[ColorCount * MDL_PLTE_ENTRY_SZ + 1] = G;
							this->Palette[ColorCount * MDL_PLTE_ENTRY_SZ + 2] = B;

							// Update index and increment color count 
							ColorIndex = ColorCount;
							ColorCount++;
						}
						else
						{
							// Too many colors
							puts("Shrinking colors ...");
							ShrinkTier++;
							Complete = false;
							break;
						}
					}

					// Put pixel index in the 8-bit indexed bitmap
					this->Bitmap[CurrentPixel] = ColorIndex;
				}

				// Retry
				if (Complete == false)
					break;
			}
		}

		// Free memory
		free(DirectImage);

		return true;
	}

	ulong TwiddleToLinear(ushort X, ushort Y)
	{
		return (Untwiddle(X) << 1) | Untwiddle(Y);
	}

	ulong Untwiddle(ulong Linear)
	{
		ulong Result = 0;
		ulong ResultBit = 1;

		// Test all set bits inside "Linear"
		while (Linear != 0)
		{
			// Test if the next bit is set
			if ((Linear & 1) != 0)
				Result |= ResultBit;	// Write converted bit to the result

			// Shift bit cursor
			Linear >>= 1;

			// Prepare next converted bit
			ResultBit <<= 2;
		}

		return Result;
	}
};
