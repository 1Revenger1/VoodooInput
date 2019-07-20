//
//  MultitouchHelpers.hpp
//
//  Created by Alexandre on 31/07/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef MULTITOUCH_HELPERS_H
#define MULTITOUCH_HELPERS_H

#define AbsoluteTime_to_scalar(x)    (*(uint64_t *)(x))

#define SUB_ABSOLUTETIME(t1, t2)                \
    (AbsoluteTime_to_scalar(t1) -=                \
        AbsoluteTime_to_scalar(t2))

#define kIOPMPowerOff                       0
#define kIOPMNumberPowerStates     2

enum {
    // transforms
    kIOFBRotateFlags                    = 0x0000000f,
    
    kIOFBSwapAxes                       = 0x00000001,
    kIOFBInvertX                        = 0x00000002,
    kIOFBInvertY                        = 0x00000004,
    
    kIOFBRotate0                        = 0x00000000,
    kIOFBRotate90                       = kIOFBSwapAxes | kIOFBInvertX,
    kIOFBRotate180                      = kIOFBInvertX  | kIOFBInvertY,
    kIOFBRotate270                      = kIOFBSwapAxes | kIOFBInvertY
};

typedef struct {
    UInt8 contact_count;
    OSArray* transducers;
} MultitouchEvent;

typedef UInt32 MultitouchReturn;

static IOPMPowerState PMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

#endif
