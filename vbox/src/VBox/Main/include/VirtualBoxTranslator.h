/* $Id: VirtualBoxTranslator.h 91391 2021-09-27 11:50:27Z vboxsync $ */
/** @file
 * VirtualBox Translator.
 */

/*
 * Copyright (C) 2005-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_VirtualBoxTranslator_h
#define MAIN_INCLUDED_VirtualBoxTranslator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <list>

#include <iprt/cpp/lock.h>
#include <VBox/com/AutoLock.h>

class QMTranslator;

class VirtualBoxTranslator
    : public util::RWLockHandle
{
public:
    virtual ~VirtualBoxTranslator();

    static VirtualBoxTranslator *instance();
    /* Returns instance if exists, returns NULL otherwise. */
    static VirtualBoxTranslator *tryInstance();
    void release();

    /* Load language based on settings in the VirtualBox config */
    HRESULT loadLanguage(ComPtr<IVirtualBox> aVirtualBox);

private:
    /** Translator component. */
    struct TranslatorComponent
    {
        QMTranslator *pTranslator;
        /** Path to translation files. It includes file prefix, i.e '/path/to/folder/file_prefix'. */
        com::Utf8Str  strPath;

        TranslatorComponent() : pTranslator(NULL) {}
    };
public:
    /** Pointer to a translator component. */
    typedef TranslatorComponent *PTRCOMPONENT;

    /**
     * Registers the translation for a component.
     *
     * @returns VBox status code
     * @param aTranslationPath  Path to the translation files, this includes the
     *                          base filename prefix.
     * @param aDefault          Use this as the default translation component, i.e.
     *                          when translate() is called with NULL for @a
     *                          aComponent.
     * @param aComponent        Pointer to where is the component handle should be
     *                          returned on success.  The return value must be used
     *                          for all subsequent calls to translate().
     */
    static int registerTranslation(const char *aTranslationPath,
                                   bool aDefault,
                                   PTRCOMPONENT *aComponent);

    /**
     * Removes translations for a component.
     *
     * @returns VBox status code
     * @param   aComponent      The component.  NULL is quietly (VWRN_NOT_FOUND)
     *                          ignored .
     */
    static int unregisterTranslation(PTRCOMPONENT aComponent);

    /**
     * Translates @a aSourceText to user language.
     * Uses component marked as default if @a aTranslationComponent is NULL
     *
     * @returns Translated string or @a aSourceText. The returned string is
     *          valid only during lifetime of the translator instance.
     */
    static const char *translate(PTRCOMPONENT aComponent,
                                 const char *aContext,
                                 const char *aSourceText,
                                 const char *aComment = NULL,
                                 const int   aNum = -1);

    /**
     * Returns source text stored in the cache if exists.
     * Otherwise, the pszTranslation itself returned.
     */
    static const char *trSource(const char *aTranslation);

    /* Convenience function used by VirtualBox::init */
    int i_loadLanguage(const char *pszLang);

    static int32_t initCritSect();

private:
    static RTCRITSECTRW s_instanceRwLock;
    static VirtualBoxTranslator *s_pInstance;

    uint32_t m_cInstanceRefs;

    typedef std::list<TranslatorComponent> TranslatorList;
    TranslatorList  m_lTranslators;
    TranslatorComponent *m_pDefaultComponent;

    /* keep the language code for registration */
    com::Utf8Str m_strLanguage;

    /** String cache that all translation strings are stored in.
     * This is a add-only cache, which allows translate() to return C-strings w/o
     * needing to think about racing loadLanguage() wrt string lifetime. */
    RTSTRCACHE    m_hStrCache;
    /** RTStrCacheCreate status code. */
    int           m_rcCache;

    VirtualBoxTranslator();

    int i_loadLanguageForComponent(TranslatorComponent *aComponent, const char *aLang);

    int i_setLanguageFile(TranslatorComponent *aComponent, const char *aFileName);

    int i_registerTranslation(const char *aTranslationPath,
                              bool aDefault,
                              PTRCOMPONENT *aComponent);

    int i_unregisterTranslation(PTRCOMPONENT aComponent);

    const char *i_translate(PTRCOMPONENT aComponent,
                            const char *aContext,
                            const char *aSourceText,
                            const char *aComment = NULL,
                            const int   aNum = -1);
};

/** Pointer to a translator component. */
typedef VirtualBoxTranslator::PTRCOMPONENT PTRCOMPONENT;

#endif /* !MAIN_INCLUDED_VirtualBoxTranslator_h */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
