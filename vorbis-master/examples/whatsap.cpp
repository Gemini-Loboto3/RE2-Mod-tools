#define _CRT_SECURE_NO_WARNINGS

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <Shlwapi.h>
#include <string>
#include <vector>

#include "..\win32\VS2010\vorbisenc\dec.h"

#pragma comment(lib, "shlwapi.lib")

bool CreateRecursiveDirectory(const char* filepath, const int max_level)
{
	bool result = false;
	char path_copy[MAX_PATH] = { 0 };
	strcat_s(path_copy, MAX_PATH, filepath);
	std::vector<std::string> path_collection;

	for (int level = 0; PathRemoveFileSpecA(path_copy) && level < max_level; level++)
	{
		path_collection.push_back(path_copy);
	}
	for (int i = path_collection.size() - 1; i >= 0; i--)
	{
		if (CreateDirectoryA(path_collection[i].c_str(), NULL))
			result = true;
	}
	return result;
}

int createDirectoryRecursively(char *path)
{
	char *npath;
	int len = strlen(path);

	npath = _strdup(path);
	int ret = CreateRecursiveDirectory(npath, 255);

	free(npath);
	return ret;
}

void list_sap(std::vector<std::string> &str, LPCSTR folder)
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char szDir[MAX_PATH];
	
	sprintf(szDir, "%s\\*.sap", folder);
	WIN32_FIND_DATAA ffd;

	// find the first file in the directory
	hFind = FindFirstFileA(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
		return;

	// list all mod files in the main folder
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			str.push_back(ffd.cFileName);
		}
	} while (FindNextFileA(hFind, &ffd) != 0);

	FindClose(hFind);
}

void longtobin(unsigned long hex, char *dst)
{
	char *s = dst;
	int clz = 1;

	for (int i = 0; i < 32; i++, hex <<= 1)
	{
		if (clz && (hex & 0x80000000) == 0)
			continue;
		if (clz && (hex & 0x80000000) == 1)
			clz = 0;
		*s++ = hex & 0x80000000 ? '1' : '0';
	}
	*s = '\0';
}

void extract_SAP(char* infile, char* outfolder)
{
	char outname[260];
	FILE *in = fopen(infile, "rb+");
	if (!in)
	{
		printf("Can't open %s.\n", infile);
		return;
	}

	CreateDirectoryA(outfolder, NULL);

	sprintf(outname, "%s\\config.ini", outfolder);
	FILE *ini = fopen(outname, "wt+");

	unsigned int flg;
	char bin[16];

	fread(&flg, 4, 1, in);
	longtobin(flg, bin);
	fprintf(ini, "[DATA]\nFlags = %s\nUse_ogg = 0\n\n[FILES]\n", bin);
	fread(&flg, 4, 1, in);

	for (int i = 0; i < 32; i++)
	{
		sprintf(outname, "%s\\%03d.wav", outfolder, i);

		WAV_CHUNK head;
		if (fread(&head, sizeof(head), 1, in) == 0)
			break;
		fprintf(ini, "%03d.wav\n", i);
		FILE *out = fopen(outname, "wb+");
		fwrite(&head, sizeof(head), 1, out);
		char *buffer = (char*)malloc(head.size);
		fread(buffer, head.size, 1, in);
		fwrite(buffer, head.size, 1, out);
		free(buffer);

		fclose(out);
	}

	fclose(ini);
	fclose(in);
}

void sap_to_ogg(LPCSTR in_name, LPCSTR out_name, float quality)
{
	FILE *in = fopen(in_name, "rb+");
	FILE *out = fopen(out_name, "wb+");

	if (!in)
	{
		printf("Can't read %s\n", in_name);
		return;
	}
	if (!out)
	{
		printf("Can't create %s\n", out_name);
		return;
	}

	fseek(in, 0, SEEK_END);
	size_t fsize = ftell(in);
	fseek(in, 0, SEEK_SET);

	if (fsize == 0)
		return;

	unsigned long h[2];
	fread(h, sizeof(h), 1, in);
	fwrite(h, sizeof(h), 1, out);

	long ptr[33];
	int i;
	for (i = 0; i < 32; i++)
	{
		ptr[i] = ftell(out);
		if ((size_t)ftell(in) >= fsize - sizeof(wav_hdr))
			break;
		convert(in, out, quality);
	}
	ptr[i + 1] = ftell(out);

	// write pointers
	fwrite(ptr, (i + 1) * 4, 1, out);
	fwrite(&i, 4, 1, out);

	fclose(out);
	fclose(in);

	printf("\rDone.          \n");
}

void batch_to_ogg(LPCSTR in_folder, LPCSTR out_folder, float quality)
{
	std::vector<std::string> list;
	list_sap(list, in_folder);

	for (size_t i = 0, si = list.size(); i < si; i++)
	{
		char in_name[MAX_PATH], out_name[MAX_PATH];
		sprintf(in_name, "%s\\%s", in_folder, list[i].c_str());
		sprintf(out_name, "%s\\%s", out_folder, list[i].c_str());
		createDirectoryRecursively(out_name);
		sap_to_ogg(in_name, out_name, quality);
	}
}

void print_usage()
{
	printf("whatSAP - a stupid converter for a stupid format.\n"
		"Usage: whatSAP <option> <input> <output>\n\n"
		"options:\n"
		" -e: extract SAP contents [-e in.sap outfolder]\n"
		" -p: pack SAP from a list [-p infolder out.sap]\n"
		" -c: convert to SAPx [-c in.sap out.sap quality<0.1-1.0>]\n"
		" -b: batch convert *.SAP [-b infolder outfolder]");
}

int main(int argc, char *argv[])
{
#if _DEBUG
	batch_to_ogg("D:\\Program Files\\BIOHAZARD 2 PC\\mod_vorbis\\HQ\\sound\\core",
		"D:\\Program Files\\BIOHAZARD 2 PC\\mod_vorbis\\common\\sound\\core", 1.0f);
#endif

	if (argc <= 1)
	{
		print_usage();
		return 0;
	}

	float quality;

	if (argv[1][0] == '-')
	{
		switch (argv[1][1])
		{
		case 'e':
			if (argc == 4)
			{
				extract_SAP(argv[2], argv[3]);
				return 0;
			}
			break;
		case 'p':
			if(argc == 4)
			{
			}
			break;
		case 'c':
			if (argc >= 4)
			{
				if (argc == 4) quality = 1.f;
				else quality = (float)atof(argv[4]);
				sap_to_ogg(argv[2], argv[3], 1.0f);
				return 0;
			}
			break;
		case 'b':
			if (argc >= 4)
			{
				if (argc == 4) quality = 1.0f;
				else quality = (float)atof(argv[4]);
				printf("Batch conversion [%s] to [%s]\n", argv[2], argv[3]);
				batch_to_ogg(argv[2], argv[3], quality);
				return 0;
			}
			break;
		}
	}

	print_usage();
}
