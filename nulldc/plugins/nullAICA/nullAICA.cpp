// nullAICA.cpp : Defines the entry point for the DLL application.
//

#include "nullAICA.h"
#include "sgc_if.h"
#include "aica.h"
#include "mem.h"
#include "audiostream.h"
#include "gui.h"
#include <tchar.h>

setts settings;
aica_init_params aica_params;
emu_info eminf;
HINSTANCE hinst;
bool APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	hinst=(HINSTANCE)hModule;
    return true;
}
void EXPORT_CALL handle_About(u32 id,void* w,void* p)
{
	MessageBoxA((HWND)w,"Made by the nullDC Team","About nullAica...",MB_ICONINFORMATION);
}
void EXPORT_CALL handle_ShowASD(u32 id,void* w,void* p)
{
	ShowDebugger((HWND)w);
}
void EXPORT_CALL handle_Config(u32 id,void* w,void* p)
{
	ShowConfig((HWND)w);
}
void EXPORT_CALL handle_SA(u32 id,void* w,void* p)
{
	if (settings.LimitFPS==1)
		settings.LimitFPS=0;
	else
		settings.LimitFPS=1;

	eminf.SetMenuItemStyle(config_stami,settings.LimitFPS==1?MIS_Checked:0,MIS_Checked);
	eminf.SetMenuItemStyle(config_stami2,settings.LimitFPS==2?MIS_Checked:0,MIS_Checked);
	SaveSettings();
}
void EXPORT_CALL handle_SA2(u32 id,void* w,void* p)
{
	if (settings.LimitFPS==2)
		settings.LimitFPS=0;
	else
		settings.LimitFPS=2;

	eminf.SetMenuItemStyle(config_stami,settings.LimitFPS==1?MIS_Checked:0,MIS_Checked);
	eminf.SetMenuItemStyle(config_stami2,settings.LimitFPS==2?MIS_Checked:0,MIS_Checked);
	SaveSettings();
}
void EXPORT_CALL handle_DSP(u32 id,void* w,void* p)
{
	if (settings.DSPEnabled)
		settings.DSPEnabled=0;
	else
		settings.DSPEnabled=1;

	eminf.SetMenuItemStyle(id,settings.DSPEnabled?MIS_Checked:0,MIS_Checked);
	SaveSettings();
}
void EXPORT_CALL handle_MCDDA(u32 id,void* w,void* p)
{
	if (settings.CDDAMute)
		settings.CDDAMute=0;
	else
		settings.CDDAMute=1;

	eminf.SetMenuItemStyle(id,settings.CDDAMute?MIS_Checked:0,MIS_Checked);
	SaveSettings();
}
void EXPORT_CALL handle_GS(u32 id,void* w,void* p)
{
	if (settings.GlobalMute)
		settings.GlobalMute=0;
	else
		settings.GlobalMute=1;

	eminf.SetMenuItemStyle(id,settings.GlobalMute?MIS_Checked:0,MIS_Checked);
	SaveSettings();
}

s32 FASTCALL OnLoad(emu_info* em)
{
	memcpy(&eminf,em,sizeof(eminf));

	LoadSettings();

	config_scmi=eminf.AddMenuItem(em->RootMenu,-1,L"Config",handle_Config,0);
	config_stami=eminf.AddMenuItem(em->RootMenu,-1,L"Limit Speed && Sync audio",handle_SA,settings.LimitFPS==1);
	config_stami2=eminf.AddMenuItem(em->RootMenu,-1,L"Sync Audio",handle_SA2,settings.LimitFPS==2);
	eminf.AddMenuItem(em->RootMenu,-1,L"DSP Emulation",handle_DSP,settings.DSPEnabled);
	eminf.AddMenuItem(em->RootMenu,-1,L"Mute CDDA",handle_MCDDA,settings.CDDAMute);
	eminf.AddMenuItem(em->RootMenu,-1,L"Mute Sound",handle_GS,settings.GlobalMute);
	

	eminf.AddMenuItem(em->RootMenu,-1,0,0,0);
	eminf.AddMenuItem(em->RootMenu,-1,L"About",handle_About,0);

	eminf.AddMenuItem(em->DebugMenu,-1,L"AICA SGC Debugger",handle_ShowASD,0);
	return rv_ok;
}

void FASTCALL OnUnload()
{
}

//called when plugin is used by emu (you should do first time init here)
s32 FASTCALL Init(aica_init_params* initp)
{
	memcpy(&aica_params,initp,sizeof(aica_params));

	init_mem();
	AICA_Init();
	InitAudio();

	eminf.SetMenuItemStyle(config_scmi,MIS_Grayed,MIS_Grayed);
	return rv_ok;
}

//called when plugin is unloaded by emu , olny if dcInit is called (eg , not called to enumerate plugins)
void FASTCALL Term()
{
	TermAudio();
	AICA_Term();
	term_mem();

	eminf.SetMenuItemStyle(config_scmi,0,MIS_Grayed);
}

//It's suposed to reset anything 
void FASTCALL Reset(bool Manual)
{

}


//Give to the emu pointers for the PowerVR interface
EXPORT void EXPORT_CALL dcGetInterface(plugin_interface* info)
{
	info->InterfaceVersion=PLUGIN_I_F_VERSION;
/*
	info->Init=dcInit;
	info->Term=dcTerm;
	info->Reset=dcReset;

	info->ThreadInit=dcThreadInit;
	info->ThreadTerm=dcThreadTerm;
	info->ShowConfig=cfgdlg;

	info->Type=PluginType::AICA;

	info->InterfaceVersion.full=AICA_PLUGIN_I_F_VERSION;

	info->ReadMem_aica_ram=ReadMem_ram;
	info->WriteMem_aica_ram=WriteMem_ram;
	info->ReadMem_aica_reg=ReadMem_reg;
	info->WriteMem_aica_reg=WriteMem_reg;
	info->UpdateAICA=UpdateSystem;
*/
#define c info->common
#define a info->aica

	wcscpy(c.Name,L"nullDC AICA [" _T(__DATE__) L"]");

	c.InterfaceVersion=AICA_PLUGIN_I_F_VERSION;
	c.Type=Plugin_AICA;

	c.Load=OnLoad;
	c.Unload=OnUnload;

	a.Init=Init;
	a.Reset=Reset;
	a.Term=Term;

	a.Update=UpdateAICA;

	a.ReadMem_aica_reg=aica_ReadMem_reg;
	a.WriteMem_aica_reg=aica_WriteMem_reg;
}

int cfgGetInt(char* key,int def)
{
	wchar t[512];
	mbstowcs(t,key,512);
	return eminf.ConfigLoadInt(L"nullAica",t,def);
}
void cfgSetInt(char* key,int def)
{
	wchar t[512];
	mbstowcs(t,key,512);
	eminf.ConfigSaveInt(L"nullAica",t,def);
}

void LoadSettings()
{
	//load default settings before init
	settings.BufferSize=cfgGetInt("BufferSize",2048);
	settings.LimitFPS=cfgGetInt("LimitFPS",1);
	settings.HW_mixing=cfgGetInt("HW_mixing",0);
	settings.SoundRenderer=cfgGetInt("SoundRenderer",1);
	settings.GlobalFocus=cfgGetInt("GlobalFocus",1);
	settings.BufferCount=cfgGetInt("BufferCount",1);
	settings.CDDAMute=cfgGetInt("CDDAMute",0);
	settings.GlobalMute=cfgGetInt("GlobalMute",0);
	settings.DSPEnabled=cfgGetInt("DSPEnabled",0);		

	settings.Volume = max(0,min(cfgGetInt("Volume",90),100));
}

void SaveSettings()
{
	Cofnig_UpdateMenuSelections();

	//load default settings before init
	cfgSetInt("BufferSize",settings.BufferSize);
	cfgSetInt("LimitFPS",settings.LimitFPS);
	cfgSetInt("HW_mixing",settings.HW_mixing);
	cfgSetInt("SoundRenderer",settings.SoundRenderer);
	cfgSetInt("GlobalFocus",settings.GlobalFocus);
	cfgSetInt("BufferCount",settings.BufferCount);
	cfgSetInt("CDDAMute",settings.CDDAMute);
	cfgSetInt("GlobalMute",settings.GlobalMute);
	cfgSetInt("DSPEnabled",settings.DSPEnabled);
	
	cfgSetInt("Volume",max(0,min(settings.Volume,100)));
}

//Windoze Code implementation of commong classes from here and after ..

void CDECL thread_run(void* params) {
	cThread* t = reinterpret_cast<cThread*>(params);

	do {
		pthread_mutex_lock(&t->_mutex);
		while (t->_is_suspended)
			pthread_cond_wait(&t->_condition, &t->_mutex);
		pthread_mutex_unlock(&t->_mutex);
	} while (t->_entry(NULL));
}

//Thread class
cThread::cThread(ThreadFunctionFP function, void* prm)
{
	_entry = function;

	if (pthread_mutex_init(&_mutex, NULL) != 0)
		exit(1);
	if (pthread_cond_init(&_condition, NULL) != 0)
		exit(1);
	if (pthread_create(&this->_tid, NULL, (ThreadRunnerFP)&thread_run, this) != 0)
		exit(1);
}
cThread::~cThread()
{
	pthread_cond_destroy(&_condition);
	pthread_mutex_destroy(&_mutex);
}
	
void cThread::Start()
{
	pthread_mutex_lock(&_mutex);
	_is_suspended = false;
	pthread_cond_signal(&_condition);
	pthread_mutex_unlock(&_mutex);
}
void cThread::Suspend()
{
	pthread_mutex_lock(&_mutex);
	_is_suspended = true;
	pthread_cond_signal(&_condition);
	pthread_mutex_unlock(&_mutex);
}

void cThread::WaitToEnd(u32 msec)
{
	// FIXME: There should be a pthread replacement for this
	WaitForSingleObject(this->_tid.p, msec);
}
//End thread class

//cResetEvent Calss
cResetEvent::cResetEvent(bool State,bool Auto)
{
		hEvent = CreateEvent( 
        NULL,             // default security attributes
		Auto?false:true,  // auto-reset event?
		State?true:false, // initial state is State
        NULL			  // unnamed object
        );
}
cResetEvent::~cResetEvent()
{
	//Destroy the event object ?
	 CloseHandle(hEvent);
}
void cResetEvent::Set()//Signal
{
	SetEvent(hEvent);
}
void cResetEvent::Reset()//reset
{
	ResetEvent(hEvent);
}
void cResetEvent::Wait(u32 msec)//Wait for signal , then reset
{
	WaitForSingleObject(hEvent,msec);
}
void cResetEvent::Wait()//Wait for signal , then reset
{
	WaitForSingleObject(hEvent,(u32)-1);
}
//End AutoResetEvent