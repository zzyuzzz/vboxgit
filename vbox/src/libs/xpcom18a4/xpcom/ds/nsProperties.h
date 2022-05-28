/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsProperties_h___
#define nsProperties_h___

#include "nsIProperties.h"
#include "nsHashtable.h"
#include "nsAgg.h"

#define NS_PROPERTIES_CID                            \
{ /* 4de2bc90-b1bf-11d3-93b6-00104ba0fd40 */         \
    0x4de2bc90,                                      \
    0xb1bf,                                          \
    0x11d3,                                          \
    {0x93, 0xb6, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

class nsIUnicharInputStream;

class nsProperties : public nsIProperties, public nsHashtable {
public:

  NS_DECL_AGGREGATED
  NS_DECL_NSIPROPERTIES

  nsProperties(nsISupports* outer);

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  static PRBool PR_CALLBACK ReleaseValues(nsHashKey* key, void* data, void* closure);

private:
  ~nsProperties();
};

#endif /* nsProperties_h___ */
