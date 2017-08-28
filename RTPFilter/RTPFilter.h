#include <windows.h>
#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#pragma warning(disable:4996)
#pragma comment (lib, "jrtplib.lib")
#pragma comment (lib, "jthread.lib")
#pragma comment (lib, "ws2_32.lib")
using namespace jrtplib;
#include "h264.h"

class RTPFilter :public CTransInPlaceFilter
{
private:
	//jrtpʵ��
	RTPSession sess;
	uint16_t portbase, destport;
	uint32_t destip;
	int status, i, num;
	unsigned int timestamp_increse = 0, ts_current = 0;
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	WSADATA dat;
public:
	RTPFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~RTPFilter();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
	// check if you can support mtIn
	virtual HRESULT CheckInputType(const CMediaType* mtIn); // PURE
	//������д�ĺ��ĺ���
	virtual HRESULT Transform(IMediaSample *pSample); // PURE
	// Delegating methods
	virtual HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin);
	//HRESULT Receive(IMediaSample *pSample);//�ɴ���dorender��transformʵ�ַ���
	//virtual HRESULT StartStreaming();
	//virtual HRESULT StopStreaming();

	//jrtp���Ͳ���
	void checkerror(int rtperr);
	bool Initjrtp(uint16_t m_portbase, std::string m_destip, uint16_t m_destport);
	bool RTPSender(IMediaSample *pMediaSample);
	bool Cleanjrtp();
public:
	NALU_t *n;//NALU�ṹ��ָ��
	unsigned char sendbuf[1500];
	unsigned char pbcopy[614400];
	unsigned char* nalu_payload;
};

