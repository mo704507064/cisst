/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/*
  $Id$

  Author(s):  Anton Deguet
  Created on: 2012-01-24

  (C) Copyright 2012 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#ifndef _sineTaskWithDelay_h
#define _sineTaskWithDelay_h

#include <cisstMultiTask/mtsComponentAddLatency.h>

// check if this module is built as a DLL
#ifdef mtsExPeriodicTaskComponents_EXPORTS
#define CISST_THIS_LIBRARY_AS_DLL
#endif
#include <cisstCommon/cmnExportMacros.h>

// avoid impact on other modules
#undef CISST_THIS_LIBRARY_AS_DLL

class CISST_EXPORT sineTaskWithDelay: public mtsComponentAddLatency {
    // used to control the log level, "Run Error" by default
    CMN_DECLARE_SERVICES(CMN_NO_DYNAMIC_CREATION, CMN_LOG_ALLOW_ERRORS_AND_WARNINGS);
 protected:
    mtsDouble SineData;

 public:
    sineTaskWithDelay(const std::string & componentName, double period); 
};

CMN_DECLARE_SERVICES_INSTANTIATION(sineTaskWithDelay);

#endif // _sineTaskWithDelay_h
