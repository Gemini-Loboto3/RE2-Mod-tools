#include "stdafx.h"
#include <string>

static LPCSTR encode =
{
	" .@@@()@@«»@012345"	// 00
	"6789:@,\"!?@ABCDEFG"	// 12
	"HIJKLMNOPQRSTUVWXY"	// 24
	"Z[/]'-·abcdefghijk"	// 36
	"lmnopqrstuvwxyz@@@"	// 48
	"@@@@@@@@@@@@@@@@@@"	// 5A
	"@@@@@@@@@@@@@@@@@@"	// 6C
	"@@@@@@@@@@@@@@@@@@"	// 7E
	"@@@@@@@@@@@@@@@@@@"	// 90
	"@@@@@@@@@@@@@@@@@@"	// A2
	"@@@@@@@@@@@@@@@@@@"	// B4
	"@@@@@@@@@@@@@@@@@@"	// C6
	"@@@@@@@@@@@@@@@@@@"	// D8
	"@@@@@@@@@@@@@@@@@@"	// EA
	"&@@@"					// FC
};

std::string DecodeString(u8 *data)
{
	std::string str;
	char temp[32];

	while (1)
	{
		u8 c = *data++;

		switch (c)
		{
		case 0xee:	// upper ranges
			str += encode[*data++ + 0xea];
			break;
		case 0xef:
		case 0xf0:
		case 0xf1:
		case 0xf2:
		case 0xf3:
		case 0xf4:
		case 0xf5:	// ??
			str += "{p}";
			break;
		case 0xf6:	// ??
			break;
		case 0xf7:	// short EOS for items
			return str;
		case 0xf8:
			str += "{string ";
			str += *data + '0';
			str += "}";
			data++;
			break;
		case 0xf9:
			str += "{color ";
			str += *data + '0';
			str += "}";
			data++;
			break;
		case 0xfa:	// non-instant text scrolling
			sprintf_s(temp, sizeof(temp), "{scroll %d}", *data++);
			str += temp;
			break;
		case 0xfb:
			sprintf_s(temp, sizeof(temp), "{branch %d}", *data++);
			str += temp;
			break;
		case 0xfc:
			str += "\\n";
			break;
		case 0xfd:
			str += "{clear ";
			str += *data + '0';
			str += "}";
			data++;
			break;
		case 0xfe:
			return str;
		case 0xff: // ??
			break;
		default:
			if (encode[c] & 0x80)
				str += "\xC2";
			str += encode[c];
		}
	}

	return str;
}

static LPCWSTR encode_eu =
{
	L"  @@@@@@@@@@012345"	// 00
	L"6789:;,\"!?@ABCDEFG"	// 12
	L"HIJKLMNOPQRSTUVWXY"	// 24
	L"Z(/)'-·abcdefghijk"	// 36
	L"lmnopqrstuvwxyzÄäÖ"	// 48
	L"öÜüßÀàÂâÈèÉéÊêÏïÎî"	// 5A
	L"ÔôÙùÛûÇçＳＴＡＲ“.…‒–+"	// 6C
	L"=&@@@@ÑñËë°ªÁáÍíÓó"	// 7E
	L"Úú¿¡ÌìÒò$*„‟`æ@@@@"	// 90
};

size_t u8_wc_toutf8(char *dest, uint32_t ch)
{
	if (ch < 0x80) {
		dest[0] = (char)ch;
		dest[1] = '\0';
		return 1;
	}
	if (ch < 0x800) {
		dest[0] = (ch >> 6) | 0xC0;
		dest[1] = (ch & 0x3F) | 0x80;
		dest[2] = '\0';
		return 2;
	}
	if (ch < 0x10000) {
		dest[0] = (ch >> 12) | 0xE0;
		dest[1] = ((ch >> 6) & 0x3F) | 0x80;
		dest[2] = (ch & 0x3F) | 0x80;
		dest[3] = '\0';
		return 3;
	}
	if (ch < 0x110000) {
		dest[0] = (ch >> 18) | 0xF0;
		dest[1] = ((ch >> 12) & 0x3F) | 0x80;
		dest[2] = ((ch >> 6) & 0x3F) | 0x80;
		dest[3] = (ch & 0x3F) | 0x80;
		dest[4] = '\0';
		return 4;
	}
	return 0;
}

std::string DecodeStringEU(u8 *data)
{
	std::string str;
	char temp[32];

	while (1)
	{
		u8 c = *data++;

		switch (c)
		{
		case 0xee:	// upper ranges
			str += encode[*data++ + 0xea];
			break;
		case 0xef:
		case 0xf0:
		case 0xf1:
		case 0xf2:
		case 0xf3:
		case 0xf4:
		case 0xf5:	// ??
			str += "{p}";
			break;
		case 0xf6:	// ??
			break;
		case 0xf7:	// short EOS for items
			return str;
		case 0xf8:
			sprintf_s(temp, sizeof(temp), "{string %d}", *data++);
			str += temp;
			break;
		case 0xf9:
			str += "{color ";
			str += *data + '0';
			str += "}";
			data++;
			break;
		case 0xfa:	// non-instant text scrolling
			sprintf_s(temp, sizeof(temp), "{scroll %d}", *data++);
			str += temp;
			break;
		case 0xfb:
			sprintf_s(temp, sizeof(temp), "{branch %d}", *data++);
			str += temp;
			break;
		case 0xfc:
			str += "\\n";
			break;
		case 0xfd:
			sprintf_s(temp, sizeof(temp), "{clear %d}", *data++ * 60 / 50);
			str += temp;
			break;
		case 0xfe:
			if (*data != 0)
			{
				sprintf_s(temp, sizeof(temp), "{timed %d}", *data * 60 / 50);
				str += temp;
			}
			return str;
		case 0xff: // ??
			break;
		default:
			u8_wc_toutf8(temp, encode_eu[c]);
			str += temp;
		}
	}

	return str;
}
