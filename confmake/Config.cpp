#define _CRT_SECURE_NO_WARNINGS

#include <stdafx.h>
#include "gfile.h"
#include "Config.h"
#include "ini.h"

static void QueryKey(HKEY hKey, std::vector<std::string> &dst);

CConfig::CConfig()
{
	bLoaded = false;
}

CConfig::~CConfig()
{
	if(bLoaded)
		WriteIni();
}

void CConfig::Init()
{
	path = "";

	savePath = path + "\\savedata";

	int hasReg = HasRegistry(),
		hasIni = HasIni();

	// ini is present, takes priority
	if (hasIni)
	{
		bLoaded = false;
		GFile f;
		f.Open("config.ini", GFile::modeRead | GFile::shareDenyNone);
		printf("INI configuration already exists (%s).\n", f.GetFilePath().c_str());
		return;
		//CreateFromIni();		// load ini
	}
	// no ini but registry is there
	else if (!hasIni && hasReg)
	{
		printf("No INI configuration was detected.\nA new INI will now be created from installed data.");
		CreateFromRegistry();	// parse registry
	}
	// no ini or registry present
	else if (!hasIni && !hasReg)
	{
		printf("Couldn't detect registry or ini settings.\nDefault settings will now be loaded.\nPlease open config.ini and insert a valid installation key.");
		CreateDefault();		// try to create a default configuration
	}

	bLoaded = true;
}

int CConfig::HasRegistry()
{
	HKEY phkResult;
	// try regular 32 bit mode
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\CAPCOM\\BIOHAZARD 2 PC", 0, KEY_READ, &phkResult) != ERROR_SUCCESS)
		// try on 64 bit machine
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\CAPCOM\\BIOHAZARD 2 PC", 0, KEY_READ, &phkResult) != ERROR_SUCCESS)
			return 0;

	RegCloseKey(phkResult);
	return 1;
}

int CConfig::HasIni()
{
	GFile f;
	//FILE *f = fopen("config.ini", "rb+");
	f.Open("config.ini", GFile::modeRead | GFile::shareDenyNone);
	if (f.GetHandle() == INVALID_HANDLE_VALUE)
	{
		//fclose(f);
		f.Close();
		return 0;
	}
	f.Close();
	return 1;
}

void CConfig::CreateDefault()
{
	data1 = "0.000000";
	data2 = "0.000000";
	data3 = 0;
	data4 = 0;

	exFlag = 0;
	exTime00 = 0;
	exTime01 = 0;
	exTime02 = 0;
	exTime10 = 0;
	exTime11 = 0;
	exTime12 = 0;
	exTime20 = 0;
	exTime21 = 0;
	exTime22 = 0;
	gallery0 = 0xffffffff;
	gallery1 = 0xfffffff0;
	gallery2 = 0xfffffffe;
	gallery3 = 0xffffffff;
	gallery4 = 0xffffffff;
	graphicFadeflag = 1;
	graphicPerspectiveCorrect = 1;
	graphicSprdataswitch = 2;
	graphicTextureInterpolation = 1;
	dsoundEnable = 1;
	soundChannels = 2;
	soundDepth = 16;
	soundFrequency = 44100;
	movieEnable = 1;
	special = 0xf;
	superHardSwitch = 0;
	PushBinaryStr("KeyDef", std::string("68 62 64 66 00 4D 00 49 00 00 4F 4A 5A 4B 00 00 00 00 00 00 00 00 00 00 00 26 28 25 27 0D 20 1B"));

	installKey = "BHxxxxxxxxxxxxxx";
	displayMode = "640x480 0bpp full:0";
	driverMode = "Direct3D HAL";
}

int CConfig::CreateFromIni()
{
	ini_t* ini = ini_load("config.ini");
	if (!ini)
		return 0;

	const char *got;
	got = ini_get(ini, "GAME", "InstallKey"); SetString("InstallKey", got);
	got = ini_get(ini, "GAME", "DATA1"); SetString("DATA1", got);
	got = ini_get(ini, "GAME", "DATA2"); SetString("DATA2", got);
	got = ini_get(ini, "GAME", "DisplayMode"); SetString("DisplayMode", got);
	got = ini_get(ini, "GAME", "DriverMode"); SetString("DriverMode", got);
	DWORD dword;
	ini_sget(ini, "GAME", "DATA3", "%d", &dword); SetDword("DATA3", dword);
	ini_sget(ini, "GAME", "DATA4", "%d", &dword); SetDword("DATA4", dword);
	ini_sget(ini, "GAME", "DirectSoundEnable", "%d", &dword); SetDword("DirectSoundEnable", dword);
	ini_sget(ini, "GAME", "MovieEnable", "%d", &dword); SetDword("MovieEnable", dword);
	ini_sget(ini, "GAME", "ExFlag", "0x%x", &dword); SetDword("ExFlag", dword);
	ini_sget(ini, "GAME", "ExTime00", "0x%x", &dword); SetDword("ExTime00", dword);
	ini_sget(ini, "GAME", "ExTime01", "0x%x", &dword); SetDword("ExTime01", dword);
	ini_sget(ini, "GAME", "ExTime02", "0x%x", &dword); SetDword("ExTime02", dword);
	ini_sget(ini, "GAME", "ExTime10", "0x%x", &dword); SetDword("ExTime10", dword);
	ini_sget(ini, "GAME", "ExTime11", "0x%x", &dword); SetDword("ExTime11", dword);
	ini_sget(ini, "GAME", "ExTime12", "0x%x", &dword); SetDword("ExTime12", dword);
	ini_sget(ini, "GAME", "ExTime20", "0x%x", &dword); SetDword("ExTime20", dword);
	ini_sget(ini, "GAME", "ExTime21", "0x%x", &dword); SetDword("ExTime21", dword);
	ini_sget(ini, "GAME", "ExTime22", "0x%x", &dword); SetDword("ExTime22", dword);
	ini_sget(ini, "GAME", "GALLERY0", "0x%x", &dword); SetDword("GALLERY0", dword);
	ini_sget(ini, "GAME", "GALLERY1", "0x%x", &dword); SetDword("GALLERY1", dword);
	ini_sget(ini, "GAME", "GALLERY2", "0x%x", &dword); SetDword("GALLERY2", dword);
	ini_sget(ini, "GAME", "GALLERY3", "0x%x", &dword); SetDword("GALLERY3", dword);
	ini_sget(ini, "GAME", "GALLERY4", "0x%x", &dword); SetDword("GALLERY4", dword);
	ini_sget(ini, "GAME", "GraphicFadeflag", "%d", &dword); SetDword("GraphicFadeflag", dword);
	ini_sget(ini, "GAME", "GraphicPerspectiveCorrect", "%d", &dword); SetDword("GraphicPerspectiveCorrect", dword);
	ini_sget(ini, "GAME", "GraphicSprdataswitch", "%d", &dword); SetDword("GraphicSprdataswitch", dword);
	ini_sget(ini, "GAME", "GraphicTextureInterpolation", "%d", &dword); SetDword("GraphicTextureInterpolation", dword);
	ini_sget(ini, "GAME", "SoundChannels", "%d", &dword); SetDword("SoundChannels", dword);
	ini_sget(ini, "GAME", "SoundDepth", "%d", &dword); SetDword("SoundDepth", dword);
	ini_sget(ini, "GAME", "SoundFrequency", "%d", &dword); SetDword("SoundFrequency", dword);
	ini_sget(ini, "GAME", "SPECIAL", "0x%x", &dword); SetDword("SPECIAL", dword);
	ini_sget(ini, "GAME", "SuperHardSwitch", "%d", &dword); SetDword("SuperHardSwitch", dword);
	// needs code for joypads!!
	std::vector<std::string> keys, values;
	got = ini_get(ini, "GAME", "KeyDef"); keys.push_back("KeyDef"); values.push_back(got);
	ini_get_seq(ini, "GAME", "JoyDef_", keys, values);
	for (size_t i = 0, si = keys.size(); i < si; i++)
		PushBinaryStr(keys[i].c_str(), values[i]);

	dword = 0; ini_sget(ini, "DLL", "DinputEnable",    "%d", &dword); dinputEnable = dword;
	dword = 0; ini_sget(ini, "DLL", "JapaneseEnable",  "%d", &dword); japaneseEnable = dword;
	dword = 1; ini_sget(ini, "DLL", "QuickturnEnable", "%d", &dword); quickturnEnable = dword;
	dword = 0; ini_sget(ini, "DLL", "FocusIgnore",     "%d", &dword); focusIgnore = dword;
	got = ini_get(ini, "DLL", "ColorMode");

	if(got == NULL) colorMode = 1;	// full is default
	else if (strcmp(got, "psx") == 0)    colorMode = 0;
	else if (strcmp(got, "full") == 0)   colorMode = 1;
	else if (strcmp(got, "legacy") == 0) colorMode = 2;
	else if (strcmp(got, "full2") == 0)  colorMode = 3;
	else colorMode = 1;	// full is default

	return 1;
}

int CConfig::CreateFromRegistry()
{
	HKEY hKey = HKEY_LOCAL_MACHINE;
	HKEY phkResult;

	// try regular 32 bit mode
	if (RegOpenKeyExA(hKey, "Software\\CAPCOM\\BIOHAZARD 2 PC", 0, KEY_READ, &phkResult) != ERROR_SUCCESS)
		// try on 64 bit machine
		if (RegOpenKeyExA(hKey, "Software\\Wow6432Node\\CAPCOM\\BIOHAZARD 2 PC", 0, KEY_READ, &phkResult) != ERROR_SUCCESS)
			return 0;

	// get all JoyDef_ values
	std::vector<std::string> joys;
	QueryKey(phkResult, joys);
	// read into object now
	char str[MAX_PATH];
	DWORD size, dword, type = REG_SZ;
	while (RegQueryValueExA(phkResult, "InstallKey", NULL, &type, (LPBYTE)str, &size)); SetString("InstallKey", str);
	while (RegQueryValueExA(phkResult, "SavePath", NULL, &type, (LPBYTE)str, &size)); SetString("SavePath", str);
	while (RegQueryValueExA(phkResult, "DATA1", NULL, &type, (LPBYTE)str, &size)); SetString("DATA1", str);
	while (RegQueryValueExA(phkResult, "DATA2", NULL, &type, (LPBYTE)str, &size)); SetString("DATA2", str);
	while (RegQueryValueExA(phkResult, "DisplayMode", NULL, &type, (LPBYTE)str, &size)); SetString("DisplayMode", str);
	while (RegQueryValueExA(phkResult, "DriverMode", NULL, &type, (LPBYTE)str, &size)); SetString("DriverMode", str);
	type = REG_DWORD;
	while (RegQueryValueExA(phkResult, "DATA3", NULL, &type, (LPBYTE)&dword, &size)); SetDword("DATA3", dword);
	while (RegQueryValueExA(phkResult, "DATA4", NULL, &type, (LPBYTE)&dword, &size)); SetDword("DATA4", dword);
	while (RegQueryValueExA(phkResult, "DirectSoundEnable", NULL, &type, (LPBYTE)&dword, &size)); SetDword("DirectSoundEnable", dword);
	while (RegQueryValueExA(phkResult, "MovieEnable", NULL, &type, (LPBYTE)&dword, &size)); SetDword("MovieEnable", dword);
	while (RegQueryValueExA(phkResult, "ExFlag", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExFlag", dword);
	while (RegQueryValueExA(phkResult, "ExTime00", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime00", dword);
	while (RegQueryValueExA(phkResult, "ExTime01", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime01", dword);
	while (RegQueryValueExA(phkResult, "ExTime02", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime02", dword);
	while (RegQueryValueExA(phkResult, "ExTime10", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime10", dword);
	while (RegQueryValueExA(phkResult, "ExTime11", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime11", dword);
	while (RegQueryValueExA(phkResult, "ExTime12", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime12", dword);
	while (RegQueryValueExA(phkResult, "ExTime20", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime20", dword);
	while (RegQueryValueExA(phkResult, "ExTime21", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime21", dword);
	while (RegQueryValueExA(phkResult, "ExTime22", NULL, &type, (LPBYTE)&dword, &size)); SetDword("ExTime22", dword);
	while (RegQueryValueExA(phkResult, "GALLERY0", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GALLERY0", dword);
	while (RegQueryValueExA(phkResult, "GALLERY1", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GALLERY1", dword);
	while (RegQueryValueExA(phkResult, "GALLERY2", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GALLERY2", dword);
	while (RegQueryValueExA(phkResult, "GALLERY3", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GALLERY3", dword);
	while (RegQueryValueExA(phkResult, "GALLERY4", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GALLERY4", dword);
	while (RegQueryValueExA(phkResult, "GraphicFadeflag", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GraphicFadeflag", dword);
	while (RegQueryValueExA(phkResult, "GraphicPerspectiveCorrect", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GraphicPerspectiveCorrect", dword);
	while (RegQueryValueExA(phkResult, "GraphicSprdataswitch", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GraphicSprdataswitch", dword);
	while (RegQueryValueExA(phkResult, "GraphicTextureInterpolation", NULL, &type, (LPBYTE)&dword, &size)); SetDword("GraphicTextureInterpolation", dword);
	while (RegQueryValueExA(phkResult, "SoundChannels", NULL, &type, (LPBYTE)&dword, &size)); SetDword("SoundChannels", dword);
	while (RegQueryValueExA(phkResult, "SoundDepth", NULL, &type, (LPBYTE)&dword, &size)); SetDword("SoundDepth", dword);
	while (RegQueryValueExA(phkResult, "SoundFrequency", NULL, &type, (LPBYTE)&dword, &size)); SetDword("SoundFrequency", dword);
	while (RegQueryValueExA(phkResult, "SPECIAL", NULL, &type, (LPBYTE)&dword, &size)); SetDword("SPECIAL", dword);
	while (RegQueryValueExA(phkResult, "SuperHardSwitch", NULL, &type, (LPBYTE)&dword, &size)); SetDword("SuperHardSwitch", dword);
	type = REG_BINARY;
	u8 def[128];
	while (RegQueryValueExA(phkResult, "KeyDef", NULL, &type, (LPBYTE)def, &size)); SetBinary("KeyDef", def, size);
	for (size_t i = 0, si = joys.size(); i < si; i++)
	{
		while (RegQueryValueExA(phkResult, joys[i].c_str(), NULL, &type, (LPBYTE)def, &size));
		SetBinary(joys[i].c_str(), def, size);
	}

	RegCloseKey(phkResult);

	dinputEnable = 0;
	japaneseEnable = 0;
	quickturnEnable = 0;
	focusIgnore = 0;
	colorMode = 1;

	return 1;
}

void CConfig::GetString(const char *name, char *dst)
{
	if (strcmp(name, "InstallKey") == 0) strcpy(dst, installKey.c_str());
	if (strcmp(name, "SavePath") == 0) strcpy(dst, savePath.c_str());
	if (strcmp(name, "DATA1") == 0) strcpy(dst, data1.c_str());
	if (strcmp(name, "DATA2") == 0) strcpy(dst, data2.c_str());
	if (strcmp(name, "DisplayMode") == 0) strcpy(dst, displayMode.c_str());
	if (strcmp(name, "DriverMode") == 0) strcpy(dst, driverMode.c_str());
}

void CConfig::SetString(const char *name, const char *str)
{
	if (strcmp(name, "InstallKey") == 0) installKey = str;
	if (strcmp(name, "SavePath") == 0) savePath = str;
	if (strcmp(name, "DATA1") == 0) data1 = str;
	if (strcmp(name, "DATA2") == 0) data2 = str;
	if (strcmp(name, "DisplayMode") == 0) displayMode = str;
	if (strcmp(name, "DriverMode") == 0) driverMode = str;
}

void CConfig::GetDword(const char *name, DWORD *dst)
{
	if (strcmp(name, "DATA3") == 0) *dst = data3;
	else if (strcmp(name, "DATA4") == 0) *dst = data4;
	else if (strcmp(name, "DirectSoundEnable") == 0) *dst = dsoundEnable;
	else if (strcmp(name, "MovieEnable") == 0) *dst = movieEnable;
	else if (strcmp(name, "ExFlag") == 0) *dst = exFlag;
	else if (strcmp(name, "ExTime00") == 0) *dst = exTime00;
	else if (strcmp(name, "ExTime01") == 0) *dst = exTime01;
	else if (strcmp(name, "ExTime02") == 0) *dst = exTime02;
	else if (strcmp(name, "ExTime10") == 0) *dst = exTime10;
	else if (strcmp(name, "ExTime11") == 0) *dst = exTime11;
	else if (strcmp(name, "ExTime12") == 0) *dst = exTime12;
	else if (strcmp(name, "ExTime20") == 0) *dst = exTime20;
	else if (strcmp(name, "ExTime21") == 0) *dst = exTime21;
	else if (strcmp(name, "ExTime22") == 0) *dst = exTime22;
	else if (strcmp(name, "GALLERY0") == 0) *dst = gallery0;
	else if (strcmp(name, "GALLERY1") == 0) *dst = gallery1;
	else if (strcmp(name, "GALLERY2") == 0) *dst = gallery2;
	else if (strcmp(name, "GALLERY3") == 0) *dst = gallery3;
	else if (strcmp(name, "GALLERY4") == 0)
		*dst = gallery4;
	else if (strcmp(name, "GraphicFadeflag") == 0) *dst = graphicFadeflag;
	else if (strcmp(name, "GraphicPerspectiveCorrect") == 0) *dst = graphicPerspectiveCorrect;
	else if (strcmp(name, "GraphicSprdataswitch") == 0) *dst = graphicSprdataswitch;
	else if (strcmp(name, "GraphicTextureInterpolation") == 0) *dst = graphicTextureInterpolation;
	else if (strcmp(name, "SoundChannels") == 0) *dst = soundChannels;
	else if (strcmp(name, "SoundDepth") == 0) *dst = soundDepth;
	else if (strcmp(name, "SoundFrequency") == 0) *dst = soundFrequency;
	else if (strcmp(name, "SPECIAL") == 0) *dst = special;
	else if (strcmp(name, "SuperHardSwitch") == 0) *dst = superHardSwitch;
	else
	{
		char str[128];
		sprintf(str, "Could not read %s\n", name);
		OutputDebugStringA(str);
		*dst = 0xdeadbeef;
	}
}

void CConfig::SetDword(const char *name, DWORD dword)
{
	if (strcmp(name, "DATA3") == 0) data3 = dword;
	else if (strcmp(name, "DATA4") == 0) data4 = dword;
	else if (strcmp(name, "DirectSoundEnable") == 0) dsoundEnable = dword;
	else if (strcmp(name, "MovieEnable") == 0) movieEnable = dword;
	else if (strcmp(name, "ExFlag") == 0) exFlag = dword;
	else if (strcmp(name, "ExTime00") == 0) exTime00 = dword;
	else if (strcmp(name, "ExTime01") == 0) exTime01 = dword;
	else if (strcmp(name, "ExTime02") == 0) exTime02 = dword;
	else if (strcmp(name, "ExTime10") == 0) exTime10 = dword;
	else if (strcmp(name, "ExTime11") == 0) exTime11 = dword;
	else if (strcmp(name, "ExTime12") == 0) exTime12 = dword;
	else if (strcmp(name, "ExTime20") == 0) exTime20 = dword;
	else if (strcmp(name, "ExTime21") == 0) exTime21 = dword;
	else if (strcmp(name, "ExTime22") == 0) exTime22 = dword;
	else if (strcmp(name, "GALLERY0") == 0) gallery0 = dword;
	else if (strcmp(name, "GALLERY1") == 0) gallery1 = dword;
	else if (strcmp(name, "GALLERY2") == 0) gallery2 = dword;
	else if (strcmp(name, "GALLERY3") == 0) gallery3 = dword;
	else if (strcmp(name, "GALLERY4") == 0) gallery4 = dword;
	else if (strcmp(name, "GraphicFadeflag") == 0) graphicFadeflag = dword;
	else if (strcmp(name, "GraphicPerspectiveCorrect") == 0) graphicPerspectiveCorrect = dword;
	else if (strcmp(name, "GraphicSprdataswitch") == 0) graphicSprdataswitch = dword;
	else if (strcmp(name, "GraphicTextureInterpolation") == 0) graphicTextureInterpolation = dword;
	else if (strcmp(name, "SoundChannels") == 0) soundChannels = dword;
	else if (strcmp(name, "SoundDepth") == 0) soundDepth = dword;
	else if (strcmp(name, "SoundFrequency") == 0) soundFrequency = dword;
	else if (strcmp(name, "SPECIAL") == 0) special = dword;
	else if (strcmp(name, "SuperHardSwitch") == 0) superHardSwitch = dword;
	else
	{
		char str[128];
		sprintf(str, "Could not read %s\n", name);
		OutputDebugStringA(str);
	}
}

void CConfig::GetBinary(const char *name, u8 *dst, size_t size)
{
	CConfigBinary *bin = FindBinary(name);
	if (bin) bin->GetData(dst, size);
}

void CConfig::SetBinary(const char *name, u8 *src, size_t size)
{
	CConfigBinary *bin = FindBinary(name);
	if (bin) bin->Set(name, src, size);
	else
	{
		CConfigBinary b;
		b.Set(name, src, size);
		bins.push_back(b);
	}	
}

void CConfig::PushBinaryStr(const char *name, std::string &data)
{
	CConfigBinary b;
	b.Set2(name, data);
	bins.push_back(b);
}

CConfigBinary* CConfig::FindBinary(const char *name)
{
	for (size_t i = 0, si = bins.size(); i < si; i++)
	{
		if (bins[i].name.compare(name) == 0)
			return &bins[i];
	}

	return NULL;
}

void CConfig::WriteIni()
{
	GFile ini;
	ini.Open("config.ini", GFile::modeWrite | GFile::shareDenyNone | GFile::modeCreate);
	//FILE *ini = fopen("config.ini", "wt");
	if (ini.GetHandle() == INVALID_HANDLE_VALUE)
		printf("Cannot create config.ini.\n");

	char buffer[4096];
	sprintf(buffer, "[GAME]\r\n"
		"DATA1 = %s\r\n"
		"DATA2 = %s\r\n"
		"DATA3 = %d\r\n"
		"DATA4 = %d\r\n"
		"DirectSoundEnable = %d\r\n"
		"DisplayMode = %s\r\n"
		"DriverMode = %s\r\n"
		"ExFlag = 0x%08x\r\n"
		"ExTime00 = 0x%08x\r\n"
		"ExTime01 = 0x%08x\r\n"
		"ExTime02 = 0x%08x\r\n"
		"ExTime10 = 0x%08x\r\n"
		"ExTime11 = 0x%08x\r\n"
		"ExTime12 = 0x%08x\r\n"
		"ExTime20 = 0x%08x\r\n"
		"ExTime21 = 0x%08x\r\n"
		"ExTime22 = 0x%08x\r\n"
		"GALLERY0 = 0x%08x\r\n"
		"GALLERY1 = 0x%08x\r\n"
		"GALLERY2 = 0x%08x\r\n"
		"GALLERY3 = 0x%08x\r\n"
		"GALLERY4 = 0x%08x\r\n"
		"GraphicFadeflag = %d\r\n"
		"GraphicPerspectiveCorrect = %d\r\n"
		"GraphicSprdataswitch = %d\r\n"
		"GraphicTextureInterpolation = %d\r\n"
		"InstallKey = %s\r\n"
		"MovieEnable = %d\r\n"
		"SoundChannels = %d\r\n"
		"SoundDepth = %d\r\n"
		"SoundFrequency = %d\r\n"
		"SPECIAL = 0x%x\r\n"
		"SuperHardSwitch = %d\r\n",
		data1.c_str(), data2.c_str(),
		data3, data4,
		dsoundEnable,
		displayMode.c_str(),
		driverMode.c_str(),
		exFlag,
		exTime00, exTime01, exTime02,
		exTime10, exTime11, exTime12,
		exTime20, exTime21, exTime22,
		gallery0, gallery1, gallery2, gallery3, gallery4,
		graphicFadeflag,
		graphicPerspectiveCorrect,
		graphicSprdataswitch,
		graphicTextureInterpolation,
		installKey.c_str(),
		movieEnable,
		soundChannels,
		soundDepth,
		soundFrequency,
		special,
		superHardSwitch);
	ini.Write(buffer, strlen(buffer));
	// variable crap
	for (size_t i = 0, si = bins.size(); i < si; i++)
	{
		std::string hex;
		bins[i].ToString(hex);
		sprintf(buffer, "%s = %s\r\n", bins[i].name_alt.c_str(), hex.c_str());
		ini.Write(buffer, strlen(buffer));
	}
	// all the stuff that isn't from the game
	static char *cmodes[]= { "psx", "full", "legacy", "full2" };
	sprintf(buffer, "\n[DLL]\r\n"
		"DinputEnable = %d\r\n"
		"JapaneseEnable = %d\r\n"
		"QuickturnEnable = %d\r\n"
		"FocusIgnore = %d\r\n"
		"ColorMode = %s\r\n",
		dinputEnable,
		japaneseEnable,
		quickturnEnable,
		focusIgnore,
		cmodes[colorMode]);
	ini.Write(buffer, strlen(buffer));

	//fclose(ini);
	ini.Close();
}

//////////////////////////////////////////////////
void CConfigBinary::Set(const char *name, u8 *data, size_t size)
{
	this->name = name;
	name_alt = name;
	string_replace(name_alt, ' ', '.');
	SetData(data, size);
}

void CConfigBinary::Set2(const char *name, std::string &data)
{
	name_alt = name;
	this->name = name_alt;
	string_replace(this->name, '.', ' ');
	FromString(data);
}

void CConfigBinary::SetData(const u8 *data, size_t size)
{
	this->data.clear();
	this->data.resize(size);
	memcpy(&this->data[0], data, size);
}

void CConfigBinary::GetData(u8* data, size_t size)
{
	memcpy(data, &this->data[0], size);
}

void CConfigBinary::FromString(std::string got)
{
	char *tok = strtok((char*)got.c_str(), " ");

	while (tok)
	{
		DWORD val;
		sscanf(tok, "%x", &val);
		data.push_back(val & 0xff);
		tok = strtok(NULL, " ");
	}
}

void CConfigBinary::ToString(std::string &str)
{
	char hex[3];
	for (size_t i = 0, si = data.size(); i < si; i++)
	{
		sprintf(hex, "%02X", data[i]);
		str += hex;
		if (i < si - 1)
			str += " ";
	}
}

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

void QueryKey(HKEY hKey, std::vector<std::string> &dst)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD i, retCode;

	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hKey,                    // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 

	// Enumerate the subkeys, until RegEnumKeyEx fails.
	if (cSubKeys)
	{
		for (i = 0; i<cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				if (strncmp(achValue, "JoyDef_", 7) == 0)
					dst.push_back(achValue);
			}
		}
	}

	// Enumerate the key values. 
	if (cValues)
	{
		for (i = 0, retCode = ERROR_SUCCESS; i<cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			retCode = RegEnumValue(hKey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				NULL,
				NULL);

			if (retCode == ERROR_SUCCESS)
			{
				if (strncmp(achValue, "JoyDef_", 7) == 0)
					dst.push_back(achValue);
			}
		}
	}
}
