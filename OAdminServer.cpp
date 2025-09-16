/******************************************************************************
MIT License

Copyright(c) 2025 René Pagel

Filename: OAdminServer.cpp
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
#include "TAdminServer.h"
//-------------------------------------------------------------------------------------------------------------------------------------------
COList* vthlstProtokollEintrag;
COAdminServer* vAdminServer = nullptr;
//-------------------------------------------------------------------------------------------------------------------------------------------
COAdminServer* __vectorcall COAdminServerV(_In_ bool bInit, _In_ bool bDoppel, _In_ bool bServer_2, _In_z_ char* c6Ausgleichen, _In_z_ char* c6Optimieren)
{
 vAdminServer = (COAdminServer*)VMBlock(vmServer, sizeof(COAdminServer));
 vAdminServer->COAdminServerV(bInit, bDoppel, bServer_2, c6Ausgleichen, c6Optimieren);
 return vAdminServer;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void CALLBACK Timer_DBCache(void* pvParam, bool bTimerOrWaitFired)
{
	if(vAdminServer){
		if(ResumeThread(vthstDBCacheLoschen->hThread) == -1){
			vthstDBCacheLoschen = (STTHDBCacheLoschen*)VMBlock(vmServer, sizeof(STTHDBCacheLoschen));
			vthstDBCacheLoschen->hThread = CreateThread(NULL, 0, thDBCacheLoschen, vthstDBCacheLoschen, CREATE_SUSPENDED, NULL);
			SetThreadPriority(vthstDBCacheLoschen->hThread, THREAD_PRIORITY_IDLE);
			ResumeThread(vthstDBCacheLoschen->hThread);

			LogEintrag_Server(pBasisServer->asName, "DBCacheLoschen ausgefallen - Neustart");
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void CALLBACK Timer_DBSynchron(void* pvParam, bool bTimerOrWaitFired)
{
	TryEnterCriticalSection(&csDBSynchron);
	if(bDBSynchron){ LeaveCriticalSection(&csDBSynchron);
		if(vAdminServer){
			if(ResumeThread(vthstDBSynchron->hThread) == -1){
				vthstDBSynchron = (STTHDBSynchron*)VMBlock(vmServer, sizeof(STTHDBSynchron));
				vthstDBSynchron->hThread = CreateThread(NULL, 0, thDBSynchron, vthstDBSynchron, CREATE_SUSPENDED, NULL);
				SetThreadPriority(vthstDBSynchron->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
				ResumeThread(vthstDBSynchron->hThread);

				LogEintrag_Server(pBasisServer->asName, "DBSynchron ausgefallen - Neustart");
			}
		}
	}
	else LeaveCriticalSection(&csDBSynchron);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void CALLBACK Timer_DBAusgleichen(void* pvParam, bool bTimerOrWaitFired)
{
	COTime* vzZeit = COTimeV(); COStringA* vasZeit = COStringAV(); vzZeit->Now()->StrTime(vasZeit)->ShortRight(3); 
	if(*vasZeit == vAdminServer->c6Ausgleichen) vAdminServer->DBAusgleichen(NULL);
	if(*vasZeit == vAdminServer->c6Optimieren) vAdminServer->DBStartOptimierung(NULL);
	VMFreiV(vzZeit); VMFreiV(vasZeit);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall frTimerStart(void)
{
	CreateTimerQueueTimer(&vAdminServer->htDBCache, hTimerQueue, (WAITORTIMERCALLBACK)Timer_DBCache, NULL, 10000, 300000, 0);
	CreateTimerQueueTimer(&vAdminServer->htDBSynchron, hTimerQueue, (WAITORTIMERCALLBACK)Timer_DBSynchron, NULL, 10000, DB_SYNCHRONWARTEN, 0);
	CreateTimerQueueTimer(&vAdminServer->htDBAusgleichen, hTimerQueue, (WAITORTIMERCALLBACK)Timer_DBAusgleichen, NULL, 10000, 60000, 0);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void CALLBACK Timer_SitzungGultig(void* pvParam, bool bTimerOrWaitFired)
{
 if(ResumeThread(thstSitzGultig->hThread) == -1){
	 thstSitzGultig = (STTHSitzGultig*)VMBlock(vmServer, sizeof(STTHSitzGultig));
	 thstSitzGultig->bEnde = false;
	 thstSitzGultig->hThread = CreateThread(NULL, 0, thAdminSitzGultig, thstSitzGultig, CREATE_SUSPENDED, NULL);
	 SetThreadPriority(thstSitzGultig->hThread, THREAD_PRIORITY_IDLE);
	 ResumeThread(thstSitzGultig->hThread);

	 LogEintrag_Server(pBasisServer->asName, "Sitzungskontrolle ausgefallen - Neustart");
 }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
/*
void CALLBACK Timer_Verbindung(void* pvParam, bool bTimerOrWaitFired)
{
 //if(ResumeThread(hthVerbindung) == -1){
 //  hthVerbindung = CreateThread(NULL, 0, thVerbindung, &thstVerbindung, CREATE_SUSPENDED, NULL);
	// SetThreadPriority(hthVerbindung, THREAD_PRIORITY_LOWEST);
	// ResumeThread(hthVerbindung);
	// STTHEintragServer* thstEintrag = (STTHEintragServer*)VMBlock(BY_STTHEINTRAGSERVER);
	// thstEintrag->pvBasisServer = vAdminServer;
	// thstEintrag->vzZeit = COTimeV()->Now();
	// thstEintrag->vasProgramm = COStringAV(vAdminServer->vasName);
	// thstEintrag->vasEintrag = COStringAV("Verbindungskontrolle ausgefallen - Neustart");
	// thstEintrag->hThread = CreateThread(NULL, 0, thEintragServer, thstEintrag, CREATE_SUSPENDED, NULL);
	// SetThreadPriority(thstEintrag->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
	// ResumeThread(thstEintrag->hThread);
 //}
}*/
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall frAufgabeIntern(COProtokollServer* pRePagServer, USHORT& usAufgabe)
{
	char c2Ungultig[2]; ZeroMem(c2Ungultig, 0); char c46IPBuffer[46]; COStringA asEintrag;
	WSABUF wsaBuffer; wsaBuffer.buf = c2Ungultig; wsaBuffer.len = 2;
	pBasisServer->WSASenden_Overlapped(pRePagServer, wsaBuffer);
	
	if(!pRePagServer->bIPV6){
		ULONG ulSockaddr = ((sockaddr_in*)pRePagServer->pstSockAdresse->vbSockaddr)->sin_addr.s_addr;
		if(vAdminServer->stServer.stSockaddrIV4_1.vbSockaddr && ((sockaddr_in*)vAdminServer->stServer.stSockaddrIV4_1.vbSockaddr)->sin_addr.s_addr == ulSockaddr)
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stAdminServer.bit128Core_1, BY_BIT128);
		else if(vAdminServer->stServer.stSockaddrIV4_2.vbSockaddr && ((sockaddr_in*)vAdminServer->stServer.stSockaddrIV4_2.vbSockaddr)->sin_addr.s_addr == ulSockaddr)
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stAdminServer.bit128Core_2, BY_BIT128);
		else if(vAdminServer->stLoginServer.stSockaddrIV4_1.vbSockaddr && ((sockaddr_in*)vAdminServer->stLoginServer.stSockaddrIV4_1.vbSockaddr)->sin_addr.s_addr == ulSockaddr)
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stLoginServer.bit128Core_1, BY_BIT128);
		else if(vAdminServer->stLoginServer.stSockaddrIV4_2.vbSockaddr && ((sockaddr_in*)vAdminServer->stLoginServer.stSockaddrIV4_2.vbSockaddr)->sin_addr.s_addr == ulSockaddr)
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stLoginServer.bit128Core_2, BY_BIT128);
		else if(vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.vbSockaddr && 
						((sockaddr_in*)vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.vbSockaddr)->sin_addr.s_addr == ulSockaddr)
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->vstProgServer[SRV_PROGSERVER].bit128Core_1, BY_BIT128);
		else if(vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr && 
						((sockaddr_in*)vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr)->sin_addr.s_addr == ulSockaddr)
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->vstProgServer[SRV_PROGSERVER].bit128Core_2, BY_BIT128);
	}
	else{
		char c46EigeneIPNummer[46]; ZeroMem(c46EigeneIPNummer, 46); char c46EmpfangeneIPNummer[46]; ZeroMem(c46EmpfangeneIPNummer, 46);
		inet_ntop(AF_INET6, &((sockaddr_in6*)pRePagServer->pstSockAdresse->vbSockaddr)->sin6_addr, c46EmpfangeneIPNummer, 46);
		if(vAdminServer->stServer.stSockaddrIV6_1.vbSockaddr && !StrCompare(c46EmpfangeneIPNummer, 46,
						inet_ntop(AF_INET6, &((sockaddr_in6*)vAdminServer->stServer.stSockaddrIV6_1.vbSockaddr)->sin6_addr, c46EigeneIPNummer, 46), 46))
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stAdminServer.bit128Core_1, BY_BIT128);
		else if(vAdminServer->stServer.stSockaddrIV6_2.vbSockaddr && !StrCompare(c46EmpfangeneIPNummer, 46,
						inet_ntop(AF_INET6, &((sockaddr_in6*)vAdminServer->stServer.stSockaddrIV6_2.vbSockaddr)->sin6_addr, c46EigeneIPNummer, 46), 46))
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stAdminServer.bit128Core_2, BY_BIT128);
		else if(vAdminServer->stLoginServer.stSockaddrIV6_1.vbSockaddr && !StrCompare(c46EmpfangeneIPNummer, 46,
						inet_ntop(AF_INET6, &((sockaddr_in6*)vAdminServer->stLoginServer.stSockaddrIV6_1.vbSockaddr)->sin6_addr, c46EigeneIPNummer, 46), 46))
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stLoginServer.bit128Core_1, BY_BIT128);
		else if(vAdminServer->stLoginServer.stSockaddrIV6_2.vbSockaddr && !StrCompare(c46EmpfangeneIPNummer, 46,
						inet_ntop(AF_INET6, &((sockaddr_in6*)vAdminServer->stLoginServer.stSockaddrIV6_2.vbSockaddr)->sin6_addr, c46EigeneIPNummer, 46), 46))
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->stLoginServer.bit128Core_2, BY_BIT128);
		else if(vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.vbSockaddr && !StrCompare(c46EmpfangeneIPNummer, 46,
						inet_ntop(AF_INET6, &((sockaddr_in6*)vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.vbSockaddr)->sin6_addr, c46EigeneIPNummer, 46), 46))
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->vstProgServer[SRV_PROGSERVER].bit128Core_1, BY_BIT128);
		else if(vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.vbSockaddr && !StrCompare(c46EmpfangeneIPNummer, 46,
						inet_ntop(AF_INET6, &((sockaddr_in6*)vAdminServer->vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.vbSockaddr)->sin6_addr, c46EigeneIPNummer, 46), 46))
			MemCopy(pRePagServer->bit128Core_Client, pBasisServer->vstProgServer[SRV_PROGSERVER].bit128Core_2, BY_BIT128);
	}
	switch(usAufgabe){
		case CS_EINTRAGPROTOKOLL:	  vAdminServer->EintragProtokoll(pRePagServer); break;
		case CS_SITZUNGSLISTE:		  vAdminServer->Sitzungsliste(pRePagServer); break;
		case CS_BENUTZERANMELDUNG:  vAdminServer->BenutzerAnmeldung(pRePagServer); break;
		case CS_BENUTZERABMELDUNG:  vAdminServer->BenutzerAbmeldung(pRePagServer); break;
		case CS_SERVER2ANMELDUNG:   vAdminServer->Server2Anmeldung(pRePagServer); break;
		case CS_SERVER2ABMELDUNG:   vAdminServer->Server2Abmeldung(pRePagServer); break;
		case CS_CLIENTWECHSEL: 		  vAdminServer->ClientWechsel(pRePagServer); break;
		case CS_PROTOKOLLANMELDUNG: vAdminServer->ProtokollAnmeldung(pRePagServer); break;
		case CS_PROTOKOLLABMELDUNG:	vAdminServer->ProtokollAbmeldung(pRePagServer); break;
		case CS_SITZUNGEN:					vAdminServer->Sitzungen(pRePagServer); break;
		case CS_VIRTUALMEMORY:			vAdminServer->VirtualMemory(pRePagServer); break;
		case CS_CORECONNECT:				vAdminServer->CoreConnect(pRePagServer); break;
		case CS_COREMEMORY:					vAdminServer->CoreMemory(pRePagServer); break;
		case CS_DBSYNCHRON:					vAdminServer->DBSynchron(pRePagServer); break;
		default: asEintrag = "Unbekannte interne Aufgabe von "; ZeroMem(c46IPBuffer, 46);
			if(!pRePagServer->bIPV6) asEintrag += inet_ntop(AF_INET, &((sockaddr_in*)pRePagServer->pstSockAdresse->vbSockaddr)->sin_addr, c46IPBuffer, 16);
			else asEintrag += inet_ntop(AF_INET6, &((sockaddr_in6*)pRePagServer->pstSockAdresse->vbSockaddr)->sin6_addr, c46IPBuffer, 46);
			LogEintrag_Server(pBasisServer->asName, asEintrag.c_Str());
	}

	if(pRePagServer->ucInfo > SF_KEINESITZUNG){
		char c11Zahl[11]; asEintrag = "Fehler in Interner-Funktion: "; asEintrag += ULONGtoCHAR(c11Zahl, usAufgabe);
		asEintrag += " /Fehlercode: "; asEintrag += ULONGtoCHAR(c11Zahl, pRePagServer->ucInfo); LogEintrag_Server(pBasisServer->asName, asEintrag.c_Str());
		pRePagServer->ucInfo = SF_UNBEKANNTEAUFGABE;
		pRePagServer->Senden();
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall frAufgabeExtern(COProtokollServer* pRePagServer, USHORT& usAufgabe, USHORT& usVersion)
{
	char c46IPBuffer[46]; COStringA asEintrag;

	switch(usAufgabe){
		case CS_EINTRAGPROTOKOLL:				if(vAdminServer->RechteKontrolle(pRePagServer, ZR_LESEN)) vAdminServer->EintragProtokoll(pRePagServer);	break;
		case CS_PROTODATEI:							if(vAdminServer->RechteKontrolle(pRePagServer, ZR_LESEN)) vAdminServer->ProtoDatei(pRePagServer);	break;
		case CS_DBTABELLELESEN:					if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_1)) vAdminServer->DBTabelleLesen(pRePagServer); break;
		case CS_DBDATENSATZANDERN:																 
		case CS_DBDATENSATZEINFUGEN: 			 
		case CS_DBDATENSATZMAXEINFUGEN: 					 
		case CS_DBDATENSATZLOSCHEN:			if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_1)) vAdminServer->DBDatensatzAnderung(pRePagServer, usAufgabe); break;
		case CS_SITZUNGEN:							if(vAdminServer->RechteKontrolle(pRePagServer, ZR_LESEN))	vAdminServer->HoleSitzungen(pRePagServer); break;
		case CS_DBAUSGLEICHEN:					if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_1)) vAdminServer->DBTabelleAusgleichen(pRePagServer); break;
		case CS_DBSTARTOPTIMIERUNG:			if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_1)) vAdminServer->DBStartOptimierung(pRePagServer); break;
		case CS_VIRTUALMEMORY:					if(vAdminServer->RechteKontrolle(pRePagServer, ZR_LESEN))	vAdminServer->HoleVirtualMemory(pRePagServer); break;
		case CS_CORECONNECT:						if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_1)) vAdminServer->GetCoreConnect(pRePagServer); break;
		case CS_COREMEMORY:							if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_1)) vAdminServer->GetCoreMemory(pRePagServer); break;
		case CS_DBENCRYPTINITAL:				if(vAdminServer->RechteKontrolle(pRePagServer, ZR_SONDER_4)) vAdminServer->DBDateiEncryptInital(pRePagServer); break;
			
		default:
			asEintrag = "Unbekannte externer Aufgabe von "; ZeroMem(c46IPBuffer, 46);
			if(!pRePagServer->bIPV6) asEintrag += inet_ntop(AF_INET, &((sockaddr_in*)pRePagServer->pstSockAdresse->vbSockaddr)->sin_addr, c46IPBuffer, 16);
			else asEintrag += inet_ntop(AF_INET6, &((sockaddr_in6*)pRePagServer->pstSockAdresse->vbSockaddr)->sin6_addr, c46IPBuffer, 46);
			LogEintrag_Server(pBasisServer->asName, asEintrag.c_Str());
	}

	if(pRePagServer->ucInfo > SF_KEINESITZUNG){
		char c11Zahl[11]; asEintrag = "Fehler in Externer-Funktion: "; asEintrag += ULONGtoCHAR(c11Zahl, usAufgabe);
		asEintrag += ULONGtoCHAR(c11Zahl, usVersion); asEintrag += " /Fehlercode: "; asEintrag += ULONGtoCHAR(c11Zahl, pRePagServer->ucInfo);
		LogEintrag_Server(pBasisServer->asName, asEintrag.c_Str());
		pRePagServer->ucInfo = SF_UNBEKANNTEAUFGABE;
		pRePagServer->Senden(); 
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------
bool __vectorcall COAdminServer::COAdminServerV(_In_ bool bInit, _In_ bool bDoppelA, _In_ bool bServer_2A, _In_z_ char* c6AusgleichenA, _In_z_ char* c6OptimierenA)
{
	//Sleep(10000);

	BDoppel = bDoppelA; bServer_2 = bServer_2A;
	MemCopy(c6Ausgleichen, c6AusgleichenA, 6); MemCopy(c6Optimieren, c6OptimierenA, 6);

	HKEY hSchlussel = NULL; char pcDatabasePath[MAX_PATH]; DWORD dwBytes = MAX_PATH;
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, pcServiceSchlussel, 0, KEY_READ, &hSchlussel)){
		if(!RegQueryValueEx(hSchlussel, "DatabasePath", 0, 0, (PBYTE)pcDatabasePath, &dwBytes)){
			MemCopy(&pcDatabasePath[dwBytes - 1], "\\", 2);
		}
		else return false;
	}
	else return false;

	HRSRC hResource = NULL;
	if(!(hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_DBADMIN), RT_RCDATA))) return false;
	COBasisServerV(hResource, pcDatabasePath);

	pfnAufgabeIntern = frAufgabeIntern; pfnAufgabeExtern = frAufgabeExtern; pfnTimerStart = frTimerStart;

	DWORD lpMinimumWorkingSetSize, lpMaximumWorkingSetSize;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
	lpMinimumWorkingSetSize = 6144000; lpMaximumWorkingSetSize = 10240000;
	SetProcessWorkingSetSize(hProcess, lpMinimumWorkingSetSize, lpMaximumWorkingSetSize);

	asName = "Admin_"; char c11Zahl[11]; asName += ULONGtoCHAR(c11Zahl, (bServer_2 + 1));
	stAdminServer.asIPClient = asName;

	WSADATA wsaData; addrinfo* addrResult = nullptr;
	addrinfo addrVorgabe = {0};
	addrVorgabe.ai_socktype = SOCK_STREAM;
	addrVorgabe.ai_protocol = IPPROTO_TCP;

	char c256HostName[256];
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) == NULL) gethostname(c256HostName, 256);
	else return false;

	addrVorgabe.ai_family = AF_INET;
	if(!getaddrinfo(DNS_QUORRA, PORT_CORE_STR, &addrVorgabe, &addrResult)){
		stCoreServer.stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
		stCoreServer.stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, stCoreServer.stSockaddrIV4_1.ulLange);
		MemCopy(stCoreServer.stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, stCoreServer.stSockaddrIV4_1.ulLange);
		stServer.stSockaddrIV4_2.vbSockaddr = nullptr; stServer.stSockaddrIV4_2.ulLange = NULL;
		
		(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_QUORRA, stCoreServer.bit128Core_1));
		freeaddrinfo(addrResult);
	}
	else{ stCoreServer.stSockaddrIV4_1.vbSockaddr = nullptr; stCoreServer.stSockaddrIV4_1.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET6;
	if(!getaddrinfo(DNS_QUORRA, PORT_CORE_STR, &addrVorgabe, &addrResult)){
		stCoreServer.stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
		stCoreServer.stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, stCoreServer.stSockaddrIV6_1.ulLange);
		MemCopy(stCoreServer.stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, stCoreServer.stSockaddrIV6_1.ulLange);
		stServer.stSockaddrIV6_2.vbSockaddr = nullptr; stServer.stSockaddrIV6_2.ulLange = NULL;
		
		if(!stCoreServer.stSockaddrIV4_1.vbSockaddr) (((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_QUORRA, stCoreServer.bit128Core_1));
		freeaddrinfo(addrResult);
	}
	else{ stCoreServer.stSockaddrIV6_1.vbSockaddr = nullptr;	stCoreServer.stSockaddrIV6_1.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET;
	if(!getaddrinfo(c256HostName, PORT_CORE_STR, &addrVorgabe, &addrResult)){
		stCoreServer.stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
		stCoreServer.stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, stCoreServer.stSockaddrIV4_2.ulLange);
		MemCopy(stCoreServer.stSockaddrIV4_2.vbSockaddr, addrResult->ai_addr, stCoreServer.stSockaddrIV4_2.ulLange);
		
		((FnSelfCoreNumber*)vbProgInternA)[FN_SELFCORENUMBER](stCoreServer.bit128Core_2);
		freeaddrinfo(addrResult);
	}
	else{ stCoreServer.stSockaddrIV4_2.vbSockaddr = nullptr; stCoreServer.stSockaddrIV4_2.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET6;
	if(!getaddrinfo(c256HostName, PORT_CORE_STR, &addrVorgabe, &addrResult)){
		stCoreServer.stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
		stCoreServer.stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, stCoreServer.stSockaddrIV6_2.ulLange);
		MemCopy(stCoreServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stCoreServer.stSockaddrIV6_2.ulLange);
		
		if(!stCoreServer.stSockaddrIV4_2.vbSockaddr) ((FnSelfCoreNumber*)vbProgInternA)[FN_SELFCORENUMBER](stCoreServer.bit128Core_2);
		freeaddrinfo(addrResult);
	}
	else{ stCoreServer.stSockaddrIV6_2.vbSockaddr = nullptr;	stCoreServer.stSockaddrIV6_2.ulLange = NULL; }
	addrResult = nullptr;

	stCoreServer.asIPClient = asName;
	InitializeCriticalSection(&stCoreServer.csServer);
	stCoreServer.usPort = PORT_CORE;
	stCoreServer.av16AES_CryptKeys = nullptr;
	stCoreServer.bServer_2 = false;

	if(!bServer_2){
		addrVorgabe.ai_family = AF_INET;
		if(!getaddrinfo(c256HostName, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
			stServer.stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
			stServer.stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV4_1.ulLange);
			MemCopy(stServer.stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV4_1.ulLange);
			stServer.stSockaddrIV4_2.vbSockaddr = nullptr; stServer.stSockaddrIV4_2.ulLange = NULL;

			stAdminServer.stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
			stAdminServer.stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV4_1.ulLange);
			MemCopy(stAdminServer.stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV4_1.ulLange);
			stAdminServer.stSockaddrIV4_2.vbSockaddr = nullptr; stAdminServer.stSockaddrIV4_2.ulLange = NULL;
		
			((FnSelfCoreNumber*)vbProgInternA)[FN_SELFCORENUMBER](stServer.bit128Core_1);
			MemCopy(stAdminServer.bit128Core_1, stServer.bit128Core_1, BY_BIT128);
			freeaddrinfo(addrResult);
		}
		else{
			stAdminServer.stSockaddrIV4_1.vbSockaddr = nullptr; stAdminServer.stSockaddrIV4_1.ulLange = NULL;
			stServer.stSockaddrIV4_1.vbSockaddr = nullptr; stServer.stSockaddrIV4_1.ulLange = NULL;
		}
		addrResult = nullptr;

		addrVorgabe.ai_family = AF_INET6;
		if(!getaddrinfo(c256HostName, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
			stServer.stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
			stServer.stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV6_1.ulLange);
			MemCopy(stServer.stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV6_1.ulLange);
			stServer.stSockaddrIV6_2.vbSockaddr = nullptr; stServer.stSockaddrIV6_2.ulLange = NULL;

			stAdminServer.stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
			stAdminServer.stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV6_1.ulLange);
			MemCopy(stAdminServer.stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV6_1.ulLange);
			stAdminServer.stSockaddrIV6_2.vbSockaddr = nullptr; stAdminServer.stSockaddrIV6_2.ulLange = NULL;

			if(!stServer.stSockaddrIV4_1.vbSockaddr){	((FnSelfCoreNumber*)vbProgInternA)[FN_SELFCORENUMBER](stServer.bit128Core_1);
				MemCopy(stAdminServer.bit128Core_1, stServer.bit128Core_1, BY_BIT128);
			}
			freeaddrinfo(addrResult);
		}
		else{ stServer.stSockaddrIV6_1.vbSockaddr = nullptr;	stAdminServer.stSockaddrIV6_1.vbSockaddr = nullptr; }
		addrResult = nullptr;

		if(BDoppel){
			addrVorgabe.ai_family = AF_INET;
			if(!getaddrinfo(DNS_ADMINSERVER_2, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
				stServer.stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
				stServer.stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV4_2.ulLange);
				MemCopy(stServer.stSockaddrIV4_2.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV4_2.ulLange);

				stAdminServer.stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
				stAdminServer.stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV4_2.ulLange);
				MemCopy(stAdminServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV4_2.ulLange);

				((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_ADMINSERVER_2, stAdminServer.bit128Core_2);
				MemCopy(stServer.bit128Core_2, stAdminServer.bit128Core_2, BY_BIT128);
				freeaddrinfo(addrResult);
			}
			else{
				stAdminServer.stSockaddrIV4_2.vbSockaddr = nullptr; stAdminServer.stSockaddrIV4_2.ulLange = NULL;
				stServer.stSockaddrIV4_2.vbSockaddr = nullptr; stServer.stSockaddrIV4_2.ulLange = NULL;
			}
			addrResult = nullptr;

			addrVorgabe.ai_family = AF_INET6;
			if(!getaddrinfo(DNS_ADMINSERVER_2, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
				stServer.stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
				stServer.stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV6_2.ulLange);
				MemCopy(stServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV6_2.ulLange);

				stAdminServer.stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
				stAdminServer.stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV6_2.ulLange);
				MemCopy(stAdminServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV6_2.ulLange);

				if(!stServer.stSockaddrIV4_2.vbSockaddr){	((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_ADMINSERVER_2, stAdminServer.bit128Core_2);
					MemCopy(stServer.bit128Core_2, stAdminServer.bit128Core_2, BY_BIT128);
				}
				freeaddrinfo(addrResult);
			}
			else{ stServer.stSockaddrIV6_2.vbSockaddr = nullptr; stServer.stSockaddrIV6_2.ulLange = NULL;
						stAdminServer.stSockaddrIV6_2.vbSockaddr = nullptr; stAdminServer.stSockaddrIV6_2.ulLange = NULL;	}
			addrResult = nullptr;
		}
	}
	else{
		addrVorgabe.ai_family = AF_INET;
		if(!getaddrinfo(c256HostName, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
			stServer.stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
			stServer.stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV4_2.ulLange);
			MemCopy(stServer.stSockaddrIV4_2.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV4_2.ulLange);
			stServer.stSockaddrIV4_1.vbSockaddr = nullptr; stServer.stSockaddrIV4_1.ulLange = NULL;

			stAdminServer.stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
			stAdminServer.stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV4_2.ulLange);
			MemCopy(stAdminServer.stSockaddrIV4_2.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV4_2.ulLange);
			stAdminServer.stSockaddrIV4_1.vbSockaddr = nullptr; stAdminServer.stSockaddrIV4_1.ulLange = NULL;
			
			((FnSelfCoreNumber*)vbProgInternA)[FN_SELFCORENUMBER](stServer.bit128Core_2);
			MemCopy(stAdminServer.bit128Core_2, stServer.bit128Core_2, BY_BIT128);
			freeaddrinfo(addrResult);
		}
		else{	stAdminServer.stSockaddrIV4_2.vbSockaddr = nullptr; stAdminServer.stSockaddrIV4_2.ulLange = NULL;
			    stServer.stSockaddrIV4_2.vbSockaddr = nullptr; stServer.stSockaddrIV4_2.ulLange = NULL;	}
		addrResult = nullptr;

		addrVorgabe.ai_family = AF_INET6;
		if(!getaddrinfo(c256HostName, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
			stServer.stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
			stServer.stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV6_2.ulLange);
			MemCopy(stServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV6_2.ulLange);
			stServer.stSockaddrIV6_1.vbSockaddr = nullptr; stServer.stSockaddrIV6_1.ulLange = NULL;

			stAdminServer.stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
			stAdminServer.stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV6_2.ulLange);
			MemCopy(stAdminServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV6_2.ulLange);
			stAdminServer.stSockaddrIV6_1.vbSockaddr = nullptr; stAdminServer.stSockaddrIV6_1.ulLange = NULL;

			if(!stServer.stSockaddrIV4_2.vbSockaddr){ ((FnSelfCoreNumber*)vbProgInternA)[FN_SELFCORENUMBER](stServer.bit128Core_2);
				MemCopy(stAdminServer.bit128Core_2, stServer.bit128Core_2, BY_BIT128);
			}
			freeaddrinfo(addrResult);
		}
		else{	stServer.stSockaddrIV6_1.vbSockaddr = nullptr;	stServer.stSockaddrIV6_1.ulLange = NULL;
					stAdminServer.stSockaddrIV6_1.vbSockaddr = nullptr; stAdminServer.stSockaddrIV6_1.ulLange = NULL;	}
		addrResult = nullptr;

		if(BDoppel){
			addrVorgabe.ai_family = AF_INET;
			if(!getaddrinfo(DNS_ADMINSERVER_1, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
				stServer.stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
				stServer.stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV4_1.ulLange);
				MemCopy(stServer.stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV4_1.ulLange);

				stAdminServer.stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
				stAdminServer.stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV4_1.ulLange);
				MemCopy(stAdminServer.stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV4_1.ulLange);
				
				(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_ADMINSERVER_1, stAdminServer.bit128Core_1));
				MemCopy(stServer.bit128Core_1, stAdminServer.bit128Core_1, BY_BIT128);
				freeaddrinfo(addrResult);
			}
			else{	stAdminServer.stSockaddrIV4_1.vbSockaddr = nullptr; stAdminServer.stSockaddrIV4_1.ulLange = NULL;
						stServer.stSockaddrIV4_1.vbSockaddr = nullptr; stServer.stSockaddrIV4_1.ulLange = NULL;	}
			addrResult = nullptr;

			addrVorgabe.ai_family = AF_INET6;
			if(!getaddrinfo(DNS_ADMINSERVER_1, PORT_ADMIN_STR, &addrVorgabe, &addrResult)){
				stServer.stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
				stServer.stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, stServer.stSockaddrIV6_1.ulLange);
				MemCopy(stServer.stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, stServer.stSockaddrIV6_1.ulLange);

				stAdminServer.stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
				stAdminServer.stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, stAdminServer.stSockaddrIV6_1.ulLange);
				MemCopy(stAdminServer.stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, stAdminServer.stSockaddrIV6_1.ulLange);

				if(!stServer.stSockaddrIV4_1.vbSockaddr){	(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_ADMINSERVER_1, stAdminServer.bit128Core_1));
					MemCopy(stServer.bit128Core_1, stAdminServer.bit128Core_1, BY_BIT128);
				}
				freeaddrinfo(addrResult);
			}
			else{ stServer.stSockaddrIV6_1.vbSockaddr = nullptr;	stServer.stSockaddrIV6_1.ulLange = NULL;
						stAdminServer.stSockaddrIV6_1.vbSockaddr = nullptr; stAdminServer.stSockaddrIV6_1.ulLange = NULL;	}
			addrResult = nullptr;
		}

	}
	stServer.asIPClient = asName;
	InitializeCriticalSection(&stServer.csServer);
	stServer.usPort = PORT_ADMIN;
	stServer.bServer_2 = false;
	stServer.av16AES_CryptKeys = nullptr;

	stAdminServer.asIPClient = asName;
	InitializeCriticalSection(&stAdminServer.csServer);
	stAdminServer.usPort = PORT_ADMIN;
	stAdminServer.bServer_2 = false;
	stAdminServer.av16AES_CryptKeys = nullptr;

	addrVorgabe.ai_family = AF_INET;
	if(!getaddrinfo(DNS_LOGINSERVER_1, PORT_LOGIN_STR, &addrVorgabe, &addrResult)){
		stLoginServer.stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
		stLoginServer.stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, stLoginServer.stSockaddrIV4_1.ulLange);
		MemCopy(stLoginServer.stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, stLoginServer.stSockaddrIV4_1.ulLange);
		stLoginServer.stSockaddrIV4_2.vbSockaddr = nullptr; stLoginServer.stSockaddrIV4_2.ulLange = NULL;
		
		(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_LOGINSERVER_1, stLoginServer.bit128Core_1));
		freeaddrinfo(addrResult);
	}
	else{ stLoginServer.stSockaddrIV4_1.vbSockaddr = nullptr; stLoginServer.stSockaddrIV4_1.ulLange = NULL; }
	addrResult = nullptr;;

	addrVorgabe.ai_family = AF_INET6;
	if(!getaddrinfo(DNS_LOGINSERVER_1, PORT_LOGIN_STR, &addrVorgabe, &addrResult)){
		stLoginServer.stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
		stLoginServer.stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, stLoginServer.stSockaddrIV6_1.ulLange);
		MemCopy(stLoginServer.stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, stLoginServer.stSockaddrIV6_1.ulLange);
		stLoginServer.stSockaddrIV6_2.vbSockaddr = nullptr; stLoginServer.stSockaddrIV6_2.ulLange = NULL;
		
		if(!stLoginServer.stSockaddrIV4_1.vbSockaddr) (((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_LOGINSERVER_1, stLoginServer.bit128Core_1));
		freeaddrinfo(addrResult);
	}
	else{ stLoginServer.stSockaddrIV6_1.vbSockaddr = nullptr; stLoginServer.stSockaddrIV6_1.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET;
	if(!getaddrinfo(DNS_LOGINSERVER_2, PORT_LOGIN_STR, &addrVorgabe, &addrResult)){
		stLoginServer.stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
		stLoginServer.stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, stLoginServer.stSockaddrIV4_2.ulLange);
		MemCopy(stLoginServer.stSockaddrIV4_2.vbSockaddr, addrResult->ai_addr, stLoginServer.stSockaddrIV4_2.ulLange);
		freeaddrinfo(addrResult);
	}
	else{ stLoginServer.stSockaddrIV4_2.vbSockaddr = nullptr; stLoginServer.stSockaddrIV4_2.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET6;
	if(!getaddrinfo(DNS_LOGINSERVER_2, PORT_LOGIN_STR, &addrVorgabe, &addrResult)){
		stLoginServer.stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
		stLoginServer.stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, stLoginServer.stSockaddrIV6_2.ulLange);
		MemCopy(stLoginServer.stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, stLoginServer.stSockaddrIV6_2.ulLange);

		if(!stLoginServer.stSockaddrIV4_2.vbSockaddr) (((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_LOGINSERVER_2, stLoginServer.bit128Core_2));
		freeaddrinfo(addrResult);
	}
	else{ stLoginServer.stSockaddrIV6_2.vbSockaddr = nullptr; stLoginServer.stSockaddrIV6_2.ulLange = NULL; }
	addrResult = nullptr;

	vstProgServer = (STServer*)VMBlock(vmServer, sizeof(STServer) * 1);
	addrVorgabe.ai_family = AF_INET;
	if(!getaddrinfo(DNS_SERVER_1, PORT_SERVER_STR, &addrVorgabe, &addrResult)){
		vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.ulLange = (ULONG)addrResult->ai_addrlen;
		vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.vbSockaddr = VMBlock(vmServer, vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.ulLange);
		MemCopy(vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.vbSockaddr, addrResult->ai_addr, vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.ulLange);
		vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr = nullptr; vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.ulLange = NULL;
		
		(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_SERVER_1, vstProgServer[SRV_PROGSERVER].bit128Core_1));
		freeaddrinfo(addrResult);
	}
	else{ vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.vbSockaddr = nullptr; vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET6;
	if(!getaddrinfo(DNS_SERVER_1, PORT_SERVER_STR, &addrVorgabe, &addrResult)){
		vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.ulLange = (ULONG)addrResult->ai_addrlen;
		vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.vbSockaddr = VMBlock(vmServer, vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.ulLange);
		MemCopy(vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.vbSockaddr, addrResult->ai_addr, vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.ulLange);
		
		if(!vstProgServer[SRV_PROGSERVER].stSockaddrIV4_1.vbSockaddr) (((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_SERVER_1, vstProgServer[SRV_PROGSERVER].bit128Core_1));
		freeaddrinfo(addrResult);
	}
	else{ vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.vbSockaddr = nullptr; vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET;
	if(!getaddrinfo(DNS_SERVER_2, PORT_SERVER_STR, &addrVorgabe, &addrResult)){
		vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.ulLange = (ULONG)addrResult->ai_addrlen;
		vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr = VMBlock(vmServer, vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.ulLange);
		MemCopy(vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr, addrResult->ai_addr, vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.ulLange);
		
		(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_SERVER_2, vstProgServer[SRV_PROGSERVER].bit128Core_2));
		freeaddrinfo(addrResult);
	}
	else{ vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr = nullptr; vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.ulLange = NULL; }
	addrResult = nullptr;

	addrVorgabe.ai_family = AF_INET6;
	if(!getaddrinfo(DNS_SERVER_2, PORT_SERVER_STR, &addrVorgabe, &addrResult)){
		vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.ulLange = (ULONG)addrResult->ai_addrlen;
		vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.vbSockaddr = VMBlock(vmServer, vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.ulLange);
		MemCopy(vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.vbSockaddr, addrResult->ai_addr, vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.ulLange);

		if(!vstProgServer[SRV_PROGSERVER].stSockaddrIV4_2.vbSockaddr)	(((FnDNSToCore*)vbProgInternA)[FN_DNSTOCORE](DNS_SERVER_2, vstProgServer[SRV_PROGSERVER].bit128Core_2));
		freeaddrinfo(addrResult);
	}
	else{	vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.vbSockaddr = nullptr; vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.ulLange = NULL;	}
	addrResult = nullptr;

	stLoginServer.asIPClient = asName;
	InitializeCriticalSection(&stLoginServer.csServer);
	stLoginServer.usPort = PORT_LOGIN;
	stLoginServer.bServer_2 = false;
	stLoginServer.av16AES_CryptKeys = nullptr;

	vstProgServer[SRV_PROGSERVER].asIPClient = asName;
	InitializeCriticalSection(&vstProgServer[SRV_PROGSERVER].csServer);
	vstProgServer[SRV_PROGSERVER].usPort = PORT_SERVER;
	vstProgServer[SRV_PROGSERVER].bServer_2 = false;
	vstProgServer[SRV_PROGSERVER].av16AES_CryptKeys = nullptr;

	STTHDBStart* thstDBStart = (STTHDBStart*)VMBlock(sizeof(STTHDBStart));
	thstDBStart->bInit = bInit;
	thstDBStart->hThread = CreateThread(NULL, 0, thDBStart, thstDBStart, CREATE_SUSPENDED, NULL);
	SetThreadPriority(thstDBStart->hThread, THREAD_PRIORITY_NORMAL);
	ResumeThread(thstDBStart->hThread);

	vthlstProtokollEintrag = COListV(vmServer, true);

	thstLogBuch.bEnde = false;
	heLogBuch = CreateEvent(NULL, false, false, NULL);
	hthLogBuch = CreateThread(NULL, 0, thLogBuch, &thstLogBuch, CREATE_SUSPENDED, NULL);
	SetThreadPriority(hthLogBuch, THREAD_PRIORITY_LOWEST);
	ResumeThread(hthLogBuch);

	//thstVerbindung.bEnde = false;
	//hthVerbindung = CreateThread(NULL, 0, thVerbindung, &thstVerbindung, CREATE_SUSPENDED, NULL);
	//SetThreadPriority(hthVerbindung, THREAD_PRIORITY_LOWEST);
	//ResumeThread(hthVerbindung);
	//CreateTimerQueueTimer(&htVerbindung, hTimerQueue, (WAITORTIMERCALLBACK)Timer_Verbindung, NULL, 30000, 30000, 0);

	thstSitzGultig = (STTHSitzGultig*)VMBlock(vmServer, sizeof(STTHSitzGultig));
	thstSitzGultig->bEnde = false;
	thstSitzGultig->hThread = CreateThread(NULL, 0, thAdminSitzGultig, thstSitzGultig, CREATE_SUSPENDED, NULL);
	SetThreadPriority(thstSitzGultig->hThread, THREAD_PRIORITY_IDLE);

	CreateTimerQueueTimer(&htSitzungGultig, hTimerQueue, (WAITORTIMERCALLBACK)Timer_SitzungGultig, NULL, 1000, 30000, 0);

 
	SetEvent(phthDBTabellen[ucTabAnzahl]);

	return true;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
VMEMORY __vectorcall COAdminServer::COFreiV(void)
{
	//LogEintrag_Server(pBasisServer->asName, "Der Server wurde beendet");

	//thstVerbindung.bEnde = true; ResumeThread(hthVerbindung); CloseHandle(hthVerbindung);
	thstLogBuch.bEnde = true; SetEvent(heLogBuch); CloseHandle(hthLogBuch);

	VMFrei(vmServer, vthlstProtokollEintrag);

	DeleteCriticalSection(&stCoreServer.csServer); VMFrei(vmServer, stCoreServer.stSockaddrIV6_1.vbSockaddr);
	DeleteCriticalSection(&stServer.csServer); DeleteCriticalSection(&stAdminServer.csServer);
	DeleteCriticalSection(&stLoginServer.csServer); DeleteCriticalSection(&vstProgServer[SRV_PROGSERVER].csServer);

	VMFrei(vmServer, stServer.stSockaddrIV6_1.vbSockaddr); VMFrei(vmServer, stServer.stSockaddrIV6_2.vbSockaddr);
	VMFrei(vmServer, stAdminServer.stSockaddrIV6_1.vbSockaddr); VMFrei(vmServer, stAdminServer.stSockaddrIV6_2.vbSockaddr);
	VMFrei(vmServer, stLoginServer.stSockaddrIV6_1.vbSockaddr); VMFrei(vmServer, stLoginServer.stSockaddrIV6_2.vbSockaddr);
	VMFrei(vmServer, vstProgServer[SRV_PROGSERVER].stSockaddrIV6_1.vbSockaddr); VMFrei(vmServer, vstProgServer[SRV_PROGSERVER].stSockaddrIV6_2.vbSockaddr);
	VMFrei(vmServer, vstProgServer);

	return ((COBasisServer*)this)->COFreiV();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::ProtokollAnmeldung(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STBenutzer *vstBenutzer = (STBenutzer*)VMBlock(vmServer, sizeof(STBenutzer));
		vstBenutzer->vzLetzteAktion = COTimeV(vmServer);
		vstBenutzer->vzAnmeldung = COTimeV(vmServer);
		pRePagServer->LeseZeit(vstBenutzer->vzAnmeldung);
		pRePagServer->Lese(&vstBenutzer->bit128Sitzung, BY_BIT128);
		pRePagServer->Lese(&vstBenutzer->ulNummer, BY_ULONG);
		pRePagServer->Lese(&vstBenutzer->stSockAdresse.ulLange, BY_ULONG);
		vstBenutzer->stSockAdresse.vbSockaddr = VMBlock(vmServer, vstBenutzer->stSockAdresse.ulLange);
		pRePagServer->Lese(vstBenutzer->stSockAdresse.vbSockaddr, vstBenutzer->stSockAdresse.ulLange);
		pRePagServer->Lese(&vstBenutzer->ucProgramm, BY_BYTE);
		pRePagServer->Lese(&vstBenutzer->ucRechte, BY_BYTE);
		vstBenutzer->bServer_2 = bServer_2;

		if(!pRePagServer->ucInfo){
			vstBenutzer->vzLetzteAktion->Now();
			vthlstBenutzer->ThToEnd(vstBenutzer);
			pRePagServer->SendeOK();

			if(BDoppel){
				STTHServer2Anmeldung* thstServer2Anmeldung = (STTHServer2Anmeldung*)VMBlock(sizeof(STTHServer2Anmeldung));
				MemCopy(thstServer2Anmeldung->bit128Sitzung, vstBenutzer->bit128Sitzung, BY_BIT128);
				thstServer2Anmeldung->hThread = CreateThread(NULL, 0, thServer2Anmeldung, thstServer2Anmeldung, CREATE_SUSPENDED, NULL);
				SetThreadPriority(thstServer2Anmeldung->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
				ResumeThread(thstServer2Anmeldung->hThread);
			}
		}
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::ProtokollAbmeldung(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){ BIT128 bit128Sitzung; void* pvLoschen = nullptr;

		pRePagServer->Lese(&bit128Sitzung, BY_BIT128);

		void* pvIterator = vthlstBenutzer->ThIteratorToBegin_Lock();
		while(pvIterator){
			if(!BIT128Compare(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->bit128Sitzung, bit128Sitzung)){
				if(BDoppel){
					STTHServer2Abmeldung* thstServer2Abmeldung = (STTHServer2Abmeldung*)VMBlock(sizeof(STTHServer2Abmeldung));
					MemCopy(thstServer2Abmeldung->bit128Sitzung, bit128Sitzung, BY_BIT128);
					thstServer2Abmeldung->ulBenutzer = ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->ulNummer;
					thstServer2Abmeldung->ucProgramm = ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->ucProgramm;
					thstServer2Abmeldung->hThread = CreateThread(NULL, 0, thServer2Abmeldung, thstServer2Abmeldung, CREATE_SUSPENDED, NULL);
					SetThreadPriority(thstServer2Abmeldung->hThread, THREAD_PRIORITY_LOWEST);
					ResumeThread(thstServer2Abmeldung->hThread);
				}

				VMFrei(vmServer, ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->stSockAdresse.vbSockaddr);
				VMFreiV(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->vzAnmeldung);
				VMFreiV(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->vzLetzteAktion);
				vthlstBenutzer->DeleteElement(pvIterator, pvLoschen, true);
				break;
			}
			vthlstBenutzer->NextElement(pvIterator, pvLoschen);
	}
	vthlstBenutzer->ThIteratorEnd();

	pRePagServer->SendeOK();
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::EintragProtokoll(COProtokollServer* pRePagServer)
{ 
	if(pRePagServer->Empfangen()){
		STProtokollEintrag* vstProtokoll = (STProtokollEintrag*)VMBlock(vmServer, sizeof(STProtokollEintrag));

		pRePagServer->LeseZeit(&vstProtokoll->zZeit);
		pRePagServer->LeseStringA(&vstProtokoll->asProgramm, FT_SHORTSTR);
		pRePagServer->LeseStringA(&vstProtokoll->asEintrag, FT_SHORTSTR);

		vthlstProtokollEintrag->ThToEnd(vstProtokoll);
		SetEvent(heLogBuch);
		pRePagServer->SendeOK();
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::ProtoDatei(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		COTime zDatum; COStringA asDatei; COStringA asDatum; COStringA asTemp;

		pRePagServer->LeseZeit(&zDatum);

		pRePagServer->NeueSendung();

		zDatum.StrDateFormat(&asDatum, "dd'.'MM'.'yyyy"); asDatum.SubString(&asTemp, 9, 10);
	//#ifdef DNSLOCAL
		asDatei = "C:\\Development\\Daten\\Log\\"; asDatei += asTemp;
	//#else
	//		*vasDatei = "C:\\Daten\\Log\\"; *vasDatei += *vasTemp;
	//#endif
		asDatum.SubString(&asTemp, 4, 5); asDatei += asTemp;
		asDatum.SubString(&asTemp, 1, 2); asDatei += asTemp;
		asDatei += ".log";

		HANDLE hDatei = CreateFile(asDatei.c_Str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if(hDatei != INVALID_HANDLE_VALUE){ int iErgebnis; ULONG ulGeleseneBytes;
			ULONG ulMenge = GetFileSize(hDatei, NULL);
			if(ulMenge != 0xFFFFFFFF){
					VMBLOCK vbDaten = VMBlock(ulMenge);
					iErgebnis = ReadFile(hDatei, vbDaten, ulMenge, &ulGeleseneBytes, NULL);
					pRePagServer->Schreibe(vbDaten, ulMenge);
					VMFrei(vbDaten);
			}
			else pRePagServer->ucInfo = 1;
			CloseHandle(hDatei);
		}
		else pRePagServer->ucInfo = 1;
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::HoleSitzungen(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;
		BYTE ucServer;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stAdminServer; bServer_1 = true;	break;
			case 1: pstServer = &stAdminServer; bServer_1 = false; break;
			case 2: pstServer = &stLoginServer; bServer_1 = true;	break;
			case 3: pstServer = &stLoginServer; bServer_1 = false; break;
			case 4: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true;	break;
			case 5: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false;	break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{
			vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_SITZUNGEN, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_SITZUNGEN, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->NurEmpfangen() == BY_BYTE){
					if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
				}
				else{
					ULONG ulBytes = vRePagClient->ulBytes;
					VMBLOCK vbTemp = VMBlock(ulBytes);
					vRePagClient->Lese(vbTemp, ulBytes);
					pRePagServer->Schreibe(vbTemp, ulBytes);
					VMFrei(vbTemp);
				}
			}

			if(vRePagClient->ucInfo){ 
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("HoleSitzungen - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);
	}
	if(!pRePagServer->ulBytes) pRePagServer->ucInfo = 1;
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::HoleVirtualMemory(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){

		STServer* pstServer = nullptr; bool bServer_1;
		BYTE ucServer, ucSpeicher; ULONG ulSeite;

		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->Lese(&ucSpeicher, BY_BYTE);
		pRePagServer->Lese(&ulSeite, BY_ULONG);

		pRePagServer->NeueSendung();

		switch(ucServer){
				case 0 : pstServer = &stAdminServer; bServer_1 = true; break;
				case 1 : pstServer = &stAdminServer; bServer_1 = false; break;
				case 2 : pstServer = &stLoginServer; bServer_1 = true; break;
				case 3 : pstServer = &stLoginServer; bServer_1 = false; break;
				case 4 : pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
				case 5 : pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
				case 6 : pstServer = &stCoreServer; bServer_1 = true; break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{
			vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			vRePagClient->Schreibe(&ucSpeicher, BY_BYTE);
			vRePagClient->Schreibe(&ulSeite, BY_ULONG);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_VIRTUALMEMORY, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_VIRTUALMEMORY, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen() == BY_BYTE){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
					else{
						ULONG ulBytes = vRePagClient->ulBytes;
						VMBLOCK vbTemp = VMBlock(ulBytes);
						vRePagClient->Lese(vbTemp, ulBytes);
						pRePagServer->Schreibe(vbTemp, ulBytes);
						VMFrei(vbTemp);
					}
				}
			}

			if(vRePagClient->ucInfo){
				if(!--ucVersuche){
					pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("HoleVirtualMemory - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::DBTabelleLesen(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;

		BYTE ucServer, ucTabelle, ucSpalte = 0; COStringA asDBParam;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->Lese(&ucTabelle, BY_BYTE);
		if(pRePagServer->ulBytes){
			pRePagServer->Lese(&ucSpalte, BY_BYTE);
			pRePagServer->LeseStringA(&asDBParam, FT_SHORTSTR);
		}
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stLoginServer;	bServer_1 = true; break;
			case 1: pstServer = &stLoginServer;	bServer_1 = false; break;
			case 2: pstServer = &vstProgServer[SRV_PROGSERVER];	bServer_1 = true; break;
			case 3: pstServer = &vstProgServer[SRV_PROGSERVER];	bServer_1 = false; break;
		}

		if(ucServer <= 1 && ucTabelle == TAB_PASSWORT && pRePagServer->ucRechte < ZR_SONDER_4) pRePagServer->ucInfo = SF_KEINRECHT;
		else if(ucServer <= 1 && ucTabelle == TAB_BERECHTIGUNG && pRePagServer->ucRechte < ZR_SONDER_4) pRePagServer->ucInfo = SF_KEINRECHT;
		else{ BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
			do{ vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

				vRePagClient->Schreibe(&ucTabelle, BY_BYTE);
				vRePagClient->Schreibe(&ucSpalte, BY_BYTE);
				vRePagClient->SchreibeStringA(&asDBParam, FT_SHORTSTR);

				bool bSendeAufgabe;
				if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_DBTABELLELESEN, CS_VINTERN);
				else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_DBTABELLELESEN, CS_VINTERN);

				if(bSendeAufgabe){
					if(vRePagClient->Senden()){
						if(vRePagClient->Empfangen() == BY_BYTE){
							if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
						}
						else{
							ULONG ulBytes = vRePagClient->ulBytes;
							VMBLOCK vbTemp = VMBlock(ulBytes);
							vRePagClient->Lese(vbTemp, ulBytes);
							pRePagServer->Schreibe(vbTemp, ulBytes);
							VMFrei(vbTemp);
						}
					}
				}

				if(vRePagClient->ucInfo){ 
					if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("DBTabelleLesen - Fehler bei Verbindung zu Server: ");
						asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
					}
				}
				else ucVersuche = 0;
				VMFreiV(vRePagClient);
			}
			while(ucVersuche);
		}
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::DBDatensatzAnderung(COProtokollServer* pRePagServer, USHORT& usAufgabe)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;

		BYTE ucServer;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		ULONG ulBytes = pRePagServer->ulBytes;
		VMBLOCK vbBuffer = VMBlock(ulBytes);
		pRePagServer->Lese(vbBuffer, ulBytes);
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stLoginServer; bServer_1 = true; break;
			case 1: pstServer = &stLoginServer; bServer_1 = false; break;
			case 2: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
			case 3: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
		}

		BYTE ucVersuche = 1; COProtokollClient* vRePagClient;
		do{ vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			vRePagClient->Schreibe(vbBuffer, ulBytes);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(usAufgabe, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(usAufgabe, CS_VINTERN);
			 
			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen()){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
				}
			}

			if(vRePagClient->ucInfo){ 
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("DBDatensatzAnderung - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);

		VMFrei(vbBuffer);

		if(!pRePagServer->ucInfo) pRePagServer->SendeOK();
	}
	pRePagServer->Senden();
} 
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::DBTabelleAusgleichen(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;

		BYTE ucServer, ucTabelle;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->Lese(&ucTabelle, BY_BYTE);
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stLoginServer; bServer_1 = true; break;
			case 1: pstServer = &stLoginServer; bServer_1 = false; break;
			case 2: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
			case 3: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{ vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			vRePagClient->Schreibe(&ucTabelle, BY_BYTE);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_DBAUSGLEICHEN, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_DBAUSGLEICHEN, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen()){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
				}
			}

			if(vRePagClient->ucInfo){ 
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("DBTabelleAusgleichen - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);

		if(!pRePagServer->ucInfo) pRePagServer->SendeOK();
	}
	pRePagServer->Senden();
} 
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::DBStartOptimierung(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;

		BYTE ucServer, ucTabelle;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->Lese(&ucTabelle, BY_BYTE);
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stLoginServer; bServer_1 = true; break;
			case 1: pstServer = &stLoginServer; bServer_1 = false; break;
			case 2: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
			case 3: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{ vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);


			vRePagClient->Schreibe(&ucTabelle, BY_BYTE);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_DBSTARTOPTIMIERUNG, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_DBSTARTOPTIMIERUNG, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen()){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
				}
			}

			if(vRePagClient->ucInfo){ 
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("DBStartOptimierung - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);

		if(!pRePagServer->ucInfo) pRePagServer->SendeOK();
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::DBDateiEncryptInital(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1; 

		BYTE ucServer;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		ULONG ulBytes = pRePagServer->ulBytes;
		VMBLOCK vbBuffer = VMBlock(ulBytes);
		pRePagServer->Lese(vbBuffer, ulBytes);

		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stLoginServer; bServer_1 = true; break;
			case 1: pstServer = &stLoginServer; bServer_1 = false; break;
			case 2: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
			case 3: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{
			vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			vRePagClient->Schreibe(vbBuffer, ulBytes);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_DBENCRYPTINITAL, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_DBENCRYPTINITAL, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen()){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
				}
			}

			if(vRePagClient->ucInfo){
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("DBEncryptInitial - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);

		if(!pRePagServer->ucInfo) pRePagServer->SendeOK();
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::GetCoreConnect(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;

		BYTE ucServer;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stAdminServer; bServer_1 = true; break;
			case 1: pstServer = &stAdminServer; bServer_1 = false; break;
			case 2: pstServer = &stLoginServer; bServer_1 = true; break;
			case 3: pstServer = &stLoginServer; bServer_1 = false; break;
			case 4: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
			case 5: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{
			vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			vRePagClient->Schreibe(&ucServer, BY_BYTE);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_CORECONNECT, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_CORECONNECT, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen() == BY_BYTE){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
					else{
						ULONG ulBytes = vRePagClient->ulBytes;
						VMBLOCK vbTemp = VMBlock(ulBytes);
						vRePagClient->Lese(vbTemp, ulBytes);
						pRePagServer->Schreibe(vbTemp, ulBytes);
						VMFrei(vbTemp);
					}
				}
			}

			if(vRePagClient->ucInfo){
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("GetCoreConnect - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		} while(ucVersuche);
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void __vectorcall COAdminServer::GetCoreMemory(COProtokollServer* pRePagServer)
{
	if(pRePagServer->Empfangen()){
		STServer* pstServer = nullptr; bool bServer_1;

		BYTE ucServer, ucSpeicher; ULONG ulSeite;
		pRePagServer->Lese(&ucServer, BY_BYTE);
		pRePagServer->Lese(&ucSpeicher, BY_BYTE);
		pRePagServer->Lese(&ulSeite, BY_ULONG);
		pRePagServer->NeueSendung();

		switch(ucServer){
			case 0: pstServer = &stAdminServer; bServer_1 = true; break;
			case 1: pstServer = &stAdminServer; bServer_1 = false; break;
			case 2: pstServer = &stLoginServer; bServer_1 = true; break;
			case 3: pstServer = &stLoginServer; bServer_1 = false; break;
			case 4: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = true; break;
			case 5: pstServer = &vstProgServer[SRV_PROGSERVER]; bServer_1 = false; break;
		}

		BYTE ucVersuche = 2; COProtokollClient* vRePagClient;
		do{
			vRePagClient = COProtokollClientV(vmProtokoll, pstServer, NULL);

			vRePagClient->Schreibe(&ucSpeicher, BY_BYTE);
			vRePagClient->Schreibe(&ulSeite, BY_ULONG);

			bool bSendeAufgabe;
			if(bServer_1) bSendeAufgabe = vRePagClient->SendeAufgabe_Server_1(CS_COREMEMORY, CS_VINTERN);
			else bSendeAufgabe = vRePagClient->SendeAufgabe_Server_2(CS_COREMEMORY, CS_VINTERN);

			if(bSendeAufgabe){
				if(vRePagClient->Senden()){
					if(vRePagClient->Empfangen() == BY_BYTE){
						if(!vRePagClient->ucInfo) vRePagClient->Lese(&pRePagServer->ucInfo, BY_BYTE);
					}
					else{
						ULONG ulBytes = vRePagClient->ulBytes;
						VMBLOCK vbTemp = VMBlock(ulBytes);
						vRePagClient->Lese(vbTemp, ulBytes);
						pRePagServer->Schreibe(vbTemp, ulBytes);
						VMFrei(vbTemp);
					}
				}
			}

			if(vRePagClient->ucInfo){
				if(!--ucVersuche){ pRePagServer->ucInfo = 1; char c11Zahl[11]; COStringA asEintrag("GetCoreMemory - Fehler bei Verbindung zu Server: ");
					asEintrag += ULONGtoCHAR(c11Zahl, ucServer); FehlerEintrag(vRePagClient, &stAdminServer, asEintrag.c_Str());
				}
			}
			else ucVersuche = 0;
			VMFreiV(vRePagClient);
		}
		while(ucVersuche);
	}
	pRePagServer->Senden();
}
//-------------------------------------------------------------------------------------------------------------------------------------------