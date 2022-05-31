/** @file
 * MS COM / XPCOM Abstraction Layer:
 * Common definitions
 */

/*
 * Copyright (C) 2006-2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___VBox_com_defs_h
#define ___VBox_com_defs_h

/* Make sure all the stdint.h macros are included - must come first! */
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

#if defined (RT_OS_OS2)

#if defined(RT_MAX) && RT_MAX != 22
# error RT_MAX already defined by <iprt/cdefs.h>! Make sure <VBox/com/defs.h> \
        is included before it.
#endif

/* Make sure OS/2 Toolkit headers are pulled in to have BOOL/ULONG/etc. typedefs
 * already defined in order to be able to redefine them using #define. It's
 * also important to do it before iprt/cdefs.h, otherwise we'll lose RT_MAX in
 * all code that uses COM Glue. */
#define INCL_BASE
#define INCL_PM
#include <os2.h>

/* OS/2 Toolkit defines TRUE and FALSE */
#undef FALSE
#undef TRUE

#endif /* defined (RT_OS_OS2) */

/* Include iprt/types.h (which also includes iprt/types.h) now to make sure iprt
 * gets to stdint.h first, otherwise a system/xpcom header might beat us and
 * we'll be without the macros that are optional in C++. */
#include <iprt/types.h>

#if !defined (VBOX_WITH_XPCOM)

#if defined (RT_OS_WINDOWS)

// Windows COM
/////////////////////////////////////////////////////////////////////////////

#include <objbase.h>
#ifndef VBOX_COM_NO_ATL
# include <atlbase.h>
#include <atlcom.h>
#endif

#define NS_DECL_ISUPPORTS
#define NS_IMPL_ISUPPORTS1_CI(a, b)

/* these are XPCOM only, one for every interface implemented */
#define NS_DECL_ISUPPORTS
#define NS_DECL_IVIRTUALBOX
#define NS_DECL_IMACHINECOLLECTION
#define NS_DECL_IMACHINE

/** Returns @c true if @a rc represents a warning result code */
#define SUCCEEDED_WARNING(rc)   (SUCCEEDED (rc) && (rc) != S_OK)

/** Immutable BSTR string */
typedef const OLECHAR *CBSTR;

/** Input BSTR argument of interface method declaration. */
#define IN_BSTR BSTR

/** Input GUID argument of interface method declaration. */
#define IN_GUID GUID
/** Output GUID argument of interface method declaration. */
#define OUT_GUID GUID*

/** Makes the name of the getter interface function (n must be capitalized). */
#define COMGETTER(n)    get_##n
/** Makes the name of the setter interface function (n must be capitalized). */
#define COMSETTER(n)    put_##n

/**
 * Declares an input safearray parameter in the COM method implementation. Also
 * used to declare the COM attribute setter parameter. Corresponds to either of
 * the following XIDL definitions:
 * <pre>
 *  <param name="arg" ... dir="in" safearray="yes"/>
 *  ...
 *  <attribute name="arg" ... safearray="yes"/>
 * </pre>
 *
 * The method implementation should use the com::SafeArray helper class to work
 * with parameters declared using this define.
 *
 * @param aType Array element type.
 * @param aArg  Parameter/attribute name.
 */
#define ComSafeArrayIn(aType, aArg)     SAFEARRAY **aArg

/**
 * Expands to @true if the given input safearray parameter is a "null pointer"
 * which makes it impossible to use it for reading safearray data.
 */
#define ComSafeArrayInIsNull(aArg)      ((aArg) == NULL || *(aArg) == NULL)

/**
 * Wraps the given parameter name to generate an expression that is suitable for
 * passing the parameter to functions that take input safearray parameters
 * declared using the ComSafeArrayIn marco.
 *
 * @param aArg  Parameter name to wrap. The given parameter must be declared
 *              within the calling function using the ComSafeArrayIn macro.
 */
#define ComSafeArrayInArg(aArg)         aArg

/**
 * Declares an output safearray parameter in the COM method implementation. Also
 * used to declare the COM attribute getter parameter. Corresponds to either of
 * the following XIDL definitions:
 * <pre>
 *  <param name="arg" ... dir="out" safearray="yes"/>
 *  <param name="arg" ... dir="return" safearray="yes"/>
 *  ...
 *  <attribute name="arg" ... safearray="yes"/>
 * </pre>
 *
 * The method implementation should use the com::SafeArray helper class to work
 * with parameters declared using this define.
 *
 * @param aType Array element type.
 * @param aArg  Parameter/attribute name.
 */
#define ComSafeArrayOut(aType, aArg)    SAFEARRAY **aArg

/**
 * Expands to @true if the given output safearray parameter is a "null pointer"
 * which makes it impossible to use it for returning a safearray.
 */
#define ComSafeArrayOutIsNull(aArg)     ((aArg) == NULL)

/**
 * Wraps the given parameter name to generate an expression that is suitable for
 * passing the parameter to functions that take output safearray parameters
 * declared using the ComSafeArrayOut marco.
 *
 * @param aArg  Parameter name to wrap. The given parameter must be declared
 *              within the calling function using the ComSafeArrayOut macro.
 */
#define ComSafeArrayOutArg(aArg)        aArg

/**
 * Version of ComSafeArrayIn for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayIn(aArg)        SAFEARRAY **aArg

/**
 * Version of ComSafeArrayInIsNull for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayInIsNull(aArg)  ComSafeArrayInIsNull (aArg)

/**
 * Version of ComSafeArrayInArg for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayInArg(aArg)     ComSafeArrayInArg (aArg)

/**
 * Version of ComSafeArrayOut for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayOut(aArg)       SAFEARRAY **aArg

/**
 * Version of ComSafeArrayOutIsNull for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayOutIsNull(aArg) ComSafeArrayOutIsNull (aArg)

/**
 * Version of ComSafeArrayOutArg for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayOutArg(aArg)    ComSafeArrayOutArg (aArg)

/**
 *  Returns the const reference to the IID (i.e., |const GUID &|) of the given
 *  interface.
 *
 *  @param i    interface class
 */
#define COM_IIDOF(I) _ATL_IIDOF (I)

#else /* defined (RT_OS_WINDOWS) */

#error "VBOX_WITH_XPCOM must be defined on a platform other than Windows!"

#endif /* defined (RT_OS_WINDOWS) */

#else /* !defined (VBOX_WITH_XPCOM) */

// XPCOM
/////////////////////////////////////////////////////////////////////////////

#if defined (RT_OS_DARWIN) || (defined (QT_VERSION) && (QT_VERSION >= 0x040000))
  /* CFBase.h defines these &
   * qglobal.h from Qt4 defines these */
# undef FALSE
# undef TRUE
#endif  /* RT_OS_DARWIN || QT_VERSION */

#include <nsID.h>

#define ATL_NO_VTABLE
#define DECLARE_CLASSFACTORY(a)
#define DECLARE_CLASSFACTORY_SINGLETON(a)
#define DECLARE_REGISTRY_RESOURCEID(a)
#define DECLARE_NOT_AGGREGATABLE(a)
#define DECLARE_PROTECT_FINAL_CONSTRUCT(a)
#define BEGIN_COM_MAP(a)
#define COM_INTERFACE_ENTRY(a)
#define COM_INTERFACE_ENTRY2(a,b)
#define END_COM_MAP(a)

#define HRESULT     nsresult
#define SUCCEEDED   NS_SUCCEEDED
#define FAILED      NS_FAILED

#define SUCCEEDED_WARNING(rc)   (NS_SUCCEEDED (rc) && (rc) != NS_OK)

#define IUnknown nsISupports

#define BOOL    PRBool
#define BYTE    PRUint8
#define SHORT   PRInt16
#define USHORT  PRUint16
#define LONG    PRInt32
#define ULONG   PRUint32
#define LONG64  PRInt64
#define ULONG64 PRUint64

#define FALSE   PR_FALSE
#define TRUE    PR_TRUE

#define OLECHAR wchar_t

/* note: typedef to semantically match BSTR on Win32 */
typedef PRUnichar *BSTR;
typedef const PRUnichar *CBSTR;
typedef BSTR *LPBSTR;

/** Input BSTR argument the interface method declaration. */
#define IN_BSTR CBSTR

/**
 * Type to define a raw GUID variable (for members use the com::Guid class
 * instead).
 */
#define GUID        nsID
/** Input GUID argument the interface method declaration. */
#define IN_GUID     const nsID &
/** Output GUID argument the interface method declaration. */
#define OUT_GUID    nsID **

/** Makes the name of the getter interface function (n must be capitalized). */
#define COMGETTER(n)    Get##n
/** Makes the name of the setter interface function (n must be capitalized). */
#define COMSETTER(n)    Set##n

/* safearray input parameter macros */
#define ComSafeArrayIn(aType, aArg)         PRUint32 aArg##Size, aType *aArg
#define ComSafeArrayInIsNull(aArg)          ((aArg) == NULL)
#define ComSafeArrayInArg(aArg)             aArg##Size, aArg

/* safearray output parameter macros */
#define ComSafeArrayOut(aType, aArg)        PRUint32 *aArg##Size, aType **aArg
#define ComSafeArrayOutIsNull(aArg)         ((aArg) == NULL)
#define ComSafeArrayOutArg(aArg)            aArg##Size, aArg

/* safearray input parameter macros for GUID */
#define ComSafeGUIDArrayIn(aArg)            PRUint32 aArg##Size, const nsID **aArg
#define ComSafeGUIDArrayInIsNull(aArg)      ComSafeArrayInIsNull (aArg)
#define ComSafeGUIDArrayInArg(aArg)         ComSafeArrayInArg (aArg)

/* safearray output parameter macros for GUID */
#define ComSafeGUIDArrayOut(aArg)           PRUint32 *aArg##Size, nsID ***aArg
#define ComSafeGUIDArrayOutIsNull(aArg)     ComSafeArrayOutIsNull (aArg)
#define ComSafeGUIDArrayOutArg(aArg)        ComSafeArrayOutArg (aArg)

/* CLSID and IID for compatibility with Win32 */
typedef nsCID   CLSID;
typedef nsIID   IID;

/* OLE error codes */
#define S_OK                ((nsresult) NS_OK)
#define E_UNEXPECTED        NS_ERROR_UNEXPECTED
#define E_NOTIMPL           NS_ERROR_NOT_IMPLEMENTED
#define E_OUTOFMEMORY       NS_ERROR_OUT_OF_MEMORY
#define E_INVALIDARG        NS_ERROR_INVALID_ARG
#define E_NOINTERFACE       NS_ERROR_NO_INTERFACE
#define E_POINTER           NS_ERROR_NULL_POINTER
#define E_ABORT             NS_ERROR_ABORT
#define E_FAIL              NS_ERROR_FAILURE
/* Note: a better analog for E_ACCESSDENIED would probably be
 * NS_ERROR_NOT_AVAILABLE, but we want binary compatibility for now. */
#define E_ACCESSDENIED      ((nsresult) 0x80070005L)

#define STDMETHOD(a) NS_IMETHOD a
#define STDMETHODIMP NS_IMETHODIMP

#define COM_IIDOF(I) NS_GET_IID (I)

/* Two very simple ATL emulator classes to provide
 * FinalConstruct()/FinalRelease() functionality on Linux. */

class CComObjectRootEx
{
public:
    HRESULT FinalConstruct() { return S_OK; }
    void FinalRelease() {}
};

template <class Base> class CComObject : public Base
{
public:
    virtual ~CComObject() { this->FinalRelease(); }
};

/* helper functions */
extern "C"
{
BSTR SysAllocString (const OLECHAR* sz);
BSTR SysAllocStringByteLen (char *psz, unsigned int len);
BSTR SysAllocStringLen (const OLECHAR *pch, unsigned int cch);
void SysFreeString (BSTR bstr);
int SysReAllocString (BSTR *pbstr, const OLECHAR *psz);
int SysReAllocStringLen (BSTR *pbstr, const OLECHAR *psz, unsigned int cch);
unsigned int SysStringByteLen (BSTR bstr);
unsigned int SysStringLen (BSTR bstr);
}

/**
 *  'Constructor' for the component class.
 *  This constructor, as opposed to NS_GENERIC_FACTORY_CONSTRUCTOR,
 *  assumes that the component class is derived from the CComObjectRootEx<>
 *  template, so it calls FinalConstruct() right after object creation
 *  and ensures that FinalRelease() will be called right before destruction.
 *  The result from FinalConstruct() is returned to the caller.
 */
#define NS_GENERIC_FACTORY_CONSTRUCTOR_WITH_RC(_InstanceClass)                \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    CComObject <_InstanceClass> *inst = new CComObject <_InstanceClass>();    \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_ADDREF(inst); /* protect FinalConstruct() */                           \
    rv = inst->FinalConstruct();                                              \
    if (NS_SUCCEEDED(rv))                                                     \
        rv = inst->QueryInterface(aIID, aResult);                             \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}

/**
 *  'Constructor' that uses an existing getter function that gets a singleton.
 *  The getter function must have the following prototype:
 *      nsresult _GetterProc (_InstanceClass **inst)
 *  This constructor, as opposed to NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR,
 *  lets the getter function return a result code that is passed back to the
 *  caller that tries to instantiate the object.
 *  NOTE: assumes that getter does an AddRef - so additional AddRef is not done.
 */
#define NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR_WITH_RC(_InstanceClass, _GetterProc) \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    rv = _GetterProc(&inst);                                                  \
    if (NS_FAILED(rv))                                                        \
        return rv;                                                            \
                                                                              \
    /* sanity check */                                                        \
    if (NULL == inst)                                                         \
        return NS_ERROR_OUT_OF_MEMORY;                                        \
                                                                              \
    /* NS_ADDREF(inst); */                                                    \
    if (NS_SUCCEEDED(rv)) {                                                   \
        rv = inst->QueryInterface(aIID, aResult);                             \
    }                                                                         \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}

#endif /* !defined (VBOX_WITH_XPCOM) */

/**
 *  Declares a wchar_t string literal from the argument.
 *  Necessary to overcome MSC / GCC differences.
 *  @param s    expression to stringify
 */
#if defined (_MSC_VER)
#   define WSTR_LITERAL(s)  L#s
#elif defined (__GNUC__)
#   define WSTR_LITERAL(s)  L""#s
#else
#   error "Unsupported compiler!"
#endif

namespace com
{

/**
 * "First worst" result type.
 *
 * Variables of this class are used instead of HRESULT variables when it is
 * desirable to memorize the "first worst" result code instead of the last
 * assigned one. In other words, an assignment operation to a variable of this
 * class will succeed only if the result code to assign has worse severity. The
 * following table demonstrate this (the first column lists the previous result
 * code stored in the variable, the first row lists the new result code being
 * assigned, 'A' means the assignment will take place, '> S_OK' means a warning
 * result code):
 *
 * {{{
 *             FAILED    > S_OK    S_OK
 * FAILED        -         -         -
 * > S_OK        A         -         -
 * S_OK          A         A         -
 *
 * }}}
 *
 * In practice, you will need to use a FWResult variable when you call some COM
 * method B after another COM method A fails and want to return the result code
 * of A even if B also fails, but want to return the failed result code of B if
 * A issues a warning or succeeds.
 */
class FWResult
{

public:

    /**
     * Constructs a new variable. Note that by default this constructor sets the
     * result code to E_FAIL to make sure a failure is returned to the caller if
     * the variable is never assigned another value (which is considered as the
     * improper use of this class).
     */
    FWResult (HRESULT aRC = E_FAIL) : mRC (aRC) {}

    FWResult &operator= (HRESULT aRC)
    {
        if ((FAILED (aRC) && !FAILED (mRC)) ||
            (mRC == S_OK && aRC != S_OK))
            mRC = aRC;

        return *this;
    }

    operator HRESULT() const { return mRC; }

    HRESULT *operator&() { return &mRC; }

private:

    HRESULT mRC;
};

/**
 * "Last worst" result type.
 *
 * Variables of this class are used instead of HRESULT variables when it is
 * desirable to memorize the "last worst" result code instead of the last
 * assigned one. In other words, an assignment operation to a variable of this
 * class will succeed only if the result code to assign has the same or worse
 * severity. The following table demonstrate this (the first column lists the
 * previous result code stored in the variable, the first row lists the new
 * assigned, 'A' means the assignment will take place, '> S_OK' means a warning
 * result code):
 *
 * {{{
 *             FAILED    > S_OK    S_OK
 * FAILED        A         -         -
 * > S_OK        A         A         -
 * S_OK          A         A         -
 *
 * }}}
 *
 * In practice, you will need to use a LWResult variable when you call some COM
 * method B after COM method A fails and want to return the result code of B
 * if B also fails, but still want to return the failed result code of A if B
 * issues a warning or succeeds.
 */
class LWResult
{

public:

    /**
     * Constructs a new variable. Note that by default this constructor sets the
     * result code to E_FAIL to make sure a failure is returned to the caller if
     * the variable is never assigned another value (which is considered as the
     * improper use of this class).
     */
    LWResult (HRESULT aRC = E_FAIL) : mRC (aRC) {}

    LWResult &operator= (HRESULT aRC)
    {
        if (FAILED (aRC) ||
            (SUCCEEDED (mRC) && aRC != S_OK))
            mRC = aRC;

        return *this;
    }

    operator HRESULT() const { return mRC; }

    HRESULT *operator&() { return &mRC; }

private:

    HRESULT mRC;
};

// use this macro to implement scriptable interfaces
#ifdef RT_OS_WINDOWS
#define VBOX_SCRIPTABLE_IMPL(iface)                                          \
    public IDispatchImpl<iface, &IID_##iface, &LIBID_VirtualBox,             \
                         kTypeLibraryMajorVersion, kTypeLibraryMinorVersion>
#else
#define VBOX_SCRIPTABLE_IMPL(iface) \
    public iface
#endif


} /* namespace com */

#endif /* ___VBox_com_defs_h */
