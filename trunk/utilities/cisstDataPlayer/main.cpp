/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/*
  $Id$
  
  Author(s): Marcin Balicki
  Created on: 2011-02-10

  (C) Copyright 2011 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstMultiTask.h>
#include <cisstCommon.h>
#include <cisstOSAbstraction.h>
#include <QApplication>
#include "cdpPlayerExample.h"
#include "cdpPlayerManager.h"


int main(int argc, char *argv[])
{
   // log configuration
    cmnLogger::SetMask(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskDefaultLog(CMN_LOG_ALLOW_ALL);
    cmnLogger::AddChannel(std::cout, CMN_LOG_ALLOW_ERRORS_AND_WARNINGS);

    std::string globalComponentManagerIP = "localhost";
    // Set global component manager IP

    if (argc == 1) {
        //globalComponentManagerIP;
    } else if (argc == 2) {
        globalComponentManagerIP = argv[1];
    } else {
        std::cerr << "Usage: " << argv[0] << " (global component manager IP)" << std::endl;
        return 1;
    }
    std::cout << "Global component manager IP: " << globalComponentManagerIP << std::endl;

    // create our components

    mtsComponentManager * componentManager;

    //NOTE: this example can only run if the oct proc is not on.
    try {
        componentManager = mtsManagerLocal::GetInstance(globalComponentManagerIP, "DataPlayerProc");
    } catch (...) {
        CMN_LOG_INIT_WARNING << "Failed to initialize local component manager" << std::endl;
        CMN_LOG_INIT_WARNING << "Running in local mode" << std::endl;
        componentManager = mtsComponentManager::GetInstance();
    }

    QApplication application(argc, argv);

    cdpPlayerManager * playerManager = new cdpPlayerManager("PlayerManager", 1.0 * cmn_ms);
    cdpPlayerExample * player1 = new cdpPlayerExample("Player1", 1.0 * cmn_ms);
    cdpPlayerExample * player2 = new cdpPlayerExample("Player2", 1.0 * cmn_ms);

    // add the components to the component manager
    componentManager->AddComponent(playerManager);
    componentManager->AddComponent(player1);
    componentManager->AddComponent(player2);

    playerManager->AddPlayer(player1);
    playerManager->AddPlayer(player2);

    playerManager->Configure();
    player1->Configure();
    player2->Configure();

    // create the components, i.e. find the commands
    componentManager->CreateAll();
    componentManager->WaitForStateAll(mtsComponentState::READY);

    // start the periodic Run
    componentManager->StartAll();
    componentManager->WaitForStateAll(mtsComponentState::ACTIVE);

    application.setStyle("Plastique");
    application.exec();

    componentManager->KillAll();
    componentManager->WaitForStateAll(mtsComponentState::FINISHED, 2.0 * cmn_s);

    componentManager->Cleanup();

    return 0;
}