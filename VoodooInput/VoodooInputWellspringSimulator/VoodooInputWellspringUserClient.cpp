//
//  VoodooInputMT1UserClient.cpp
//  VoodooInput
//
//  Created by Avery Black on 12/31/22.
//  Copyright © 2022 Kishor Prins. All rights reserved.
//

#include "VoodooInputWellspringUserClient.hpp"
#include "VoodooInputWellspringSimulator.hpp"

#define super IOUserClient
OSDefineMetaClassAndStructors(VoodooInputWellspringUserClient, IOUserClient);

// https://github.com/acidanthera/MacKernelSDK/commit/e57a05e419adafe3c6efdf323b44f1c410b6aa48
// https://github.com/acidanthera/MacKernelSDK#extensions-and-modifications
// brief explanation:: No more IOExternalMethodACID custom struct in newer MacKernelSDK revisions, so not
// necessary to later cast back to IOExternalMethod in getTargetAndMethodForIndex().
// Still use different initializers per arch, but now also need different function pointer 
// forms per arch. 
// 
// longer explanation: There are now two different callable types for the function pointers in IOExternalMethod::func
// IOMethodACID32, a purely C function pointer that seemingly reconstructs the C++ ABI with an explicit "this"
// parameter, and the normal IOMethod which is the normal C++ pointer to member function.
// In C++, as it turns out, the 'static' keyword (in userclient header) indicates omitting the implicit "this" pointer
// and essentially reduces the pointer to a normal C function pointer, and then padding is used 
// Presumably this is the fix to the clang i386 ABI that the mackernelsdk devs devised to match old gcc,
// by manually constructing the member to function pointer...
// This concept for the fix is unchanged between the two revisions. I'm guessing the idea is that the C function pointer
// is stable but the C++ ABI is not.

// Now they've moved this fix to the real IOExternalMethod struct, by falling through to the normal member 
// function pointers when not on i386 + clang, but they still use the IOMethodACID32 on i386, and the
// struct is two different layouts depending on which arch you're on.
// Since the real layout is used on x86_64, static functions can't be used anymore since 
// you can't cast between the two pointer types, where before you *could* cast the entire IOExternalMethodACID struct.
// so instead, the below selects between the two functions based on arch to construct the correct IOExternalMethod entry
// that IOUserClient.h expects.

// Flipped the logic to condition first on i386 AND clang so theoretically i386 on gcc from old Mac OS X
// SDKs should compile ...but who's doing that?
#if defined(__i386__) && defined(__clang__)
#define MTExternalMethod(acidMethod, normalMethod, flags, inputs, outputs) \
    {0, kIOExternalMethodACID32Padding, reinterpret_cast<IOMethodACID32>(acidMethod), flags, inputs, outputs}
#else
#define MTExternalMethod(acidMethod, normalMethod, flags, inputs, outputs) \
    {0, reinterpret_cast<IOMethod>(normalMethod), flags, inputs, outputs}
#endif

IOExternalMethod VoodooInputWellspringUserClient::sMethods[VoodooInputMT1UserClientMethodsNumMethods] = {
    // VoodooInputMT1UserClientMethodsSetSendsFrames
    MTExternalMethod(&VoodooInputWellspringUserClient::sSetSendFrames, &VoodooInputWellspringUserClient::mSetSendFrames, kIOUCScalarIScalarO, 1, 0),
    // VoodooInputMT1UserClientMethodsGetReport
    MTExternalMethod(&VoodooInputWellspringUserClient::sGetReport, &VoodooInputWellspringUserClient::mGetReport, kIOUCStructIStructO, sizeof(MTDeviceReportStruct), sizeof(MTDeviceReportStruct)),
    // VoodooInputMT1UserClientMethodsSetReport
    MTExternalMethod(&VoodooInputWellspringUserClient::sNoop, &VoodooInputWellspringUserClient::mNoop, kIOUCStructIStructO, sizeof(MTDeviceReportStruct), sizeof(MTDeviceReportStruct)),
    // VoodooInputMT1UserClientMethodsSetSendLogs
    MTExternalMethod(&VoodooInputWellspringUserClient::sNoop, &VoodooInputWellspringUserClient::mNoop, kIOUCScalarIScalarO, 1, 0),
    // VoodooInputMT1UserClientMethodsIssueDriverRequest
    MTExternalMethod(&VoodooInputWellspringUserClient::sNoop, &VoodooInputWellspringUserClient::mNoop, kIOUCStructIStructO, 0x204, 0x204),
    MTExternalMethod(&VoodooInputWellspringUserClient::sPostRelativeMouse, &VoodooInputWellspringUserClient::mPostRelativeMouse, kIOUCScalarIScalarO, 3, 0),
    MTExternalMethod(&VoodooInputWellspringUserClient::sPostScrollWheel, &VoodooInputWellspringUserClient::mPostScrollWheel, kIOUCScalarIScalarO, 3, 0),
    MTExternalMethod(&VoodooInputWellspringUserClient::sPostKeyboard, &VoodooInputWellspringUserClient::mPostKeyboard, kIOUCScalarIScalarO, 2, 0),
    MTExternalMethod(&VoodooInputWellspringUserClient::sNoop, &VoodooInputWellspringUserClient::mNoop, kIOUCScalarIScalarO, 1, 0),    // Map Clicks
    // VoodooInputMT1UserClientMethodsRecacheProperties
    MTExternalMethod(&VoodooInputWellspringUserClient::sNoop, &VoodooInputWellspringUserClient::mNoop, kIOUCScalarIScalarO, 0, 0),
    MTExternalMethod(&VoodooInputWellspringUserClient::sMomentumScroll, &VoodooInputWellspringUserClient::mMomentumScroll, kIOUCScalarIScalarO, 3, 0),
};

bool VoodooInputWellspringUserClient::start(IOService *provider) {
    if (!super::start(provider)) return false;
    simulator = OSDynamicCast(VoodooInputWellspringSimulator, provider);
    
    if (simulator == nullptr) {
        IOLog("%s Invalid provider!\n", getName());
        return false;
    }
    
    dataQueue = IOSharedDataQueue::withCapacity(0x10004);
    // I haven't seen the log queue used yet.
    // Leaving this here with a small size in case userspace decides to ask for it at some point.
    logQueue = IOSharedDataQueue::withCapacity(0x1);
    
    if (dataQueue == nullptr || logQueue == nullptr) {
        return false;
    }
    
    dataQueueDesc = dataQueue->getMemoryDescriptor();
    logQueueDesc = logQueue->getMemoryDescriptor();
    if (dataQueueDesc == nullptr || logQueueDesc == nullptr) {
        return false;
    }
    
    return true;
}

void VoodooInputWellspringUserClient::free() {
    OSSafeReleaseNULL(dataQueueDesc);
    OSSafeReleaseNULL(dataQueue);
    OSSafeReleaseNULL(logQueueDesc);
    OSSafeReleaseNULL(logQueue);
    super::free();
}

IOReturn VoodooInputWellspringUserClient::registerNotificationPort(mach_port_t port, UInt32 type, UInt32 refCon) {
    IOLog("%s client notif port\n", getName());
    dataQueue->setNotificationPort(port);
    logQueue->setNotificationPort(port);
    return kIOReturnSuccess;
}

IOReturn VoodooInputWellspringUserClient::clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) {
    // The memory descriptors get released when being mapped in IOUserClient::mapClientMemory
    if (type != 0x10) {
        dataQueueDesc->retain();
        *memory = dataQueueDesc;
    } else {
        logQueueDesc->retain();
        *memory = logQueue->getMemoryDescriptor();
    }
    
    *options = 0;
    return kIOReturnSuccess;
}

IOExternalMethod *VoodooInputWellspringUserClient::getTargetAndMethodForIndex(IOService **targetP, UInt32 index) {
    IOLog("%s External Method %u\n", getName(), index);
    if (index >= VoodooInputMT1UserClientMethodsNumMethods) {
        return nullptr;
    }
    
    *targetP = this;
    // no cast anymore
    return &sMethods[index];
}

// Member shims to reuse the static functions
IOReturn VoodooInputWellspringUserClient::mSetSendFrames(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sSetSendFrames(this, static_cast<bool>(reinterpret_cast<uintptr_t>(p1)));
}

IOReturn VoodooInputWellspringUserClient::mGetReport(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sGetReport(this, reinterpret_cast<MTDeviceReportStruct *>(p1), reinterpret_cast<MTDeviceReportStruct *>(p2));
}

IOReturn VoodooInputWellspringUserClient::mNoop(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sNoop(this, p1, p2, p3, p4, p5, p6);
}

IOReturn VoodooInputWellspringUserClient::mPostRelativeMouse(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sPostRelativeMouse(this, static_cast<SInt32>(reinterpret_cast<intptr_t>(p1)),
                              static_cast<SInt32>(reinterpret_cast<intptr_t>(p2)),
                              static_cast<UInt32>(reinterpret_cast<uintptr_t>(p3)));
}

IOReturn VoodooInputWellspringUserClient::mPostScrollWheel(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sPostScrollWheel(this, static_cast<SInt32>(reinterpret_cast<intptr_t>(p1)),
                            static_cast<SInt32>(reinterpret_cast<intptr_t>(p2)),
                            static_cast<SInt32>(reinterpret_cast<intptr_t>(p3)));
}

IOReturn VoodooInputWellspringUserClient::mPostKeyboard(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sPostKeyboard(this, static_cast<UInt32>(reinterpret_cast<uintptr_t>(p1)),
                         static_cast<UInt32>(reinterpret_cast<uintptr_t>(p2)));
}

IOReturn VoodooInputWellspringUserClient::mMomentumScroll(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    return sMomentumScroll(this, static_cast<SInt32>(reinterpret_cast<intptr_t>(p1)),
                           static_cast<SInt32>(reinterpret_cast<intptr_t>(p2)),
                           static_cast<SInt32>(reinterpret_cast<intptr_t>(p3)));
}

IOReturn VoodooInputWellspringUserClient::sSetSendFrames(VoodooInputWellspringUserClient *that, bool enableReports) {
    bool success = true;
    IOLog("%s Set Send Frames: %d\n", that->getName(), enableReports);
    
    if (enableReports) {
        success = that->simulator->registerUserClient(that);
    } else {
        that->simulator->unregisterUserClient(that);
    }
    
    return success ? kIOReturnSuccess : kIOReturnError;
}

// I'm not really sure why they use two different structs here???
IOReturn VoodooInputWellspringUserClient::sGetReport(VoodooInputWellspringUserClient *that, MTDeviceReportStruct *input, MTDeviceReportStruct *output) {
    if (input == nullptr || output == nullptr) {
        return kIOReturnBadArgument;
    }
    
    IOLog("%s Get Report: %u\n", that->getName(), input->reportId);
    
    IOReturn ret = that->simulator->getReport(input);
    if (ret == kIOReturnSuccess) {
        memmove(output->data, input->data, input->dataSize);
        output->dataSize = input->dataSize;
    }
    
    return ret;
}

void VoodooInputWellspringUserClient::enqueueData(void *data, size_t size) {
    if (dataQueue == nullptr) return;
    dataQueue->enqueue(data, (UInt32) size);
}

IOReturn VoodooInputWellspringUserClient::sNoop(VoodooInputWellspringUserClient *that, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6) {
    IOLog("%s Noop was called!\n", that->getName());
    return kIOReturnSuccess; // noop
}


IOReturn VoodooInputWellspringUserClient::sPostRelativeMouse(VoodooInputWellspringUserClient *that, SInt32 dx, SInt32 dy, UInt32 buttonState) {
    that->simulator->dispatchRelativePointerEvent(dx, dy, buttonState);
    return kIOReturnSuccess;
}

IOReturn VoodooInputWellspringUserClient::sPostScrollWheel(VoodooInputWellspringUserClient *that, SInt32 dlt1, SInt32 dlt2, SInt32 dlt3) {
    that->simulator->dispatchScrollWheelEvent(dlt1, dlt2, dlt3);
    return kIOReturnSuccess;
}

IOReturn VoodooInputWellspringUserClient::sPostKeyboard(VoodooInputWellspringUserClient *that, UInt32 usagePage, UInt32 usage) {
    that->simulator->dispatchKeyboardEvent(usagePage, usage);
    return kIOReturnSuccess;
}

IOReturn VoodooInputWellspringUserClient::sMomentumScroll(VoodooInputWellspringUserClient *that, SInt32 dlt1, SInt32 dlt2, SInt32 dlt3) {
    that->simulator->dispatchMomentumScrollWheelEvent(dlt1, dlt2, dlt3);
    return kIOReturnSuccess;
}
