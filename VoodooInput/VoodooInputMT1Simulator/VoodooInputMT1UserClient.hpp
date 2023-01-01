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
#include "./VoodooInputMT1Simulator.hpp"

#include "../VoodooInput.hpp"
#include "../VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInputMultitouch/VoodooInputEvent.h"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

enum VoodooInputMT1UserClientMethods {
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

class EXPORT VoodooInputMT1UserClient : public IOUserClient {
    OSDeclareDefaultStructors(VoodooInputMT1UserClient);

public:
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    virtual IOReturn registerNotificationPort(mach_port_t port, UInt32 type, UInt32 refCon) override;
    virtual IOReturn clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) override;
    
    virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **targetP, UInt32 index) override;
    IOReturn sSetSendFrames(IOService *svc, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn sGetReport(IOService *svc, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
    IOReturn sNoop(IOService *svc, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6);
private:
    VoodooInputMT1Simulator *simulator {nullptr};
    
    IOSharedDataQueue *dataQueue {nullptr};
    IOSharedDataQueue *logQueue {nullptr};
    IOMemoryDescriptor *dataQueueDesc {nullptr};
    IOMemoryDescriptor *logQueueDesc {nullptr};
    
    static IOExternalMethodACID sMethods[VoodooInputMT1UserClientMethodsNumMethods];
};

#endif /* VoodooInputMT1UserClient_hpp */
