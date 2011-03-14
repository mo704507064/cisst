/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Balazs Vagvolgyi
  Created on: 2010

  (C) Copyright 2006-2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#include <cisstStereoVision/svlFilterOutput.h>
#include <cisstStereoVision/svlFilterBase.h>
#include <cisstStereoVision/svlStreamManager.h>
#include <cisstStereoVision/svlStreamBranchSource.h>
#include <cisstStereoVision/svlFilterInput.h>

#include <cisstOSAbstraction/osaSleep.h>  // PK TEMP

svlFilterOutput::svlFilterOutput(svlFilterBase* filter, bool trunk, const std::string &name) :
    BaseType(name, filter),
    Filter(filter),
    Trunk(trunk),
    Connected(false),
    Connection(0),
    ConnectedFilter(0),
    Type(svlTypeInvalid),
    ThreadCount(2),
    BufferSize(3),
    Blocked(false),
    Stream(0),
    BranchSource(0),
    Timestamp(-1.0)
{
}

svlFilterOutput::~svlFilterOutput(void)
{
    if (Stream) delete Stream;
    if (BranchSource) delete BranchSource;
}

bool svlFilterOutput::IsTrunk(void) const
{
    return Trunk;
}

svlStreamType svlFilterOutput::GetType(void) const
{
    return Type;
}

svlFilterBase* svlFilterOutput::GetFilter(void)
{
    return Filter;
}

svlFilterBase* svlFilterOutput::GetConnectedFilter(void)
{
    return ConnectedFilter;
}

bool svlFilterOutput::IsConnected(void) const
{
    return Connected;
}

int svlFilterOutput::SetType(svlStreamType type)
{
    if (!Filter || Filter->Initialized) return SVL_FAIL;
    Type = type;
    return SVL_OK;
}

svlFilterInput* svlFilterOutput::GetConnection(void)
{
    return Connection;
}

int svlFilterOutput::GetDroppedSampleCount(void)
{
    if (!BranchSource) return SVL_FAIL;
    return static_cast<int>(BranchSource->GetDroppedSampleCount());
}

int svlFilterOutput::GetBufferUsage(void)
{
    if (!BranchSource) return SVL_FAIL;
    return BranchSource->GetBufferUsage();
}

double svlFilterOutput::GetBufferUsageRatio(void)
{
    if (!BranchSource) return -1.0;
    return BranchSource->GetBufferUsageRatio();
}

int svlFilterOutput::SetThreadCount(unsigned int threadcount)
{
    if (!Filter || Filter->Initialized) return SVL_FAIL;

    // Thread count is inherited on the trunk
    if (Trunk || threadcount < 1) return SVL_FAIL;
    ThreadCount = threadcount;
    return SVL_OK;
}

int svlFilterOutput::SetBufferSize(unsigned int buffersize)
{
    if (!Filter || Filter->Initialized) return SVL_FAIL;

    // There is no buffering on the trunk
    if (Trunk || buffersize < 3) return SVL_FAIL;
    BufferSize = buffersize;
    return SVL_OK;
}

int svlFilterOutput::SetBlock(bool block)
{
    if (!Filter) return SVL_FAIL;

    // Trunk output cannot be blocked
    if (Trunk) return SVL_FAIL;
    Blocked = block;
    return SVL_OK;
}

int svlFilterOutput::ConnectInternal(svlFilterInput *input)
{
    if (!this->Filter) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: this output is not associated to any filter" << std::endl;
        return SVL_FAIL;
    }
    if (!input) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: null input pointer passed to this method" << std::endl;
        return SVL_FAIL;
    }
    if (!input->Filter) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: input passed to this method is not associated to any filter" << std::endl;
        return SVL_FAIL;
    }
    if (this->Connected) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: this output is already connected" << std::endl;
        return SVL_FAIL;
    }
    if (input->Connected) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: input passed to this method is already connected" << std::endl;
        return SVL_FAIL;
    }
    if (this->Filter->Initialized) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: filter associated to this output is already initialized" << std::endl;
        return SVL_FAIL;
    }
    if (input->Filter->Initialized) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: filter associated to the input is already initialized" << std::endl;
        return SVL_FAIL;
    }

    // Setup output types in the connected filter
    if (input->Trunk && input->Filter->AutoType) {
        // Automatic setup
        if (!input->IsTypeSupported(Type)) {
            CMN_LOG_CLASS_INIT_ERROR << "Connect: input doesn't support output type (auto)" << std::endl;
            return SVL_FAIL;
        }
        svlFilterOutput* output = input->Filter->GetOutput();
        if (output) output->SetType(Type);
    }
    else {
        // Manual setup
        if (input->Filter->OnConnectInput(*input, Type) != SVL_OK) {
            CMN_LOG_CLASS_INIT_ERROR << "Connect: input doesn't support output type (manual)" << std::endl;
            return SVL_FAIL;
        }
    }

    if (!Trunk && input->Trunk) {
        // Create stream branch if not trunk
        Stream = new svlStreamManager(ThreadCount);
        BranchSource = new svlStreamBranchSource(Type, BufferSize);
        Stream->SetSourceFilter(BranchSource);

        // Connect filters
        svlFilterOutput* output = BranchSource->GetOutput();
        if (output) output->Connect(input);
    }
    else {
        if (!input->Trunk) {
            input->Buffer = new svlBufferSample(Type);
        }

        // Connect filters
        input->Connected = true;
        input->Connection = this;
        input->Type = Type;
        input->ConnectedFilter = Filter;
    }

    Connection = input;
    ConnectedFilter = input->Filter;
    Connected = true;

    return SVL_OK;
}

int svlFilterOutput::Connect(svlFilterInput *input)
{
    if (!input) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: null input pointer passed to this method" << std::endl;
        return SVL_FAIL;
    }
    if (!input->Filter) {
        CMN_LOG_CLASS_INIT_ERROR << "Connect: input passed to this method is not associated to any filter" << std::endl;
        return SVL_FAIL;
    }
    mtsManagerLocal *LCM = mtsManagerLocal::GetInstance();
    LCM->AddComponent(input->Filter);
    if (LCM->Connect(input->Filter->GetName(), input->GetName(),
                     this->Filter->GetName(), this->GetName())) {
        osaSleep(0.5);  // PK TEMP (remove when Connect is a blocking command)
        return SVL_OK;
    }
    else
        return SVL_FAIL;
}

int svlFilterOutput::Disconnect(void)
{
    // TO DO
    return SVL_FAIL;
}

void svlFilterOutput::SetupSample(svlSample* sample)
{
    if (sample &&
        Filter && !Filter->Initialized &&
        !Trunk &&
        Connected) {

        if (Connection->Trunk) BranchSource->SetInput(sample);
    }
}

void svlFilterOutput::PushSample(const svlSample* sample)
{
    if (sample &&
        Filter && Filter->Initialized &&
        !Trunk && Connected && !Blocked) {

        if (Connection->Trunk) BranchSource->PushSample(sample);
        else if (Connection->Buffer) Connection->Buffer->Push(sample);

        // Store timestamp
        Timestamp = sample->GetTimestamp();
    }
}

double svlFilterOutput::GetTimestamp(void)
{
    return Timestamp;
}
