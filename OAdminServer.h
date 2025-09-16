/******************************************************************************
MIT License

Copyright(c) 2025 René Pagel

Filename: OAdminServer.h
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
#pragma once
//---------------------------------------------------------------------------
typedef struct STProtokollEintrag{
 COTime zZeit;
 COStringA asProgramm;
 COStringA asEintrag;
} STProtokollEintrag;
//#ifndef _64bit
//#define BY_STPROTOKOLLEINTRAG 12
//#else
//#define BY_STPROTOKOLLEINTRAG 24
//#endif

//---------------------------------------------------------------------------
class COAdminServer : public COBasisServer
{
	friend void __vectorcall frTimerStart(void);
	friend void CALLBACK Timer_DBAusgleichen(void* pvParam, bool bTimerOrWaitFired);
	friend void __vectorcall frAufgabeExtern(COProtokollServer* pRePagServer, USHORT& usAufgabe, USHORT& usVersion);
	friend void __vectorcall frAufgabeIntern(COProtokollServer* pRePagServer, USHORT& usAufgabe);

private:
	HANDLE htSitzungGultig;
	//HANDLE htVerbindung;
	void __vectorcall ProtoDatei(COProtokollServer* pRePagServer);
	void __vectorcall EintragProtokoll(COProtokollServer* pRePagServer);
	void __vectorcall Update(COProtokollServer* pRePagServer);
	void __vectorcall ProgVersion(COProtokollServer* pRePagServer);
	void __vectorcall NeueSitzung(COProtokollServer* pRePagServer);
	void __vectorcall SchlusselDateiNeu(COProtokollServer* pRePagServer);
	void __vectorcall ProtokollAbmeldung(COProtokollServer* pRePagServer);
	void __vectorcall ProtokollAnmeldung(COProtokollServer* pRePagServer);
	void __vectorcall HoleSitzungen(COProtokollServer* pRePagServer);
	void __vectorcall HoleVirtualMemory(COProtokollServer* pRePagServer);
	void __vectorcall DBTabelleLesen(COProtokollServer* pRePagServer);
	void __vectorcall DBDatensatzAnderung(COProtokollServer* pRePagServer, USHORT& usAufgabe);
	void __vectorcall DBTabelleAusgleichen(COProtokollServer* pRePagServer);
	void __vectorcall DBStartOptimierung(COProtokollServer* pRePagServer);
	void __vectorcall DBDateiEncryptInital(COProtokollServer* pRePagServer);
	void __vectorcall GetCoreConnect(COProtokollServer* pRePagServer);
	void __vectorcall GetCoreMemory(COProtokollServer* pRePagServer);

protected: 

public:
	bool __vectorcall COAdminServerV(_In_ bool bInit, _In_ bool bDoppelA, _In_ bool bServer_2A, _In_z_ char* c6AusgleichenA, _In_z_ char* c6OptimierenA);
	VMEMORY __vectorcall COFreiV(void);

};
//---------------------------------------------------------------------------
COAdminServer* __vectorcall COAdminServerV(_In_ bool bInit, _In_ bool bDoppel, _In_ bool bServer_2, _In_z_ char* c6Ausgleichen, _In_z_ char* c6Optimieren);
extern COAdminServer* vAdminServer;
extern COList* vthlstProtokollEintrag;
//extern STTHAdminSitzGutlig* thstAdminSitzGutlig;
//---------------------------------------------------------------------------