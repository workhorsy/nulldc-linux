#include "common.h"
#include "cdi.h"
#include "mds.h"
#include "gdi.h"
#include "chd.h"
#include "ioctl.h"

#include <memory.h>
#include <windows.h>
u32 NullDriveDiscType;
Disc* disc;
Disc*(*drivers[])(wchar* path)=
{
	gdi_parse,
	cdi_parse,
	ioctl_parse,
	chd_parse,
	mds_parse,
	0
};

DriveNotifyEventFP* DriveNotifyEvent;
u8 q_subchannel[96];

int msgboxf(wchar* text,unsigned int type,...)
{
	va_list args;

	wchar temp[2048];
	va_start(args, type);
	vswprintf(temp, WCHAR_LEN(temp), text, args);
	va_end(args);


	return MessageBox(NULL,temp,emu_name,type | MB_TASKMODAL);
}
void PatchRegion_0(u8* sector,int size)
{
	if (settings.PatchRegion==0)
		return;

	u8* usersect=sector;

	if (size!=2048)
	{
		printf("PatchRegion_0 -> sector size %d , skiping patch\n",size);
	}

	//patch meta info
	u8* p_area_symbol=&usersect[0x30];
	memcpy(p_area_symbol,"JUE        ",8);
}
void PatchRegion_6(u8* sector,int size)
{
	if (settings.PatchRegion==0)
		return;

	u8* usersect=sector;

	if (size!=2048)
	{
		printf("PatchRegion_6 -> sector size %d , skiping patch\n",size);
	}

	//patch area symbols
	u8* p_area_text=&usersect[0x700];
	memcpy(&p_area_text[4],"For JAPAN,TAIWAN,PHILIPINES.",28);
	memcpy(&p_area_text[4 + 32],"For USA and CANADA.         ",28);
	memcpy(&p_area_text[4 + 32 + 32],"For EUROPE.                 ",28);
}
bool ConvertSector(u8* in_buff , u8* out_buff , int from , int to,int sector)
{
	//get subchannel data, if any
	if (from==2448)
	{
		memcpy(q_subchannel,in_buff+2352,96);
		from-=96;
	}
	//if no convertion
	if (to==from)
	{
		memcpy(out_buff,in_buff,to);
		return true;
	}
	switch (to)
	{
	case 2340:
		{
			verify((from==2352));
			memcpy(out_buff,&in_buff[12],2340);
		}
		break;
	case 2328:
		{
			verify((from==2352));
			memcpy(out_buff,&in_buff[24],2328);
		}
		break;
	case 2336:
		verify(from>=2336);
		verify((from==2352));
		memcpy(out_buff,&in_buff[0x10],2336);
		break;
	case 2048:
		{
			verify(from>=2048);
			verify((from==2448) || (from==2352) || (from==2336));
			if ((from == 2352) || (from == 2448))
			{
				if (in_buff[15]==1)
				{
					memcpy(out_buff,&in_buff[0x10],2048); //0x10 -> mode1
				}
				else
					memcpy(out_buff,&in_buff[0x18],2048); //0x18 -> mode2 (all forms ?)
			}
			else
				memcpy(out_buff,&in_buff[0x8],2048);	//hmm only possible on mode2.Skip the mode2 header
		}
		break;
	case 2352:
		//if (from >= 2352)
		{
			memcpy(out_buff,&in_buff[0],2352);
		}
		break;
	default :
		printf("Sector convertion from %d to %d not supported \n", from , to);
		break;
	}

	return true;
}

bool InitDrive_(wchar* fn)
{
	TermDrive();

	//try all drivers
	for (int i=0;drivers[i] && !(disc=drivers[i](fn));i++) ;

	if (disc!=0)
	{
		NullDriveDiscType=Busy;
		DriveNotifyEvent(DiskChange,0);
		Sleep(400); //busy for a bit
		return true;
	}
	else
	{
		NullDriveDiscType=NoDisk; //no disc :)
	}
	return false;
}

bool InitDrive(u32 fileflags)
{
	if (settings.LoadDefaultImage)
	{
		wprintf(L"Loading default image \"%s\"\n",settings.DefaultImage);
		if (!InitDrive_(settings.DefaultImage))
		{
			msgboxf(L"Default image \"%s\" failed to load",MB_ICONERROR);
			return false;
		}
		else
			return true;
	}

	wchar fn[512];
	wcscpy(fn,settings.LastImage);
#ifdef BUILD_DREAMCAST
	int gfrv=GetFile(fn,0,fileflags);
#else
	int gfrv=0;
#endif
	if (gfrv==0)
	{
		NullDriveDiscType=NoDisk;
		return true;
	}
	else if (gfrv==-1)
		return false;

	wcscpy(settings.LastImage,fn);
	SaveSettings();

	if (!InitDrive_(fn))
	{
		msgboxf(L"Selected image failed to load",MB_ICONERROR);
		return false;
	}
	else
		return true;
}

void TermDrive()
{
	if (disc!=0)
		delete disc;

	disc=0;
}


//
//convert our nice toc struct to dc's native one :)

u32 CreateTrackInfo(u32 ctrl,u32 addr,u32 fad)
{
	u8 p[4];
	p[0]=(ctrl<<4)|(addr<<0);
	p[1]=fad>>16;
	p[2]=fad>>8;
	p[3]=fad>>0;

	return *(u32*)p;
}
u32 CreateTrackInfo_se(u32 ctrl,u32 addr,u32 tracknum)
{
	u8 p[4];
	p[0]=(ctrl<<4)|(addr<<0);
	p[1]=tracknum;
	p[2]=0;
	p[3]=0;
	return *(u32*)p;
}


void GetDriveSector(u8 * buff,u32 StartSector,u32 SectorCount,u32 secsz)
{
	if (disc)
	{
		disc->ReadSectors(StartSector,SectorCount,buff,secsz);
		if (disc->type == GdRom && StartSector==45150 && SectorCount==7)
		{
			PatchRegion_0(buff,secsz);
			PatchRegion_6(buff+2048*6,secsz);
		}
	}
}
void GetDriveToc(u32* to,DiskArea area)
{
	if (!disc)
		return;
	memset(to,0xFFFFFFFF,102*4);

	//can't get toc on the second area on discs that don't have it
	verify(area != DoubleDensity || disc->type == GdRom);

	//normal CDs: 1 .. tc
	//GDROM: area0 is 1 .. 2, area1 is 3 ... tc

	u32 first_track=1;
	u32 last_track=disc->tracks.size();
	if (area==DoubleDensity)
		first_track=3;
	else if (disc->type==GdRom)
	{
		last_track=2;
	}

	//Geneate the TOC info

	//-1 for 1..99 0 ..98
	to[99]=CreateTrackInfo_se(disc->tracks[first_track-1].CTRL,disc->tracks[first_track-1].ADDR,first_track); 
	to[100]=CreateTrackInfo_se(disc->tracks[last_track-1].CTRL,disc->tracks[last_track-1].ADDR,last_track); 
	
	if (disc->type==GdRom)
	{
		//use smaller LEADOUT
		if (area==SingleDensity)
			to[101]=CreateTrackInfo(disc->LeadOut.CTRL,disc->LeadOut.ADDR,13085);
	}
	else
		to[101]=CreateTrackInfo(disc->LeadOut.CTRL,disc->LeadOut.ADDR,disc->LeadOut.StartFAD);

	for (u32 i=first_track-1;i<last_track;i++)
	{
		to[i]=CreateTrackInfo(disc->tracks[i].CTRL,disc->tracks[i].ADDR,disc->tracks[i].StartFAD); 
	}
}

void GetDriveSessionInfo(u8* to,u8 session)
{
	if (!disc)
		return;
	to[0]=2;//status , will get overwrited anyway
	to[1]=0;//0's
	
	if (session==0)
	{
		to[2]=disc->sessions.size();//count of sessions
		to[3]=disc->EndFAD>>16;//fad is sessions end
		to[4]=disc->EndFAD>>8;
		to[5]=disc->EndFAD>>0;
	}
	else
	{
		to[2]=disc->sessions[session-1].FirstTrack;//start track of this session
		to[3]=disc->sessions[session-1].StartFAD>>16;//fad is session start
		to[4]=disc->sessions[session-1].StartFAD>>8;
		to[5]=disc->sessions[session-1].StartFAD>>0;
	}
}

void printtoc(TocInfo* toc,SessionInfo* ses)
{
	printf("Sessions %d\n",ses->SessionCount);
	for (u32 i=0;i<ses->SessionCount;i++)
	{
		printf("Session %d: FAD %d,First Track %d\n",i+1,ses->SessionFAD[i],ses->SessionStart[i]);
		for (u32 t=toc->FistTrack-1;t<=toc->LastTrack;t++)
		{
			if (toc->tracks[t].Session==i+1)
			{
				printf("\tTrack %d : FAD %d CTRL %d ADR %d\n",t,toc->tracks[t].FAD,toc->tracks[t].Control,toc->tracks[t].Addr);
			}
		}
	}
	printf("Session END: FAD END %d\n",ses->SessionsEndFAD);
}

DiscType GuessDiscType(bool m1, bool m2, bool da)
{
	if ((m1==true) && (da==false) && (m2==false))
		return  CdRom;
	else if (m2)
		return  CdRom_XA;
	else if (da && m1) 
		return CdRom_Extra;
	else
		return CdRom;
}