#include "resource.h"
#include<Windows.h>
#include<Processthreadsapi.h>
#include<sddl.h>
#include<NTSecAPI.h>
#include <WindowsX.h>
#include<Psapi.h>
#include<strsafe.h>
#include <CommCtrl.h>

#define LOAD_PROCESS_BUTTON 8
#define CHECKING_PRIVILEGES_BUTTON 9

HINSTANCE handle;
CONST CHAR *privelegiesNames[5] = { "SeCreatePermanentPrivilege", "SeEnableDelegationPrivilege","SeIncreaseBasePriorityPrivilege","SeManageVolumePrivilege","SeShutdownPrivilege" };
HWND ProcessesListBox, hWnd = NULL, sidHWND, userName, groupsListBox, privilegesListBox;

VOID Exit(HWND hWnd) {
	int mbResult = MessageBox(hWnd, TEXT("Действительно выйти?"), TEXT("Выход"), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	if (IDYES == mbResult)
		PostQuitMessage(EXIT_SUCCESS);
}

VOID CreatingElements(HINSTANCE hInstance, HWND hWnd)
{
	CreateWindow("BUTTON", "Процессы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 10, 510, 470, hWnd, (HMENU)-1, hInstance, NULL);
	ProcessesListBox = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("listbox"), "", WS_CHILD | WS_VISIBLE |WS_HSCROLL |WS_VSCROLL | ES_AUTOVSCROLL |ES_AUTOHSCROLL | LBS_NOTIFY, 15, 55, 500, 300, hWnd, NULL, NULL, NULL);
	CreateWindow("static", "Имя учётной записи", WS_CHILD | WS_VISIBLE, 15, 410, 100, 30, hWnd, (HMENU)1, hInstance, NULL);
	userName = CreateWindow("static", "", WS_CHILD | WS_VISIBLE, 115, 410, 400, 30, hWnd, (HMENU)1, hInstance, NULL);
	CreateWindow("static", "SID учётной записи", WS_CHILD | WS_VISIBLE, 15, 440, 100, 30, hWnd, (HMENU)1, hInstance, NULL);
	sidHWND = CreateWindow("static", "", WS_CHILD | WS_VISIBLE, 115, 440, 400, 30, hWnd, (HMENU)1, hInstance, NULL);
	groupsListBox = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("listbox"), "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | WS_HSCROLL, 540, 35, 350, 440, hWnd, NULL, NULL, NULL);
	privilegesListBox = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("listbox"), "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | WS_HSCROLL, 920, 35, 350, 440, hWnd, NULL, NULL, NULL);
	CreateWindow("BUTTON", "Привилегии", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 910, 10, 370, 470, hWnd, (HMENU)-1, hInstance, NULL);
	CreateWindow("BUTTON", "Группы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 530, 10, 370, 470, hWnd, (HMENU)-1, hInstance, NULL);
	SendMessage(ProcessesListBox, LB_SETHORIZONTALEXTENT, 700, 0);
	SendMessage(groupsListBox, LB_SETHORIZONTALEXTENT, 700, 0);
	SendMessage(privilegesListBox, LB_SETHORIZONTALEXTENT, 700, 0);
	CreateWindow("BUTTON", "Проверка привилегий", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 355, 200, 30, hWnd, (HMENU)CHECKING_PRIVILEGES_BUTTON, hInstance, NULL);
	CreateWindow("BUTTON", "Загрузить список всех процессов", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 355, 250, 30, hWnd, (HMENU)LOAD_PROCESS_BUTTON, hInstance, NULL);
	CreateWindow("static", "PID", WS_CHILD | WS_VISIBLE, 20, 30, 100, 20, hWnd, (HMENU)1, hInstance, NULL);
	CreateWindow("static", "Название", WS_CHILD | WS_VISIBLE, 120, 30, 100, 20, hWnd, (HMENU)1, hInstance, NULL);
}

VOID LoadProcesses()
{
	ListBox_ResetContent(ProcessesListBox);
	DWORD aProcessIds[1024], cbNeeded;
	if (EnumProcesses(aProcessIds, sizeof(aProcessIds), &cbNeeded))
	{
		TCHAR szName[MAX_PATH], szBuffer[300];
		DWORD n = cbNeeded / sizeof(DWORD);
		for (DWORD i = 0; i < n; ++i)
		{
			DWORD dwProcessId = aProcessIds[i], cch = 0;
			if (0 == dwProcessId) continue;
			HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
			if (NULL != hProcess)
			{
				cch = GetModuleBaseName(hProcess, NULL, szName, MAX_PATH);
				CloseHandle(hProcess);
			}
			if (0 == cch)
				StringCchCopy(szName, MAX_PATH, TEXT("Неизвестный процесс"));
			StringCchPrintf(szBuffer, _countof(szBuffer), TEXT("%.6u      %s"), dwProcessId, szName);
			int iItem = ListBox_AddString(ProcessesListBox, szBuffer);
			ListBox_SetItemData(ProcessesListBox, iItem, dwProcessId);
		}
	}
}

VOID UserNameSID() {
	int index = ListBox_GetCurSel(ProcessesListBox);
	DWORD ProcessId = ListBox_GetItemData(ProcessesListBox, index);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (NULL != hProcess)
	{
		HANDLE token;
		if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &token) != FALSE)
		{
			DWORD size;
			GetTokenInformation(token, TokenUser, NULL, 0, &size);
			PTOKEN_USER pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, size);
			if (GetTokenInformation(token,TokenUser, pTokenUser, size, &size) != FALSE)
			{
				LPTSTR sidString;
				ConvertSidToStringSid(pTokenUser->User.Sid, &sidString);
				SetWindowText(sidHWND, sidString);
				LPTSTR name, domainName;
				DWORD nameSize = 0, domainNameSize = 0;
				SID_NAME_USE SIDtype;
				LookupAccountSid(NULL, pTokenUser->User.Sid, NULL, &nameSize, NULL, &domainNameSize, NULL);
				if ((nameSize > 0) && (domainNameSize > 0))
				{
					name = new TCHAR[nameSize];
					domainName = new TCHAR[domainNameSize];
					if (LookupAccountSid(NULL, pTokenUser->User.Sid, name, &nameSize, domainName, &domainNameSize, &SIDtype) != FALSE)
					{
						StringCchPrintf(sidString, nameSize + domainNameSize + 2, TEXT("%s\\%s"), domainName, name);
						SetWindowText(userName, sidString);
					}
					else {
						SetWindowText(sidHWND, "Ошибка");
						SetWindowText(userName, "Ошибка");
					}
					delete[]domainName;
					delete[]name;
				}
				else {
					SetWindowText(sidHWND, "Ошибка");
					SetWindowText(userName, "Ошибка");
				}
			}
			else
			{
				SetWindowText(sidHWND, "Ошибка");
				SetWindowText(userName, "Ошибка");
			}
			CloseHandle(token);
		}
		else
		{
			SetWindowText(sidHWND, "Ошибка");
			SetWindowText(userName, "Ошибка");
		}
		CloseHandle(hProcess);
	}
	else
	{
		SetWindowText(sidHWND, "Ошибка");
		SetWindowText(userName, "Ошибка");
	}
}

VOID GroupsSID()
{
	ListBox_ResetContent(groupsListBox);
	int index = ListBox_GetCurSel(ProcessesListBox);
	DWORD ProcessId = ListBox_GetItemData(ProcessesListBox, index);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (NULL != hProcess)
	{
		HANDLE token;
		if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &token) != FALSE)
		{
			DWORD size;
			GetTokenInformation(token, TokenGroups, NULL, 0, &size);
			PTOKEN_GROUPS pTokenGroups = (PTOKEN_GROUPS)LocalAlloc(LPTR, size);
			if (GetTokenInformation(token, TokenGroups, pTokenGroups, size, &size) != FALSE)
			{
				for (DWORD i = 0; i < pTokenGroups->GroupCount; i++)
				{
					LPTSTR name, domainName;
					DWORD nameSize = 0, domainNameSize = 0;
					SID_NAME_USE SIDtype;
					LookupAccountSid(NULL, pTokenGroups->Groups[i].Sid, NULL, &nameSize, NULL, &domainNameSize, NULL);
					if ((nameSize > 0) && (domainNameSize > 0))
					{
						name = new TCHAR[nameSize];
						domainName = new TCHAR[domainNameSize];
						if (LookupAccountSid(NULL, pTokenGroups->Groups[i].Sid, name, &nameSize, domainName, &domainNameSize, &SIDtype) != FALSE)
						{
							LPTSTR sidString = new CHAR[nameSize + domainNameSize + 2];
							StringCchPrintf(sidString, nameSize + domainNameSize + 2, TEXT("%s\\%s"), domainName, name);
							ListBox_AddString(groupsListBox, sidString);
							delete[]sidString;
						}
						else 
						{
							ListBox_ResetContent(groupsListBox);
						}
						delete[]domainName;
						delete[]name;
					}
					else
					{
						ListBox_ResetContent(groupsListBox);
					}
				}
			}
			else
			{
				ListBox_ResetContent(groupsListBox);
			}
			CloseHandle(token);
		}
		else
		{
			ListBox_ResetContent(groupsListBox);
		}
		CloseHandle(hProcess);
	}
	else
	{
		ListBox_ResetContent(groupsListBox);
	}
}

VOID ErrorReceivingPrivileges( HWND hwndList) {
	ListView_DeleteAllItems(hwndList);
	for (INT i = 0; i < 5; i++)
	{
		LVITEM lvItem = { };
		lvItem.iItem = ListView_InsertItem(hwndList, &lvItem);
		ListView_SetItemText(hwndList, lvItem.iItem, 0, (LPSTR)privelegiesNames[i]);
		ListView_SetItemText(hwndList, lvItem.iItem, 1, (LPSTR)"Error");
		ListView_SetItemText(hwndList, lvItem.iItem, 2, (LPSTR)"Error");
	}
}

VOID Privileges()
{
	ListBox_ResetContent(privilegesListBox);
	INT index = ListBox_GetCurSel(ProcessesListBox);
	DWORD ProcessId = ListBox_GetItemData(ProcessesListBox, index);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (NULL != hProcess)
	{
		HANDLE token;
		if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &token) != FALSE)
		{
			DWORD size;
			GetTokenInformation(token, TokenPrivileges, NULL, 0, &size);
			PTOKEN_PRIVILEGES pTokenPrivileges = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, size);
			if (GetTokenInformation(token, TokenPrivileges, pTokenPrivileges, size, &size) != FALSE)
			{
				for (DWORD i = 0; i < pTokenPrivileges->PrivilegeCount; i++)
				{
					CHAR name[MAX_PATH];
					DWORD nameSize= MAX_PATH;
					LookupPrivilegeName(NULL , &(LUID)(pTokenPrivileges->Privileges[i].Luid), name, &nameSize);
					if (SE_PRIVILEGE_ENABLED & ((DWORD)(pTokenPrivileges->Privileges[i].Attributes)))
					{
						CHAR priligeString[2*MAX_PATH];
						StringCchPrintf(priligeString, 2 * MAX_PATH, TEXT("ENABLED - %s"), name);
						ListBox_AddString(privilegesListBox, priligeString);
					}
					else
					{
						CHAR priligeString[MAX_PATH];
						StringCchPrintf(priligeString, 2 * MAX_PATH, TEXT("DISABLED - %s"), name);
						ListBox_AddString(privilegesListBox, priligeString);
					}
				}
			}
			else
			{
				ListBox_ResetContent(privilegesListBox);
			}
			CloseHandle(token);
		}
		else
		{
			ListBox_ResetContent(privilegesListBox);
		}
		CloseHandle(hProcess);
	}
	else
	{
		ListBox_ResetContent(privilegesListBox);
	}
}

BOOL CALLBACK PrivilegesCheckingFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST2);
	switch (uMsg)
	{
	case WM_INITDIALOG:{
		SendMessage(hwndDlg, WM_SETTEXT, 0, (LPARAM)"Проверка привилегий");
		LVCOLUMN lvColumns[] = { { LVCF_WIDTH | LVCF_TEXT, 0, 200, (LPTSTR)TEXT("Название") },  { LVCF_WIDTH | LVCF_TEXT, 0, 100, (LPTSTR)TEXT("Наличие") }, { LVCF_WIDTH | LVCF_TEXT, 0, 150, (LPTSTR)TEXT("Состояние") } };
		
		for (INT i = 0; i < _countof(lvColumns); ++i)
		{
			ListView_InsertColumn(hwndList, i, &lvColumns[i]);
		}
		for (INT i = 0; i < 5; i++)
		{
			LVITEM lvItem = {};
			lvItem.iItem = ListView_InsertItem(hwndList, &lvItem);
			ListView_SetItemText(hwndList, lvItem.iItem, 0, (LPSTR)privelegiesNames[i]);
		}
		INT index = ListBox_GetCurSel(ProcessesListBox);
		DWORD ProcessId = ListBox_GetItemData(ProcessesListBox, index);
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
		if (NULL != hProcess)
		{
			HANDLE token;
			if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &token) != FALSE)
			{
				DWORD size;
				GetTokenInformation(token, TokenPrivileges, NULL, 0, &size);
				PTOKEN_PRIVILEGES pTokenPrivileges = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, size);
				if (GetTokenInformation(token, TokenPrivileges, pTokenPrivileges, size, &size) != FALSE)
				{
					for (INT j = 0; j < 5; j++)
					{
						INT index;
						LUID luid;
						LookupPrivilegeValue(NULL, privelegiesNames[j], &luid);
						BOOL check = FALSE;
						for (DWORD i = 0; i < pTokenPrivileges->PrivilegeCount; i++)
						{
							INT res = memcmp(&pTokenPrivileges->Privileges[i].Luid, &luid, sizeof(LUID));
							if (res == 0)
							{
								check = TRUE;
								index = i;
								break;
							}
						}
						LVFINDINFO fi = {  };
						fi.psz = privelegiesNames[j];
						INT iItem = ListView_FindItem(hwndList, -1, &fi);
						if (check)
						{
							ListView_SetItemText(hwndList, iItem, 1, (LPSTR)"+");
							if (SE_PRIVILEGE_ENABLED & ((DWORD)(pTokenPrivileges->Privileges[index].Attributes)))
							{
								ListView_SetItemText(hwndList, iItem, 2, (LPSTR)"Enabled");
							}
							else
							{
								ListView_SetItemText(hwndList, iItem, 2, (LPSTR)"Disabled");
							}
						}
						else {
							ListView_SetItemText(hwndList, iItem, 1, (LPSTR)"-");
							ListView_SetItemText(hwndList, iItem, 2, (LPSTR)"-");
						}
					}
				}
				else
				{
					ErrorReceivingPrivileges(hwndList);
				}
				CloseHandle(token);
			}
			else
			{
				ErrorReceivingPrivileges(hwndList);
			}
			CloseHandle(hProcess);
		}
		else
		{
			ErrorReceivingPrivileges(hwndList);
		}
		return TRUE;
	}
	case WM_CLOSE:{
		EndDialog(hwndDlg, 0);
		return TRUE;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK)
			EndDialog(hwndDlg, 0);
		if (LOWORD(wParam) == IDCANCEL)
			EndDialog(hwndDlg, 0);
		if (LOWORD(wParam) == IDHELP) {
			INT iPos = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
			if (iPos == -1) {
				MessageBox(hWnd, TEXT("Не выбран ни один элемент!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
			}
			else {
				CHAR buffer[15];
				ListView_GetItemText(hwndList, iPos, 2, buffer, 15);
				if ((buffer[0] == '-') || (buffer[1] == 'r')){
					MessageBox(hWnd, TEXT("Выбранное состояние изменить невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
				}
				else {
					INT index = ListBox_GetCurSel(ProcessesListBox);
					DWORD ProcessId = ListBox_GetItemData(ProcessesListBox, index);
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
					if (NULL != hProcess)
					{
						HANDLE token;
						if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &token) != FALSE)
						{
							DWORD size;
							GetTokenInformation(token, TokenPrivileges, NULL, 0, &size);
							PTOKEN_PRIVILEGES pTokenPrivileges = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, size);
							if (GetTokenInformation(token, TokenPrivileges, pTokenPrivileges, size, &size) != FALSE)
							{
								TOKEN_PRIVILEGES Privileges;
								Privileges.PrivilegeCount = 1;
								CHAR buffer[15];
								ListView_GetItemText(hwndList, iPos, 2, buffer, 15);
								BOOL what;
								if (buffer[0] == 'E'){
									Privileges.Privileges[0].Attributes = 0;
									what = FALSE;
								}
								else {
									Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
									what = TRUE;

								}
								CHAR lpszPrivilegeName[30];
								ListView_GetItemText(hwndList, iPos, 0, lpszPrivilegeName, 30);
								if (FALSE != LookupPrivilegeValue(NULL, lpszPrivilegeName, &Privileges.Privileges[0].Luid)) {
									if (AdjustTokenPrivileges(token, FALSE, &Privileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL) != FALSE) {
										if (what) {
											ListView_SetItemText(hwndList, iPos, 2, (LPSTR)"Enabled");
										}
										else
											ListView_SetItemText(hwndList, iPos, 2, (LPSTR)"Disabled");
									}
									else {
										MessageBox(hWnd, TEXT("Состояние привилегии не изменяется(((!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
									}
								}
								else {
									MessageBox(hWnd, TEXT("Состояние привилегии не изменяется(((!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
								}
							}
							else
							{
								MessageBox(hWnd, TEXT("Состояние привилегии не изменяется(((!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);

							}
							CloseHandle(token);
						}
						else
						{
							MessageBox(hWnd, TEXT("Состояние привилегии не изменяется(((!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
						}
						CloseHandle(hProcess);
					}
					else
					{
						MessageBox(hWnd, TEXT("Состояние привилегии не изменяется(((!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
					}
				}
			}
		}
		return TRUE;
	}
	}
	return FALSE;
}

VOID PrivilegesChecking()
{
	if (ListBox_GetCurSel(ProcessesListBox) == -1)
	{
		MessageBox(hWnd, TEXT("Не выбран процесс"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
	}
	else
	{
		DialogBox(handle, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)PrivilegesCheckingFunc);
	}
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		Exit(hWnd);
		return 0;
	case WM_QUIT:
		DestroyWindow(hWnd);
		return 0;
	case WM_CREATE:
		CreatingElements(handle, hWnd);
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) == LOAD_PROCESS_BUTTON)
		{
			LoadProcesses();
		}
		if (LOWORD(wParam) == CHECKING_PRIVILEGES_BUTTON)
		{
			PrivilegesChecking();
			Privileges();

		}
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			UserNameSID();
			GroupsSID();
			Privileges();
		}
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

WNDCLASSEX RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX window = { sizeof(WNDCLASSEX) };
	window.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	window.lpfnWndProc = WindowProcedure;
	window.hInstance = hInstance;
	window.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	window.hCursor = LoadCursor(NULL, IDC_ARROW);
	window.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	window.lpszMenuName = NULL;
	window.lpszClassName = TEXT("WindowClass");
	window.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	return window;
}

VOID MessagesProcessing() {
	MSG  msg;
	BOOL bRet;
	while (bRet = GetMessage(&msg, NULL, 0, 0) != FALSE)
	{
		if (bRet != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpszCmdLine, int nCmdShow)
{
	handle = hInstance;
	if (!RegisterClassEx(&RegisterWindowClass(hInstance)))
		return EXIT_FAILURE;
	LoadLibrary(TEXT("ComCtl32.dll"));
	hWnd = CreateWindowEx(0, TEXT("WindowClass"), TEXT("Моя прога"), WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 1310, 530, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL)
		return EXIT_FAILURE;
	ShowWindow(hWnd, nCmdShow);
	MessagesProcessing();
	return EXIT_SUCCESS;
}