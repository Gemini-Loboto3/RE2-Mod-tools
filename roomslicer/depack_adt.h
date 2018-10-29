/*
	ADT file depacker

	Copyright (C) 2007	Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#pragma once
#include "memstream.h"
#include "image.h"

/*
	Depack an ADT file

	src		Source file
	dstPointer	Pointer to depacked file buffer (NULL if failed)
	dstLength	Length of depacked file (0 if failed)
*/
void adt_depack(MEM_STREAM *src, u8 **dstPointer, int *dstLength);

/*
	Create a SDL_Surface, for a depacked ADT file
	source		Pointer to depacked file
	dest		Reference to destination surface
*/
void adt_surface(u16 *source, Image &dest/*, int reorganize*/);

void extract_roomcut(LPCSTR filename, LPCSTR folder);
