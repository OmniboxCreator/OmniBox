
#ifndef J2534_H
#define J2534_H

#include "../shared/j2534_abi.h"

#ifdef _WIN32
 #define J2534_API __declspec(dllexport)
 #define WINAPI __stdcall
#else
 #define J2534_API
 #define WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned long ProtocolVersion;
  unsigned long CapabilityFlags;
  unsigned long CanChannels;
  unsigned long KlineChannels;
} OMNIBOX_CAPS;

J2534_API long WINAPI PassThruOpen(void *pName, unsigned long *pDeviceID);
J2534_API long WINAPI PassThruClose(unsigned long DeviceID);
J2534_API long WINAPI PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID,
                   unsigned long Flags, unsigned long BaudRate, unsigned long *pChannelID);
J2534_API long WINAPI PassThruDisconnect(unsigned long ChannelID);
J2534_API long WINAPI PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
                    unsigned long *pNumMsgs, unsigned long Timeout);
J2534_API long WINAPI PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
                    unsigned long *pNumMsgs, unsigned long Timeout);
J2534_API long WINAPI PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
                        unsigned long *pMsgID, unsigned long TimeInterval);
J2534_API long WINAPI PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
J2534_API long WINAPI PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType,
                       PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg,
                       PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID);
J2534_API long WINAPI PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID);
J2534_API long WINAPI PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long PinNumber,
                          unsigned long Voltage);
J2534_API long WINAPI PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion,
                     char *pDllVersion, char *pApiVersion);
J2534_API long WINAPI PassThruGetLastError(char *pErrorDescription);
J2534_API long WINAPI PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID,
                  void *pInput, void *pOutput);
J2534_API long WINAPI OmniBoxGetCaps(OMNIBOX_CAPS *pCaps);
J2534_API long WINAPI OmniBoxSetChannelConfig(unsigned long ChannelID,
                       unsigned long Parameter, unsigned long Value);
J2534_API long WINAPI OmniBoxGetChannelConfig(unsigned long ChannelID,
                       unsigned long Parameter, unsigned long *pValue);

#ifdef __cplusplus
}
#endif

#endif 
