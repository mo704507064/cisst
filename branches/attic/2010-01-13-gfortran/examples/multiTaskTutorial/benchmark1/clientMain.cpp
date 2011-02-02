/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/* $Id: clientMain.cpp 671 2009-08-13 02:41:31Z adeguet1 $ */

#include <cisstCommon.h>
#include <cisstOSAbstraction.h>
#include <cisstMultiTask.h>
#include <cisstParameterTypes.h>

#include "clientTask.h"

#include <sstream>


int main(int argc, char * argv[])
{

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " GlobalManagerIP" << std::endl;
        exit(-1);
    }

    // before we start, estimate overhead related to serialization and de-serialization of parameters
    osaTimeServer timeServer;
    std::stringstream serializationStream;
    cmnSerializer serialization(serializationStream);
    cmnDeSerializer deSerialization(serializationStream);
    unsigned int index;
    prmPositionCartesianGet parameter;
    cmnGenericObject * generated;
    timeServer.SetTimeOrigin();
    for (index = 0; index < confNumberOfSamples; index++) {
        serialization.Serialize(parameter);
        generated = deSerialization.DeSerialize();
        if (generated->Services() != parameter.Services()) {
            std::cout << "Serialization test failed!" << std::endl;
            exit(0);
        }
        // delete generated object created by de-serialization
        delete generated;
    }
    double elapsedTime = timeServer.GetRelativeTime();
    std::cout << std::endl << std::endl
              << "Serialization, dynamic creation and deserialization time for "
              << confNumberOfSamples << " samples of " << parameter.Services()->GetName()
              << ": " << cmnInternalTo_ms(elapsedTime) << " (ms)" << std::endl
              << "Per sample: " << cmnInternalTo_ms(elapsedTime / confNumberOfSamples) << " (ms)" << std::endl
              << std::endl << std::endl;

    // networking part
    std::string globalTaskManagerIP(argv[1]);

    // Log configuration
    cmnLogger::SetLoD(CMN_LOG_LOD_RUN_ERROR);
    // add a log per thread
    osaThreadedLogFile threadedLog("example9Client");
    cmnLogger::GetMultiplexer()->AddChannel(threadedLog, CMN_LOG_LOD_RUN_ERROR);

    // create our server task
    clientTask * client = new clientTask("Client", confClientPeriod);

    // Get the TaskManager instance and set operation mode
    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();
    taskManager->AddTask(client);        
    taskManager->SetGlobalTaskManagerIP(globalTaskManagerIP);
    
    // Set the type of task manager either as a server or as a client.
    // mtsTaskManager::SetTaskManagerType() should be called before
    // mtsTaskManager::Connect()
    taskManager->SetTaskManagerType(mtsTaskManager::TASK_MANAGER_CLIENT);

    //
    // TODO: Hide this waiting routine inside mtsTaskManager using events or other things.
    //
    osaSleep(0.5 * cmn_s);

    // Connect the tasks across networks
    taskManager->Connect("Client", "Required1", "Server", "Provided1");
    taskManager->Connect("Client", "Required2", "Server", "Provided2");

    // create the tasks, i.e. find the commands
    taskManager->CreateAll();
    // start the periodic Run
    taskManager->StartAll();

    // run while the benchmarks are not over
    while (!(client->BenchmarkDone())) {
        osaSleep(10.0 * cmn_ms);
    }
    
    // cleanup
    taskManager->KillAll();

    return 0;
}

/*
  Author(s):  Ankur Kapoor, Peter Kazanzides, Anton Deguet, Min Yang Jung
  Created on: 2004-04-30

  (C) Copyright 2004-2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/