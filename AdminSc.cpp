/******************************************************************************
MIT License

Copyright(c) 2025 René Pagel

Filename: AdminSc.cpp
For more information see https://github.com/RePag-net/AdminSc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
#include "HAdminSc.h"
#include "OAdminServer.h"
//---------------------------------------------------------------------------
SERVICE_STATUS stServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatusHandle;
HANDLE heStopEvent = NULL;
char pcServiceName[] = "RePagAdmin";
char pcServiceSchlussel[] = "SYSTEM\\CurrentControlSet\\Services\\RePagAdmin";
#define ProgVersion "2.3.3.4"
DWORD dwEineSpeicherSeite;
DWORD dwDoppelSpeicherSeite;

void WINAPI ServiceMain(_In_ DWORD argc, _In_z_ LPTSTR *argv);
void WINAPI ServiceControlHandler(_In_ DWORD dwControl);
void __vectorcall ServiceStatus(_In_ DWORD dwCurrentState, _In_ DWORD dwWin32ExitCode, _In_ DWORD dwWaitHint);
bool __vectorcall ServiceStart(_In_ SC_HANDLE hsvmSCManager, _In_ SC_HANDLE hsvService);
DWORD __vectorcall ServiceStopp(_In_ SC_HANDLE hsvmSCManager);
#pragma warning(disable : 4996)
#define AdminEntwicklung
//--------------------------------------------------------------------------
void _tmain(_In_ int argc, _In_z_ TCHAR *argv[])
{
	if(argc < 2){
		SERVICE_TABLE_ENTRY DispatchTable[] = {{pcServiceName, ServiceMain },{ NULL, NULL }};
		if(StartServiceCtrlDispatcher(DispatchTable)) return;
	}
  else if(argc == 2){
    SC_HANDLE hsvmSCManager, hsvService;

    if(!StrCompare(argv[1], "-?") || !StrCompare(argv[1], "/?") || !StrCompare(argv[1], "-h") || !StrCompare(argv[1], "/h")){
      printf("\n  RePag Admin\n\n"
             "  Optionen:\n"
             "  -?          Dieser Hilfetext\n"
             "  -install    Installiert den Service\n"
             "  -update     Aktualisiert den Service\n"
             "  -uninstall  Deinstalliert den Service\n");
      return;
    }
#ifdef _DEBUG
    if(!(hsvmSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))){ printf("Fehler beim öffnen des SCM (%d)\n", GetLastError()); return; }
    if(!StrCompare(argv[1], "-install") || !StrCompare(argv[1], "/install")){
      TCHAR szPath[MAX_PATH]; DWORD dwBytes;
      if(!(dwBytes = GetModuleFileName(NULL, szPath, MAX_PATH))){ printf("GetModuleFileName failed (%d)\n", GetLastError()); return; }
      szPath[dwBytes] = 0;

      char c11Abhangigkeiten[11]; c11Abhangigkeiten[9] = NULL; c11Abhangigkeiten[10] = NULL;
      MemCopy(c11Abhangigkeiten, "RePagCore", 9);

      if(!(hsvService = CreateService(hsvmSCManager, pcServiceName, "RePag Admin", SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, szPath, NULL, NULL, c11Abhangigkeiten, NULL, NULL))){
        printf("Fehler bei Service eintragen (%d)\n", GetLastError()); return;
      }
      else{
        HKEY hSchlussel;
        CloseServiceHandle(hsvService);
        if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, pcServiceSchlussel, 0, KEY_ALL_ACCESS, &hSchlussel)){
          if(RegQueryValueEx(hSchlussel, "Doppel", 0, 0, NULL, NULL)){

            printf("\nName: RePag Admin\nVersion: "); printf(ProgVersion); printf("\n");

            BOOL BBool = 0;
            RegSetValueEx(hSchlussel, "Doppel", 0, REG_DWORD, (PBYTE)&BBool, 4);
            printf("Doppel: False\n");

            RegSetValueEx(hSchlussel, "Server_2", 0, REG_DWORD, (PBYTE)&BBool, 4);
            printf("Server_2: False\n");

            ULONG ulTemp = StrLength(szPath) - 12;
            //MemCopy(&szPath[ulTemp], "\\Database", 9); szPath[ulTemp += 9] = 0;
            MemCopy(szPath, "C:\\Development\\Daten\\Admin\\", 16); szPath[16] = 0;
            RegSetValueEx(hSchlussel, "DatabasePath", 0, REG_SZ, (PBYTE)szPath, ulTemp);
            printf("DatabasePath: "); printf(szPath); printf("\n");
          }
          RegCloseKey(hSchlussel);
        }
        printf("Service erfolgreich eingetragen\n"); return;
      }
    }

    if(!StrCompare(argv[1], "-uninstall") || !StrCompare(argv[1], "/uninstall")){
      if(!(hsvService = OpenService(hsvmSCManager, pcServiceName, DELETE))){ printf("Fehler beim öffnen des Service (%d)\n", GetLastError()); return; }
      if(!DeleteService(hsvService)){ printf("Fehler beim löschen des Service (%d)\n", GetLastError()); return; }
      else printf("\nService erfolgreich ausgetragen\n");
      CloseServiceHandle(hsvService);
    }
  }
#else
    if(!(hsvmSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))){
      if(GetLastError() == ERROR_ACCESS_DENIED){
        printf("\nFehler beim öffnen des Service-Control-Manager\n");
        printf("Fehlercode: Zugriff verweigert\n");
        printf("Die Eingabeaufforderung muss 'Als Administrator' geöffnet werden\n");
      }
      else printf("Fehler beim öffnen des Service-Control-Manager (%d)\n", GetLastError());
      return;
    }
    char pcFehlerServiceEingetragen[] = "Der Service wurde nicht eingetragen\n";
    char pcFehlerServiceInstallieren[] = "\nFehler beim Installieren des Service\nFehlercode: (%d)\n";
    char pcFehlerAktualisieren[] = "\nFehler beim aktualisieren des Service\nFehlercode: (%d)\n";

    if(!(hsvService = OpenService(hsvmSCManager, pcServiceName, SERVICE_QUERY_STATUS))){
      if(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST && !StrCompare(argv[1], "-install") || !StrCompare(argv[1], "/install")){
        HANDLE hAccessToken = NULL; PWSTR pszPath = NULL; char cPfad_Ziel[MAX_PATH];
        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken)){
          printf(pcFehlerServiceInstallieren, GetLastError()); printf(pcFehlerServiceEingetragen); return;
        }

        if(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, hAccessToken, &pszPath)){
          printf(pcFehlerServiceInstallieren, GetLastError()); printf(pcFehlerServiceEingetragen);
          CloseHandle(hAccessToken); if(pszPath) CoTaskMemFree(pszPath); return;
        }
        CloseHandle(hAccessToken);

        char cPath_Quelle[MAX_PATH]; DWORD dwBytes_Path_Quelle;
        if(!(dwBytes_Path_Quelle = GetModuleFileName(NULL, cPath_Quelle, MAX_PATH))){
          printf(pcFehlerServiceInstallieren, GetLastError()); printf(pcFehlerServiceEingetragen);
          CoTaskMemFree(pszPath); return;
        }
        cPath_Quelle[dwBytes_Path_Quelle] = 0;

        int iBytes = wcstombs(cPfad_Ziel, pszPath, MAX_PATH);
        MemCopy(&cPfad_Ziel[iBytes], "\\RePag", 6); iBytes += 6; cPfad_Ziel[iBytes] = 0;
        if(!CreateDirectory(cPfad_Ziel, NULL) && GetLastError() != ERROR_ALREADY_EXISTS){
          printf("\nFehler beim Erstellen des Programmverzeichnis\n"); printf(pcFehlerServiceEingetragen);
          CoTaskMemFree(pszPath); return;
        }
        CoTaskMemFree(pszPath);

        BYTE ucBytes_DateiName; char cDateiName[13];
        if(!(ucBytes_DateiName = GetModuleBaseName(GetCurrentProcess(), NULL, cDateiName, 13))){
          printf("\nFehler beim Erstellen des Programmdatei\n"); printf(pcFehlerServiceEingetragen);
          return;
        }

        cDateiName[ucBytes_DateiName++] = 0;
        MemCopy(&cPfad_Ziel[iBytes++], "\\", 1);
        bool bGleicherPfad;
        MemCopy(&cPfad_Ziel[iBytes], cDateiName, ucBytes_DateiName);
        if(cPfad_Ziel[0] >= 0x61 && cPfad_Ziel[0] <= 0x7a) cPfad_Ziel[0] -= 0x20;
        if(cPath_Quelle[0] >= 0x61 && cPath_Quelle[0] <= 0x7a) cPath_Quelle[0] -= 0x20;
        if(bGleicherPfad = StrCompare(cPfad_Ziel, cPath_Quelle)){
          if(!CopyFile(cPath_Quelle, cPfad_Ziel, FALSE)){
            printf("\nFehler beim kopieren der Datei in das Programmverzeichnis\n"); printf(pcFehlerServiceEingetragen);
            return;
          }
        }

        char c11Abhangigkeiten[11]; c11Abhangigkeiten[9] = NULL; c11Abhangigkeiten[10] = NULL;
        MemCopy(c11Abhangigkeiten, "RePagCore", 9);

        if(!(hsvService = CreateService(hsvmSCManager, pcServiceName, "RePag Admin", SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, cPfad_Ziel, NULL, NULL, c11Abhangigkeiten, NULL, NULL))){
          printf("Fehler bei Service eintragen (%d)\n", GetLastError()); return;
        }
        CloseServiceHandle(hsvService);

        HKEY hSchlussel;
        if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, pcServiceSchlussel, 0, KEY_ALL_ACCESS, &hSchlussel)){
          if(RegQueryValueEx(hSchlussel, "Doppel", 0, 0, NULL, NULL)){

            printf("\nName: RePag Admin\nVersion: "); printf(ProgVersion); printf("\n");

            BOOL BBool = 0;
            RegSetValueEx(hSchlussel, "Doppel", 0, REG_DWORD, (PBYTE)&BBool, 4);
            printf("Doppel: False\n");

            RegSetValueEx(hSchlussel, "Server_2", 0, REG_DWORD, (PBYTE)&BBool, 4);
            printf("Server_2: False\n");

            ULONG ulTemp = StrLength(cPfad_Ziel) - 12;
            //MemCopy(&cPfad_Ziel[ulTemp], "\\Database\\", 11); cPfad_Ziel[ulTemp += 11] = 0;
            MemCopy(cPfad_Ziel, "C:\\Daten\\Admin\\", 16); cPfad_Ziel[16] = 0;
            RegSetValueEx(hSchlussel, "DatabasePath", 0, REG_SZ, (PBYTE)cPfad_Ziel, ulTemp);
            printf("DatabasePath: "); printf(cPfad_Ziel); printf("\n");

            if(bGleicherPfad) RegSetValueEx(hSchlussel, "InstallPath", 0, REG_EXPAND_SZ, (PBYTE)&cPath_Quelle, ++dwBytes_Path_Quelle);
          }
          RegCloseKey(hSchlussel);
        }

        printf("Service erfolgreich eingetragen\n");
        ServiceStart(hsvmSCManager, hsvService);
        return;
      }
    }
    else{
      if(!StrCompare(argv[1], "-update") || !StrCompare(argv[1], "/update") || !StrCompare(argv[1], "-install") || !StrCompare(argv[1], "/install")){
        if(ServiceStopp(hsvmSCManager) == SERVICE_STOPPED){
          char cPath_Quelle[MAX_PATH]; DWORD dwBytes_Path_Quelle, dwBytes;
          if(!(dwBytes_Path_Quelle = GetModuleFileName(NULL, cPath_Quelle, MAX_PATH))){
            printf(pcFehlerAktualisieren, GetLastError());
            CloseServiceHandle(hsvService); return;
          }
          cPath_Quelle[dwBytes_Path_Quelle] = 0;

          HKEY hSchlussel; char cPfad_Ziel[MAX_PATH];
          if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, pcServiceSchlussel, 0, KEY_ALL_ACCESS, &hSchlussel)){
            printf(pcFehlerAktualisieren, GetLastError());
            CloseServiceHandle(hsvService); return;
          }
          else{ dwBytes = MAX_PATH;
            if(RegQueryValueEx(hSchlussel, "ImagePath", 0, 0, (PBYTE)cPfad_Ziel, &dwBytes)){
              printf(pcFehlerAktualisieren, GetLastError());
              CloseServiceHandle(hsvService); return;
            }
            else{
              cPfad_Ziel[dwBytes] = 0;
              if(cPfad_Ziel[0] >= 0x61 && cPfad_Ziel[0] <= 0x7a) cPfad_Ziel[0] -= 0x20;
              if(cPath_Quelle[0] >= 0x61 && cPath_Quelle[0] <= 0x7a) cPath_Quelle[0] -= 0x20;
              if(StrCompare(cPfad_Ziel, cPath_Quelle)){
                if(!CopyFile(cPath_Quelle, cPfad_Ziel, FALSE)){
                  printf("\nFehler beim kopieren der Datei in das Programmverzeichnis\n");
                  CloseServiceHandle(hsvService); return;
                }
                RegSetValueEx(hSchlussel, "InstallPath", 0, REG_EXPAND_SZ, (PBYTE)&cPath_Quelle, ++dwBytes_Path_Quelle);
              }
            }
          }

          printf("Service erfolgreich aktualisiert\n");
          ServiceStart(hsvmSCManager, hsvService);
          return;
        }
      }

      if(!StrCompare(argv[1], "-uninstall") || !StrCompare(argv[1], "/uninstall")){
        if(!(hsvService = OpenService(hsvmSCManager, pcServiceName, DELETE))){ printf("Fehler beim öffnen des Service (%d)\n", GetLastError()); return; }
        if(ServiceStopp(hsvmSCManager) == SERVICE_STOPPED){
          if(!DeleteService(hsvService)){ printf("Fehler beim löschen des Service (%d)\n", GetLastError()); return; }
          else printf("Service erfolgreich ausgetragen\n");
        }
      }
      CloseServiceHandle(hsvService); return;
    }
}
#endif
}
//--------------------------------------------------------------------------
bool __vectorcall ServiceStart(_In_ SC_HANDLE hsvmSCManager, _In_ SC_HANDLE hsvService)
{
  CloseServiceHandle(hsvService);
  printf("Service wird gestartet...\n");
  Sleep(2000);
  if(hsvService = OpenService(hsvmSCManager, pcServiceName, SERVICE_START)){
    if(StartService(hsvService, NULL, NULL)) printf("Service wurde gestartet\n");
    else printf("Service konnte nicht gestartet werden\n");
    CloseServiceHandle(hsvService);
  }
  else printf("Service konnte nicht gestartet werden\n");
  return false;
}
//---------------------------------------------------------------------------
DWORD __vectorcall ServiceStopp(_In_ SC_HANDLE hsvmSCManager)
{
  SC_HANDLE hsvService; SERVICE_STATUS stServiceStatus; stServiceStatus.dwCurrentState = SERVICE_STOPPED;
  if(!(hsvService = OpenService(hsvmSCManager, pcServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE))) return 0;
  else{
    DWORD dwBytes; SERVICE_STATUS_PROCESS stServiceStatusProzess;
    if(!QueryServiceStatusEx(hsvService, SC_STATUS_PROCESS_INFO, (LPBYTE)&stServiceStatusProzess, sizeof(SERVICE_STATUS_PROCESS), &dwBytes)) return 0;
    else{
      switch(stServiceStatusProzess.dwCurrentState){
        case SERVICE_RUNNING          :
        case SERVICE_START_PENDING    :
        case SERVICE_CONTINUE_PENDING :
        case SERVICE_PAUSE_PENDING    :
        case SERVICE_PAUSED           : ControlService(hsvService, SERVICE_CONTROL_STOP, &stServiceStatus);
                                        printf("\nService wird gestoppt...\n");
                                        Sleep(1000);
                                        if(stServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) stServiceStatus.dwCurrentState = SERVICE_STOPPED;
                                        if(stServiceStatus.dwCurrentState == SERVICE_STOPPED) printf("Service wurde gestoppt\n");
                                        else printf("Service konnte nicht gestoppt werden\n");
                                        break;
        case SERVICE_STOP_PENDING     :
        case SERVICE_STOPPED          : printf("\n"); break;
      }
    }
    CloseServiceHandle(hsvService);
  }
  return stServiceStatus.dwCurrentState;
}
//---------------------------------------------------------------------------
bool __vectorcall Admin_Start(_In_ bool bInit)
{ 
  SYSTEM_INFO stSystem_Info; GetSystemInfo(&stSystem_Info); dwEineSpeicherSeite = stSystem_Info.dwPageSize; dwDoppelSpeicherSeite = stSystem_Info.dwPageSize * 2;

  HKEY hSchlussel; BOOL BServer_2, BDoppel;	
  if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, pcServiceSchlussel, NULL, KEY_ALL_ACCESS, &hSchlussel)){ DWORD dwBytes = MAX_PATH; char cPath[MAX_PATH];
    if(!RegQueryValueEx(hSchlussel, "InstallPath", 0, 0, (PBYTE)cPath, &dwBytes)){
      Sleep(3000);
      cPath[dwBytes] = 0; DeleteFile(cPath);
      RegDeleteValue(hSchlussel, "InstallPath");
    }

    dwBytes = 4;
    RegQueryValueEx(hSchlussel, "Server_2", 0, 0, (PBYTE)&BServer_2, &dwBytes);
    RegQueryValueEx(hSchlussel, "Doppel", 0, 0, (PBYTE)&BDoppel, &dwBytes);

		char c6Ausgleichen[6]; char c6Optimieren[6];
		dwBytes = 6;
		if(RegQueryValueEx(hSchlussel, "BSTPerfectBalance", 0, 0, (PBYTE)c6Ausgleichen, &dwBytes) == ERROR_FILE_NOT_FOUND){
			ZeroMem(c6Ausgleichen, 6); RegSetValueEx(hSchlussel, "BSTPerfectBalance", 0, REG_SZ, (PBYTE)c6Ausgleichen, 6);
		}

		dwBytes = 6;
		if(RegQueryValueEx(hSchlussel, "StartOptimize", 0, 0, (PBYTE)c6Optimieren, &dwBytes) == ERROR_FILE_NOT_FOUND){
			ZeroMem(c6Optimieren, 6);	RegSetValueEx(hSchlussel, "StartOptimize", 0, REG_SZ, (PBYTE)c6Optimieren, 6);
		}
    RegCloseKey(hSchlussel);

		vmServer = InitVirtualMem(false, "Server");
		if(!(vAdminServer = COAdminServerV(bInit, BDoppel, BServer_2, c6Ausgleichen, c6Optimieren))) return false;
  }
	else return false;
  return true;
}
//---------------------------------------------------------------------------
bool __vectorcall Admin_Ende(void)
{
  VMFreiV(vAdminServer);
	FreeVirtualMem(vmServer);
  return true;
}
//---------------------------------------------------------------------------
void WINAPI ServiceMain(_In_ DWORD dwArgc, _In_z_ LPTSTR* pcArgv)
{
  bool bInit;
  if(dwArgc == 2){
    char* pcInit = *&pcArgv[1];
    if(!StrCompare(pcInit, "Init") || !StrCompare(pcInit, "init") || !StrCompare(pcInit, "-Init") || !StrCompare(pcInit, "-init")) bInit = true;
    else bInit = false;
  }
  else bInit = false;

  if(!(hServiceStatusHandle = RegisterServiceCtrlHandler(pcServiceName, ServiceControlHandler))) return;

  stServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  stServiceStatus.dwServiceSpecificExitCode = 0;
  ServiceStatus(SERVICE_START_PENDING, NO_ERROR, 5000);

  if(!(heStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL))){ ServiceStatus(SERVICE_STOPPED, NO_ERROR, 1000); return; }

  if(!Admin_Start(bInit)){ ServiceStatus(SERVICE_STOPPED, NO_ERROR, 5000); return; }

  ServiceStatus(SERVICE_RUNNING, NO_ERROR, 5000);

  while(TRUE){
    WaitForSingleObject(heStopEvent, INFINITE);
    ServiceStatus(SERVICE_STOPPED, NO_ERROR, 5000);
    Admin_Ende();
    return;
  }
}
//---------------------------------------------------------------------------
void __vectorcall ServiceStatus(_In_ DWORD dwCurrentState, _In_ DWORD dwWin32ExitCode, _In_ DWORD dwWaitHint)
{
  static DWORD dwCheckPoint = 1;

  stServiceStatus.dwCurrentState = dwCurrentState;
  stServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
  stServiceStatus.dwWaitHint = dwWaitHint;

  if(dwCurrentState == SERVICE_START_PENDING) stServiceStatus.dwControlsAccepted = 0;
  else stServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

  if((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) stServiceStatus.dwCheckPoint = 0;
  else stServiceStatus.dwCheckPoint = dwCheckPoint++;

  SetServiceStatus(hServiceStatusHandle, &stServiceStatus);
}
//---------------------------------------------------------------------------
void WINAPI ServiceControlHandler(_In_ DWORD dwControl)
{
  switch(dwControl){
      case SERVICE_CONTROL_SHUTDOWN    :
      case SERVICE_CONTROL_STOP        : ServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);
                                         SetEvent(heStopEvent);
                                         ServiceStatus(stServiceStatus.dwCurrentState, NO_ERROR, 1000);
                                         return;
      case SERVICE_CONTROL_INTERROGATE : break;
                               default : break;
  }
}
//---------------------------------------------------------------------------