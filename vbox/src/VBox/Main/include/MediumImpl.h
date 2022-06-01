/* $Id: MediumImpl.h 27831 2010-03-30 14:55:27Z vboxsync $ */
/** @file
 *
 * VirtualBox COM class implementation
 */

/*
 * Copyright (C) 2008-2010 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ____H_MEDIUMIMPL
#define ____H_MEDIUMIMPL

#include "VirtualBoxBase.h"

class Progress;

namespace settings
{
    struct Medium;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Medium component class for all media types.
 */
class ATL_NO_VTABLE Medium :
    public VirtualBoxBase,
    public com::SupportErrorInfoImpl<Medium, IMedium>,
    public VirtualBoxSupportTranslation<Medium>,
    VBOX_SCRIPTABLE_IMPL(IMedium)
{
public:
    class MergeChain;
    class ImageChain;

    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(Medium)

    DECLARE_NOT_AGGREGATABLE(Medium)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(Medium)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IMedium)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    DECLARE_EMPTY_CTOR_DTOR(Medium)

    HRESULT FinalConstruct();
    void FinalRelease();

    enum HDDOpenMode  { OpenReadWrite, OpenReadOnly };
                // have to use a special enum for the overloaded init() below;
                // can't use AccessMode_T from XIDL because that's mapped to an int
                // and would be ambiguous

    // public initializer/uninitializer for internal purposes only
    HRESULT init(VirtualBox *aVirtualBox,
                 CBSTR aFormat,
                 CBSTR aLocation,
                 bool *pfNeedsSaveSettings);
    HRESULT init(VirtualBox *aVirtualBox,
                 CBSTR aLocation,
                 HDDOpenMode enOpenMode,
                 DeviceType_T aDeviceType,
                 BOOL aSetImageId,
                 const Guid &aImageId,
                 BOOL aSetParentId,
                 const Guid &aParentId);
    // initializer used when loading settings
    HRESULT init(VirtualBox *aVirtualBox,
                 Medium *aParent,
                 DeviceType_T aDeviceType,
                 const settings::Medium &data);
    // initializer for host floppy/DVD
    HRESULT init(VirtualBox *aVirtualBox,
                 DeviceType_T aDeviceType,
                 CBSTR aLocation,
                 CBSTR aDescription = NULL);
    void uninit();

    void deparent();

    // IMedium properties
    STDMETHOD(COMGETTER(Id))(BSTR *aId);
    STDMETHOD(COMGETTER(Description))(BSTR *aDescription);
    STDMETHOD(COMSETTER(Description))(IN_BSTR aDescription);
    STDMETHOD(COMGETTER(State))(MediumState_T *aState);
    STDMETHOD(COMGETTER(Location))(BSTR *aLocation);
    STDMETHOD(COMSETTER(Location))(IN_BSTR aLocation);
    STDMETHOD(COMGETTER(Name))(BSTR *aName);
    STDMETHOD(COMGETTER(DeviceType))(DeviceType_T *aDeviceType);
    STDMETHOD(COMGETTER(HostDrive))(BOOL *aHostDrive);
    STDMETHOD(COMGETTER(Size))(ULONG64 *aSize);
    STDMETHOD(COMGETTER(Format))(BSTR *aFormat);
    STDMETHOD(COMGETTER(Type))(MediumType_T *aType);
    STDMETHOD(COMSETTER(Type))(MediumType_T aType);
    STDMETHOD(COMGETTER(Parent))(IMedium **aParent);
    STDMETHOD(COMGETTER(Children))(ComSafeArrayOut(IMedium *, aChildren));
    STDMETHOD(COMGETTER(Base))(IMedium **aBase);
    STDMETHOD(COMGETTER(ReadOnly))(BOOL *aReadOnly);
    STDMETHOD(COMGETTER(LogicalSize))(ULONG64 *aLogicalSize);
    STDMETHOD(COMGETTER(AutoReset))(BOOL *aAutoReset);
    STDMETHOD(COMSETTER(AutoReset))(BOOL aAutoReset);
    STDMETHOD(COMGETTER(LastAccessError))(BSTR *aLastAccessError);
    STDMETHOD(COMGETTER(MachineIds))(ComSafeArrayOut(BSTR, aMachineIds));

    // IMedium methods
    STDMETHOD(RefreshState)(MediumState_T *aState);
    STDMETHOD(GetSnapshotIds)(IN_BSTR aMachineId,
                              ComSafeArrayOut(BSTR, aSnapshotIds));
    STDMETHOD(LockRead)(MediumState_T *aState);
    STDMETHOD(UnlockRead)(MediumState_T *aState);
    STDMETHOD(LockWrite)(MediumState_T *aState);
    STDMETHOD(UnlockWrite)(MediumState_T *aState);
    STDMETHOD(Close)();
    STDMETHOD(GetProperty)(IN_BSTR aName, BSTR *aValue);
    STDMETHOD(SetProperty)(IN_BSTR aName, IN_BSTR aValue);
    STDMETHOD(GetProperties)(IN_BSTR aNames,
                             ComSafeArrayOut(BSTR, aReturnNames),
                             ComSafeArrayOut(BSTR, aReturnValues));
    STDMETHOD(SetProperties)(ComSafeArrayIn(IN_BSTR, aNames),
                             ComSafeArrayIn(IN_BSTR, aValues));
    STDMETHOD(CreateBaseStorage)(ULONG64 aLogicalSize,
                                 MediumVariant_T aVariant,
                                 IProgress **aProgress);
    STDMETHOD(DeleteStorage)(IProgress **aProgress);
    STDMETHOD(CreateDiffStorage)(IMedium *aTarget,
                                 MediumVariant_T aVariant,
                                 IProgress **aProgress);
    STDMETHOD(MergeTo)(IN_BSTR aTargetId, IProgress **aProgress);
    STDMETHOD(CloneTo)(IMedium *aTarget, MediumVariant_T aVariant,
                        IMedium *aParent, IProgress **aProgress);
    STDMETHOD(Compact)(IProgress **aProgress);
    STDMETHOD(Resize)(ULONG64 aLogicalSize, IProgress **aProgress);
    STDMETHOD(Reset)(IProgress **aProgress);

    // public methods for internal purposes only
    const ComObjPtr<Medium>& getParent() const;
    const MediaList& getChildren() const;

    const Guid& getId() const;
    MediumState_T getState() const;
    const Utf8Str& getLocation() const;
    const Utf8Str& getLocationFull() const;
    uint64_t getSize() const;

    HRESULT attachTo(const Guid &aMachineId,
                     const Guid &aSnapshotId = Guid::Empty);
    HRESULT detachFrom(const Guid &aMachineId,
                       const Guid &aSnapshotId = Guid::Empty);

    const Guid* getFirstMachineBackrefId() const;
    const Guid* getFirstMachineBackrefSnapshotId() const;

#ifdef DEBUG
    void dumpBackRefs();
#endif

    HRESULT updatePath(const char *aOldPath, const char *aNewPath);
    void updatePaths(const char *aOldPath, const char *aNewPath);

    ComObjPtr<Medium> getBase(uint32_t *aLevel = NULL);

    bool isReadOnly();

    HRESULT saveSettings(settings::Medium &data);

    HRESULT compareLocationTo(const char *aLocation, int &aResult);

    /**
     * Shortcut to #deleteStorage() that doesn't wait for operation completion
     * and implies the progress object will be used for waiting.
     */
    HRESULT deleteStorageNoWait(ComObjPtr<Progress> &aProgress)
    { return deleteStorage(&aProgress, false /* aWait */, NULL /* pfNeedsSaveSettings */); }

    /**
     * Shortcut to #deleteStorage() that wait for operation completion by
     * blocking the current thread.
     */
    HRESULT deleteStorageAndWait(ComObjPtr<Progress> *aProgress, bool *pfNeedsSaveSettings)
    { return deleteStorage(aProgress, true /* aWait */, pfNeedsSaveSettings); }

    /**
     * Shortcut to #createDiffStorage() that doesn't wait for operation
     * completion and implies the progress object will be used for waiting.
     */
    HRESULT createDiffStorageNoWait(ComObjPtr<Medium> &aTarget,
                                    MediumVariant_T aVariant,
                                    ComObjPtr<Progress> &aProgress)
    { return createDiffStorage(aTarget, aVariant, &aProgress, false /* aWait */, NULL /* pfNeedsSaveSettings*/ ); }

    /**
     * Shortcut to #createDiffStorage() that wait for operation completion by
     * blocking the current thread.
     */
    HRESULT createDiffStorageAndWait(ComObjPtr<Medium> &aTarget,
                                     MediumVariant_T aVariant,
                                     bool *pfNeedsSaveSettings)
    { return createDiffStorage(aTarget, aVariant, NULL /*aProgress*/, true /* aWait */, pfNeedsSaveSettings); }

    HRESULT prepareMergeTo(Medium *aTarget, MergeChain * &aChain,
                            bool aIgnoreAttachments = false);

    /**
     * Shortcut to #mergeTo() that doesn't wait for operation completion and
     * implies the progress object will be used for waiting.
     */
    HRESULT mergeToNoWait(MergeChain *aChain,
                          ComObjPtr<Progress> &aProgress)
    { return mergeTo(aChain, &aProgress, false /* aWait */, NULL /*pfNeedsSaveSettings*/); }

    /**
     * Shortcut to #mergeTo() that wait for operation completion by
     * blocking the current thread.
     */
    HRESULT mergeToAndWait(MergeChain *aChain,
                           ComObjPtr<Progress> *aProgress,
                           bool *pfNeedsSaveSettings)
    { return mergeTo(aChain, aProgress, true /* aWait */, pfNeedsSaveSettings); }

    void cancelMergeTo(MergeChain *aChain);

    Utf8Str getName();

    HRESULT prepareDiscard(MergeChain * &aChain);
    HRESULT discard(ComObjPtr<Progress> &aProgress, ULONG ulWeight, MergeChain *aChain, bool *pfNeedsSaveSettings);
    void cancelDiscard(MergeChain *aChain);

    /** Returns a preferred format for a differencing hard disk. */
    Bstr preferredDiffFormat();

    // unsafe inline public methods for internal purposes only (ensure there is
    // a caller and a read lock before calling them!)
    MediumType_T getType() const;

    /** For com::SupportErrorInfoImpl. */
    static const char *ComponentName() { return "Medium"; }

private:

    HRESULT queryInfo();

    /**
     * Performs extra checks if the medium can be closed and returns S_OK in
     * this case. Otherwise, returns a respective error message. Called by
     * Close() under the medium tree lock and the medium lock.
     */
    HRESULT canClose();

    /**
     * Unregisters this medium with mVirtualBox. Called by Close() under
     * the medium tree lock.
     */
    HRESULT unregisterWithVirtualBox(bool *pfNeedsSaveSettings);

    HRESULT setStateError();

    HRESULT deleteStorage(ComObjPtr<Progress> *aProgress, bool aWait, bool *pfNeedsSaveSettings);

    HRESULT createDiffStorage(ComObjPtr<Medium> &aTarget,
                              MediumVariant_T aVariant,
                              ComObjPtr<Progress> *aProgress,
                              bool aWait,
                              bool *pfNeedsSaveSettings);

    HRESULT mergeTo(MergeChain *aChain,
                    ComObjPtr<Progress> *aProgress,
                    bool aWait,
                    bool *pfNeedsSaveSettings);

    HRESULT setLocation(const Utf8Str &aLocation, const Utf8Str &aFormat = Utf8Str());
    HRESULT setFormat(CBSTR aFormat);

    Utf8Str vdError(int aVRC);

    static DECLCALLBACK(void) vdErrorCall(void *pvUser, int rc, RT_SRC_POS_DECL,
                                          const char *pszFormat, va_list va);

    static DECLCALLBACK(bool) vdConfigAreKeysValid(void *pvUser,
                                                   const char *pszzValid);
    static DECLCALLBACK(int) vdConfigQuerySize(void *pvUser, const char *pszName,
                                               size_t *pcbValue);
    static DECLCALLBACK(int) vdConfigQuery(void *pvUser, const char *pszName,
                                           char *pszValue, size_t cchValue);

    class Task;
    class CreateBaseTask;
    class CreateDiffTask;
    class CloneTask;
    class CompactTask;
    class ResetTask;
    class DeleteTask;
    class MergeTask;
    friend class Task;
    friend class CreateBaseTask;
    friend class CreateDiffTask;
    friend class CloneTask;
    friend class CompactTask;
    friend class ResetTask;
    friend class DeleteTask;
    friend class MergeTask;

    HRESULT startThread(Medium::Task *pTask);
    HRESULT runNow(Medium::Task *pTask, bool *pfNeedsSaveSettings);

    HRESULT taskThreadCreateBase(Medium::CreateBaseTask &task);
    HRESULT taskThreadCreateDiff(Medium::CreateDiffTask &task);
    HRESULT taskThreadMerge(Medium::MergeTask &task);
    HRESULT taskThreadClone(Medium::CloneTask &task);
    HRESULT taskThreadDelete(Medium::DeleteTask &task);
    HRESULT taskThreadReset(Medium::ResetTask &task);
    HRESULT taskThreadCompact(Medium::CompactTask &task);

    struct Data;            // opaque data struct, defined in MediumImpl.cpp
    Data *m;
};

#endif /* ____H_MEDIUMIMPL */

