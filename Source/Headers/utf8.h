/****************************/
/*        UTF8.C            */
/* (c)2023 Iliyas Jorio     */
/****************************/

static uint32_t ReadNextCodepointFromUTF8(const char** utf8TextPtr)
{
#define TRY_ADVANCE(t) do { if (!*(t)) return 0; else (t)++; } while(0)

	uint32_t codepoint = 0;
	const uint8_t* t = (const uint8_t*) *utf8TextPtr;

	if ((*t & 0b10000000) == 0)
	{
		// 1 byte code point, ASCII
		codepoint |= (*t & 0b01111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 1;
	}
	else if ((*t & 0b11100000) == 0b11000000)
	{
		// 2 byte code point
		codepoint |= (*t & 0b00011111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 2;
	}
	else if ((**utf8TextPtr & 0b11110000) == 0b11100000)
	{
		// 3 byte code point
		codepoint |= (*t & 0b00001111) << 12;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);
		*utf8TextPtr += 3;
	}
	else
	{
		// 4 byte code point
		codepoint |= (*t & 0b00000111) << 18;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 12;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 4;
	}

	return codepoint;

#undef TRY_ADVANCE
}

static uint32_t ToUpperUnicode(uint32_t c)
{
	if ((c >= 0x0061 && c <= 0x007A)		// ascii: a...z
		|| (c >= 0x00E0 && c <= 0x00F6)		// latin-1: agrave...ouml
		|| (c >= 0x00F8 && c <= 0x00FE))	// latin-1: oslash...thorn
	{
		return c - 0x0020;
	}
	else if (c == 0x00FF)					// yuml
	{
		return 0x0178;
	}
	else if ((c >= 0x0100 && c <= 0x0137)	// latin extended-A (uppercase even indices)
		|| (c >= 0x014A && c <= 0x0177))
	{
		return c & ~1;
	}
	else if ((c >= 0x0139 && c <= 0x0148)	// latin extended-A (uppercase odd indices)
		|| (c >= 0x179 && c <= 0x017E))
	{
		return c | 1;
	}

	return c;
}
