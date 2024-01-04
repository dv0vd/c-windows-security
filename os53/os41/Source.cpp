#include<Windows.h>
#include <strsafe.h>
#include"resource.h"
#include <Shlobj_core.h>
#include<AclAPI.h>
#include<sddl.h>

#define CHOOSE_FILE_BUTTON 1
#define CHANGE_OWNER_BUTTON 2
#define REMOVE_ACE 3
#define CHOOSE_FOLDER_BUTTON 4
#define ADD_ACE 5
#define IDC_RADIO1                      1007
#define IDC_LIST 6

HINSTANCE handle;
HWND hWnd = NULL, hName, hOwner, hwndLV;
CHAR fileName[MAX_PATH], ownerName[MAX_PATH], newOwnerName[MAX_PATH]; 
PACL pDacl=NULL;
constexpr DWORD dwInherit[7] = {
	NO_INHERITANCE,
	SUB_CONTAINERS_AND_OBJECTS_INHERIT,
	SUB_CONTAINERS_ONLY_INHERIT,
	SUB_OBJECTS_ONLY_INHERIT,
	INHERIT_ONLY | SUB_CONTAINERS_AND_OBJECTS_INHERIT,
	INHERIT_ONLY | SUB_CONTAINERS_ONLY_INHERIT,
	INHERIT_ONLY | SUB_OBJECTS_ONLY_INHERIT
};
constexpr LPCTSTR szInheritText[7] = {
	TEXT("Только для этого каталога"),
	TEXT("Для этого каталога, его подкаталогов и файлов"),
	TEXT("Для этого каталога и его подкаталогов"),
	TEXT("Для этого каталога и его файлов"),
	TEXT("Только для подкаталогов и файлов"),
	TEXT("Только для подкаталогов"),
	TEXT("Только для файлов")
};

VOID CreatingElements(HINSTANCE hInstance, HWND hWnd)
{
	CreateWindow("static", "Имя", WS_CHILD | WS_VISIBLE, 10, 10, 130, 30, hWnd, (HMENU)1, hInstance, NULL);
	hName = CreateWindow("static", "", WS_CHILD | WS_VISIBLE, 140, 10, 570, 30, hWnd, (HMENU)1, hInstance, NULL);
	CreateWindow("static", "Владелец", WS_CHILD | WS_VISIBLE, 10, 40, 130, 30, hWnd, (HMENU)1, hInstance, NULL);
	hOwner = CreateWindow("static", "", WS_CHILD | WS_VISIBLE, 140, 40, 570, 30, hWnd, (HMENU)1, hInstance, NULL);
	hwndLV = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT| LVS_SINGLESEL, 10, 80, 700, 200,hWnd, (HMENU)IDC_LIST,hInstance,NULL);
	ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
	LVCOLUMN lvColumns[] = {
		{ LVCF_WIDTH | LVCF_TEXT, 0, 100, (LPTSTR)TEXT("Тип") },
		{ LVCF_WIDTH | LVCF_TEXT, 0, 200, (LPTSTR)TEXT("Имя") },
		{ LVCF_WIDTH | LVCF_TEXT, 0, 400, (LPTSTR)TEXT("Применять") },
	};
	for (int i = 0; i < _countof(lvColumns); ++i)
	{
		ListView_InsertColumn(hwndLV, i, &lvColumns[i]);
	} 
	CreateWindow("BUTTON", "Выбрать файл", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10 , 220+100, 130, 50, hWnd, (HMENU)CHOOSE_FILE_BUTTON, hInstance, NULL);
	CreateWindow("BUTTON", "Выбрать каталог", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 150, 220+100, 130, 50, hWnd, (HMENU)CHOOSE_FOLDER_BUTTON, hInstance, NULL);
	CreateWindow("BUTTON", "Изменить владельца", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 290, 220+100, 150, 50, hWnd, (HMENU)CHANGE_OWNER_BUTTON, hInstance, NULL);
	CreateWindow("BUTTON", "Удалить ACE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 450, 220 + 100, 130, 50, hWnd, (HMENU)REMOVE_ACE, hInstance, NULL);
	CreateWindow("BUTTON", "Добавить ACE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 590, 220 + 100, 130, 50, hWnd, (HMENU)ADD_ACE, hInstance, NULL);
}

VOID ClearingFileName(BOOL whatToClear) {
	if (whatToClear) {
		INT l;
		for (INT i = 0; i < MAX_PATH; i++)
			if (fileName[i] == '\0') {
				l = i;
				break;
			}
		for (INT i = l; i < MAX_PATH; i++)
			fileName[i] = '\0';
	}
	else {
		INT l;
		for (INT i = 0; i < MAX_PATH; i++)
			if (newOwnerName[i] == '\0') {
				l = i;
				break;
			}
		for (INT i = l; i < MAX_PATH; i++)
			newOwnerName[i] = '\0';
	}
}

BOOL CALLBACK NewNameDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendMessage(hwndDlg, WM_SETTEXT, 0, LPARAM(TEXT("Новый владелец")));
		return TRUE;
	case WM_COMMAND: {
		if (LOWORD(wParam) == IDOK) {
			HWND hDialog = GetActiveWindow();
			GetDlgItemText(hDialog, IDHELP, newOwnerName, _countof(newOwnerName));
			ClearingFileName(FALSE);
			EndDialog(hwndDlg, 0);
		}
		if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hwndDlg, 1);
		}
		return TRUE;
	}
	}
	return FALSE;
}

BOOL CALLBACK NewAceDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendMessage(hwndDlg, WM_SETTEXT, 0, LPARAM(TEXT("Новый ACE")));
		CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
		CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO3);
		return TRUE;
	case WM_COMMAND: {
		if (LOWORD(wParam) == IDOK) {
			CHAR ACEName[MAX_PATH];
			HWND hDialog = GetActiveWindow();
			GetDlgItemText(hDialog, IDC_EDIT1, ACEName, _countof(ACEName));
			if (ACEName[0] != '\0')
			{
				DWORD cbSid = 0, domainNameSize = 0;
				LookupAccountName(NULL, ACEName, NULL, &cbSid, NULL, &domainNameSize, NULL);
				if ((cbSid > 0) && (domainNameSize > 0))
				{
					LPTSTR domainName;
					domainName = new TCHAR[domainNameSize];
					PSID ownerSid = (PSID)LocalAlloc(LMEM_FIXED, cbSid);
					SID_NAME_USE SIDtype;
					if (FALSE != LookupAccountName(NULL, ACEName, ownerSid, &cbSid, domainName, &domainNameSize, &SIDtype))
					{
						INT count = 0;
						EXPLICIT_ACCESS ea;
						SECURITY_DESCRIPTOR sd;
						if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) != FALSE)
						{
							ea.Trustee.TrusteeForm = TRUSTEE_IS_SID; 
							ea.Trustee.ptstrName = (LPTSTR)ownerSid;
							if (IsDlgButtonChecked(hDialog, IDC_RADIO1))
							{
								ea.grfAccessMode = GRANT_ACCESS;
							}
							else
							{
								ea.grfAccessMode = DENY_ACCESS;
							}
							ea.grfAccessPermissions = 0;
							if (IsDlgButtonChecked(hDialog, IDC_CHECK1))
							{
								count++;
								ea.grfAccessPermissions |= FILE_TRAVERSE;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK2))
							{
								count++;
								ea.grfAccessPermissions |= FILE_LIST_DIRECTORY;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK3))
							{
								count++;
								ea.grfAccessPermissions |= FILE_READ_ATTRIBUTES;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK4))
							{
								count++;
								ea.grfAccessPermissions |= FILE_READ_EA;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK5))
							{
								count++;
								ea.grfAccessPermissions |= FILE_ADD_FILE;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK6))
							{
								count++;
								ea.grfAccessPermissions |= FILE_ADD_SUBDIRECTORY;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK7))
							{
								count++;
								ea.grfAccessPermissions |= FILE_WRITE_ATTRIBUTES;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK8))
							{
								count++;
								ea.grfAccessPermissions |= FILE_WRITE_EA;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK9))
							{
								count++;
								ea.grfAccessPermissions |= FILE_DELETE_CHILD;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK10))
							{
								count++;
								ea.grfAccessPermissions |= DELETE;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK11))
							{
								count++;
								ea.grfAccessPermissions |= READ_CONTROL;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK12))
							{
								count++;
								ea.grfAccessPermissions |= WRITE_DAC;
							}
							if (IsDlgButtonChecked(hDialog, IDC_CHECK13))
							{
								count++;
								ea.grfAccessPermissions |= WRITE_OWNER;
							}
							if (count == 0)
							{
								MessageBox(hWnd, TEXT("Не выбрано ни одного расширения!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
							}
							else
							{
								if (IsDlgButtonChecked(hDialog, IDC_RADIO3))
								{
									ea.grfInheritance = NO_INHERITANCE;
								}
								else
								{
									if (IsDlgButtonChecked(hDialog, IDC_RADIO4))
									{
										ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
									}
									else
									{
										if (IsDlgButtonChecked(hDialog, IDC_RADIO5))
										{
											ea.grfInheritance = SUB_CONTAINERS_ONLY_INHERIT;
										}
										else
										{
											if (IsDlgButtonChecked(hDialog, IDC_RADIO6))
											{
												ea.grfInheritance = SUB_OBJECTS_ONLY_INHERIT;
											}
											else
											{
												if (IsDlgButtonChecked(hDialog, IDC_RADIO7))
												{
													ea.grfInheritance = (INHERIT_ONLY | SUB_CONTAINERS_AND_OBJECTS_INHERIT);
												}
												else
												{
													if (IsDlgButtonChecked(hDialog, IDC_RADIO8))
													{
														ea.grfInheritance = (INHERIT_ONLY | SUB_CONTAINERS_ONLY_INHERIT);
													}
													else
													{
														ea.grfInheritance = (INHERIT_ONLY | SUB_OBJECTS_ONLY_INHERIT);
													}
												}
											}
										}
									}
								}
								if ((NO_INHERITANCE != ea.grfInheritance) && (IsDlgButtonChecked(hDialog, IDC_CHECK14)))
								{
									ea.grfInheritance |= INHERIT_NO_PROPAGATE;
								}
								if (SetSecurityDescriptorOwner(&sd, ownerSid, FALSE) != FALSE)
								{
									PSECURITY_DESCRIPTOR pOldSD = NULL;
									PACL pOldDacl = NULL;
									BOOL bDaclDefaulted = FALSE;
									DWORD cb = 0;
									GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, NULL, 0, &cb);
									PACL pNewDacl = NULL;
									pOldSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cb);
									if (pOldSD != NULL)
									{
										if (GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, pOldSD, cb, &cb) != FALSE)
										{
											BOOL bDaclPresent;
											GetSecurityDescriptorDacl(pOldSD, &bDaclPresent, &pOldDacl, &bDaclDefaulted);
											if (SetEntriesInAcl(1, &ea, pOldDacl, &pNewDacl) == ERROR_SUCCESS)
											{
												if (SetSecurityDescriptorDacl(&sd, TRUE, pNewDacl, bDaclDefaulted) != FALSE)
												{
													SECURITY_INFORMATION si = 0;
													if (NULL != ownerSid) si |= OWNER_SECURITY_INFORMATION;
													if (NULL != pNewDacl) si |= DACL_SECURITY_INFORMATION;
													if (SetFileSecurity(fileName, si, &sd) == FALSE)
													{
														MessageBox(hWnd, TEXT("Добавить АСЕ невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
														EndDialog(hwndDlg, 0);
													}
													else
													{
														EndDialog(hwndDlg, 0);
													}
												}
												else
												{
													MessageBox(hWnd, TEXT("Добавить АСЕ невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
												}
											}
											else
											{
												MessageBox(hWnd, TEXT("Добавить АСЕ невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
											}
										}
										else
										{
											MessageBox(hWnd, TEXT("Добавить АСЕ невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
										}

									}
									else
									{
										MessageBox(hWnd, TEXT("Добавить АСЕ невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
									}
									LocalFree(pOldSD);

								}
								else
								{
									MessageBox(hWnd, TEXT("Невозможно установить владельца дескриптора безопасности!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
								}
								LocalFree(ownerSid);
							}
						}
						else
						{
							MessageBox(hWnd, TEXT("Ошибка инициализации дескриптора безопасности!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
						}
					}
					delete[]domainName;
				}
				else
				{
					MessageBox(hWnd, TEXT("Проверьте правильность ввода имени!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
				}
			}
			else
			{
				MessageBox(hWnd, TEXT("Имя владельца не может быть пустым!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
			}
		}
		if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hwndDlg, 1);
		}
		if (IDC_RADIO1 <= LOWORD(wParam) && LOWORD(wParam) <= IDC_RADIO2)
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
			{
				CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO3);
			}
			else
			{
				if (IsDlgButtonChecked(hwndDlg, IDC_RADIO4))
				{
					CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO4);
				}
				else
				{
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO5))
					{
						CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO5);
					}
					else
					{
						if (IsDlgButtonChecked(hwndDlg, IDC_RADIO6))
						{
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO6);
						}
						else
						{
							if (IsDlgButtonChecked(hwndDlg, IDC_RADIO7))
							{
								CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO7);
							}
							else
							{
								if (IsDlgButtonChecked(hwndDlg, IDC_RADIO8))
								{
									CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO8);
								}
								else
								{
									CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO9);
								}
							}
						}
					}
				}
			}
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, LOWORD(wParam));
		}
		if (IDC_RADIO3 <= LOWORD(wParam) && LOWORD(wParam) <= IDC_RADIO9)
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
			{
				CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
			}
			else
			{
				CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
			}
			CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, LOWORD(wParam));
		}
		return TRUE;
	}
	}
	return FALSE;
}

VOID MakeInactive(HWND hwndDlg)
{
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO1), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO2), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO3), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO4), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO5), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO6), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO7), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO8), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO9), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK1), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK2), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK3), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK4), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK5), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK6), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK7), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK8), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK9), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK10), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK11), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK12), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK13), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK14), FALSE);
}

BOOL CALLBACK LoadACEDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SendMessage(hwndDlg, WM_SETTEXT, 0, LPARAM(TEXT("Элемент разрешения")));
		MakeInactive(hwndDlg);
		INT iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
		DWORD cb = 0;
		GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, NULL, 0, &cb);
		PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cb);
		if (pSD != FALSE)
		{
			if (GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, pSD, cb, &cb))
			{
				PACL pDacl;
				BOOL bDaclPresent, bDaclDefaulted;
				if (GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted) != FALSE)
				{
					ULONG uCount = 0;
					PEXPLICIT_ACCESS pEA = NULL;
					if (GetExplicitEntriesFromAcl(pDacl, &uCount, &pEA) == ERROR_SUCCESS)
					{
						if (pEA[iItem].grfAccessMode == GRANT_ACCESS)
						{
							CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
						}
						else
						{
							CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
						}
						INT index;
						DWORD grfInheritance = pEA[iItem].grfInheritance & (~INHERIT_NO_PROPAGATE);
						for (int j = 0; j < _countof(dwInherit); ++j)
						{
							if (grfInheritance == dwInherit[j])
							{
								index = j;
								break;
							}
						}
						switch (index)
						{
						case 0:
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO3);
							break;
						case 1:
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO4);
							break;
						case 2:
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO5);
							break;
						case 3:
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO6);
							break;
						case 4:
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO7);
							break;
						case 5:
							CheckRadioButton(hwndDlg, IDC_RADIO3, IDC_RADIO9, IDC_RADIO8);
							break;
						}
						if (INHERIT_NO_PROPAGATE & pEA[iItem].grfInheritance)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK14, 1);
						}
						if (TRUSTEE_IS_SID == pEA[iItem].Trustee.TrusteeForm)
						{
							LPTSTR name = NULL;
							LPTSTR domainName = NULL;
							DWORD nameSize = 0, domainNameSize = 0;
							LookupAccountSid(NULL, pEA[iItem].Trustee.ptstrName, NULL, &nameSize, NULL, &domainNameSize, NULL);
							if ((nameSize > 0) && (domainNameSize > 0))
							{
								name = new TCHAR[nameSize];
								domainName = new TCHAR[domainNameSize];
								SID_NAME_USE SIDtype;
								if (LookupAccountSid(NULL, pEA[iItem].Trustee.ptstrName, name, &nameSize, domainName, &domainNameSize, &SIDtype) != FALSE)
								{
									SendMessage(GetDlgItem(hwndDlg, IDC_EDIT1), WM_SETTEXT, 0, LPARAM(name));
									delete[]name;
									delete[]domainName;
								}
							}
						}
						if (pEA[iItem].grfAccessPermissions & FILE_TRAVERSE)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK1, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_LIST_DIRECTORY)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK2, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_READ_ATTRIBUTES)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK3, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_READ_EA)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK4, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_ADD_FILE)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK5, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_ADD_SUBDIRECTORY)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK6, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_WRITE_ATTRIBUTES)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK7, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_WRITE_EA)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK8, 1);
						}
						if (pEA[iItem].grfAccessPermissions & FILE_DELETE_CHILD)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK9, 1);
						}
						if (pEA[iItem].grfAccessPermissions & DELETE)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK10, 1);

						}
						if (pEA[iItem].grfAccessPermissions & READ_CONTROL)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK11, 1);
						}
						if (pEA[iItem].grfAccessPermissions & WRITE_DAC)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK12, 1);
						}
						if (pEA[iItem].grfAccessPermissions & WRITE_OWNER)
						{
							CheckDlgButton(hwndDlg, IDC_CHECK13, 1);
						}
					}
				}
			}
		}
		LocalFree(pSD);
		return TRUE;
	}
	case WM_COMMAND: 
	{
		if ((LOWORD(wParam) == IDCANCEL) || (LOWORD(wParam) == IDOK)) 
		{
			EndDialog(hwndDlg, 1);
		}
		return TRUE;
	}
	}
	return FALSE;
}

VOID LoadOwner() {
	DWORD cb = 0;
	GetFileSecurity(fileName, OWNER_SECURITY_INFORMATION, NULL, 0, &cb);
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cb);
	SetSecurityDescriptorControl(pSD, SE_DACL_PRESENT, SE_DACL_PRESENT);
	if (FALSE != GetFileSecurity(fileName, OWNER_SECURITY_INFORMATION, pSD, cb, &cb))
	{
		PSID ownerSid;
		BOOL bDefaulted;
		if (GetSecurityDescriptorOwner(pSD, &ownerSid, &bDefaulted) != FALSE)
		{
			LPTSTR name, domainName;
			DWORD nameSize = 0, domainNameSize = 0;
			SID_NAME_USE SIDtype;
			LookupAccountSid(NULL, ownerSid, NULL, &nameSize, NULL, &domainNameSize, NULL);
			if((nameSize > 0) && (domainNameSize > 0))
			{
				name = new TCHAR[nameSize];
				domainName = new TCHAR[domainNameSize];
				if (LookupAccountSid(NULL, ownerSid, name, &nameSize, domainName, &domainNameSize, &SIDtype) != FALSE)
				{
					for (INT i = 0; i < MAX_PATH; i++) {
						ownerName[i] = name[i];
					}
					SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)ownerName);
				}
				else {
					SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)"Error obtaining information");

				}
				delete[]domainName;
				delete[]name;
			}
			else {
				SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)"Error obtaining information");
			}
		}
		else {
			SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)"Error obtaining information");
		}
	}
	else
	{
		if(fileName[0] == '\0')
			SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)"");
		else
			SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)"Error obtaining information");
	}
	LocalFree(pSD);
}

VOID ChangeOwner() {
	if (DialogBox(handle, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)NewNameDialogProc) == 0) {
		if (newOwnerName[0] != '\0') 
		{
			DWORD cbSid = 0, domainNameSize = 0;
			LookupAccountName(NULL, newOwnerName, NULL, &cbSid, NULL, &domainNameSize, NULL);
			if ((cbSid > 0) && (domainNameSize > 0)) 
			{
				LPTSTR domainName;
				domainName = new TCHAR[domainNameSize];
				PSID ownerSid = (PSID)LocalAlloc(LMEM_FIXED, cbSid);
				SID_NAME_USE SIDtype;
				if(FALSE != LookupAccountName(NULL, newOwnerName, ownerSid, &cbSid, domainName, &domainNameSize, &SIDtype))
				{
					SECURITY_DESCRIPTOR sd;
					if (FALSE != InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
					{
						if (FALSE != SetSecurityDescriptorOwner(&sd, ownerSid, FALSE))
						{
							SECURITY_INFORMATION si = 0;
							si |= OWNER_SECURITY_INFORMATION;
							if (SetFileSecurity(fileName, si, &sd) == FALSE)
							{
								MessageBox(hWnd, TEXT("Изменить владельца невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
							}
						}
						else 
						{
							MessageBox(hWnd, TEXT("Изменить владельца невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
						}
					}
					else
					{
						MessageBox(hWnd, TEXT("Изменить владельца невозможно!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
					}
				}
				delete[]domainName;
				LocalFree(ownerSid);
			}
			else
			{
				MessageBox(hWnd, TEXT("Проверьте правильность ввода имени!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
			}
		}
		else {
			MessageBox(hWnd, TEXT("Имя владельца не может быть пустым!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
		}
	}
}

VOID LoadDACL()
{
	ListView_DeleteAllItems(hwndLV);
	DWORD cb = 0;
	GetFileSecurity(fileName,  DACL_SECURITY_INFORMATION, NULL, 0, &cb);
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cb);
	if (pSD != FALSE)
	{
		if(GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, pSD, cb, &cb)) 
		{
			ULONG uCount = 0; 
			PEXPLICIT_ACCESS pEA = NULL;
			BOOL bDaclPresent, bDaclDefaulted;
			BOOL bRet = GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted);
			if (FALSE != bRet && FALSE != bDaclPresent)
			{
				DWORD dwResult = GetExplicitEntriesFromAcl(pDacl, &uCount, &pEA);
				for (ULONG i = 0; i < uCount; ++i)
				{
					LVITEM lvItem = { LVIF_TEXT | LVIF_PARAM };
					lvItem.iItem = (INT)i;
					lvItem.lParam = (LPARAM)pEA[i].grfAccessPermissions;
					switch (pEA[i].grfAccessMode)
					{
						case GRANT_ACCESS:
							lvItem.pszText = (LPTSTR)TEXT("Разрешить");
							break;
						case DENY_ACCESS:
							lvItem.pszText = (LPTSTR)TEXT("Запретить");
							break;
					}
					INT iItem = ListView_InsertItem(hwndLV, &lvItem);
					if (iItem == -1) continue; 
					if (TRUSTEE_IS_SID == pEA[i].Trustee.TrusteeForm)
					{
						LPTSTR name = NULL; 
						LPTSTR domainName = NULL;
						DWORD nameSize = 0, domainNameSize = 0;
						LookupAccountSid(NULL, pEA[i].Trustee.ptstrName, NULL, &nameSize, NULL, &domainNameSize, NULL);
						if ((nameSize > 0) && (domainNameSize > 0))
						{
							name = new TCHAR[nameSize];
							domainName = new TCHAR[domainNameSize];
							SID_NAME_USE SIDtype;
							if (LookupAccountSid(NULL, pEA[i].Trustee.ptstrName, name, &nameSize, domainName, &domainNameSize, &SIDtype) != FALSE)
							{
								ListView_SetItemText(hwndLV, iItem, 1, name);
								delete[]name;
								delete[]domainName;
							}
						}
					}
					DWORD grfInheritance = pEA[i].grfInheritance & (~INHERIT_NO_PROPAGATE);
					for (int j = 0; j < _countof(dwInherit); ++j)
					{
						if (grfInheritance == dwInherit[j]) 
						{
							ListView_SetItemText(hwndLV, iItem, 2, (LPTSTR)szInheritText[j]);
							break;
						} 
					} 
				} 
				LocalFree(pEA);
			} 
		}
	}
	LocalFree(pSD);
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_CLOSE: {
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
	}
	case WM_QUIT: {
		DestroyWindow(hWnd);
		return 0;
	}
	case WM_CREATE:{
		CreatingElements(handle, hWnd);
		SetWindowText(hWnd, fileName);
		SendMessage(hName, WM_SETTEXT, 0, (LPARAM)fileName);
		SendMessage(hOwner, WM_SETTEXT, 0, (LPARAM)fileName);
		return 0;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == CHOOSE_FILE_BUTTON) {
			ClearingFileName(TRUE);
			OPENFILENAME ofn = { sizeof(OPENFILENAME) };
			ofn.hInstance = handle;
			ofn.lpstrFilter = TEXT("Все файлы");
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = _countof(fileName);
			ofn.lpstrTitle = TEXT("Выбрать файл");
			ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn) != FALSE) {
				SetWindowText(hWnd, fileName);
				SendMessage(hName, WM_SETTEXT, 0, (LPARAM)fileName);
				LoadOwner();
				LoadDACL();
			}
		}
		if (LOWORD(wParam) == CHOOSE_FOLDER_BUTTON) {
			ClearingFileName(TRUE);
			BROWSEINFO bi = { hWnd, NULL, NULL, "Выбрать папку", BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_BROWSEFORCOMPUTER, NULL, NULL, 0 };
			LPCITEMIDLIST lpItemDList = SHBrowseForFolder(&bi);
			if (lpItemDList != NULL){
				SHGetPathFromIDList(lpItemDList, (LPSTR)fileName);
				SetWindowText(hWnd, fileName);
				SendMessage(hName, WM_SETTEXT, 0, (LPARAM)fileName);
				LoadOwner();
				LoadDACL();
			}
		}
		if (LOWORD(wParam) == CHANGE_OWNER_BUTTON) {
			if (fileName[0] == '\0')
			{
				MessageBox(hWnd, TEXT("Выберите файл или каталог!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
			}
			else
			{
				ChangeOwner();
				LoadOwner();
				LoadDACL();
			}
		}
		if (LOWORD(wParam) == REMOVE_ACE) {
			INT iPos = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
			if (iPos == -1)
			{
				MessageBox(hWnd, TEXT("Выберите элемент!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
			}
			else
			{
				INT mbResult = MessageBox(hWnd, TEXT("Действительно удалить?"), TEXT("Выход"), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
				if (IDYES == mbResult)
				{
					DWORD cb = 0;
					GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, NULL, 0, &cb);
					PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cb);
					if (pSD != FALSE)
					{
						if (GetFileSecurity(fileName, DACL_SECURITY_INFORMATION, pSD, cb, &cb))
						{
							PACL pDacl;
							BOOL bDaclPresent, bDaclDefaulted;
							if (GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted) != FALSE)
							{
								if (FALSE != DeleteAce(pDacl, iPos))
								{
									if (SetFileSecurity(fileName, DACL_SECURITY_INFORMATION, pSD) == FALSE)
									{
										MessageBox(hWnd, TEXT("Удаление невозможно(("), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
									}
									else
									{
										LoadDACL();
									}
								}
								else {
									MessageBox(hWnd, TEXT("Удаление невозможно(("), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
								}
							}
							else {
								MessageBox(hWnd, TEXT("Удаление невозможно(("), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
							}
						}
						else {
							MessageBox(hWnd, TEXT("Ошибка получения дескриптора безопасности!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
						}
					}
					else {
						MessageBox(hWnd, TEXT("Ошибка инициализации дескриптора безопасности!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
					}
					LocalFree(pSD);
				}
			}
		}
		if (LOWORD(wParam) == ADD_ACE)
		{
			if (fileName[0] == '\0')
			{
				MessageBox(hWnd, TEXT("Выберите файл или каталог!"), TEXT("Ошибка"), MB_OK | MB_ICONERROR);
			}
			else
			{
				if (DialogBox(handle, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)NewAceDialogProc) == 0)
				{
					LoadDACL();
				}
			}
		}
		return 0;
	}
	case WM_NOTIFY:
	{
		LPNMHDR lpnmhdr = (LPNMHDR)lParam;
		if (NM_DBLCLK == lpnmhdr->code && IDC_LIST == lpnmhdr->idFrom)
		{
			INT iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
			if (iItem != -1)
			{
				DialogBox(handle, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)LoadACEDialogProc);
			}
		}
		return 0;
	}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

WNDCLASSEX RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX window = { sizeof(WNDCLASSEX) };
	window.style = CS_HREDRAW | CS_VREDRAW;
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	handle = hInstance;
	if (!RegisterClassEx(&RegisterWindowClass(hInstance)))
		return EXIT_FAILURE;
	LoadLibrary(TEXT("ComCtl32.dll"));
	hWnd = CreateWindowEx(0, TEXT("WindowClass"), TEXT(""), WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 740, 450, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL)
		return EXIT_FAILURE;
	ShowWindow(hWnd, nCmdShow);
	MessagesProcessing();
	return EXIT_SUCCESS;
}
