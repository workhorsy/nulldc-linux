#include "TexCache.h"
#include "regs.h"

u32 detwiddle[2][8][1024];
//input : address in the yyyyyxxxxx format
//output : address in the xyxyxyxy format
//U : x resolution , V : y resolution
//twidle works on 64b words
u32 FASTCALL twiddle_fast(u32 x,u32 y,u32 bcx,u32 bcy)
{
	return detwiddle[0][bcy][x]+detwiddle[1][bcx][y];
}

u32 FASTCALL twiddle_razi_(u32 x,u32 y,u32 x_sz,u32 y_sz)
{
	u32 rv=0;//low 2 bits are directly passed  -> needs some misc stuff to work.However
			 //Pvr internaly maps the 64b banks "as if" they were twidled :p

	u32 sh=0;
	x_sz>>=1;
	y_sz>>=1;
	while(x_sz!=0 || y_sz!=0)
	{
		if (y_sz)
		{
			u32 temp=y&1;
			rv|=temp<<sh;

			y_sz>>=1;
			y>>=1;
			sh++;
		}
		if (x_sz)
		{
			u32 temp=x&1;
			rv|=temp<<sh;

			x_sz>>=1;
			x>>=1;
			sh++;
		}
	}	
	return rv;
}

u32 unpack_4_to_8_tw[16];
u32 unpack_5_to_8_tw[32];
u32 unpack_6_to_8_tw[64];

void BuildTwiddleTables()
{
	for (u32 s=0;s<8;s++)
	{
		u32 x_sz=1024;
		u32 y_sz=8<<s;
		for (u32 i=0;i<x_sz;i++)
		{
			detwiddle[0][s][i]=twiddle_razi_(i,0,x_sz,y_sz);
			detwiddle[1][s][i]=twiddle_razi_(0,i,y_sz,x_sz);
		}
	}

	//also fill in the texture cvt tables !

	for (int i=0;i<(1<<4);i++)
	{
		unpack_4_to_8_tw[i]=((i)<<4)|(i>>0);
	}

	for (int i=0;i<(1<<5);i++)
	{
		unpack_5_to_8_tw[i]=((i)<<3)|(i>>2);
	}

	for (int i=0;i<(1<<6);i++)
	{
		unpack_6_to_8_tw[i]=((i)<<2)|(i>>4);
	}
}

//# define twop( val, n )	twidle_razi( (val), (n),(n) )
#define twop twiddle_fast

u8* vq_codebook;
u32 palette_index;
u32 palette_lut[1024];
bool pal_needs_update=true;
u32 _pal_rev_256[4]={0};
u32 _pal_rev_16[64]={0};
u32 pal_rev_256[4]={0};
u32 pal_rev_16[64]={0};
void palette_update()
{
	if (pal_needs_update==false)
		return;
	memcpy(pal_rev_256,_pal_rev_256,sizeof(pal_rev_256));
	memcpy(pal_rev_16,_pal_rev_16,sizeof(pal_rev_16));

#define PixelPacker pp_dx
	pal_needs_update=false;
	switch(PAL_RAM_CTRL&3)
	{
	case 0:
		for (int i=0;i<1024;i++)
		{
			palette_lut[i]=ARGB1555_TW(PALETTE_RAM[i]);
		}
		break;

	case 1:
		for (int i=0;i<1024;i++)
		{
			palette_lut[i]=ARGB565_TW(PALETTE_RAM[i]);
		}
		break;

	case 2:
		for (int i=0;i<1024;i++)
		{
			palette_lut[i]=ARGB4444_TW(PALETTE_RAM[i]);
		}
		break;

	case 3:
		for (int i=0;i<1024;i++)
		{
			palette_lut[i]=PALETTE_RAM[i];//argb 8888 :p
		}
		break;
	}

}