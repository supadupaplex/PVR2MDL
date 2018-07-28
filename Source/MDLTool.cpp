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
// This file contains model conversion and texture extraction functions
//

////////// Includes //////////
#include "main.h"

////////// Global variables //////////

////////// Functions //////////
void ExtractMDLTextures(const char * FileName);																		// Extract textures from PC model
void ConvertPVRToMDL(const char * FileName);																		// Convert model from PS2 to PC format
int CheckModel(const char * FileName);																				// Check model type



int CheckModel(const char * FileName)	// Check model type
{
	FILE * ptrModelFile;

	sModelHeader ModelHeader;

	int ModelType;

	// Open model file
	SafeFileOpen(&ptrModelFile, FileName, "rb");

	// Check for dummy model (consists of signature, name and file size fields only)
	if (FileSize(&ptrModelFile) < sizeof(sModelHeader))
	{
		ModelType = DUMMY_MODEL;
	}
	else
	{
		// Load header
		ModelHeader.UpdateFromFile(&ptrModelFile);

		// Get model type
		ModelType = ModelHeader.CheckModel();
	}

	fclose(ptrModelFile);

	return ModelType;
}

void ConvertPVRToMDL(const char * FileName)		// Convert model from Dreamcast to PC format 
{
	sModelHeader ModelHeader;					// Model file header
	sModelTextureEntry * ModelTextureTable;		// Model texture table
	ulong ModelTextureTableSize;				// Model texture table size (how many textures)
	sTexture * Textures;						// Pointer to textures data

	FILE * ptrInFile;
	char cNewModelName[64];
	FILE * ptrOutFile;
	char cInFileName[255];

	ulong ModelSize;

	// Backup original file
	FileGetFullName(FileName, cInFileName, sizeof(cInFileName));
	strcat(cInFileName, "-backup.mdl");
	FileSafeRename((char *) FileName, cInFileName);

	// Open file
	SafeFileOpen(&ptrInFile, cInFileName, "rb");

	// Get header from file
	ModelHeader.UpdateFromFile(&ptrInFile);

	// Check model
	if (ModelHeader.CheckModel() == NORMAL_MODEL)
	{
		printf("Internal name: %s \nTextures: %i, Texture table offset: 0x%X \n", ModelHeader.Name, ModelHeader.TextureCount, ModelHeader.TextureTableOffset);
	}
	else
	{
		puts("Incorrect model file.");

		// Restore original file
		fclose(ptrInFile);
		FileSafeRename(cInFileName, (char *) FileName);
		
		return;
	}

	// Allocate memory for textures
	ModelTextureTableSize = ModelHeader.TextureCount * sizeof(sModelTextureEntry);
	ModelTextureTable = (sModelTextureEntry *)malloc(ModelTextureTableSize);
	Textures = (sTexture *)malloc(sizeof(sTexture) * ModelHeader.TextureCount);

	// Load and convert textures
	uint BitmapOffset;
	uint BitmapSize;
	uint PaletteOffset;
	uint PaletteSize;
	for (int i = 0; i < ModelHeader.TextureCount; i++)
	{
		bool Result;
		char NewName[64];

		ModelTextureTable[i].UpdateFromFile(&ptrInFile, ModelHeader.TextureTableOffset, i);
		printf("\nTexture #%i \nName: %s \n", i + 1, ModelTextureTable[i].Name);

		// Check for PVR textures
		char Extension[5];
		FileGetExtension(ModelTextureTable[i].Name, Extension, sizeof(Extension));
		if (!strcmp(Extension, ".bmp") == true)
		{
			puts("Normal model, ignoring ...");

			// Restore original file
			fclose(ptrInFile);
			FileSafeRename(cInFileName, (char *)FileName);

			return;
		}

		// Load & convert texture
		FileGetName(ModelTextureTable[i].Name, NewName, sizeof(NewName), false);
		strcat(NewName, ".bmp");
		strcpy(ModelTextureTable[i].Name, NewName);
		Textures[i].Initialize();
		Result = Textures[i].UpdateFromPVR(&ptrInFile, ModelTextureTable[i].Offset, ModelTextureTable[i].Name);
		if (Result == false)
		{
			printf("Warning: can't recognise texture: %s.\nPress any key to exit ...", ModelTextureTable[i].Name);
			getchar();

			// Restore original file
			fclose(ptrInFile);
			FileSafeRename(cInFileName, (char *)FileName);

			return;
		}
	}

	// Write results to output file
	// Open output file
	SafeFileOpen(&ptrOutFile, FileName, "wb");

	// Write modified header
	ModelHeader.TextureDataOffset = ModelHeader.TextureTableOffset + sizeof(sModelTextureEntry) * ModelHeader.TextureCount + ModelHeader.SkinCount * ModelHeader.SkinEntrySize * 2;
	FileWriteBlock(&ptrOutFile, (char *)&ModelHeader, sizeof(sModelHeader));

	// Write model data
	uchar * ModelData;
	ModelData = (uchar *)malloc(ModelHeader.TextureTableOffset - sizeof(sModelHeader));
	FileReadBlock(&ptrInFile, (char *)ModelData, sizeof(sModelHeader), ModelHeader.TextureTableOffset - sizeof(sModelHeader));
	FileWriteBlock(&ptrOutFile, (char *)ModelData, ModelHeader.TextureTableOffset - sizeof(sModelHeader));
	free(ModelData);

	// Write modified texture table
	uint Offset = ModelHeader.TextureDataOffset;
	for (int i = 0; i < ModelHeader.TextureCount; i++)
	{
		ModelTextureTable[i].Width = Textures[i].Width;
		ModelTextureTable[i].Height = Textures[i].Height;
		ModelTextureTable[i].Offset = Offset;

		Offset += Textures[i].Width * Textures[i].Height + Textures[i].PaletteSize;
	}
	FileWriteBlock(&ptrOutFile, (char *)ModelTextureTable, ModelTextureTableSize);

	// Write skin data
	uchar * SkinTable;
	ulong SkinTableSize = ModelHeader.SkinCount * ModelHeader.SkinEntrySize * 2;
	SkinTable = (uchar *)malloc(SkinTableSize);
	FileReadBlock(&ptrInFile, SkinTable, ModelHeader.SkinTableOffset, SkinTableSize);
	FileWriteBlock(&ptrOutFile, SkinTable, SkinTableSize);
	free(SkinTable);

	// Write textures
	for (int i = 0; i < ModelHeader.TextureCount; i++)
	{
		FileWriteBlock(&ptrOutFile, (char *)Textures[i].Bitmap, Textures[i].Width * Textures[i].Height);		
		FileWriteBlock(&ptrOutFile, (char *)Textures[i].Palette, Textures[i].PaletteSize);
	}

	// Update model size field
	ModelSize = FileSize(&ptrOutFile);
	FileWriteBlock(&ptrOutFile, &ModelSize, 0x48, sizeof(ModelSize));	// 0x48 - address of model size field

	// Free memory
	free(ModelTextureTable);
	free(Textures);
	
	// Close files
	fclose(ptrInFile);
	fclose(ptrOutFile);

	puts("\nDone!\n\n\n");
}

void ExtractMDLTextures(const char * FileName)	// Extract textures from PC model
{
	sModelHeader ModelHeader;					// Model file header
	sModelTextureEntry * ModelTextureTable;		// Model texture table
	ulong ModelTextureTableSize;				// Model texture table size (how many textures)
	sTexture * Textures;						// Pointer to textures data

	FILE * ptrInFile;
	sBMPHeader BMPHeader;						// BMP header
	FILE * ptrBMPOutput;
	char cOutFileName[255];
	char cOutFolderName[255];

	// Open file
	SafeFileOpen(&ptrInFile, FileName, "rb");

	// Load model header
	ModelHeader.UpdateFromFile(&ptrInFile);

	// Check model
	if (ModelHeader.CheckModel() == NORMAL_MODEL)
	{
		printf("Internal name: %s \nTextures: %i, Texture table offset: 0x%X \n", ModelHeader.Name, ModelHeader.TextureCount, ModelHeader.TextureTableOffset);
	}
	else
	{
		puts("Can't extract textures.");
		return;
	}

	// Allocate memory for textutes
	ModelTextureTable = (sModelTextureEntry *)malloc(ModelHeader.TextureCount * sizeof(sModelTextureEntry));
	Textures = (sTexture *)malloc(sizeof(sTexture) * ModelHeader.TextureCount);

	// Prepare folder for output files
	strcpy(cOutFolderName, FileName);
	strcat(cOutFolderName, "-textures\\");
	NewDir(cOutFolderName);

	uint BitmapOffset;
	uint BitmapSize;
	uint PaletteOffset;
	uint PaletteSize;
	bool PVRExtract = false;
	char TexExtension[5];
	for (int i = 0; i < ModelHeader.TextureCount; i++)
	{
		ModelTextureTable[i].UpdateFromFile(&ptrInFile, ModelHeader.TextureTableOffset, i);
		printf("\n\nTexture #%i \n Name: %s \n Width: %i \n Height: %i \n Offset: %x \n", i + 1, ModelTextureTable[i].Name, ModelTextureTable[i].Width, ModelTextureTable[i].Height, ModelTextureTable[i].Offset);

		// PVR check
		if (PVRExtract == false)
		{
			FileGetExtension(ModelTextureTable[i].Name, TexExtension, sizeof(TexExtension));
			if (!strcmp(TexExtension, ".pvr") == true)
			{
				PVRExtract = true;
				puts("Found PVR textures ...");
			}
		}

		// Extract texture
		if (PVRExtract == false)
		{
			// Normal texture //

			BitmapOffset = ModelTextureTable[i].Offset;
			BitmapSize = ModelTextureTable[i].Height * ModelTextureTable[i].Width;
			PaletteOffset = ModelTextureTable[i].Offset + ModelTextureTable[i].Width * ModelTextureTable[i].Height;
			PaletteSize = _8BIT_PLTE_SZ * MDL_PLTE_ENTRY_SZ;

			// Load texture
			Textures[i].Initialize();
			Textures[i].UpdateFromFile(&ptrInFile, BitmapOffset, BitmapSize, PaletteOffset, PaletteSize, ModelTextureTable[i].Name, ModelTextureTable[i].Width, ModelTextureTable[i].Height);
		}
		else
		{
			// PVR texture //
			bool Result;

			// Load texture
			Textures[i].Initialize();
			Result = Textures[i].UpdateFromPVR(&ptrInFile, ModelTextureTable[i].Offset, ModelTextureTable[i].Name);
			if (Result == false)
			{
				printf("Warning: can't recognise texture: %s.\nPress any key to confirm ...", ModelTextureTable[i].Name);
				getchar();
				continue;
			}
		}

		// Prepare texture to be saved in BMP format
		Textures[i].FlipBitmap();
		Textures[i].PaletteSwapRedAndGreen(MDL_PLTE_ENTRY_SZ);
		Textures[i].PaletteAddSpacers(0x00);

		// Save texture to *.bmp file
		char Name[64];
		FileGetName(ModelTextureTable[i].Name, Name, sizeof(Name), false);
		strcat(Name, ".bmp");
		strcpy(cOutFileName, cOutFolderName);
		strcat(cOutFileName, Name);
		SafeFileOpen(&ptrBMPOutput, cOutFileName, "wb");

		BMPHeader.Update(Textures[i].Width, Textures[i].Height);
		FileWriteBlock(&ptrBMPOutput, (char *)&BMPHeader, sizeof(sBMPHeader));
		FileWriteBlock(&ptrBMPOutput, (char *)Textures[i].Palette, Textures[i].PaletteSize);
		FileWriteBlock(&ptrBMPOutput, (char *)Textures[i].Bitmap, Textures[i].Width * Textures[i].Height);

		// Close output file
		fclose(ptrBMPOutput);
	}

	// Free memory
	free(ModelTextureTable);
	free(Textures);

	// Close files
	fclose(ptrInFile);

	puts("\nDone!\n\n\n");
}

int main(int argc, char * argv[])
{
	FILE * ptrInputFile;

	// Output info
	printf("\nPVR2MDL v%s \n", PROG_VERSION);

	// Check arguments
	if (argc == 1)
	{
		// No arguments - show help screen
		puts("\nDeveloped by Alexey Leusin. \nCopyright (c) 2018, Alexey Leushin. All rights reserved.\n");
		puts("How to use: \n1) Windows explorer - drag and drop model file on pvr2mdl.exe \n2) Command line/Batch - pvr2mdl [model_file_name] \nOptional feature: extract textures - pvr2mdl extract [model_file_name]  \n\nFor more info read ReadMe.txt \n");
		puts("Press any key to exit ...");

		_getch();
	}
	else if (argc == 2)		// Convert model
	{
		char cFileExtension[5];

		FileGetExtension(argv[1], cFileExtension, 5);

		printf("\nProcessing file: %s\n", argv[1]);

		if (!strcmp(".mdl", cFileExtension))
		{
			if (CheckModel(argv[1]) == NORMAL_MODEL)
			{
				ConvertPVRToMDL(argv[1]);
			}
			else if (CheckModel(argv[1]) == SEQ_MODEL || CheckModel(argv[1]) == NOTEXTURES_MODEL || CheckModel(argv[1]) == DUMMY_MODEL)
			{
				puts("Can't find texture data ...");
			}
			else
			{
				puts("Can't recognise model file ...");
			}
		}
		else
		{
			puts("Wrong file extension.");
		}
	}
	else if (argc == 3 && !strcmp(argv[1], "extract") == true)		// Extract textures from model
	{
		char cFileExtension[5];

		FileGetExtension(argv[2], cFileExtension, 5);

		printf("\nProcessing file: %s\n", argv[2]);

		if (!strcmp(".mdl", cFileExtension))
		{
			if (CheckModel(argv[2]) == NORMAL_MODEL)
				ExtractMDLTextures(argv[2]);
			else
				puts("Can't find texture data ...");
		}
		else
		{
			puts("Wrong file extension.");
		}
	}
	else
	{
		puts("Can't recognise arguments.");
	}

	//getchar();
}
