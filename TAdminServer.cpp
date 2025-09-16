/******************************************************************************
MIT License

Copyright(c) 2025 René Pagel

Filename: TAdminServer.cpp
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
#include "TAdminServer.h"
#include "OAdminServer.h"
//---------------------------------------------------------------------------
STTHLogBuch thstLogBuch;
HANDLE heLogBuch;
HANDLE hthLogBuch;
//STTHVerbindung thstVerbindung;
//HANDLE hthVerbindung;
//---------------------------------------------------------------------------
DWORD WINAPI thAdminSitzGultig(void* pvParam)
{
#ifndef HCORE
	void* pvIterator; void* pvLoschen; COList* vlstLogout = COListV(vmServer, false); COTime zDifferenz;
	while(!((STTHSitzGultig*)pvParam)->bEnde){
		pvIterator = vthlstBenutzer->ThIteratorToBegin_Lock(); pvLoschen = nullptr; zDifferenz.Now(); zDifferenz -= SV_SITZUNGGULTIG;
		while(pvIterator){
			if(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->bServer_2 == vAdminServer->bServer_2){
				if(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->ucProgramm == PR_ADMIN_C){
					if(*((STBenutzer*)vthlstBenutzer->Element(pvIterator))->vzLetzteAktion < zDifferenz){
						STLogout* vstLogout = (STLogout*)VMBlock(vmServer, sizeof(STLogout));
						MemCopy(vstLogout->bit128Sitzung, ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->bit128Sitzung, BY_BIT128);
						vstLogout->ulBenutzer = ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->ulNummer;
						vstLogout->ucProgramm = ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->ucProgramm;
						VMFrei(vmServer, ((STBenutzer*)vthlstBenutzer->Element(pvIterator))->stSockAdresse.vbSockaddr);
						VMFreiV(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->vzAnmeldung);
						VMFreiV(((STBenutzer*)vthlstBenutzer->Element(pvIterator))->vzLetzteAktion);
						vlstLogout->ToEnd(vstLogout);
						vthlstBenutzer->DeleteElement(pvIterator, pvLoschen, true);
					}
				}
			}
			vthlstBenutzer->NextElement(pvIterator, pvLoschen);
		}
		vthlstBenutzer->ThIteratorEnd();

		if(vlstLogout->Number()){
			STTHExternSystemAbmeldung* thstExternSystemAbmeldung;
			thstExternSystemAbmeldung = (STTHExternSystemAbmeldung*)VMBlock(sizeof(STTHExternSystemAbmeldung));
			thstExternSystemAbmeldung->ulGrosse = vlstLogout->Number() * 16;
			thstExternSystemAbmeldung->cstLogout = VMBlock(thstExternSystemAbmeldung->ulGrosse);
			thstExternSystemAbmeldung->hThread = CreateThread(NULL, 0, thExternSystemAbmeldung, thstExternSystemAbmeldung, CREATE_SUSPENDED, NULL);
			SetThreadPriority(thstExternSystemAbmeldung->hThread, THREAD_PRIORITY_IDLE);

			pvIterator = vlstLogout->IteratorToBegin(); ULONG ulPosition = 0;
			while(pvIterator){
				if(pBasisServer->BDoppel){
					STTHServer2Abmeldung* thstServer2Abmeldung = (STTHServer2Abmeldung*)VMBlock(sizeof(STTHServer2Abmeldung));
					MemCopy(thstServer2Abmeldung->bit128Sitzung, ((STLogout*)vlstLogout->Element(pvIterator))->bit128Sitzung, BY_BIT128);
					thstServer2Abmeldung->ulBenutzer = ((STLogout*)vlstLogout->Element(pvIterator))->ulBenutzer;
					thstServer2Abmeldung->ucProgramm = ((STLogout*)vlstLogout->Element(pvIterator))->ucProgramm;
					thstServer2Abmeldung->hThread = CreateThread(NULL, 0, thServer2Abmeldung, thstServer2Abmeldung, CREATE_SUSPENDED, NULL);
					SetThreadPriority(thstServer2Abmeldung->hThread, THREAD_PRIORITY_LOWEST);
					ResumeThread(thstServer2Abmeldung->hThread);
				}

				MemCopy(&thstExternSystemAbmeldung->cstLogout[ulPosition], ((STLogout*)vlstLogout->Element(pvIterator))->bit128Sitzung, BY_BIT128);
				ulPosition += BY_BIT128;
				vlstLogout->DeleteFirstElement(pvIterator, true);
			}
			ResumeThread(thstExternSystemAbmeldung->hThread);
		}
		SuspendThread(((STTHSitzGultig*)pvParam)->hThread);
	}
	VMFreiV(vlstLogout);
#endif
	HANDLE hThread = ((STTHSitzGultig*)pvParam)->hThread;
	VMFrei(pvParam);
	CloseHandle(hThread);
	return NULL;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
DWORD WINAPI thLogBuch(void* pvParam)
{
  COStringA asProtoEintrage;
	void* pvIterator;
  while(!((STTHLogBuch*)pvParam)->bEnde){
    pvIterator = vthlstProtokollEintrag->ThIteratorToBegin_Lock();
    while(pvIterator){
      ((STProtokollEintrag*)vthlstProtokollEintrag->Element(pvIterator))->zZeit.StrDateTimeFormat(&asProtoEintrage, "dd'.'MM'.'yyyy' '", "HH':'mm':'ss", true);
      asProtoEintrage += " ";
			asProtoEintrage += ((STProtokollEintrag*)vthlstProtokollEintrag->Element(pvIterator))->asProgramm;
      asProtoEintrage += " \"";
      asProtoEintrage += ((STProtokollEintrag*)vthlstProtokollEintrag->Element(pvIterator))->asEintrag;
      asProtoEintrage += "\"\n";
      vthlstProtokollEintrag->DeleteFirstElement(pvIterator, true);

			HANDLE hDatei = NULL; ULONG ulPointer; ULONG ulKontrollBytes; int iErgebnis; char cWiederholung = 4;
			COStringA* vasDatum = COStringAV(); COTime* vzDateiDatum = COTimeV();
      vzDateiDatum->Now()->StrDateFormat(vasDatum, "yyMMdd");
      COStringA* vasDateiName = COStringAV("C:\\Development\\Daten\\Log\\"); *vasDateiName += *vasDatum + ".log";

      do{
        try{hDatei = CreateFile(vasDateiName->c_Str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_WRITE_THROUGH, NULL);
          if(hDatei == INVALID_HANDLE_VALUE){ throw 0;}
          ulPointer = SetFilePointer(hDatei, NULL, NULL, FILE_END);
          if(ulPointer == 0xFFFFFFFF){ throw 0;}
          iErgebnis = WriteFile(hDatei, asProtoEintrage.c_Str(), asProtoEintrage.Length(), &ulKontrollBytes, NULL);
          if(!iErgebnis){ throw 0;}
          CloseHandle(hDatei);
          cWiederholung = 0;
        }
        catch(...){ CloseHandle(hDatei); if(!cWiederholung--) throw 0; Sleep(1000);}
      }
      while(cWiederholung);
			VMFreiV(vasDatum); VMFreiV(vzDateiDatum); VMFreiV(vasDateiName);
    }
    vthlstProtokollEintrag->ThIteratorEnd();
    WaitForSingleObject(heLogBuch, INFINITE);
  }
	return NULL;
}
//---------------------------------------------------------------------------
/*
DWORD WINAPI thVerbindung(void* pvParam)
{
 bool bgetrennt = false;
 SOCKET csSocket = NULL; int iTimeOut = 5000, iKeepAlive = 128, iBuffer = 1024; linger stlinger; stlinger.l_onoff = 1; stlinger.l_linger = 3;
 sockaddr_in adrService;
 adrService.sin_family = AF_INET;
 adrService.sin_port = htons(80);
 adrService.sin_addr.S_un.S_un_b.s_b1 = 188;
 adrService.sin_addr.S_un.S_un_b.s_b2 = 246;
 adrService.sin_addr.S_un.S_un_b.s_b3 = 17;
 adrService.sin_addr.S_un.S_un_b.s_b4 = 14;
 
 while(!((STTHVerbindung*)pvParam)->bEnde){
	 //csSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);		 
	 //setsockopt(csSocket, SOL_SOCKET, SO_SNDBUF, (char*)&iBuffer, BY_LONG);
	 //setsockopt(csSocket, SOL_SOCKET, SO_RCVBUF, (char*)&iBuffer, BY_LONG);
	 //setsockopt(csSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeOut, BY_LONG);
	 //setsockopt(csSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&iTimeOut, BY_LONG);
	 //setsockopt(csSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, BY_LONG);
	 //setsockopt(csSocket, SOL_SOCKET, SO_LINGER, (char*)&stlinger, BY_LONG);
 
	 //if(connect(csSocket, (SOCKADDR*)&adrService, sizeof(adrService)) == SOCKET_ERROR){
		// if(!bgetrennt){
		//	 STTHEintragServer* thstEintrag = (STTHEintragServer*)VMBlock(BY_STTHEINTRAGSERVER);
		//	 thstEintrag->pvBasisServer = vAdminServer;
		//	 thstEintrag->vzZeit = COTimeV(); thstEintrag->vzZeit->Now();
		//	 thstEintrag->vasProgramm = COStringAV(vAdminServer->vasName);
		//	 thstEintrag->vasEintrag = COStringAV("Verbindung unterbrochen");
		//	 thstEintrag->hThread = CreateThread(NULL, 0, thEintragServer, thstEintrag, CREATE_SUSPENDED, NULL);
		//	 SetThreadPriority(thstEintrag->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		//	 ResumeThread(thstEintrag->hThread);
		//	 bgetrennt = true;
		// }
		// WSASetLastError(0);
	 //}
	 //else if(bgetrennt){
		// STTHEintragServer* thstEintrag = (STTHEintragServer*)VMBlock(BY_STTHEINTRAGSERVER);
		// thstEintrag->pvBasisServer = vAdminServer;
		// thstEintrag->vzZeit = COTimeV(); thstEintrag->vzZeit->Now();
		// thstEintrag->vasProgramm = COStringAV(vAdminServer->vasName);
		// thstEintrag->vasEintrag = COStringAV("Verbindung wieder hergestellt");
		// thstEintrag->hThread = CreateThread(NULL, 0, thEintragServer, thstEintrag, CREATE_SUSPENDED, NULL);
		// SetThreadPriority(thstEintrag->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		// ResumeThread(thstEintrag->hThread);
		// bgetrennt = false;
	 //}

  // switch(closesocket(csSocket)){
  //     case 0              : break;
  //     case WSAEWOULDBLOCK : closesocket(csSocket);
  // } 
	 SuspendThread(hthVerbindung);
 }
 return NULL;
}*/
//---------------------------------------------------------------------------