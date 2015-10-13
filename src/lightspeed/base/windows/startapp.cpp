/*
 * app.cpp
 *
 *  Created on: 14.12.2009
 *      Author: ondra
 */

#include "winpch.h"
#include "../framework/app.h"
#include "../interface.tcc"
#include <iostream>
#include <tchar.h>
#include "hinstance.h"
#include "../containers/autoArray.tcc"

void exceptionOnStartup(HINSTANCE hInstance, const std::exception &e);

class MemoryMonitor {
public:
	~MemoryMonitor () {
		OutputDebugStringA("DUMP LEAKS: Checking memory after all objects are destroyed.\r\n");
		_CrtDumpMemoryLeaks();
		OutputDebugStringA("DUMP LEAKS: Check is complete.\r\n");
	}
};



#include "Shellapi.h"

namespace LightSpeed {
	int WINAPI startApp( HINSTANCE hInstance );
	void stubEnterDaemonMode(DWORD_PTR handle);
	extern const wchar_t *enterDaemonCmd;

}

int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd) {

	return LightSpeed::startApp(hInstance);

}

static void recalcLayout(HWND topWindow, int cx, int cy) {
	HDWP dwp = BeginDeferWindowPos(10);
	HWND button = GetDlgItem(topWindow,IDCANCEL);
	HWND text = GetDlgItem(topWindow,IDHELP);
	dwp = DeferWindowPos(dwp,text,0,5,5,cx-10,cy - 40,SWP_NOZORDER);
	dwp = DeferWindowPos(dwp,button,0,cx-80,cy-30,75,25,SWP_NOZORDER);
	EndDeferWindowPos(dwp);
}


LRESULT WINAPI debugWindowWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_SIZE) {
		recalcLayout(hWnd,LOWORD(lParam),HIWORD(lParam));
		return 0;
	}
	if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) {
		DestroyWindow(hWnd);
	}
	return DefWindowProcW(hWnd,message,wParam,lParam);
}

static const wchar_t *registerDebugClass(HINSTANCE hInstance) {

	static const wchar_t *classname = L"LightSpeed-Exception-Debug-Window";

	WNDCLASSW wclass;
	ZeroMemory(&wclass,sizeof(wclass));
	wclass.hCursor = ::LoadCursor(0,IDC_ARROW);
	wclass.hIcon = ::LoadIcon(0,IDI_ERROR);
	wclass.lpfnWndProc = (WNDPROC)debugWindowWinProc;
	wclass.lpszClassName = classname;
	wclass.hInstance = hInstance;
	wclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	RegisterClassW(&wclass);
	return classname;

}


typedef struct _MARGINS {
	int cxLeftWidth;
	int cxRightWidth;
	int cyTopHeight;
	int cyBottomHeight;
} MARGINS, *PMARGINS;


static void exceptionWindow(HINSTANCE hInstance, const wchar_t *wmsg) {
	const wchar_t *classname = registerDebugClass(hInstance);

	wchar_t appName[MAX_PATH];
	GetModuleFileNameW(hInstance,appName,MAX_PATH);

	HWND topWnd = CreateWindowExW(0,classname,appName,WS_OVERLAPPEDWINDOW
		|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,800,300,0,0,hInstance,0);
	RECT rc;

	GetClientRect(topWnd,&rc);
	HWND button = CreateWindowW(L"BUTTON",L"OK",BS_PUSHBUTTON
		|BS_CENTER|WS_CHILD|WS_VISIBLE,0,0,0,0,topWnd,(HMENU)IDCANCEL,
		hInstance,0);
	HWND textArea = CreateWindowW(L"EDIT",wmsg,
		WS_VISIBLE|WS_CHILD|ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_BORDER,
		0,0,0,0,topWnd,(HMENU)IDHELP,hInstance,0);
	recalcLayout(topWnd,rc.right,rc.bottom);
	MSG msg;
	SendMessage(textArea,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);;
	SendMessage(button,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);;
	MessageBeep(MB_ICONERROR);
	while (GetMessage(&msg,0,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (!IsWindow(topWnd)) break;
	}
}


void LightSpeed::IApp::exceptionDebug(const Exception &e, IApp *app) {
	HINSTANCE hInst = 0;
	IServices *svc = dynamic_cast<IServices *>(app);
	if (svc) {
		HInstanceService *hinstsvc = app->getIfcPtr<HInstanceService>();
		if (hinstsvc) {
			hInst = hinstsvc->getHInstance();
		}
	}
	if (hInst == 0) 
		hInst = (HINSTANCE)GetModuleHandle(0);

	

	exceptionWindow(hInst,String(e.getMessageWithReason() + String("\r\n\r\n-----\r\n")+e.getLongMessage()).c_str());
}

void LightSpeed::AppBase::exceptionDebugLog(const Exception &e) {
	String longMsg = e.getLongMessage();
	OutputDebugStringW(String((longMsg + ConstStrW(L"\r\n"))).c_str());
	onException(e);
}

static void exceptionOnStartup(HINSTANCE hInstance, const std::exception &e) {

	const char *instr = e.what();
	int len = (int)strlen(instr);
	int wlen = MultiByteToWideChar(CP_UTF8,0,instr,len,0,0);
	wchar_t *wmsg = (wchar_t *)alloca(sizeof(wchar_t)*(wlen+1));
	MultiByteToWideChar(CP_UTF8,0,instr,len,wmsg,wlen);
	wmsg[wlen] = 0;
	exceptionWindow(hInstance,wmsg);

}

int _tmain(int argc, _TCHAR *argv[]) {

	return LightSpeed::startApp(GetModuleHandle(0));
}

namespace LightSpeed {

static void disableHandleInheritance(HANDLE h) {
	if (h == 0 || h == INVALID_HANDLE_VALUE) return;
	SetHandleInformation(h,HANDLE_FLAG_INHERIT,0);
}

static void disableHandleInheritance() {
	
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	disableHandleInheritance(h);
	h = GetStdHandle(STD_OUTPUT_HANDLE);
	disableHandleInheritance(h);
	h = GetStdHandle(STD_ERROR_HANDLE);
	disableHandleInheritance(h);
}

int WINAPI startApp( HINSTANCE hInstance )
{

	try {

		AppBase &app = AppBase::current();
		IServices &svc = dynamic_cast<IServices &>(app);		
		HInstanceService hinstSvc(hInstance,app);

		LPWSTR *szArglist;
		int nArgs;


		szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if( NULL == szArglist )
		{
			throw ErrNoWithDescException(THISLOCATION,GetLastError(),L"Failed to read command line");
		}		

		//support for ProgInstance
		if (nArgs == 3 && wcsstr(szArglist[1], enterDaemonCmd) == 0) {			
			unsigned long long h;
			if (wscanf_s(szArglist[2], L"%llu", &h) != 1) return -1;
			DWORD_PTR handle = (DWORD_PTR)h;
			stubEnterDaemonMode(h);
			return 0;
		}

		String str;

		{
			AutoArray<wchar_t> buffer;
			buffer.resize(32768);
			DWORD sz = GetModuleFileNameW(hInstance,buffer.data(),32768);
			buffer.resize(sz);
			str = buffer;
		}

		disableHandleInheritance();
		
		app.init();
		int res = app.main_entry(nArgs,szArglist,str);
		app.done();
		LocalFree(szArglist);
		_CrtDumpMemoryLeaks();
		return res;
	} catch (std::exception &e) {
		exceptionOnStartup(hInstance,e);
		return -1;
	}
}
}

#pragma init_seg (lib)
MemoryMonitor memMon;

 