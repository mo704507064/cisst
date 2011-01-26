/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):	Rajesh Kumar, Anton Deguet
  Created on:   2008-03-12

  (C) Copyright 2008 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstParameterTypes/prmPositionCartesianGet.h>

CMN_IMPLEMENT_SERVICES(prmPositionCartesianGet);

prmPositionCartesianGet::~prmPositionCartesianGet()
{
}


void prmPositionCartesianGet::ToStream(std::ostream & outputStream) const
{
    BaseType::ToStream(outputStream);
    outputStream << "\nReference frame: " << this->ReferenceFrameMember
                 << "\nMoving frame: " << this->MovingFrameMember
                 << "\nPosition: " << this->PositionMember;
}