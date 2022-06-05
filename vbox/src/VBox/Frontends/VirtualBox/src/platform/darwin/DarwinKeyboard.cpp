/* $Id: DarwinKeyboard.cpp 48789 2013-10-01 08:25:31Z vboxsync $ */
/** @file
 * Common GUI Library - Darwin Keyboard routines.
 *
 * @todo Move this up somewhere so that the two SDL GUIs can use parts of this code too (-HID crap).
 */

/*
 * Copyright (C) 2006-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_GUI
//#define VBOX_WITH_KBD_LEDS_SYNC

#include "DarwinKeyboard.h"
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/time.h>
#include <iprt/mem.h>
#include <VBox/log.h>
#ifdef DEBUG_PRINTF
# include <iprt/stream.h>
#endif

#ifdef USE_HID_FOR_MODIFIERS
# include <mach/mach.h>
# include <mach/mach_error.h>
# include <IOKit/IOKitLib.h>
# include <IOKit/IOCFPlugIn.h>
# include <IOKit/hid/IOHIDUsageTables.h>
# include <CoreFoundation/CoreFoundation.h>
#endif
# include <IOKit/usb/USB.h>
#include <IOKit/hid/IOHIDLib.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

#ifndef USE_HID_FOR_MODIFIERS
# include "CocoaEventHelper.h"
#endif


RT_C_DECLS_BEGIN
/* Private interface in 10.3 and later. */
typedef int CGSConnection;
typedef enum
{
    kCGSGlobalHotKeyEnable = 0,
    kCGSGlobalHotKeyDisable,
    kCGSGlobalHotKeyDisableExceptUniversalAccess,
    kCGSGlobalHotKeyInvalid = -1 /* bird */
} CGSGlobalHotKeyOperatingMode;
extern CGSConnection _CGSDefaultConnection(void);
extern CGError CGSGetGlobalHotKeyOperatingMode(CGSConnection Connection, CGSGlobalHotKeyOperatingMode *enmMode);
extern CGError CGSSetGlobalHotKeyOperatingMode(CGSConnection Connection, CGSGlobalHotKeyOperatingMode enmMode);
RT_C_DECLS_END


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/

#define QZ_RMETA        0x36
#define QZ_LMETA        0x37
#define QZ_LSHIFT       0x38
#define QZ_CAPSLOCK     0x39
#define QZ_LALT         0x3A
#define QZ_LCTRL        0x3B
#define QZ_RSHIFT       0x3C
#define QZ_RALT         0x3D
#define QZ_RCTRL        0x3E
/* Found the definition of the fn-key in:
 * http://stuff.mit.edu/afs/sipb/project/darwin/src/modules/IOHIDFamily/IOHIDSystem/IOHIKeyboardMapper.cpp &
 * http://stuff.mit.edu/afs/sipb/project/darwin/src/modules/AppleADBKeyboard/AppleADBKeyboard.cpp
 * Maybe we need this in the future.*/
#define QZ_FN           0x3F
#define QZ_NUMLOCK      0x47

/** short hand for an extended key. */
#define K_EX            VBOXKEY_EXTENDED
/** short hand for a modifier key. */
#define K_MOD           VBOXKEY_MODIFIER
/** short hand for a lock key. */
#define K_LOCK          VBOXKEY_LOCK

#ifdef USE_HID_FOR_MODIFIERS

/** An attempt at catching reference leaks. */
#define MY_CHECK_CREFS(cRefs)   do { AssertMsg(cRefs < 25, ("%ld\n", cRefs)); NOREF(cRefs); } while (0)

#endif

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * This is derived partially from SDL_QuartzKeys.h and partially from testing.
 *
 * (The funny thing about the virtual scan codes on the mac is that they aren't
 * offically documented, which is rather silly to say the least. Thus, the need
 * for looking at SDL and other odd places for docs.)
 */
static const uint16_t g_aDarwinToSet1[] =
{
 /*  set-1                           SDL_QuartzKeys.h    */
    0x1e,                       /* QZ_a            0x00 */
    0x1f,                       /* QZ_s            0x01 */
    0x20,                       /* QZ_d            0x02 */
    0x21,                       /* QZ_f            0x03 */
    0x23,                       /* QZ_h            0x04 */
    0x22,                       /* QZ_g            0x05 */
    0x2c,                       /* QZ_z            0x06 */
    0x2d,                       /* QZ_x            0x07 */
    0x2e,                       /* QZ_c            0x08 */
    0x2f,                       /* QZ_v            0x09 */
    0x56,                       /* between lshift and z. 'INT 1'? */
    0x30,                       /* QZ_b            0x0B */
    0x10,                       /* QZ_q            0x0C */
    0x11,                       /* QZ_w            0x0D */
    0x12,                       /* QZ_e            0x0E */
    0x13,                       /* QZ_r            0x0F */
    0x15,                       /* QZ_y            0x10 */
    0x14,                       /* QZ_t            0x11 */
    0x02,                       /* QZ_1            0x12 */
    0x03,                       /* QZ_2            0x13 */
    0x04,                       /* QZ_3            0x14 */
    0x05,                       /* QZ_4            0x15 */
    0x07,                       /* QZ_6            0x16 */
    0x06,                       /* QZ_5            0x17 */
    0x0d,                       /* QZ_EQUALS       0x18 */
    0x0a,                       /* QZ_9            0x19 */
    0x08,                       /* QZ_7            0x1A */
    0x0c,                       /* QZ_MINUS        0x1B */
    0x09,                       /* QZ_8            0x1C */
    0x0b,                       /* QZ_0            0x1D */
    0x1b,                       /* QZ_RIGHTBRACKET 0x1E */
    0x18,                       /* QZ_o            0x1F */
    0x16,                       /* QZ_u            0x20 */
    0x1a,                       /* QZ_LEFTBRACKET  0x21 */
    0x17,                       /* QZ_i            0x22 */
    0x19,                       /* QZ_p            0x23 */
    0x1c,                       /* QZ_RETURN       0x24 */
    0x26,                       /* QZ_l            0x25 */
    0x24,                       /* QZ_j            0x26 */
    0x28,                       /* QZ_QUOTE        0x27 */
    0x25,                       /* QZ_k            0x28 */
    0x27,                       /* QZ_SEMICOLON    0x29 */
    0x2b,                       /* QZ_BACKSLASH    0x2A */
    0x33,                       /* QZ_COMMA        0x2B */
    0x35,                       /* QZ_SLASH        0x2C */
    0x31,                       /* QZ_n            0x2D */
    0x32,                       /* QZ_m            0x2E */
    0x34,                       /* QZ_PERIOD       0x2F */
    0x0f,                       /* QZ_TAB          0x30 */
    0x39,                       /* QZ_SPACE        0x31 */
    0x29,                       /* QZ_BACKQUOTE    0x32 */
    0x0e,                       /* QZ_BACKSPACE    0x33 */
    0x9c,                       /* QZ_IBOOK_ENTER  0x34 */
    0x01,                       /* QZ_ESCAPE       0x35 */
    0x5c|K_EX|K_MOD,            /* QZ_RMETA        0x36 */
    0x5b|K_EX|K_MOD,            /* QZ_LMETA        0x37 */
    0x2a|K_MOD,                 /* QZ_LSHIFT       0x38 */
    0x3a|K_LOCK,                /* QZ_CAPSLOCK     0x39 */
    0x38|K_MOD,                 /* QZ_LALT         0x3A */
    0x1d|K_MOD,                 /* QZ_LCTRL        0x3B */
    0x36|K_MOD,                 /* QZ_RSHIFT       0x3C */
    0x38|K_EX|K_MOD,            /* QZ_RALT         0x3D */
    0x1d|K_EX|K_MOD,            /* QZ_RCTRL        0x3E */
       0,                       /*                      */
       0,                       /*                      */
    0x53,                       /* QZ_KP_PERIOD    0x41 */
       0,                       /*                      */
    0x37,                       /* QZ_KP_MULTIPLY  0x43 */
       0,                       /*                      */
    0x4e,                       /* QZ_KP_PLUS      0x45 */
       0,                       /*                      */
    0x45|K_LOCK,                /* QZ_NUMLOCK      0x47 */
       0,                       /*                      */
       0,                       /*                      */
       0,                       /*                      */
    0x35|K_EX,                  /* QZ_KP_DIVIDE    0x4B */
    0x1c|K_EX,                  /* QZ_KP_ENTER     0x4C */
       0,                       /*                      */
    0x4a,                       /* QZ_KP_MINUS     0x4E */
       0,                       /*                      */
       0,                       /*                      */
    0x0d/*?*/,                  /* QZ_KP_EQUALS    0x51 */
    0x52,                       /* QZ_KP0          0x52 */
    0x4f,                       /* QZ_KP1          0x53 */
    0x50,                       /* QZ_KP2          0x54 */
    0x51,                       /* QZ_KP3          0x55 */
    0x4b,                       /* QZ_KP4          0x56 */
    0x4c,                       /* QZ_KP5          0x57 */
    0x4d,                       /* QZ_KP6          0x58 */
    0x47,                       /* QZ_KP7          0x59 */
       0,                       /*                      */
    0x48,                       /* QZ_KP8          0x5B */
    0x49,                       /* QZ_KP9          0x5C */
    0x7d,                       /* yen, | (JIS)    0x5D */
    0x73,                       /* _, ro (JIS)     0x5E */
       0,                       /*                      */
    0x3f,                       /* QZ_F5           0x60 */
    0x40,                       /* QZ_F6           0x61 */
    0x41,                       /* QZ_F7           0x62 */
    0x3d,                       /* QZ_F3           0x63 */
    0x42,                       /* QZ_F8           0x64 */
    0x43,                       /* QZ_F9           0x65 */
    0x29,                       /* Zen/Han (JIS)   0x66 */
    0x57,                       /* QZ_F11          0x67 */
    0x29,                       /* Zen/Han (JIS)   0x68 */
    0x37|K_EX,                  /* QZ_PRINT / F13  0x69 */
    0x63,                       /* QZ_F16          0x6A */
    0x46|K_LOCK,                /* QZ_SCROLLOCK    0x6B */
       0,                       /*                      */
    0x44,                       /* QZ_F10          0x6D */
    0x5d|K_EX,                  /*                      */
    0x58,                       /* QZ_F12          0x6F */
       0,                       /*                      */
       0/* 0xe1,0x1d,0x45*/,    /* QZ_PAUSE        0x71 */
    0x52|K_EX,                  /* QZ_INSERT / HELP 0x72 */
    0x47|K_EX,                  /* QZ_HOME         0x73 */
    0x49|K_EX,                  /* QZ_PAGEUP       0x74 */
    0x53|K_EX,                  /* QZ_DELETE       0x75 */
    0x3e,                       /* QZ_F4           0x76 */
    0x4f|K_EX,                  /* QZ_END          0x77 */
    0x3c,                       /* QZ_F2           0x78 */
    0x51|K_EX,                  /* QZ_PAGEDOWN     0x79 */
    0x3b,                       /* QZ_F1           0x7A */
    0x4b|K_EX,                  /* QZ_LEFT         0x7B */
    0x4d|K_EX,                  /* QZ_RIGHT        0x7C */
    0x50|K_EX,                  /* QZ_DOWN         0x7D */
    0x48|K_EX,                  /* QZ_UP           0x7E */
    0x5e|K_EX,                  /* QZ_POWER        0x7F */ /* have different break key! */
};


/** Whether we've connected or not. */
static bool g_fConnectedToCGS = false;
/** Cached connection. */
static CGSConnection g_CGSConnection;


#ifdef USE_HID_FOR_MODIFIERS

/** The IO Master Port. */
static mach_port_t g_MasterPort = NULL;

/** Keyboards in the cache. */
static unsigned g_cKeyboards = 0;
/** Array of cached keyboard data. */
static struct KeyboardCacheData
{
    /** The device interface. */
    IOHIDDeviceInterface  **ppHidDeviceInterface;
    /** The queue interface. */
    IOHIDQueueInterface   **ppHidQueueInterface;

    /* cookie translation array. */
    struct KeyboardCacheCookie
    {
        /** The cookie. */
        IOHIDElementCookie  Cookie;
        /** The corresponding modifier mask. */
        uint32_t            fMask;
    }                       aCookies[64];
    /** Number of cookies in the array. */
    unsigned                cCookies;
}                   g_aKeyboards[128];
/** The keyboard cache creation timestamp. */
static uint64_t     g_u64KeyboardTS = 0;

/** HID queue status. */
static bool         g_fHIDQueueEnabled;
/** The current modifier mask. */
static uint32_t     g_fHIDModifierMask;
/** The old modifier mask. */
static uint32_t     g_fOldHIDModifierMask;

#endif /* USE_HID_FOR_MODIFIERS */

#ifdef VBOX_WITH_KBD_LEDS_SYNC
/* HID LEDs synchronization data: LED states. */
typedef struct VBoxHidLeds_t {
    bool             fNumLockOn;
    bool             fCapsLockOn;
    bool             fScrollLockOn;
} VBoxHidLeds_t;

/* HID LEDs synchronization data: IOKit specific data. */
typedef struct VBoxHidsState_t {
    IOHIDManagerRef     hidManagerRef;
    IOHIDDeviceRef     *hidDevicesCollection;
    VBoxHidLeds_t      *hidLedsCollection;
    CFIndex             cDevices;
    CFMachPortRef       pTapRef;
    CFRunLoopSourceRef  pLoopSourceRef;
} VBoxHidsState_t;

/* A *sync* between IOKit and Carbon callbacks. */
static VBoxHidLeds_t *g_LastTouchedState;
#endif // !VBOX_WITH_KBD_LEDS_SYNC

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#ifdef USE_HID_FOR_MODIFIERS
static void darwinBruteForcePropertySearch(CFDictionaryRef DictRef, struct KeyboardCacheData *pKeyboardEntry);
#endif


/**
 * Converts a darwin (virtual) key code to a set 1 scan code.
 *
 * @returns set 1 scan code.
 * @param   uKeyCode        The darwin key code.
 */
unsigned DarwinKeycodeToSet1Scancode(unsigned uKeyCode)
{
    if (uKeyCode >= RT_ELEMENTS(g_aDarwinToSet1))
        return 0;
    return g_aDarwinToSet1[uKeyCode];
}


/**
 * Converts a single modifier to a set 1 scan code.
 *
 * @returns Set 1 scan code.
 * @returns ~0U if more than one modifier is set.
 * @param   fModifiers      The darwin modifier mask.
 */
unsigned DarwinModifierMaskToSet1Scancode(UInt32 fModifiers)
{
    unsigned uScanCode = DarwinModifierMaskToDarwinKeycode(fModifiers);
    if (uScanCode < RT_ELEMENTS(g_aDarwinToSet1))
        uScanCode = g_aDarwinToSet1[uScanCode];
    else
        Assert(uScanCode == ~0U);
    return uScanCode;
}


/**
 * Converts a single modifier to a darwin keycode.
 *
 * @returns Darwin keycode.
 * @returns 0 if none of the support masks were set.
 * @returns ~0U if more than one modifier is set.
 * @param   fModifiers      The darwin modifier mask.
 */
unsigned DarwinModifierMaskToDarwinKeycode(UInt32 fModifiers)
{
    unsigned uKeyCode;

    /** @todo find symbols for these keycodes... */
    fModifiers &= shiftKey | rightShiftKey | controlKey | rightControlKey | optionKey | rightOptionKey | cmdKey
                | kEventKeyModifierRightCmdKeyMask | kEventKeyModifierNumLockMask | alphaLock | kEventKeyModifierFnMask;
    if (fModifiers == shiftKey)
        uKeyCode = QZ_LSHIFT;
    else if (fModifiers == rightShiftKey)
        uKeyCode = QZ_RSHIFT;
    else if (fModifiers == controlKey)
        uKeyCode = QZ_LCTRL;
    else if (fModifiers == rightControlKey)
        uKeyCode = QZ_RCTRL;
    else if (fModifiers == optionKey)
        uKeyCode = QZ_LALT;
    else if (fModifiers == rightOptionKey)
        uKeyCode = QZ_RALT;
    else if (fModifiers == cmdKey)
        uKeyCode = QZ_LMETA;
    else if (fModifiers == kEventKeyModifierRightCmdKeyMask /* hack */)
        uKeyCode = QZ_RMETA;
    else if (fModifiers == alphaLock)
        uKeyCode = QZ_CAPSLOCK;
    else if (fModifiers == kEventKeyModifierNumLockMask)
        uKeyCode = QZ_NUMLOCK;
    else if (fModifiers == kEventKeyModifierFnMask)
        uKeyCode = QZ_FN;
    else if (fModifiers == 0)
        uKeyCode = 0;
    else
        uKeyCode = ~0U; /* multiple */
    return uKeyCode;
}


/**
 * Converts a darwin keycode to a modifier mask.
 *
 * @returns Darwin modifier mask.
 * @returns 0 if the keycode isn't a modifier we know.
 * @param   uKeyCode        The darwin
 */
UInt32 DarwinKeyCodeToDarwinModifierMask(unsigned uKeyCode)
{
    UInt32 fModifiers;

    /** @todo find symbols for these keycodes... */
    if (uKeyCode == QZ_LSHIFT)
        fModifiers = shiftKey;
    else if (uKeyCode == QZ_RSHIFT)
        fModifiers = rightShiftKey;
    else if (uKeyCode == QZ_LCTRL)
        fModifiers = controlKey;
    else if (uKeyCode == QZ_RCTRL)
        fModifiers = rightControlKey;
    else if (uKeyCode == QZ_LALT)
        fModifiers = optionKey;
    else if (uKeyCode == QZ_RALT)
        fModifiers = rightOptionKey;
    else if (uKeyCode == QZ_LMETA)
        fModifiers = cmdKey;
    else if (uKeyCode == QZ_RMETA)
        fModifiers = kEventKeyModifierRightCmdKeyMask; /* hack */
    else if (uKeyCode == QZ_CAPSLOCK)
        fModifiers = alphaLock;
    else if (uKeyCode == QZ_NUMLOCK)
        fModifiers = kEventKeyModifierNumLockMask;
    else if (uKeyCode == QZ_FN)
        fModifiers = kEventKeyModifierFnMask;
    else
        fModifiers = 0;
    return fModifiers;
}


/**
 * Disables or enabled global hot keys.
 *
 * @param   fDisable    Pass 'true' to disable the hot keys, pass 'false' to re-enable them.
 */
void DarwinDisableGlobalHotKeys(bool fDisable)
{
    static unsigned s_cComplaints = 0;

    /*
     * Lazy connect to the core graphics service.
     */
    if (!g_fConnectedToCGS)
    {
        g_CGSConnection = _CGSDefaultConnection();
        g_fConnectedToCGS = true;
    }

    /*
     * Get the current mode.
     */
    CGSGlobalHotKeyOperatingMode enmMode = kCGSGlobalHotKeyInvalid;
    CGSGetGlobalHotKeyOperatingMode(g_CGSConnection, &enmMode);
    if (    enmMode != kCGSGlobalHotKeyEnable
        &&  enmMode != kCGSGlobalHotKeyDisable
        &&  enmMode != kCGSGlobalHotKeyDisableExceptUniversalAccess)
    {
        AssertMsgFailed(("%d\n", enmMode));
        if (s_cComplaints++ < 32)
            LogRel(("DarwinDisableGlobalHotKeys: Unexpected enmMode=%d\n", enmMode));
        return;
    }

    /*
     * Calc the new mode.
     */
    if (fDisable)
    {
        if (enmMode != kCGSGlobalHotKeyEnable)
            return;
        enmMode = kCGSGlobalHotKeyDisable;
    }
    else
    {
        if (enmMode != kCGSGlobalHotKeyDisable)
            return;
        enmMode = kCGSGlobalHotKeyEnable;
    }

    /*
     * Try set it and check the actual result.
     */
    CGSSetGlobalHotKeyOperatingMode(g_CGSConnection, enmMode);
    CGSGlobalHotKeyOperatingMode enmNewMode = kCGSGlobalHotKeyInvalid;
    CGSGetGlobalHotKeyOperatingMode(g_CGSConnection, &enmNewMode);
    if (enmNewMode != enmMode)
    {
        /* If the screensaver kicks in we should ignore failure here. */
        AssertMsg(enmMode == kCGSGlobalHotKeyEnable, ("enmNewMode=%d enmMode=%d\n", enmNewMode, enmMode));
        if (s_cComplaints++ < 32)
            LogRel(("DarwinDisableGlobalHotKeys: Failed to change mode; enmNewMode=%d enmMode=%d\n", enmNewMode, enmMode));
    }
}

#ifdef USE_HID_FOR_MODIFIERS

/**
 * Callback function for consuming queued events.
 *
 * @param   pvTarget    queue?
 * @param   rcIn        ?
 * @param   pvRefcon    Pointer to the keyboard cache entry.
 * @param   pvSender    ?
 */
static void darwinQueueCallback(void *pvTarget, IOReturn rcIn, void *pvRefcon, void *pvSender)
{
    struct KeyboardCacheData *pKeyboardEntry = (struct KeyboardCacheData *)pvRefcon;
    if (!pKeyboardEntry->ppHidQueueInterface)
        return;
    NOREF(pvTarget);
    NOREF(rcIn);
    NOREF(pvSender);

    /*
     * Consume the events.
     */
    g_fOldHIDModifierMask = g_fHIDModifierMask;
    for (;;)
    {
#ifdef DEBUG_PRINTF
        RTPrintf("dbg-ev: "); RTStrmFlush(g_pStdOut);
#endif
        IOHIDEventStruct Event;
        AbsoluteTime ZeroTime = {0,0};
        IOReturn rc = (*pKeyboardEntry->ppHidQueueInterface)->getNextEvent(pKeyboardEntry->ppHidQueueInterface,
                                                                           &Event, ZeroTime, 0);
        if (rc != kIOReturnSuccess)
            break;

        /* Translate the cookie value to a modifier mask. */
        uint32_t fMask = 0;
        unsigned i = pKeyboardEntry->cCookies;
        while (i-- > 0)
        {
            if (pKeyboardEntry->aCookies[i].Cookie == Event.elementCookie)
            {
                fMask = pKeyboardEntry->aCookies[i].fMask;
                break;
            }
        }

        /*
         * Adjust the modifier mask.
         *
         * Note that we don't bother to deal with anyone pressing the same modifier
         * on 2 or more keyboard. That's not worth the effort involved.
         */
        if (Event.value)
            g_fHIDModifierMask |= fMask;
        else
            g_fHIDModifierMask &= ~fMask;
#ifdef DEBUG_PRINTF
        RTPrintf("t=%d c=%#x v=%#x cblv=%d lv=%p m=%#X\n", Event.type, Event.elementCookie, Event.value, Event.longValueSize, Event.value, fMask); RTStrmFlush(g_pStdOut);
#endif
    }
#ifdef DEBUG_PRINTF
    RTPrintf("dbg-ev: done\n"); RTStrmFlush(g_pStdOut);
#endif
}



/**
 * Element enumeration callback.
 */
static void darwinBruteForcePropertySearchApplier(const void *pvValue, void *pvCacheEntry)
{
    if (CFGetTypeID(pvValue) == CFDictionaryGetTypeID())
        darwinBruteForcePropertySearch((CFMutableDictionaryRef)pvValue, (struct KeyboardCacheData *)pvCacheEntry);
}


/**
 * Recurses thru the keyboard properties looking for certain keys.
 *
 * @remark  Yes, this can probably be done in a more efficient way. If you
 *          know how to do this, don't hesitate to let us know!
 */
static void darwinBruteForcePropertySearch(CFDictionaryRef DictRef, struct KeyboardCacheData *pKeyboardEntry)
{
    CFTypeRef ObjRef;

    /*
     * Check for the usage page and usage key we want.
     */
    long lUsage;
    ObjRef = CFDictionaryGetValue(DictRef, CFSTR(kIOHIDElementUsageKey));
    if (    ObjRef
        &&  CFGetTypeID(ObjRef) == CFNumberGetTypeID()
        &&  CFNumberGetValue((CFNumberRef)ObjRef, kCFNumberLongType, &lUsage))
    {
        switch (lUsage)
        {
            case kHIDUsage_KeyboardLeftControl:
            case kHIDUsage_KeyboardLeftShift:
            case kHIDUsage_KeyboardLeftAlt:
            case kHIDUsage_KeyboardLeftGUI:
            case kHIDUsage_KeyboardRightControl:
            case kHIDUsage_KeyboardRightShift:
            case kHIDUsage_KeyboardRightAlt:
            case kHIDUsage_KeyboardRightGUI:
            {
                long lPage;
                ObjRef = CFDictionaryGetValue(DictRef, CFSTR(kIOHIDElementUsagePageKey));
                if (    !ObjRef
                    ||  CFGetTypeID(ObjRef) != CFNumberGetTypeID()
                    ||  !CFNumberGetValue((CFNumberRef)ObjRef, kCFNumberLongType, &lPage)
                    ||  lPage != kHIDPage_KeyboardOrKeypad)
                    break;

                if (pKeyboardEntry->cCookies >= RT_ELEMENTS(pKeyboardEntry->aCookies))
                {
                    AssertMsgFailed(("too many cookies!\n"));
                    break;
                }

                /*
                 * Get the cookie and modifier mask.
                 */
                long lCookie;
                ObjRef = CFDictionaryGetValue(DictRef, CFSTR(kIOHIDElementCookieKey));
                if (    !ObjRef
                    ||  CFGetTypeID(ObjRef) != CFNumberGetTypeID()
                    ||  !CFNumberGetValue((CFNumberRef)ObjRef, kCFNumberLongType, &lCookie))
                    break;

                uint32_t fMask;
                switch (lUsage)
                {
                    case kHIDUsage_KeyboardLeftControl : fMask = controlKey; break;
                    case kHIDUsage_KeyboardLeftShift   : fMask = shiftKey; break;
                    case kHIDUsage_KeyboardLeftAlt     : fMask = optionKey; break;
                    case kHIDUsage_KeyboardLeftGUI     : fMask = cmdKey; break;
                    case kHIDUsage_KeyboardRightControl: fMask = rightControlKey; break;
                    case kHIDUsage_KeyboardRightShift  : fMask = rightShiftKey; break;
                    case kHIDUsage_KeyboardRightAlt    : fMask = rightOptionKey; break;
                    case kHIDUsage_KeyboardRightGUI    : fMask = kEventKeyModifierRightCmdKeyMask; break;
                    default: AssertMsgFailed(("%ld\n",lUsage)); fMask = 0; break;
                }

                /*
                 * If we've got a queue, add the cookie to the queue.
                 */
                if (pKeyboardEntry->ppHidQueueInterface)
                {
                    IOReturn rc = (*pKeyboardEntry->ppHidQueueInterface)->addElement(pKeyboardEntry->ppHidQueueInterface, (IOHIDElementCookie)lCookie, 0);
                    AssertMsg(rc == kIOReturnSuccess, ("rc=%d\n", rc));
#ifdef DEBUG_PRINTF
                    RTPrintf("dbg-add: u=%#lx c=%#lx\n", lUsage, lCookie);
#endif
                }

                /*
                 * Add the cookie to the keyboard entry.
                 */
                pKeyboardEntry->aCookies[pKeyboardEntry->cCookies].Cookie = (IOHIDElementCookie)lCookie;
                pKeyboardEntry->aCookies[pKeyboardEntry->cCookies].fMask = fMask;
                ++pKeyboardEntry->cCookies;
                break;
            }
        }
    }


    /*
     * Get the elements key and recursively iterate the elements looking
     * for they key cookies.
     */
    ObjRef = CFDictionaryGetValue(DictRef, CFSTR(kIOHIDElementKey));
    if (    ObjRef
        &&  CFGetTypeID(ObjRef) == CFArrayGetTypeID())
    {
        CFArrayRef ArrayObjRef = (CFArrayRef)ObjRef;
        CFRange Range = {0, CFArrayGetCount(ArrayObjRef)};
        CFArrayApplyFunction(ArrayObjRef, Range, darwinBruteForcePropertySearchApplier, pKeyboardEntry);
    }
}


/**
 * Creates a keyboard cache entry.
 *
 * @returns true if the entry was created successfully, otherwise false.
 * @param   pKeyboardEntry      Pointer to the entry.
 * @param   KeyboardDevice      The keyboard device to create the entry for.
 *
 */
static bool darwinHIDKeyboardCacheCreateEntry(struct KeyboardCacheData *pKeyboardEntry, io_object_t KeyboardDevice)
{
    unsigned long cRefs = 0;
    memset(pKeyboardEntry, 0, sizeof(*pKeyboardEntry));

    /*
     * Query the HIDDeviceInterface for this HID (keyboard) object.
     */
    SInt32 Score = 0;
    IOCFPlugInInterface **ppPlugInInterface = NULL;
    IOReturn rc = IOCreatePlugInInterfaceForService(KeyboardDevice, kIOHIDDeviceUserClientTypeID,
                                                    kIOCFPlugInInterfaceID, &ppPlugInInterface, &Score);
    if (rc == kIOReturnSuccess)
    {
        IOHIDDeviceInterface **ppHidDeviceInterface = NULL;
        HRESULT hrc = (*ppPlugInInterface)->QueryInterface(ppPlugInInterface,
                                                           CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID),
                                                           (LPVOID *)&ppHidDeviceInterface);
        cRefs = (*ppPlugInInterface)->Release(ppPlugInInterface); MY_CHECK_CREFS(cRefs);
        ppPlugInInterface = NULL;
        if (hrc == S_OK)
        {
            rc = (*ppHidDeviceInterface)->open(ppHidDeviceInterface, 0);
            if (rc == kIOReturnSuccess)
            {
                /*
                 * create a removal callback.
                 */
                /** @todo */


                /*
                 * Create the queue so we can insert elements while searching the properties.
                 */
                IOHIDQueueInterface   **ppHidQueueInterface = (*ppHidDeviceInterface)->allocQueue(ppHidDeviceInterface);
                if (ppHidQueueInterface)
                {
                    rc = (*ppHidQueueInterface)->create(ppHidQueueInterface, 0, 32);
                    if (rc != kIOReturnSuccess)
                    {
                        AssertMsgFailed(("rc=%d\n", rc));
                        cRefs = (*ppHidQueueInterface)->Release(ppHidQueueInterface); MY_CHECK_CREFS(cRefs);
                        ppHidQueueInterface = NULL;
                    }
                }
                else
                    AssertFailed();
                pKeyboardEntry->ppHidQueueInterface = ppHidQueueInterface;

                /*
                 * Brute force getting of attributes.
                 */
                /** @todo read up on how to do this in a less resource intensive way! Suggestions are welcome! */
                CFMutableDictionaryRef PropertiesRef = 0;
                kern_return_t krc = IORegistryEntryCreateCFProperties(KeyboardDevice, &PropertiesRef, kCFAllocatorDefault, kNilOptions);
                if (krc == KERN_SUCCESS)
                {
                    darwinBruteForcePropertySearch(PropertiesRef, pKeyboardEntry);
                    CFRelease(PropertiesRef);
                }
                else
                    AssertMsgFailed(("krc=%#x\n", krc));

                if (ppHidQueueInterface)
                {
                    /*
                     * Now install our queue callback.
                     */
                    CFRunLoopSourceRef RunLoopSrcRef = NULL;
                    rc = (*ppHidQueueInterface)->createAsyncEventSource(ppHidQueueInterface, &RunLoopSrcRef);
                    if (rc == kIOReturnSuccess)
                    {
                        CFRunLoopRef RunLoopRef = (CFRunLoopRef)GetCFRunLoopFromEventLoop(GetMainEventLoop());
                        CFRunLoopAddSource(RunLoopRef, RunLoopSrcRef, kCFRunLoopDefaultMode);
                    }

                    /*
                     * Now install our queue callback.
                     */
                    rc = (*ppHidQueueInterface)->setEventCallout(ppHidQueueInterface, darwinQueueCallback, ppHidQueueInterface, pKeyboardEntry);
                    if (rc != kIOReturnSuccess)
                        AssertMsgFailed(("rc=%d\n", rc));
                }

                /*
                 * Complete the new keyboard cache entry.
                 */
                pKeyboardEntry->ppHidDeviceInterface = ppHidDeviceInterface;
                pKeyboardEntry->ppHidQueueInterface = ppHidQueueInterface;
                return true;
            }

            AssertMsgFailed(("rc=%d\n", rc));
            cRefs = (*ppHidDeviceInterface)->Release(ppHidDeviceInterface); MY_CHECK_CREFS(cRefs);
        }
        else
            AssertMsgFailed(("hrc=%#x\n", hrc));
    }
    else
        AssertMsgFailed(("rc=%d\n", rc));

    return false;
}


/**
 * Destroys a keyboard cache entry.
 *
 * @param   pKeyboardEntry      The entry.
 */
static void darwinHIDKeyboardCacheDestroyEntry(struct KeyboardCacheData *pKeyboardEntry)
{
    unsigned long cRefs;

    /*
     * Destroy the queue
     */
    if (pKeyboardEntry->ppHidQueueInterface)
    {
        IOHIDQueueInterface **ppHidQueueInterface = pKeyboardEntry->ppHidQueueInterface;
        pKeyboardEntry->ppHidQueueInterface = NULL;

        /* stop it just in case we haven't done so. doesn't really matter I think. */
        (*ppHidQueueInterface)->stop(ppHidQueueInterface);

        /* deal with the run loop source. */
        CFRunLoopSourceRef RunLoopSrcRef = (*ppHidQueueInterface)->getAsyncEventSource(ppHidQueueInterface);
        if (RunLoopSrcRef)
        {
            CFRunLoopRef RunLoopRef = (CFRunLoopRef)GetCFRunLoopFromEventLoop(GetMainEventLoop());
            CFRunLoopRemoveSource(RunLoopRef, RunLoopSrcRef, kCFRunLoopDefaultMode);

            CFRelease(RunLoopSrcRef);
        }

        /* dispose of and release the queue. */
        (*ppHidQueueInterface)->dispose(ppHidQueueInterface);
        cRefs = (*ppHidQueueInterface)->Release(ppHidQueueInterface); MY_CHECK_CREFS(cRefs);
    }

    /*
     * Release the removal hook?
     */
    /** @todo */

    /*
     * Close and release the device interface.
     */
    if (pKeyboardEntry->ppHidDeviceInterface)
    {
        IOHIDDeviceInterface **ppHidDeviceInterface = pKeyboardEntry->ppHidDeviceInterface;
        pKeyboardEntry->ppHidDeviceInterface = NULL;

        (*ppHidDeviceInterface)->close(ppHidDeviceInterface);
        cRefs = (*ppHidDeviceInterface)->Release(ppHidDeviceInterface); MY_CHECK_CREFS(cRefs);
    }
}


/**
 * Zap the keyboard cache.
 */
static void darwinHIDKeyboardCacheZap(void)
{
    /*
     * Release the old cache data first.
     */
    while (g_cKeyboards > 0)
    {
        unsigned i = --g_cKeyboards;
        darwinHIDKeyboardCacheDestroyEntry(&g_aKeyboards[i]);
    }
}


/**
 * Updates the cached keyboard data.
 *
 * @todo The current implementation is very brute force...
 *       Rewrite it so that it doesn't flush the cache completely but simply checks whether
 *       anything has changed in the HID config. With any luck, there might even be a callback
 *       or something we can poll for HID config changes...
 *       setRemovalCallback() is a start...
 */
static void darwinHIDKeyboardCacheDoUpdate(void)
{
    g_u64KeyboardTS = RTTimeMilliTS();

    /*
     * Dispense with the old cache data.
     */
    darwinHIDKeyboardCacheZap();

    /*
     * Open the master port on the first invocation.
     */
    if (!g_MasterPort)
    {
        kern_return_t krc = IOMasterPort(MACH_PORT_NULL, &g_MasterPort);
        AssertReturnVoid(krc == KERN_SUCCESS);
    }

    /*
     * Create a matching dictionary for searching for keyboards devices.
     */
    static const UInt32 s_Page = kHIDPage_GenericDesktop;
    static const UInt32 s_Usage = kHIDUsage_GD_Keyboard;
    CFMutableDictionaryRef RefMatchingDict = IOServiceMatching(kIOHIDDeviceKey);
    AssertReturnVoid(RefMatchingDict);
    CFDictionarySetValue(RefMatchingDict, CFSTR(kIOHIDPrimaryUsagePageKey),
                         CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &s_Page));
    CFDictionarySetValue(RefMatchingDict, CFSTR(kIOHIDPrimaryUsageKey),
                         CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &s_Usage));

    /*
     * Perform the search and get a collection of keyboard devices.
     */
    io_iterator_t Keyboards = NULL;
    IOReturn rc = IOServiceGetMatchingServices(g_MasterPort, RefMatchingDict, &Keyboards);
    AssertMsgReturnVoid(rc == kIOReturnSuccess, ("rc=%d\n", rc));
    RefMatchingDict = NULL; /* the reference is consumed by IOServiceGetMatchingServices. */

    /*
     * Enumerate the keyboards and query the cache data.
     */
    unsigned i = 0;
    io_object_t KeyboardDevice;
    while (   i < RT_ELEMENTS(g_aKeyboards)
           && (KeyboardDevice = IOIteratorNext(Keyboards)) != 0)
    {
        if (darwinHIDKeyboardCacheCreateEntry(&g_aKeyboards[i], KeyboardDevice))
            i++;
        IOObjectRelease(KeyboardDevice);
    }
    g_cKeyboards = i;

    IOObjectRelease(Keyboards);
}


/**
 * Updates the keyboard cache if it's time to do it again.
 */
static void darwinHIDKeyboardCacheUpdate(void)
{
    if (    !g_cKeyboards
        /*||  g_u64KeyboardTS - RTTimeMilliTS() > 7500*/ /* 7.5sec */)
        darwinHIDKeyboardCacheDoUpdate();
}


/**
 * Queries the modifier keys from the (IOKit) HID Manager.
 *
 * @returns Carbon modifier mask with right/left set correctly.
 */
static UInt32 darwinQueryHIDModifiers(void)
{
    /*
     * Iterate thru the keyboards collecting their modifier masks.
     */
    UInt32 fHIDModifiers = 0;
    unsigned i = g_cKeyboards;
    while (i-- > 0)
    {
        IOHIDDeviceInterface **ppHidDeviceInterface = g_aKeyboards[i].ppHidDeviceInterface;
        if (!ppHidDeviceInterface)
            continue;

        unsigned j = g_aKeyboards[i].cCookies;
        while (j-- > 0)
        {
            IOHIDEventStruct HidEvent;
            IOReturn rc = (*ppHidDeviceInterface)->getElementValue(ppHidDeviceInterface,
                                                                   g_aKeyboards[i].aCookies[j].Cookie,
                                                                   &HidEvent);
            if (rc == kIOReturnSuccess)
            {
                if (HidEvent.value)
                    fHIDModifiers |= g_aKeyboards[i].aCookies[j].fMask;
            }
            else
                AssertMsgFailed(("rc=%#x\n", rc));
        }
    }

    return fHIDModifiers;
}

#endif /* USE_HID_FOR_MODIFIERS */

/**
 * Left / right adjust the modifier mask using the current
 * keyboard state.
 *
 * @returns left/right adjusted fModifiers.
 * @param   fModifiers      The mask to adjust.
 * @param   pvCocoaEvent    The associated Cocoa keyboard event. This is NULL
 *                          when using HID for modifier corrections.
 *
 */
UInt32 DarwinAdjustModifierMask(UInt32 fModifiers, const void *pvCocoaEvent)
{
    /*
     * Check if there is anything to adjust and perform the adjustment.
     */
    if (fModifiers & (shiftKey | rightShiftKey | controlKey | rightControlKey | optionKey | rightOptionKey | cmdKey | kEventKeyModifierRightCmdKeyMask))
    {
#ifndef USE_HID_FOR_MODIFIERS
        /*
         * Convert the Cocoa modifiers to Carbon ones (the Cocoa modifier
         * definitions are tucked away in Objective-C headers, unfortunately).
         *
         * Update: CGEventTypes.h includes what looks like the Cocoa modifiers
         *         and the NX_* defines should be available as well. We should look
         *         into ways to intercept the CG (core graphics) events in the Carbon
         *         based setup and get rid of all this HID mess.
         */
        AssertPtr(pvCocoaEvent);
        //::darwinPrintEvent("dbg-adjMods: ", pvCocoaEvent);
        uint32_t fAltModifiers = ::darwinEventModifierFlagsXlated(pvCocoaEvent);

#else  /* USE_HID_FOR_MODIFIERS */
        /*
         * Update the keyboard cache.
         */
        darwinHIDKeyboardCacheUpdate();
        const UInt32 fAltModifiers = g_fHIDModifierMask;

#endif /* USE_HID_FOR_MODIFIERS */
#ifdef DEBUG_PRINTF
        RTPrintf("dbg-fAltModifiers=%#x fModifiers=%#x", fAltModifiers, fModifiers);
#endif
        if (   (fModifiers    & (rightShiftKey | shiftKey))
            && (fAltModifiers & (rightShiftKey | shiftKey)))
        {
            fModifiers &= ~(rightShiftKey | shiftKey);
            fModifiers |= fAltModifiers & (rightShiftKey | shiftKey);
        }

        if (   (fModifiers    & (rightControlKey | controlKey))
            && (fAltModifiers & (rightControlKey | controlKey)))
        {
            fModifiers &= ~(rightControlKey | controlKey);
            fModifiers |= fAltModifiers & (rightControlKey | controlKey);
        }

        if (   (fModifiers    & (optionKey | rightOptionKey))
            && (fAltModifiers & (optionKey | rightOptionKey)))
        {
            fModifiers &= ~(optionKey | rightOptionKey);
            fModifiers |= fAltModifiers & (optionKey | rightOptionKey);
        }

        if (   (fModifiers    & (cmdKey | kEventKeyModifierRightCmdKeyMask))
            && (fAltModifiers & (cmdKey | kEventKeyModifierRightCmdKeyMask)))
        {
            fModifiers &= ~(cmdKey | kEventKeyModifierRightCmdKeyMask);
            fModifiers |= fAltModifiers & (cmdKey | kEventKeyModifierRightCmdKeyMask);
        }
#ifdef DEBUG_PRINTF
        RTPrintf(" -> %#x\n", fModifiers);
#endif
    }
    return fModifiers;
}


/**
 * Start grabbing keyboard events.
 *
 * This only concerns itself with modifiers and disabling global hotkeys (if requested).
 *
 * @param   fGlobalHotkeys      Whether to disable global hotkeys or not.
 */
void     DarwinGrabKeyboard(bool fGlobalHotkeys)
{
    LogFlow(("DarwinGrabKeyboard: fGlobalHotkeys=%RTbool\n", fGlobalHotkeys));

#ifdef USE_HID_FOR_MODIFIERS
    /*
     * Update the keyboard cache.
     */
    darwinHIDKeyboardCacheUpdate();

    /*
     * Start the keyboard queues and query the current mask.
     */
    g_fHIDQueueEnabled = true;

    unsigned i = g_cKeyboards;
    while (i-- > 0)
    {
        if (g_aKeyboards[i].ppHidQueueInterface)
            (*g_aKeyboards[i].ppHidQueueInterface)->start(g_aKeyboards[i].ppHidQueueInterface);
    }

    g_fHIDModifierMask = darwinQueryHIDModifiers();
#endif /* USE_HID_FOR_MODIFIERS */

    /*
     * Disable hotkeys if requested.
     */
    if (fGlobalHotkeys)
        DarwinDisableGlobalHotKeys(true);
}


/**
 * Reverses the actions taken by DarwinGrabKeyboard.
 */
void     DarwinReleaseKeyboard(void)
{
    LogFlow(("DarwinReleaseKeyboard\n"));

    /*
     * Re-enable hotkeys.
     */
    DarwinDisableGlobalHotKeys(false);

#ifdef USE_HID_FOR_MODIFIERS
    /*
     * Stop and drain the keyboard queues.
     */
    g_fHIDQueueEnabled = false;

#if 0
    unsigned i = g_cKeyboards;
    while (i-- > 0)
    {
        if (g_aKeyboards[i].ppHidQueueInterface)
        {

            (*g_aKeyboards[i].ppHidQueueInterface)->stop(g_aKeyboards[i].ppHidQueueInterface);

            /* drain it */
            IOReturn rc;
            unsigned c = 0;
            do
            {
                IOHIDEventStruct Event;
                AbsoluteTime MaxTime = {0,0};
                rc = (*g_aKeyboards[i].ppHidQueueInterface)->getNextEvent(g_aKeyboards[i].ppHidQueueInterface,
                                                                          &Event, MaxTime, 0);
            } while (   rc == kIOReturnSuccess
                     && c++ < 32);
        }
    }
#else

    /*
     * Kill the keyboard cache.
     * This will hopefully fix the crash in getElementValue()/fillElementValue()...
     */
    darwinHIDKeyboardCacheZap();
#endif

    /* Clear the modifier mask. */
    g_fHIDModifierMask = 0;
#endif /* USE_HID_FOR_MODIFIERS */
}

#ifdef VBOX_WITH_KBD_LEDS_SYNC
/** Prepare dictionary that will be used to match HID LED device(s) while discovering. */
static CFDictionaryRef darwinGetLedDeviceMatchingDictionary()
{
    CFDictionaryRef deviceMatchingDictRef;

    /* Use two (key, value) pairs:
     *      - (kIOHIDDeviceUsagePageKey, kHIDPage_GenericDesktop),
     *      - (kIOHIDDeviceUsageKey,     kHIDUsage_GD_Keyboard). */

    CFNumberRef usagePageKeyCFNumberRef; int usagePageKeyCFNumberValue = kHIDPage_GenericDesktop;
    CFNumberRef usageKeyCFNumberRef;     int usageKeyCFNumberValue     = kHIDUsage_GD_Keyboard;

    usagePageKeyCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePageKeyCFNumberValue);
    if (usagePageKeyCFNumberRef)
    {
        usageKeyCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usageKeyCFNumberValue);
        if (usageKeyCFNumberRef)
        {
            CFStringRef dictionaryKeys[2] = { CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey) };
            CFNumberRef dictionaryVals[2] = { usagePageKeyCFNumberRef,         usageKeyCFNumberRef         };

            deviceMatchingDictRef = CFDictionaryCreate(kCFAllocatorDefault,
                                                       (const void **)dictionaryKeys,
                                                       (const void **)dictionaryVals,
                                                       2, /** two (key, value) pairs */
                                                       &kCFTypeDictionaryKeyCallBacks,
                                                       &kCFTypeDictionaryValueCallBacks);

            CFRelease(usageKeyCFNumberRef);
            CFRelease(usagePageKeyCFNumberRef);

            return deviceMatchingDictRef;
        }

        CFRelease(usagePageKeyCFNumberRef);
    }

    return NULL;
}

/** Prepare dictionary that will be used to match HID LED device element(s) while discovering. */
static CFDictionaryRef darwinGetLedElementMatchingDictionary()
{
    CFDictionaryRef elementMatchingDictRef;

    /* Use only one (key, value) pair to match LED device element:
     *      - (kIOHIDElementUsagePageKey, kHIDPage_LEDs).  */

    CFNumberRef usagePageKeyCFNumberRef; int usagePageKeyCFNumberValue = kHIDPage_LEDs;

    usagePageKeyCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePageKeyCFNumberValue);
    if (usagePageKeyCFNumberRef)
    {
        CFStringRef dictionaryKeys[1] = { CFSTR(kIOHIDElementUsagePageKey), };
        CFNumberRef dictionaryVals[1] = { usagePageKeyCFNumberRef,          };

        elementMatchingDictRef = CFDictionaryCreate(kCFAllocatorDefault,
                                                    (const void **)dictionaryKeys,
                                                    (const void **)dictionaryVals,
                                                    1, /** one (key, value) pair */
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);

        CFRelease(usagePageKeyCFNumberRef);
        return elementMatchingDictRef;
    }

    return NULL;
}

/** Turn ON or OFF a particular LED. */
static int darwinLedElementSetValue(IOHIDDeviceRef hidDevice, IOHIDElementRef element, bool fEnabled)
{
    IOHIDValueRef valueRef;
    IOReturn      rc = kIOReturnError;

    valueRef = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, 0, (fEnabled) ? 1 : 0);
    if (valueRef)
    {
        rc = IOHIDDeviceSetValue(hidDevice, element, valueRef);
        if (rc != kIOReturnSuccess)
            Log2(("Warning! Something went wrong in attempt to turn %s HID device led (error %d)!\n", ((fEnabled) ? "on" : "off"), rc));
        else
            Log2(("Led (%d) is turned %s\n", (int)IOHIDElementGetUsage(element), ((fEnabled) ? "on" : "off")));

        CFRelease(valueRef);
    }

    return rc;
}

/** Get state of a particular led. */
static int darwinLedElementGetValue(IOHIDDeviceRef hidDevice, IOHIDElementRef element, bool *fEnabled)
{
    IOHIDValueRef valueRef;
    IOReturn      rc;
    CFIndex       integerValue;

    rc = IOHIDDeviceGetValue(hidDevice, element, &valueRef);
    if (rc == kIOReturnSuccess)
    {
        integerValue = IOHIDValueGetIntegerValue(valueRef);
        switch (integerValue)
        {
            case 0:
                *fEnabled = false;
                break;
            case 1:
                *fEnabled = true;
                break;
            default:
                rc = kIOReturnError;
        }
    }

    return rc;
}

/** Set corresponding states from NumLock, CapsLock and ScrollLock leds. */
static int darwinSetDeviceLedsState(IOHIDDeviceRef hidDevice, CFDictionaryRef elementMatchingDict,
                                    bool fNumLockOn, bool fCapsLockOn, bool fScrollLockOn)
{
    CFArrayRef matchingElementsArrayRef;
    int        rc2 = 0;

    matchingElementsArrayRef = IOHIDDeviceCopyMatchingElements(hidDevice, elementMatchingDict, 0);
    if (matchingElementsArrayRef)
    {
        CFIndex cElements = CFArrayGetCount(matchingElementsArrayRef);

        /* Cycle though all the elements we found */
        for (CFIndex i = 0; i < cElements; i++)
        {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(matchingElementsArrayRef, i);
            uint32_t        usage   = IOHIDElementGetUsage(element);
            int             rc = 0;

            switch (usage)
            {
                case kHIDUsage_LED_NumLock:
                    rc = darwinLedElementSetValue(hidDevice, element, fNumLockOn);
                    break;

                case kHIDUsage_LED_CapsLock:
                    rc = darwinLedElementSetValue(hidDevice, element, fCapsLockOn);
                    break;

                case kHIDUsage_LED_ScrollLock:
                    rc = darwinLedElementSetValue(hidDevice, element, fScrollLockOn);
                    break;
            }
            if (rc != 0)
            {
                Log2(("Failed to set led (%d) state\n", (int)IOHIDElementGetUsage(element)));
                rc2 = kIOReturnError;
            }

        }
    }

    return rc2;
}

/** Get corresponding states for NumLock, CapsLock and ScrollLock leds. */
static int darwinGetDeviceLedsState(IOHIDDeviceRef hidDevice, CFDictionaryRef elementMatchingDict,
                                    bool *fNumLockOn, bool *fCapsLockOn, bool *fScrollLockOn)
{
    CFArrayRef matchingElementsArrayRef;
    int        rc2 = 0;

    matchingElementsArrayRef = IOHIDDeviceCopyMatchingElements(hidDevice, elementMatchingDict, 0);
    if (matchingElementsArrayRef)
    {
        CFIndex cElements = CFArrayGetCount(matchingElementsArrayRef);

        /* Cycle though all the elements we found */
        for (CFIndex i = 0; i < cElements; i++)
        {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(matchingElementsArrayRef, i);
            uint32_t        usage   = IOHIDElementGetUsage(element);
            int             rc = 0;

            switch (usage)
            {
                case kHIDUsage_LED_NumLock:
                    rc = darwinLedElementGetValue(hidDevice, element, fNumLockOn);
                    break;

                case kHIDUsage_LED_CapsLock:
                    rc = darwinLedElementGetValue(hidDevice, element, fCapsLockOn);
                    break;

                case kHIDUsage_LED_ScrollLock:
                    rc = darwinLedElementGetValue(hidDevice, element, fScrollLockOn);
                    break;
            }
            if (rc != 0)
            {
                Log2(("Failed to get led (%d) state\n", (int)IOHIDElementGetUsage(element)));
                rc2 = kIOReturnError;
            }
        }
    }

    return rc2;
}

/** IOKit key press callback. Triggered before Carbon callback. */
static void darwinHidInputCallback(void *pData, IOReturn unused, void *unused1, IOHIDValueRef valueRef)
{
    (void)unused;
    (void)unused1;

    IOHIDElementRef pElementRef = IOHIDValueGetElement(valueRef);

    if (IOHIDElementGetUsagePage(pElementRef) == kHIDPage_KeyboardOrKeypad)        /* Keyboard or keypad event */
        if (IOHIDValueGetIntegerValue(valueRef) == 1)                              /* key has been pressed down */
            if (IOHIDElementGetUsage(pElementRef) == kHIDUsage_KeyboardCapsLock || /* CapsLock key has been pressed */
                IOHIDElementGetUsage(pElementRef) == kHIDUsage_KeypadNumLock)      /* ... or NumLock key has been pressed */
            {
                Log2(("A modifier key has been pressed\n"));
                g_LastTouchedState = (VBoxHidLeds_t *)pData;
            }
}

/** Carbon key press callback. Triggered after IOKit callback. */
#define VBOX_BOOL_TO_STR_STATE(x) (x) ? "ON" : "OFF"
static CGEventRef darwinCarbonGlobalKeyPressCallback(CGEventTapProxy unused, CGEventType type, CGEventRef pEventRef, void *unused1)
{
    (void)unused;
    (void)unused1;

    /* Skip events we are not interested in. */
    if (type != kCGEventKeyDown && type != kCGEventFlagsChanged)
        return pEventRef;

    CGEventFlags fMask = CGEventGetFlags(pEventRef);
    bool         fCaps = (bool)(fMask & NX_ALPHASHIFTMASK);
    bool         fNum  = (bool)(fMask & NX_NUMERICPADMASK);
    CGKeyCode    key   = CGEventGetIntegerValueField(pEventRef, kCGKeyboardEventKeycode);

    if (key == kHIDUsage_KeyboardCapsLock ||
        key == kHIDUsage_KeypadNumLock)
    {
        Log2(("carbon event: caps=%s, num=%s\n", VBOX_BOOL_TO_STR_STATE(fCaps), VBOX_BOOL_TO_STR_STATE(fNum)));
        if (g_LastTouchedState)
        {
            g_LastTouchedState->fCapsLockOn = fCaps;
            g_LastTouchedState->fNumLockOn  = fNum;

            /* Forget device */
            g_LastTouchedState = NULL;
        }
    }

    return pEventRef;
}
#undef VBOX_BOOL_TO_STR_STATE

/** Register Carbon key press callback. */
static int darwinAddCarbonGlobalKeyPressHandler(VBoxHidsState_t *pState)
{
    CFMachPortRef pTapRef;
    CGEventMask   fMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);

    g_LastTouchedState = NULL;

    pTapRef = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, fMask, darwinCarbonGlobalKeyPressCallback, NULL);
    if (pTapRef)
    {
        CFRunLoopSourceRef pLoopSourceRef;
        pLoopSourceRef = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, pTapRef, 0);
        if (pLoopSourceRef)
        {
            CFRunLoopAddSource(CFRunLoopGetCurrent(), pLoopSourceRef, kCFRunLoopCommonModes);
            CGEventTapEnable(pTapRef, true);

            pState->pTapRef = pTapRef;
            pState->pLoopSourceRef = pLoopSourceRef;

            return 0;
        }
        else
            Log2(("Unable to create a loop source\n"));

        CFRelease(pTapRef);
    }
    else
        Log2(("Unable to create an event tap\n"));

    return kIOReturnError;
}

/** Remove Carbon key press callback. */
static void darwinRemoveCarbonGlobalKeyPressHandler(VBoxHidsState_t *pState)
{
    AssertReturnVoid(pState);
    AssertReturnVoid(pState->pTapRef);
    AssertReturnVoid(pState->pLoopSourceRef);

    g_LastTouchedState = NULL;

    CGEventTapEnable(pState->pTapRef, false);
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), pState->pLoopSourceRef, kCFRunLoopCommonModes);
    CFRelease(pState->pLoopSourceRef);
}

/** Some keyboard devices might freeze after LEDs manipulation. We filter out such devices here.
 * In the list below, devices which verified to be stable against LEDs manipulation.
 * If you want to add new device, add it here. Currently, we only filter devices by Vendor ID.
 * In future it might make sense to take Product ID into account as well. */
static bool darwinHidDeviceSupported(IOHIDDeviceRef pHidDeviceRef)
{
    bool      fSupported = false;
    CFTypeRef pNumberRef;
    uint32_t  vendorId = 0;

    pNumberRef = IOHIDDeviceGetProperty(pHidDeviceRef, CFSTR(kIOHIDVendorIDKey));
    if (pNumberRef)
    {
        if (CFGetTypeID(pNumberRef) == CFNumberGetTypeID())
        {
            if (CFNumberGetValue((CFNumberRef)pNumberRef, kCFNumberSInt32Type, &vendorId))
            {
                switch (vendorId)
                {
                    case kIOUSBVendorIDAppleComputer:   /** Apple devices always in the list */
                    case 0x03F0:                        /** Hewlett-Packard (verified with model KU-0316) */
                        fSupported = true;
                        break;
                }

                Log2(("HID device Vendor ID 0x%X %s in the list of supported devices.\n", vendorId, (fSupported ? "is" : "is not")));
            }
        }
    }

    return fSupported;
}

#endif // !VBOX_WITH_KBD_LEDS_SYNC

/** Save the states of leds for all HID devices attached to the system and return it. */
void * DarwinHidDevicesKeepLedsState(void)
{
#ifdef VBOX_WITH_KBD_LEDS_SYNC
    IOReturn         rc;
    VBoxHidsState_t *hidsState;

    hidsState = (VBoxHidsState_t *)RTMemAllocZ(sizeof(VBoxHidsState_t));
    if (hidsState)
    {
        hidsState->hidManagerRef = IOHIDManagerCreate(kCFAllocatorDefault, 0);
        if (hidsState->hidManagerRef)
        {

            CFDictionaryRef deviceMatchingDictRef = darwinGetLedDeviceMatchingDictionary();
            if (deviceMatchingDictRef)
            {
                IOHIDManagerSetDeviceMatching(hidsState->hidManagerRef, deviceMatchingDictRef);

                rc = IOHIDManagerOpen(hidsState->hidManagerRef, kIOHIDOptionsTypeNone);
                if (rc == kIOReturnSuccess)
                {
                    CFSetRef hidDevicesSetRef;

                    hidDevicesSetRef = IOHIDManagerCopyDevices(hidsState->hidManagerRef);
                    if (hidDevicesSetRef)
                    {
                        /* Get all the available devices and cycle through them. */
                        hidsState->cDevices = CFSetGetCount(hidDevicesSetRef);
                        hidsState->hidDevicesCollection = (IOHIDDeviceRef *)RTMemAllocZ((size_t)hidsState->cDevices * sizeof(IOHIDDeviceRef));
                        if (hidsState->hidDevicesCollection)
                        {
                            hidsState->hidLedsCollection = (VBoxHidLeds_t *)RTMemAllocZ((size_t)hidsState->cDevices * sizeof(VBoxHidLeds_t));
                            if (hidsState->hidLedsCollection)
                            {
                                CFSetGetValues(hidDevicesSetRef, (const void **)hidsState->hidDevicesCollection);

                                CFDictionaryRef elementMatchingDict = darwinGetLedElementMatchingDictionary();
                                if (elementMatchingDict)
                                {
                                    for (CFIndex i = 0; i < hidsState->cDevices; i++)
                                    {
                                        if (IOHIDDeviceConformsTo(hidsState->hidDevicesCollection[i], kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard)
                                         && darwinHidDeviceSupported(hidsState->hidDevicesCollection[i]))
                                        {
                                            rc = darwinGetDeviceLedsState(hidsState->hidDevicesCollection[i],
                                                                          elementMatchingDict,
                                                                          &hidsState->hidLedsCollection[i].fNumLockOn,
                                                                          &hidsState->hidLedsCollection[i].fCapsLockOn,
                                                                          &hidsState->hidLedsCollection[i].fScrollLockOn);
                                            /* This should never happen, but if happened -- mark all the leds of current
                                             * device as turned OFF. */
                                            if (rc != 0)
                                            {
                                                Log2(("Unable to get leds state for device %d. Mark leds as turned off\n", (int)i));
                                                hidsState->hidLedsCollection[i].fNumLockOn    =
                                                hidsState->hidLedsCollection[i].fCapsLockOn   =
                                                hidsState->hidLedsCollection[i].fScrollLockOn = false;
                                            }

                                            /* Register per-device input callback */
                                            IOHIDDeviceRegisterInputValueCallback(hidsState->hidDevicesCollection[i], darwinHidInputCallback, (void *)&hidsState->hidLedsCollection[i]);
                                            IOHIDDeviceScheduleWithRunLoop(hidsState->hidDevicesCollection[i], CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                                        }
                                    }

                                    CFRelease(elementMatchingDict);

                                    if (darwinAddCarbonGlobalKeyPressHandler(hidsState) == 0)
                                    {
                                        CFRelease(deviceMatchingDictRef);
                                        return hidsState;
                                    }
                                }

                                RTMemFree(hidsState->hidLedsCollection);
                            }

                            RTMemFree(hidsState->hidDevicesCollection);
                        }
                    }

                    rc = IOHIDManagerClose(hidsState->hidManagerRef, 0);
                    if (rc != kIOReturnSuccess)
                        Log2(("Warning! Something went wrong in attempt to close HID device manager!\n"));
                }

                CFRelease(deviceMatchingDictRef);
            }

            CFRelease(hidsState->hidManagerRef);
        }

        RTMemFree(hidsState);
    }

    return NULL;
#else // !VBOX_WITH_KBD_LEDS_SYNC
    return NULL;
#endif
}

/**
 * Apply LEDs state stored in *pState and release resources aquired by *pState.
 *
 * @param pState            Pointer to saved LEDs state
 *
 * @return 0 on success, error code otherwise.
 */
int DarwinHidDevicesApplyAndReleaseLedsState(void *pState)
{
#ifdef VBOX_WITH_KBD_LEDS_SYNC
    VBoxHidsState_t *hidsState = (VBoxHidsState_t *)pState;

    CFIndex     i;
    IOReturn    rc, rc2 = 0;

    if (hidsState)
    {
        /* Need to unregister Carbon stuff first */
        darwinRemoveCarbonGlobalKeyPressHandler(hidsState);

        CFDictionaryRef elementMatchingDict = darwinGetLedElementMatchingDictionary();
        if (elementMatchingDict)
        {
            /* Restore LEDs */
            for (i = 0; i < hidsState->cDevices; i++)
            {
                /* Cycle through supported devices only. */
                if (darwinHidDeviceSupported(hidsState->hidDevicesCollection[i]))
                {
                    rc = darwinSetDeviceLedsState(hidsState->hidDevicesCollection[i],
                                                  elementMatchingDict,
                                                  hidsState->hidLedsCollection[i].fNumLockOn,
                                                  hidsState->hidLedsCollection[i].fCapsLockOn,
                                                  hidsState->hidLedsCollection[i].fScrollLockOn);
                    if (rc != 0)
                    {
                        Log2(("Unable to restore led states for device (%d)!\n", (int)i));
                        rc2 = kIOReturnError;
                    }

                    IOHIDDeviceRegisterInputValueCallback(hidsState->hidDevicesCollection[i], NULL, NULL);
                    IOHIDDeviceUnscheduleFromRunLoop(hidsState->hidDevicesCollection[i], CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                }
            }
        }

        /* Free resources */

        RTMemFree(hidsState->hidLedsCollection);
        RTMemFree(hidsState->hidDevicesCollection);

        rc = IOHIDManagerClose(hidsState->hidManagerRef, 0);
        if (rc != kIOReturnSuccess)
            Log2(("Warning! Something went wrong in attempt to close HID device manager!\n"));

        CFRelease(hidsState->hidManagerRef);

        RTMemFree(hidsState);
    }

    return rc2;
#else // !VBOX_WITH_KBD_LEDS_SYNC
    (void)pState;
    return 0;
#endif
}

/**
 * Set states for host keyboard LEDs.
 *
 * NOTE: This function will set led values for all
 * keyboard devices attached to the system.
 *
 * @param fNumLockOn        Turn on NumLock led if TRUE, turn off otherwise
 * @param fCapsLockOn       Turn on CapsLock led if TRUE, turn off otherwise
 * @param fScrollLockOn     Turn on ScrollLock led if TRUE, turn off otherwise
 */
void DarwinHidDevicesBroadcastLeds(bool fNumLockOn, bool fCapsLockOn, bool fScrollLockOn)
{
/* Temporary disabled */
#ifdef VBOX_WITH_KBD_LEDS_SYNC
    IOReturn        rc;
    IOHIDManagerRef hidManagerRef;

    hidManagerRef = IOHIDManagerCreate(kCFAllocatorDefault, 0);
    if (hidManagerRef)
    {
        CFDictionaryRef deviceMatchingDictRef = darwinGetLedDeviceMatchingDictionary();
        if (deviceMatchingDictRef)
        {
            IOHIDManagerSetDeviceMatching(hidManagerRef, deviceMatchingDictRef);

            rc = IOHIDManagerOpen(hidManagerRef, kIOHIDOptionsTypeNone);
            if (rc == kIOReturnSuccess)
            {
                CFSetRef hidDevicesSetRef;

                hidDevicesSetRef = IOHIDManagerCopyDevices(hidManagerRef);
                if (hidDevicesSetRef)
                {
                    /* Get all the available devices and cycle through them. */
                    CFIndex cDevices = CFSetGetCount(hidDevicesSetRef);
                    IOHIDDeviceRef *hidDevicesCollection = (IOHIDDeviceRef *)RTMemAllocZ((size_t)cDevices * sizeof(IOHIDDeviceRef));
                    if (hidDevicesCollection)
                    {
                        CFSetGetValues(hidDevicesSetRef, (const void **)hidDevicesCollection);

                        CFDictionaryRef elementMatchingDict = darwinGetLedElementMatchingDictionary();
                        if (elementMatchingDict)
                        {
                            Log2(("Start LEDs broadcast\n"));

                            for (CFIndex i = 0; i < cDevices; i++)
                            {
                                if (IOHIDDeviceConformsTo(hidDevicesCollection[i], kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard)
                                 && darwinHidDeviceSupported(hidDevicesCollection[i]))
                                {
                                    rc = darwinSetDeviceLedsState(hidDevicesCollection[i], elementMatchingDict,
                                                                  fNumLockOn, fCapsLockOn, fScrollLockOn);
                                    if (rc != 0)
                                        Log2(("Unable to broadcast LEDs state to device (%d)\n", (int)i));
                                }
                            }

                            Log2(("LEDs broadcast completed\n"));
                            CFRelease(elementMatchingDict);
                        }

                        RTMemFree(hidDevicesCollection);
                    }
                }

                rc = IOHIDManagerClose(hidManagerRef, 0);
                if (rc != kIOReturnSuccess)
                    Log2(("Warning! Something went wrong in attempt to close HID device manager!\n"));
            }

            CFRelease(deviceMatchingDictRef);
        }

        CFRelease(hidManagerRef);
    }
#else // !VBOX_WITH_KBD_LEDS_SYNC

    (void)fNumLockOn;
    (void)fCapsLockOn;
    (void)fScrollLockOn;
#endif
}
