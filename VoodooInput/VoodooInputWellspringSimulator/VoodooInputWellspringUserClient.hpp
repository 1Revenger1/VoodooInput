//
//  VoodooInputMT1UserClient.hpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#ifndef VoodooInputMT1UserClient_hpp
#define VoodooInputMT1UserClient_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOSharedDataQueue.h>
#include <IOKit/IOUserClient.h>

#include "../VoodooInput.hpp"
#include "../VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInputMultitouch/VoodooInputEvent.h"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

enum VoodooInputWellspringUserClientMethods {
    VoodooInputMT1UserClientMethodsSetSendsFrames,
    VoodooInputMT1UserClientMethodsGetReport,
    VoodooInputMT1UserClientMethodsSetReport,
    VoodooInputMT1UserClientMethodsSetSendsLogs,
    VoodooInputMT1UserClientMethodsIssueDriverRequest,
    VoodooInputMT1UserClientMethodsPostRelativeMouseMovement,
    VoodooInputMT1UserClientMethodsPostScrollWheelEvent,
    VoodooInputMT1UserClientMethodsPostKeyboardEvent,
    VoodooInputMT1UserClientMethodsSetMapClicks,
    VoodooInputMT1UserClientMethodsRecacheProperties,
    VoodooInputMT1UserClientMethodsMomentumScroll,
    VoodooInputMT1UserClientMethodsNumMethods
};

static_assert(VoodooInputMT1UserClientMethodsNumMethods == 11, "Invalid number of Userclient methods");

struct MTDeviceReportStruct;
class VoodooInputWellspringSimulator;

class EXPORT VoodooInputWellspringUserClient : public IOUserClient {
    OSDeclareDefaultStructors(VoodooInputWellspringUserClient);

public:
    virtual bool start(IOService *provider) override;
    virtual void free() override;
    
    virtual IOReturn registerNotificationPort(mach_port_t port, UInt32 type, UInt32 refCon) override;
    virtual IOReturn clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) override;
    
    virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **targetP, UInt32 index) override;


    // Normal IOMethod form for member functions, must be defined exactly as an IOMethod function pointer
    // is in IOUserClient.h: 
    // typedef IOReturn (IOService::*IOMethod)(void * p1, void * p2, void * p3,
    // void * p4, void * p5, void * p6 );
    //
    // I think the original IOKitUser idea
    // is to standardize the calling convention for functions in the IOExternalMethod table
    // so IOKit can call them with a consistent syntax when an IOUserClient selector matches 
    // the function corresponding to an index in the table
    IOReturn mSetSendFrames(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn mGetReport(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn mNoop(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn mPostRelativeMouse(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn mPostScrollWheel(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn mPostKeyboard(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn mMomentumScroll(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);

    // the original static functions that can be called in C function pointer form emulating
    // the C++ ABI:
    // typedef IOReturn (*IOMethodACID32)(IOService * svc, void * p1, void * p2, void * p3,
    // void * p4, void * p5, void * p6 );
    // Type used to be IOMethodACID, renamed in the MacKernelSDK commit 
    static IOReturn sSetSendFrames(VoodooInputWellspringUserClient *that, bool enableFrames);
    static IOReturn sGetReport(VoodooInputWellspringUserClient *that, MTDeviceReportStruct *input, MTDeviceReportStruct *output);
    static IOReturn sNoop(VoodooInputWellspringUserClient *that, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    
    static IOReturn sPostRelativeMouse(VoodooInputWellspringUserClient *that, SInt32 dx, SInt32 dy, UInt32 buttonState);
    static IOReturn sPostScrollWheel(VoodooInputWellspringUserClient *that, SInt32 dlt1, SInt32 dlt2, SInt32 dlt3);
    static IOReturn sPostKeyboard(VoodooInputWellspringUserClient *that, UInt32 usagePage, UInt32 usage);
    static IOReturn sMomentumScroll(VoodooInputWellspringUserClient *that, SInt32 dlt1, SInt32 dlt2, SInt32 dlt3);
    
    void enqueueData(void *data, size_t size);
private:
    VoodooInputWellspringSimulator *simulator {nullptr};
    
    IOSharedDataQueue *dataQueue {nullptr};
    IOSharedDataQueue *logQueue {nullptr};
    IOMemoryDescriptor *dataQueueDesc {nullptr};
    IOMemoryDescriptor *logQueueDesc {nullptr};
    
    static IOExternalMethod sMethods[VoodooInputMT1UserClientMethodsNumMethods];
};

#endif /* VoodooInputMT1UserClient_hpp */
