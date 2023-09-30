/****************************/
/*        LZSS.C           */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#include "game.h"

#define RING_BUFF_SIZE	4096			// size of ring buffer
#define F				18				// upper limit for match_length
#define THRESHOLD		2				// encode string into position and length if match_length is greater than this

long LZSS_Decode(short fRefNum, Ptr destPtr, long sourceSize)
{
short  		i, j, k, r;
unsigned short  flags;
Ptr			srcOriginalPtr;
uint8_t		*sourcePtr;
uint8_t		c;
Ptr			destStartPtr = destPtr;

				/* GET MEMORY FOR LZSS DATA */

	// ring buffer of size N, with extra F-1 bytes to facilitate string comparison
	uint8_t* text_buf = (uint8_t *)AllocPtrClear(RING_BUFF_SIZE + F - 1);

	// left & right children & parents -- These constitute binary search trees.
	short* lson = (short *)AllocPtrClear(sizeof(short)*(RING_BUFF_SIZE + 1));
	short* rson = (short *)AllocPtrClear(sizeof(short)*(RING_BUFF_SIZE + 257));
	short* dad  = (short *)AllocPtrClear(sizeof(short)*(RING_BUFF_SIZE + 1));

	GAME_ASSERT(text_buf && lson && rson && dad);

	srcOriginalPtr = (Ptr)AllocPtrClear(sourceSize+1);
	GAME_ASSERT(srcOriginalPtr);

	sourcePtr = (uint8_t *)srcOriginalPtr;

				/* READ LZSS DATA */

	FSRead(fRefNum,&sourceSize,srcOriginalPtr);



					/* DECOMPRESS IT */

	for (i = 0; i < (RING_BUFF_SIZE - F); i++)						// clear buff to "default char"? (BLG)
		text_buf[i] = ' ';

	r = RING_BUFF_SIZE - F;
	flags = 0;
	for ( ; ; )
	{
		if (((flags >>= 1) & 256) == 0)
		{
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			c = *sourcePtr++;					// get a source byte
			flags = (unsigned short)c | 0xff00;							// uses higher byte cleverly
		}
													// to count eight
		if (flags & 1)
		{
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			c = *sourcePtr++;					// get a source byte
			*destPtr++ = c;
			text_buf[r++] = c;
			r &= (RING_BUFF_SIZE - 1);
		}
		else
		{
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			i = *sourcePtr++;					// get a source byte
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			j = *sourcePtr++;					// get a source byte

			i |= ((j & 0xf0) << 4);
			j = (j & 0x0f) + THRESHOLD;
			for (k = 0; k <= j; k++)
			{
				c = text_buf[(i + k) & (RING_BUFF_SIZE - 1)];
				*destPtr++ = c;
				text_buf[r++] = c;
				r &= (RING_BUFF_SIZE - 1);
			}
		}
	}

	ptrdiff_t decompSize = destPtr - destStartPtr;		// calc size of decompressed data


			/* CLEANUP */

	SafeDisposePtr(srcOriginalPtr);
	SafeDisposePtr((Ptr)lson);
	SafeDisposePtr((Ptr)rson);
	SafeDisposePtr((Ptr)dad);
	SafeDisposePtr((Ptr)text_buf);

	return (long) decompSize;
}
