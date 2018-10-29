#pragma once
#include <string>
#include <vector>

static void string_replace(std::string &str, char ch, char with)
{
	for (size_t i = 0, si = str.size(); i < si; i++)
		if (str[i] == ch) str[i] = with;
}

class CConfigBinary
{
public:
	std::string name;
	std::string name_alt;
	std::vector<u8> data;

	void Set(const char *name, u8 *data, size_t size);
	void Set2(const char *name, std::string &data);
	void SetData(const u8 *data, size_t size);
	void GetData(u8* data, size_t size);
	void FromString(std::string got);
	void ToString(std::string &str);
};

class CConfig
{
public:
	CConfig();
	~CConfig();

	void Init();

	void CreateDefault();
	int  CreateFromIni();
	int  CreateFromRegistry();

	int  HasRegistry();
	int  HasIni();
	void WriteIni();

	std::string data1,
		data2;
	u32 data3,
		data4;
	int dsoundEnable,
		soundChannels,
		soundDepth,
		soundFrequency,
		movieEnable;
	std::string displayMode,
		driverMode,
		executeProgram,	// regenerated
		installKey,		// regenerated
		savePath;
	u32 exFlag,
		exTime00,
		exTime01,
		exTime02,
		exTime10,
		exTime11,
		exTime12,
		exTime20,
		exTime21,
		exTime22,
		gallery0,
		gallery1,
		gallery2,
		gallery3,
		gallery4;
	int graphicFadeflag,
		graphicPerspectiveCorrect,
		graphicSprdataswitch,
		graphicTextureInterpolation;
	int special,
		superHardSwitch;
	std::vector<CConfigBinary> bins;

	// my stuff
	int dinputEnable,
		japaneseEnable,
		quickturnEnable,
		focusIgnore,
		colorMode;
	std::string path;	// work path

	void GetString(const char *name, char *dst);
	void GetDword (const char *name, DWORD *dst);
	void SetString(const char *name, const char *str);
	void SetDword (const char *name, DWORD dword);
	void GetBinary(const char *name, u8 *dst, size_t size);
	void SetBinary(const char *name, u8 *src, size_t size);
	void PushBinaryStr(const char *name, std::string &data);

	CConfigBinary* FindBinary(const char *name);

	bool bLoaded;
};
