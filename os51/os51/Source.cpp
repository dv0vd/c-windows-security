#include<Windows.h>
#include<sddl.h>
#include<NTSecAPI.h>
#include<locale.h>
#include <tchar.h>

PSID userSid, computerSid;
LSA_HANDLE lsahPolicy;
LSA_OBJECT_ATTRIBUTES ObjectAttributes;
CONST LPCWSTR privelegiesConstants[46] =  {SE_BATCH_LOGON_NAME,
SE_DENY_BATCH_LOGON_NAME, 
SE_DENY_INTERACTIVE_LOGON_NAME, 
SE_DENY_NETWORK_LOGON_NAME, 
SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME, 
SE_DENY_SERVICE_LOGON_NAME, 
SE_INTERACTIVE_LOGON_NAME, 
SE_NETWORK_LOGON_NAME ,
SE_REMOTE_INTERACTIVE_LOGON_NAME,
SE_SERVICE_LOGON_NAME,
SE_ASSIGNPRIMARYTOKEN_NAME,
SE_AUDIT_NAME,
SE_BACKUP_NAME,
SE_CHANGE_NOTIFY_NAME,
SE_CREATE_GLOBAL_NAME,
SE_CREATE_PAGEFILE_NAME,
SE_CREATE_PERMANENT_NAME,
SE_CREATE_SYMBOLIC_LINK_NAME,
SE_CREATE_TOKEN_NAME,
SE_DEBUG_NAME,
SE_DELEGATE_SESSION_USER_IMPERSONATE_NAME,
SE_ENABLE_DELEGATION_NAME,
SE_IMPERSONATE_NAME,
SE_INC_BASE_PRIORITY_NAME,
SE_INCREASE_QUOTA_NAME,
SE_INC_WORKING_SET_NAME,
SE_LOAD_DRIVER_NAME,
SE_LOCK_MEMORY_NAME,
SE_MACHINE_ACCOUNT_NAME,
SE_MANAGE_VOLUME_NAME,
SE_PROF_SINGLE_PROCESS_NAME,
SE_RELABEL_NAME,
SE_REMOTE_SHUTDOWN_NAME,
SE_RESTORE_NAME,
SE_SECURITY_NAME,
SE_SHUTDOWN_NAME,
SE_SYNC_AGENT_NAME,
SE_SYSTEM_ENVIRONMENT_NAME,
SE_SYSTEM_PROFILE_NAME,
SE_SYSTEMTIME_NAME,
SE_TAKE_OWNERSHIP_NAME,
SE_TCB_NAME,
SE_TIME_ZONE_NAME,
SE_TRUSTED_CREDMAN_ACCESS_NAME,
SE_UNDOCK_NAME,
SE_UNSOLICITED_INPUT_NAME
};
CONST WCHAR *privelegiesNames[46] = { L"Required for an account to log on using the batch logon type", 
L"Explicitly denies an account the right to log on using the batch logon type", 
L"Explicitly denies an account the right to log on using the interactive logon type", 
L"Explicitly denies an account the right to log on using the network logon type",
L"Explicitly denies an account the right to log on remotely using the interactive logon type",
L"Explicitly denies an account the right to log on using the service logon type",
L"Required for an account to log on using the interactive logon type",
L"Required for an account to log on using the network logon type",
L"Required for an account to log on remotely using the interactive logon type",
L"Required for an account to log on using the service logon type",
L"Replace a process-level token"
L"Generate security audits",
L"Back up files and directories",
L"Bypass traverse checking",
L"Create global objects",
L"Create a pagefile",
L"Create permanent shared objects",
L"Create symbolic links",
L"Create a token object",
L"Debug programs",
L"Impersonate other users",
L"Enable computer and user accounts to be trusted for delegation",
L"Impersonate a client after authentication",
L"Increase scheduling priority",
L"Adjust memory quotas for a process",
L"Increase a process working set"
L"Load and unload device drivers",
L"Lock pages in memory",
L"Add workstations to domain",
L"Manage the files on a volume",
L"Profile single process",
L"Modify an object label",
L"Force shutdown from a remote system",
L"Restore files and directories",
L"Manage auditing and security log",
L"Shut down the system",
L"Right: Synchronize directory service data",
L"Modify firmware environment values",
L"Profile system performance",
L"Change the system time",
L"Take ownership of files or other objects",
L"Act as part of the operating system",
L"Change the time zone",
L"Access Credential Manager as a trusted caller",
L"Remove computer from docking station",
L"Not applicable",
};

VOID ComputerSID() {
	DWORD cbSid = 0, domainNameSize = 0;
	TCHAR computerName[MAX_PATH];
	DWORD computerNameSize = MAX_PATH;
	if (GetComputerName(computerName, &computerNameSize))
	{
		LookupAccountName(NULL, computerName, NULL, &cbSid, NULL, &domainNameSize, NULL);
		if ((cbSid > 0) && (domainNameSize > 0))
		{
			SID_NAME_USE SIDtype;
			LPTSTR domainName = NULL;
			domainName = new TCHAR[domainNameSize];
			computerSid = new SID[cbSid];
			if (LookupAccountName(NULL, computerName, computerSid, &cbSid, domainName, &domainNameSize, &SIDtype) != FALSE)
			{
				LPTSTR sidString;
				ConvertSidToStringSid(computerSid, &sidString);
				_tprintf(TEXT("SID локального компьютера:     %s\n"), sidString);
			}
			delete[]domainName;
		}
	}
}

VOID CurrentUserSID() {
	DWORD cbSid = 0, size = 0;
	TCHAR userName[MAX_PATH];
	DWORD userNameSize = MAX_PATH;
	if (GetUserName(userName, &userNameSize))
	{
		LookupAccountName(NULL, userName, NULL, &cbSid, NULL, &size, NULL);
		if ((cbSid > 0) && (size > 0))
		{
			SID_NAME_USE SIDtype;
			LPTSTR name = NULL;
			name = new TCHAR[size];
			userSid = new SID[cbSid];
			if (LookupAccountName(NULL, userName, userSid, &cbSid, name, &size, &SIDtype) != FALSE)
			{
				LPTSTR sidString;
				ConvertSidToStringSid(userSid, &sidString);
				_tprintf(TEXT("SID учётной записи текущего пользователя:     %s\n"), sidString);
			}
			delete[]name;
		}
	}
}

VOID NamesForWellKnownSID(){
	CONST TCHAR *sidsNameMassive[6] = { TEXT("WinConsoleLogonSid"),TEXT("WinBatchSid"),TEXT("WinLocalServiceSid"),TEXT("WinAccountGuestSid"),TEXT("WinBuiltinGuestsSid"),TEXT("WinBuiltinRemoteDesktopUsersSid") };
	_tprintf(TEXT("**************************************************************************************************\nИмена учетных записей для хорошо известных SID и их права и привелегии:"));
	WELL_KNOWN_SID_TYPE sidTypes[] = { WinConsoleLogonSid ,WinBatchSid ,WinLocalServiceSid ,WinAccountGuestSid , WinBuiltinGuestsSid, WinBuiltinRemoteDesktopUsersSid };
	ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	if (LSA_SUCCESS(LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_ALL_ACCESS, &lsahPolicy)))
	{
		for (INT i = 0; i < 6; i++)
		{
			PSID sid = NULL;
			SID_NAME_USE SIDtype;
			DWORD cbSid = SECURITY_MAX_SID_SIZE;
			sid = new SID[cbSid];
			if (CreateWellKnownSid(sidTypes[i], computerSid, sid, &cbSid) != FALSE)
			{
				LPTSTR name, domainName;
				DWORD nameSize = 0, domainNameSize = 0;
				LookupAccountSid(NULL, sid, NULL, &nameSize, NULL, &domainNameSize, NULL);
				if ((nameSize > 0) && (domainNameSize > 0))
				{
					name = new TCHAR[nameSize];
					domainName = new TCHAR[domainNameSize];
					if (LookupAccountSid(NULL, sid, name, &nameSize, domainName, &domainNameSize, &SIDtype) != FALSE)
					{
						LPTSTR sidString;
						ConvertSidToStringSid(sid, &sidString);
						_tprintf(TEXT("\n     -%s ( %s ): %s"), sidsNameMassive[i], sidString, name);
					}
					
					delete[]domainName;
					delete[]name;
				}
				else
				{
					LPTSTR sidString;
					ConvertSidToStringSid(sid, &sidString);
					_tprintf(TEXT("\n     -%s ( %s ): неизвестное имя"), sidsNameMassive[i], sidString);
				}
				PLSA_UNICODE_STRING UserRights;
				ULONG nCountOfRights;
				if (LSA_SUCCESS(LsaEnumerateAccountRights(lsahPolicy,sid, &UserRights, &nCountOfRights)))
				{
					for (ULONG i = 0; i < nCountOfRights; i++)
					{
						TCHAR name[MAX_PATH];
						DWORD sizeName = _countof(name), lang;
						LPCWSTR sUserRight = UserRights[i].Buffer;
						if (FALSE != LookupPrivilegeDisplayName(NULL, sUserRight, name, &sizeName, &lang))
						{
							_tprintf(TEXT("\n          -%s ---> %s"), sUserRight, name);
						}
						else 
						{
							for (INT j = 0; j < 46; j++)
							{
								if (_tcscmp(privelegiesConstants[j], sUserRight) == 0)
								{
									_tprintf(TEXT("\n          -%s ---> %s"), sUserRight, privelegiesNames[j]);
									break;
								}
							}
						}
					}
				}
			}
			delete[]sid;
		}
		LsaClose(lsahPolicy);
	}
}

VOID CurrentUserRights() {
	_tprintf(TEXT("\n**************************************************************************************************\nПривилегии и права учетной записи текущего пользователя:"));
	ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	if (LSA_SUCCESS(LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_ALL_ACCESS, &lsahPolicy)))
	{
		PLSA_UNICODE_STRING UserRights;
		ULONG nCountOfRights;
		if (LSA_SUCCESS(LsaEnumerateAccountRights(lsahPolicy, userSid, &UserRights, &nCountOfRights)))
		{
			for (ULONG i = 0; i < nCountOfRights; ++i)
			{
				TCHAR name[MAX_PATH];
				DWORD sizeName = _countof(name), lang;
				LPCWSTR sUserRight = UserRights[i].Buffer;
				if (FALSE != LookupPrivilegeDisplayName(NULL, sUserRight, name, &sizeName, &lang))
				{
					_tprintf(TEXT("\n          -%s ---> %s"), sUserRight, name);
				}
				else
				{
					for (INT j = 0; j < 46; j++)
					{
						if (_tcscmp(privelegiesConstants[j], sUserRight) == 0)
						{
							_tprintf(TEXT("\n          -%s ---> %s"), sUserRight, privelegiesNames[j]);
							break;
						}
					}
				}
			}
			LsaFreeMemory(UserRights);
		}
		LsaClose(lsahPolicy);
	}
}

INT main()
{
	setlocale(LC_ALL, "rus");
	ComputerSID();
	CurrentUserSID();
	NamesForWellKnownSID();
	CurrentUserRights();
	_tprintf(TEXT("\n"));
	system("pause");
}