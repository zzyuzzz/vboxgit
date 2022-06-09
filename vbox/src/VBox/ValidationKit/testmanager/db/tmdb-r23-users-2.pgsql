-- $Id: tmdb-r23-users-2.pgsql 69111 2017-10-17 14:26:02Z vboxsync $
--- @file
-- VBox Test Manager Database - Adds fReadOnly column to Users.
--

--
-- Copyright (C) 2013-2017 Oracle Corporation
--
-- This file is part of VirtualBox Open Source Edition (OSE), as
-- available from http://www.virtualbox.org. This file is free software;
-- you can redistribute it and/or modify it under the terms of the GNU
-- General Public License (GPL) as published by the Free Software
-- Foundation, in version 2 as it comes in the "COPYING" file of the
-- VirtualBox OSE distribution. VirtualBox OSE is distributed in the
-- hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
--
-- The contents of this file may alternatively be used under the terms
-- of the Common Development and Distribution License Version 1.0
-- (CDDL) only, as it comes in the "COPYING.CDDL" file of the
-- VirtualBox OSE distribution, in which case the provisions of the
-- CDDL are applicable instead of those of the GPL.
--
-- You may elect to license modified versions of this file under the
-- terms and conditions of either the GPL or the CDDL or both.
--

--
-- Cleanup after failed runs.
--
DROP TABLE IF EXISTS OldTestBoxes;

--
-- Die on error from now on.
--
\set ON_ERROR_STOP 1
\set AUTOCOMMIT 0



-- This change can be implemented using ALTER TABLE. Yeah!
\d+ Users;

ALTER TABLE Users
    ADD COLUMN fReadOnly BOOLEAN NOT NULL DEFAULT FALSE;

COMMIT;

\d Users;
ANALYZE VERBOSE Users;

