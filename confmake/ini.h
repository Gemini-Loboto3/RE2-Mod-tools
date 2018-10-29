/**
 * Copyright (c) 2016 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ini.c` for details.
 */
#pragma once
#include <vector>
#include <string>

#define INI_VERSION "0.1.1"

typedef struct ini_t ini_t;

ini_t*      ini_load(const char *filename);
ini_t*		ini_load(unsigned char* buffer, size_t size);
void        ini_free(ini_t *ini);
const char* ini_get(ini_t *ini, const char *section, const char *key);
void        ini_get_seq(ini_t *ini, const char *section, const char *key, std::vector<std::string> &keys, std::vector<std::string> &values);
int         ini_sget(ini_t *ini, const char *section, const char *key, const char *scanfmt, void *dst);
