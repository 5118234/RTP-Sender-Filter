// RTPFilter.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#include "stdafx.h"
#include <streams.h>
#include <windows.h>
#include <initguid.h>
#include <olectl.h>
#include "RTPFilteruid.h"
#include "RTPFilter.h"
#if (1100 > _MSC_VER)
#include <olectlid.h>
#endif
#include <iostream>

//jrtplib
//RTPSession sess;
//uint16_t portbase, destport;
//uint32_t destip;
//int status, i, num;
//unsigned int timestamp_increse = 0, ts_current = 0;
//RTPUDPv4TransmissionParams transparams;
//RTPSessionParams sessparams;
//WSADATA dat;


// setup data
//
//ע��ʱ�����Ϣ
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_NULL,            // Major type
	&MEDIASUBTYPE_NULL          // Minor type
};
//ע��ʱ�����Ϣ
const AMOVIESETUP_PIN psudPins[] =
{
	{
		L"Input",           // String pin name
		FALSE,              // Is it rendered
		FALSE,              // Is it an output
		FALSE,              // Allowed none
		FALSE,              // Allowed many
		&CLSID_NULL,        // Connects to filter
		L"Output",          // Connects to pin
		1,                  // Number of types
		&sudPinTypes }
};

//ע��ʱ�����Ϣ
const AMOVIESETUP_FILTER sudFilter =
{
	&CLSID_RTPFilter,       // Filter CLSID
	L"RTP Renderer Filter",    // Filter name
	MERIT_DO_NOT_USE,        // Its merit
	1,                       // Number of pins
	psudPins                 // Pin details
};

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
//ע��g_Templates�����ǹ̶���
CFactoryTemplate g_Templates[1] =
{
	{
		L"RTP Renderer Filter",
		&CLSID_RTPFilter,
		RTPFilter::CreateInstance,
		NULL,
		&sudFilter
	}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// ----------------------------------------------------------------------------
//            Filter implementation
// ----------------------------------------------------------------------------
RTPFilter::RTPFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransInPlaceFilter(tszName, punk, CLSID_RTPFilter, phr)
{
	//��ʼ��JRTPLIB������˿ں�Ŀ��IP
	Initjrtp(8000, "172.20.111.200", 9000);
	//�����˿ڡ������ն�IP�������ն˶˿�
}
	

RTPFilter::~RTPFilter()
{
	Cleanjrtp();
}

CUnknown * WINAPI RTPFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	RTPFilter *pNewObject = new RTPFilter(NAME("RTP Renderer Filter"), punk, phr);
	if (pNewObject == NULL)
	{
		*phr = E_OUTOFMEMORY;
	}
	return pNewObject;
}
//*****************************DoRenderSample��������RTP*****************************
//DoRenderSample��������RTP
//*********************************��Ҫ��ʵ��*********************************
HRESULT RTPFilter::Transform(IMediaSample *pSample)
{
	RTPSender(pSample);//���÷�������������ʵ���ں�
	return NOERROR;
}
// Basic COM - used here to reveal our own interfaces
//��¶�ӿڣ�ʹ�ⲿ�������QueryInterface���ò��ֿ�����Ӹ���IP�Ľӿ�
STDMETHODIMP RTPFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv, E_POINTER);
	return CTransInPlaceFilter::NonDelegatingQueryInterface(riid, ppv);
} // NonDelegatingQueryInterface

HRESULT RTPFilter::CheckInputType(const CMediaType* mtIn)
{
	// Dynamic format change will never be allowed!
	if (IsStopped() && *mtIn->Type() == MEDIATYPE_Video)
	{
		if (*mtIn->Subtype() == MEDIASUBTYPE_H264)
		{
			return NOERROR;
		}
	}
	return E_INVALIDARG;
}
HRESULT RTPFilter::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
	HRESULT hr = CTransInPlaceFilter::CompleteConnect(direction, pReceivePin);
	return hr;
}

//*********************filter�������������****************************
//�������������jrtplib�ĳ�ʼ��������
//�Ǳ���ʵ�ֺ���
//*********************************************************************
//HRESULT RTPFilter::StartStreaming()
//{
//	BOOL pass = mOverlayController->StartTitleOverlay();
//	return pass ? S_OK : E_FAIL;
//}
//
//HRESULT RTPFilter::StopStreaming()
//{
//	mOverlayController->StopTitleOverlay();
//	return NOERROR;
//}

//******************************JRTP******************************
//JRTP��������
//******************************JRTP******************************
void RTPFilter::checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}
//******************************��ʼ��******************************
bool RTPFilter::Initjrtp(uint16_t m_portbase, std::string m_destip, uint16_t m_destport)
{
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	std::string ipstr = m_destip;
	std::cout << "Using version " << RTPLibraryVersion::GetVersion().GetVersionString() << std::endl;
	portbase = m_portbase;
	destip = inet_addr(ipstr.c_str());
	destport = m_destport;
	if (destip == INADDR_NONE)
	{
		std::cerr << "Bad IP address specified" << std::endl;
		return FALSE;
	}
	// The inet_addr function returns a value in network byte order, but
	// we need the IP address in host byte order, so we use a call to
	// ntohl
	destip = ntohl(destip);
	std::cout << portbase << " " << destport << " " << ipstr << std::endl;//����̨��ӡIP��Ϣ
	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)
	sessparams.SetOwnTimestampUnit(1.0 / 90000.0);//����Ƶ��
	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(portbase);
	status = sess.Create(sessparams, &transparams);
	checkerror(status);
	RTPIPv4Address addr(destip, destport);
	status = sess.AddDestination(addr);
	sess.SetDefaultPayloadType(96);//װ������Ϊ96ʱ��Ϊ����Ƶ������ݣ������Э��
	sess.SetDefaultMark(false);//����session��ȱʡ���ã�markΪfalse
	sess.SetDefaultTimestampIncrement(90000.0 / 25.0); // ʱ��������ٶȣ�һ��25֡����һ��NALU���ͺ�ʱ����������ȣ�
	checkerror(status);
	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)

}
//******************************����ͷ�******************************
//JRTP�ͷź�����ͬʱ���NALU�ṹ����ͷ�
bool RTPFilter::Cleanjrtp()
{
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK
	return TRUE;
}
//******************************��������******************************
bool RTPFilter::RTPSender(IMediaSample *pMediaSample)
{
	long size = pMediaSample->GetActualDataLength();
	BYTE *pb=NULL;
	if (FAILED(pMediaSample->GetPointer(&pb)))
	{
		return 0;
	}
	//������д���
	//FILE *fp;
	//fp = fopen("G://all.txt","w");
	//fwrite(pb, size, 1, fp);
	//fclose(fp)*/;
	//
	int cout = 0;//����ʼ���ֳ�
	unsigned char a;//����NALUͷ
	a = *pb;
	//startcodeΪ 0x00000001(��λ)��0x000001(��λ)
	//x264 codec���������Ϊ��λstartcode
	while (a == 0)//�ж�0x00��char��Ĭ��0x00Ϊnull
	{
		pb++;
		a=*pb;
		cout++;
	}
	if (a == 1)//�ж�0x01
	{
		cout++;
		pb++;
	}
	else
	{
		return 0;
	}
	a = *pb;//ȡ��header 8λ/1�ֽڣ�TYPE����NRI����F��
	pb++;//ȥ������ʼλ4�ֽ�+ͷ1�ֽ�,*pbָ��NALU���ݲ���
	int startcode_len = cout;
	long len = size;//NALU��С������ ��ʼλ ͷ��
	long actual_len = size - cout-1;//ȥ����ʼλ4λ size
	NALU_HEADER nh;//NALUͷ �ṹ��
	//NALUͷ��ʽ��
	//�ߡ�����λ
	//F-NRI-TYPE
	//1--2----5 bit
	nh.TYPE = a;//�ṹ��ض��ַ�a�ĵ�5λ
	nh.NRI = a >> 5;//a����5λ��ȡ��6��7λ
	nh.F = a >> 2;//a����1λ��ȡ�õ�8λ
	//*************RTP����********************
	//��NALU��ԪС��AX_RTP_PKT_LENGTH��1360�ֽڣ�ʱ��ʹ��һ��RTP����һ֡
	//�����򲻽��ж�֡��������
	//ÿһ��NALU������ɺ��ڲ�ֵ�����RTP������Ҫ �� markλΪtrue�����ն˸���RTP���Ƿ���markʶ���Ƿ�������
	//����Ƶ��Ϊ25֡=3600ms����session��Ƶ������ͳһ
	//RTP�ְ������ӳ٣��������һ�������������ӳ�����
	//*************RTP����********************

	if (actual_len <= MAX_RTP_PKT_LENGTH)
	{
		memset(sendbuf, 0, 1500);
		//����NALU HEADER,�������HEADER����sendbuf[12]
		nalu_hdr = (NALU_HEADER*)&sendbuf[0]; //��sendbuf[12]�ĵ�ַ����nalu_hdr��֮���nalu_hdr��д��ͽ�д��sendbuf�У�
		nalu_hdr->F = nh.F;
		nalu_hdr->NRI = nh.NRI;//��Ч������n->nal_reference_idc�ĵ�6��7λ����Ҫ����5λ���ܽ���ֵ����nalu_hdr->NRI��
		nalu_hdr->TYPE = nh.TYPE;
		nalu_payload = &sendbuf[1];//ͬ��sendbuf[13]����nalu_payload
		memcpy(nalu_payload, pb, actual_len);//ȥ��naluͷ��naluʣ������д��sendbuf[13]��ʼ���ַ�����
		status = sess.SendPacket((void *)sendbuf, actual_len, 96, true, 3600);
		//����RTP��ʽ���ݰ���ָ����������Ϊ96
		if (status < 0)
		{
			std::cerr << RTPGetErrorString(status) << std::endl;
			exit(-1);
		}
	}
	else if (actual_len>MAX_RTP_PKT_LENGTH)
	{
		//�õ���nalu��Ҫ�ö��ٳ���ΪMAX_RTP_PKT_LENGTH�ֽڵ�RTP��������
		int k = 0, l = 0;
		k = actual_len / MAX_RTP_PKT_LENGTH;//��Ҫk��MAX_RTP_PKT_LENGTH�ֽڵ�RTP��
		l = actual_len%MAX_RTP_PKT_LENGTH;//���һ��RTP������Ҫװ�ص��ֽ���
		int t = 0;//����ָʾ��ǰ���͵��ǵڼ�����ƬRTP��

		//*************RTP���ѭ��********************
		//�˲�����Ҫʵ�ֶ�NALU��ֹ���������RTP������С������MAX_RTP_PKT_LENGTH��1360�ֽڣ�
		//ÿһ��NALU������ɺ��ڲ�ֵ�����RTP������Ҫ �� markλΪtrue�����ն˸���RTP���Ƿ���markʶ���Ƿ�������
		//����Ƶ��Ϊ25֡=3600ms����session��Ƶ������ͳһ
		//RTP�ְ������ӳ٣��������һ�������������ӳ�����
		//�ӳ����ý�Ӱ����ն˵Ľ������ʺͲ���
		//********************************************

		while (t <= k)
		{
			if (!t)//����һ����Ҫ��Ƭ��NALU�ĵ�һ����Ƭ����FU HEADER��Sλ
			{
				//printf("dddd1");
				memset(sendbuf, 0, 1500);
				//session.SetDefaultMark(false);
				//����FU INDICATOR,�������HEADER����sendbuf[12]
				fu_ind = (FU_INDICATOR*)&sendbuf[0]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
				fu_ind->F = nh.F;
				fu_ind->NRI = nh.NRI;
				fu_ind->TYPE = 28;
				//����FU HEADER,�������HEADER����sendbuf[13]
				fu_hdr = (FU_HEADER*)&sendbuf[1];
				fu_hdr->E = 0;
				fu_hdr->R = 0;
				fu_hdr->S = 1;
				fu_hdr->TYPE = nh.TYPE;				
				nalu_payload = &sendbuf[2];//ͬ��sendbuf[2]����nalu_payload
				memcpy(nalu_payload, pb, MAX_RTP_PKT_LENGTH);//ȥ��NALUͷ
				status = sess.SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH + 2, 96, false, 0);
				//������д���
				//FILE *fp;
				//fp = fopen("G://get.txt", "a+");
				//fwrite(sendbuf+2, MAX_RTP_PKT_LENGTH, 1, fp);
				//fwrite((void*)"\n", 1, 1, fp);
				//fclose(fp);
				//std::cout << sendbuf << std::endl;
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}
				t++;
			}
			//����һ����Ҫ��Ƭ��NALU�ķǵ�һ����Ƭ������FU HEADER��Sλ������÷�Ƭ�Ǹ�NALU�����һ����Ƭ����FU HEADER��Eλ
			else if (k == t&&l!=0)//���͵������һ����Ƭ��ע�����һ����Ƭ�ĳ��ȿ��ܳ���MAX_RTP_PKT_LENGTH�ֽڣ���l>1386ʱ����
			{
				//printf("dddd3\n");
				memset(sendbuf, 0, 1500);
				//session.SetDefaultMark(true);
				//����FU INDICATOR,�������HEADER����sendbuf[0]
				fu_ind = (FU_INDICATOR*)&sendbuf[0]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
				fu_ind->F = nh.F;
				fu_ind->NRI = nh.NRI;
				fu_ind->TYPE = 28;
				//����FU HEADER,�������HEADER����sendbuf[1]
				fu_hdr = (FU_HEADER*)&sendbuf[1];
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->TYPE = nh.TYPE;
				fu_hdr->E = 1;
				nalu_payload = &sendbuf[2];//ͬ��sendbuf[2]��ַ����nalu_payload
				memcpy(nalu_payload, pb + t*MAX_RTP_PKT_LENGTH , l);//��nalu���ʣ���l-1(ȥ����һ���ֽڵ�NALUͷ)�ֽ�����д��sendbuf[14]��ʼ���ַ�����
				status = sess.SendPacket((void *)sendbuf, l + 2, 96, true, 3600);
				//������д���
				//FILE *fp;
				//fp = fopen("G://get.txt", "a+");
				//fwrite(sendbuf+2, MAX_RTP_PKT_LENGTH, 1, fp);
				//fwrite((void*)"\n", 1, 1, fp);
				//fclose(fp);
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}
				t++;
				//	Sleep(100);
			}
			else if (t<k && 0 != t)
			{
				memset(sendbuf, 0, 1500);
				//����FU INDICATOR,�������HEADER����sendbuf[0]
				fu_ind = (FU_INDICATOR*)&sendbuf[0]; //��sendbuf[0]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
				fu_ind->F = nh.F;
				fu_ind->NRI = nh.NRI;
				fu_ind->TYPE = 28;
				//����FU HEADER,�������HEADER����sendbuf[1]
				fu_hdr = (FU_HEADER*)&sendbuf[1];
				//fu_hdr->E=0;
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->E = 0;
				fu_hdr->TYPE = nh.TYPE;
				nalu_payload = &sendbuf[2];//ͬ��sendbuf[2]�ĵ�ַ����nalu_payload
				memcpy(nalu_payload, pb + t*MAX_RTP_PKT_LENGTH , MAX_RTP_PKT_LENGTH);//ȥ����ʼǰ׺��naluʣ������д��sendbuf[14]��ʼ���ַ�����
				status = sess.SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH + 2, 96, false, 0);
				//������д���
				//FILE *fp;
				//fp = fopen("G://get.txt", "a+");
				//fwrite(sendbuf+2, MAX_RTP_PKT_LENGTH, 1, fp);
				//fwrite((void*)"\n", 1, 1, fp);
				//fclose(fp);
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}
				t++;
			}
			else if (k == t&&l == 0)
			{
				t++;
			}
		}
	}	
	RTPTime delay(0.0015); //�����ӳ٣����ڵ����������ʺ�֡���ʵĲ��죩�Ǳ��룬����������������
	RTPTime::Wait(delay);
}

/******************************Public Routine******************************\
* exported entry points for registration and
* unregistration (in this case they only call
* through to default implmentations).
*
*
*
* History:
*
\**************************************************************************/
//****************************DLLע�����***********************************
//DllEntryPoint
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD dwReason,
	LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//extern "C" _declspec(dllexport)BOOL DLLRegisterServer;

