/*
   Copyright (C) 2021, 2022 MariaDB Corporation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

/* handling of the conversion of string prefixes to int64_t for quick range checking */

#include "collation.h"
#include "joblisttypes.h"

#include "string_prefixes.h"

// XXX: string (or, actually, a BLOB) with all NUL chars will be encoded into zero. Which corresponds to
//      encoding of empty string, or NULL.
int64_t encodeStringPrefix(const uint8_t* str, size_t len, int charsetNumber)
{
  datatypes::Charset cset(charsetNumber);
  int64_t fixedLenPrefix=0;
  cset.strnxfrm(reinterpret_cast<uint8_t*>(&fixedLenPrefix), 8, 8, str, len, 0);
  int64_t ret =
      (fixedLenPrefix >> 56) | ((fixedLenPrefix << 40) & 0x00FF000000000000ULL) |
      ((fixedLenPrefix << 24) & 0x0000FF0000000000ULL) |((fixedLenPrefix << 8) & 0x000000FF00000000ULL) | 
      ((fixedLenPrefix >> 8) & 0x00000000FF000000ULL) |((fixedLenPrefix >> 24) & 0x0000000000FF0000ULL) | 
      ((fixedLenPrefix >> 40) & 0x000000000000FF00ULL) | (fixedLenPrefix << 56);
  return ret;
}

int64_t encodeStringPrefix_check_null(const uint8_t* str, size_t len, int charsetNumber)
{
  if (len < 1)
  {
    return joblist::UBIGINTNULL;
  }
  return encodeStringPrefix(str, len, charsetNumber);
}
