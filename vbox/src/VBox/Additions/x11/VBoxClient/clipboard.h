/** $Id: clipboard.h 86968 2020-11-24 13:46:03Z vboxsync $ */
/** @file
 * Guest Additions - X11 Shared Clipboard - Main header.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/**
 * Struct keeping a Shared Clipboard context.
 */
struct SHCLCONTEXT
{
    /** Client command context */
    VBGLR3SHCLCMDCTX CmdCtx;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /** Associated transfer data. */
    SHCLTRANSFERCTX  TransferCtx;
#endif
    /** X11 clipboard context. */
    SHCLX11CTX       X11;
};

