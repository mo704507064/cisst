/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsCommandAndEventNetworkedTest.cpp 1897 2010-10-15 16:52:19Z adeguet1 $

  Author(s):  Anton Deguet
  Created on: 2010-10-20

  (C) Copyright 2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include "mtsCommandAndEventNetworkedTest.h"

CMN_IMPLEMENT_SERVICES(mtsCommandAndEventNetworkedTest);

#include <cisstMultiTask/mtsManagerGlobal.h>
#include <cisstMultiTask/mtsManagerLocal.h>

#include "mtsTestComponents.h"

#define P1 "P1"
#define P2 "P2"
#define P1_OBJ localManager1
#define P2_OBJ localManager2

#define DEFAULT_PROCESS_NAME "LCM"

const double TransitionDelay = 10.0 * cmn_s;

mtsCommandAndEventNetworkedTest::mtsCommandAndEventNetworkedTest():
    PipeComponentManager("component_manager"),
    PipeProcessServer("server"),
    PipeProcessClient("client"),
    PipeConfigurationManager("configuration_manager")
{
}


bool mtsCommandAndEventNetworkedTest::SendAndReceive(osaPipeExec & pipe,
                                                     const std::string & send,
                                                     std::string & received,
                                                     const double & timeOut)
{
    pipe.Write(send.c_str(), send.size());
    pipe.Write("\n", 1);
    // read data until \n arrives while not exceeding time alloted
    received = "";
    char charRead = '\0';
    size_t byteRead;
    const osaTimeServer & timeServer = mtsComponentManager::GetInstance()->GetTimeServer();
    const double endTime = timeServer.GetRelativeTime() + timeOut;
    bool timeExpired = false;
    while ((charRead != '\n')
           && (!timeExpired)) {
        byteRead = pipe.Read(&charRead, 1);
        if ((byteRead == 1)
            && (charRead != '\n')) {
            received = received + charRead;
        }
        timeExpired = (timeServer.GetRelativeTime() > endTime);
    }
    if (timeExpired) {
        CMN_LOG_CLASS_RUN_ERROR << "SendAndReceive: timed out while sending \"" << send
                                << "\" on pipe \"" << pipe.GetName() << "\", allowed time was "
                                << timeOut << " seconds" << std::endl;
        return false;
    }
    return true;
}


void mtsCommandAndEventNetworkedTest::SendAndVerify(osaPipeExec & pipe,
                                                    const std::string & send,
                                                    const std::string & expected,
                                                    const double & timeOut)
{
    std::string pipeName = "pipe " + pipe.GetName(); 
    CPPUNIT_ASSERT_MESSAGE(pipeName, pipe.IsConnected());
    std::string answer;
    if (!SendAndReceive(pipe, send, answer, timeOut)) {
        std::string message = pipeName + ": time out while waiting for reply to \"" + send + "\"";
        CPPUNIT_FAIL(message);
        return;
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(pipeName, expected, answer);
}


void mtsCommandAndEventNetworkedTest::StartAllComponents(void)
{
    SendAndVerify(PipeComponentManager, "connect", "component_manager connected");
    SendAndVerify(PipeComponentManager, "start", "start succeeded");
    SendAndVerify(PipeProcessServer, "connect", "server connected");
    SendAndVerify(PipeProcessServer, "start", "start succeeded");
    SendAndVerify(PipeProcessClient, "connect", "client connected");
    SendAndVerify(PipeProcessClient, "start", "start succeeded");
    SendAndVerify(PipeConfigurationManager, "connect", "configuration_manager connected");
    SendAndVerify(PipeConfigurationManager, "start", "start succeeded");
}


void mtsCommandAndEventNetworkedTest::StopAllComponents(void)
{
    SendAndVerify(PipeComponentManager, "stop", "stop succeeded");
    SendAndVerify(PipeProcessServer, "stop", "stop succeeded");
    SendAndVerify(PipeProcessClient, "stop", "stop succeeded");
    SendAndVerify(PipeConfigurationManager, "stop", "stop succeeded");
}


void mtsCommandAndEventNetworkedTest::PingAllComponents(void)
{
    SendAndVerify(PipeComponentManager, "ping", "ok");
    SendAndVerify(PipeProcessServer, "ping", "ok");
    SendAndVerify(PipeProcessClient, "ping", "ok");
    SendAndVerify(PipeConfigurationManager, "ping", "ok");
}


void mtsCommandAndEventNetworkedTest::VerifyProcesses(void)
{
    SendAndVerify(PipeComponentManager, "has_process client", "client found");
    SendAndVerify(PipeComponentManager, "has_process server", "server found");
    SendAndVerify(PipeComponentManager, "has_process configuration_manager", "configuration_manager found");
    SendAndVerify(PipeComponentManager, "has_process whatever_that_should_not_exist", "whatever_that_should_not_exist not found");
}


void mtsCommandAndEventNetworkedTest::setUp(void)
{
    std::string executable;
    std::vector<std::string> arguments;

    std::string executablePath =
        std::string(CISST_BUILD_ROOT) + std::string("/tests/bin/")
        + CMAKE_CFG_INTDIR_WITH_QUOTES + std::string("/");

    // start network manager
    executable = executablePath + std::string("cisstMultiTaskTestsComponentManager");
	if (!PipeComponentManager.Open(executable, "rw")) {
        std::cout << "Error occurred while starting component manager" << std::endl;
    }

    // start server process
    executable = executablePath + std::string("cisstMultiTaskTestsProcess");
    arguments.resize(1);
    arguments[0] = std::string("server");
	if (!PipeProcessServer.Open(executable, arguments, "rw")) {
        std::cout << "Error occurred while starting server process" << std::endl;
    }

    // start client process
    arguments[0] = std::string("client");
	if (!PipeProcessClient.Open(executable, arguments, "rw")) {
        std::cout << "Error occurred while starting client process" << std::endl;
    }

    // start configuration manager
    executable = executablePath + std::string("cisstMultiTaskTestsConfigurationManager");
    arguments.resize(0);
	if (!PipeConfigurationManager.Open(executable, "rw")) {
        std::cout << "Error occurred while starting configuration manager" << std::endl;
    }
}


void mtsCommandAndEventNetworkedTest::tearDown(void)
{
    // close and kill all processes
    PipeComponentManager.Close();
    PipeProcessServer.Close();
    PipeProcessClient.Close();
    PipeConfigurationManager.Close();
}



#if 0
template <class _clientType, class _serverType>
void mtsCommandAndEventNetworkedTest::TestExecution(_clientType * client, _serverType * server,
                                                    double clientExecutionDelay, double serverExecutionDelay,
                                                    double blockingDelay)
{
    mtsComponentManager * manager = mtsComponentManager::GetInstance();

    // we assume both client and servers use the same type
    typedef typename _serverType::value_type value_type;

    // add to manager and start all
    manager->AddComponent(client);
    manager->AddComponent(server);
    manager->Connect(client->GetName(), "r1", server->GetName(), "p1");
    manager->CreateAll();
    CPPUNIT_ASSERT(manager->WaitForStateAll(mtsComponentState::READY, TransitionDelay));
    manager->StartAll();
    CPPUNIT_ASSERT(manager->WaitForStateAll(mtsComponentState::ACTIVE, TransitionDelay));

    // test commands and timing
    const double queueingDelay = 10.0 * cmn_ms;
    const osaTimeServer & timeServer = manager->GetTimeServer();
    double startTime, stopTime;

    // check initial values
    CPPUNIT_ASSERT_EQUAL(-1, server->InterfaceProvided1.GetValue()); // initial value
    CPPUNIT_ASSERT_EQUAL(-1, client->InterfaceRequired1.GetValue()); // initial value

    // value we used to make sure commands are processed, default is
    // -1, void command set to 0
    value_type valueWrite, valueWritePlusOne, valueRead;
    valueWrite = 4;

    // loop over void and write commands to alternate blocking and non
    // blocking commands
    unsigned int index;
    for (index = 0; index < 3; index++) {
        // test void command non blocking
        startTime = timeServer.GetRelativeTime();
        client->InterfaceRequired1.FunctionVoid();
        stopTime = timeServer.GetRelativeTime();
        CPPUNIT_ASSERT((stopTime - startTime) <= queueingDelay); // make sure execution is fast
        osaSleep(serverExecutionDelay + blockingDelay); // time to dequeue and let command execute
        CPPUNIT_ASSERT_EQUAL(0,  server->InterfaceProvided1.GetValue()); // reset
        CPPUNIT_ASSERT_EQUAL(-1, client->InterfaceRequired1.GetValue()); // unchanged

        // test write command
        startTime = timeServer.GetRelativeTime();
        client->InterfaceRequired1.FunctionWrite(valueWrite);
        stopTime = timeServer.GetRelativeTime();
        CPPUNIT_ASSERT((stopTime - startTime) <= queueingDelay); // make sure execution is fast
        osaSleep(serverExecutionDelay + blockingDelay);  // time to dequeue and let command execute
        CPPUNIT_ASSERT(valueWrite == server->InterfaceProvided1.GetValue()); // set to new value
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

        // test filtered write command
        startTime = timeServer.GetRelativeTime();
        client->InterfaceRequired1.FunctionFilteredWrite(valueWrite);
        stopTime = timeServer.GetRelativeTime();
        CPPUNIT_ASSERT((stopTime - startTime) <= queueingDelay); // make sure execution is fast
        osaSleep(serverExecutionDelay + blockingDelay);  // time to dequeue and let command execute
        CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // set to new value + 1 (filter)
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

        // test void command blocking
        if (blockingDelay > 0.0) {
            startTime = timeServer.GetRelativeTime();
            client->InterfaceRequired1.FunctionVoid.ExecuteBlocking();
            stopTime = timeServer.GetRelativeTime();
            std::stringstream message;
            message << "Actual: " << (stopTime - startTime) << " >= " << (blockingDelay * 0.9);
            CPPUNIT_ASSERT_MESSAGE(message.str(), (stopTime - startTime) >= (blockingDelay * 0.9));
        } else {
            // no significant delay but result should be garanteed without sleep
            client->InterfaceRequired1.FunctionVoid.ExecuteBlocking();
        }
        CPPUNIT_ASSERT(0 == server->InterfaceProvided1.GetValue()); // reset
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

        // test write command blocking
        if (blockingDelay > 0.0) {
            startTime = timeServer.GetRelativeTime();
            client->InterfaceRequired1.FunctionWrite.ExecuteBlocking(valueWrite);
            stopTime = timeServer.GetRelativeTime();
            std::stringstream message;
            message << "Actual: " << (stopTime - startTime) << " >= " << (blockingDelay * 0.9);
            CPPUNIT_ASSERT_MESSAGE(message.str(), (stopTime - startTime) >= (blockingDelay * 0.9));
        } else {
            // no significant delay but result should be garanteed without sleep
            client->InterfaceRequired1.FunctionWrite.ExecuteBlocking(valueWrite);
        }
        CPPUNIT_ASSERT(valueWrite == server->InterfaceProvided1.GetValue()); // set to new value
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

        // test filtered write command blocking
        if (blockingDelay > 0.0) {
            startTime = timeServer.GetRelativeTime();
            client->InterfaceRequired1.FunctionFilteredWrite.ExecuteBlocking(valueWrite);
            stopTime = timeServer.GetRelativeTime();
            std::stringstream message;
            message << "Actual: " << (stopTime - startTime) << " >= " << (blockingDelay * 0.9);
            CPPUNIT_ASSERT_MESSAGE(message.str(), (stopTime - startTime) >= (blockingDelay * 0.9));
        } else {
            // no significant delay but result should be garanteed without sleep
            client->InterfaceRequired1.FunctionFilteredWrite.ExecuteBlocking(valueWrite);
        }
        CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // set to new value + 1 (filter)
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

        // test void return command (always blocking)
        value_type result;
        if (blockingDelay > 0.0) {
            startTime = timeServer.GetRelativeTime();
            client->InterfaceRequired1.FunctionVoidReturn(result);
            stopTime = timeServer.GetRelativeTime();
            std::stringstream message;
            message << "Actual: " << (stopTime - startTime) << " >= " << (blockingDelay * 0.9);
            CPPUNIT_ASSERT_MESSAGE(message.str(), (stopTime - startTime) >= (blockingDelay * 0.9));
        } else {
            // no significant delay but result should be garanteed without sleep
            client->InterfaceRequired1.FunctionVoidReturn(result);
        }
        CPPUNIT_ASSERT(result == 1); // number was positive
        CPPUNIT_ASSERT((-(valueWrite + 1)) == server->InterfaceProvided1.GetValue()); // negated
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

        // call write return to change sign of value back
        if (blockingDelay > 0.0) {
            startTime = timeServer.GetRelativeTime();
            valueWritePlusOne = valueWrite + 1;
            client->InterfaceRequired1.FunctionWriteReturn(valueWritePlusOne, valueRead);
            result = valueRead;
            stopTime = timeServer.GetRelativeTime();
            std::stringstream message;
            message << "Actual: " << (stopTime - startTime) << " >= " << (blockingDelay * 0.9);
            CPPUNIT_ASSERT_MESSAGE(message.str(), (stopTime - startTime) >= (blockingDelay * 0.9));
        } else {
            // no significant delay but result should be garanteed without sleep
            valueWritePlusOne = valueWrite + 1;
            client->InterfaceRequired1.FunctionWriteReturn(valueWritePlusOne, valueRead);
            result = valueRead;
        }
        CPPUNIT_ASSERT(result == -1); // number was negative
        CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // negated back
        CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

    }

    // test read command
    valueRead = 0;
    client->InterfaceRequired1.FunctionRead(valueRead);
    CPPUNIT_ASSERT((valueWrite + 1) == valueRead);
    CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // unchanged
    CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

    // test qualified read command
    valueRead = 0;
    client->InterfaceRequired1.FunctionQualifiedRead(valueWrite, valueRead);
    CPPUNIT_ASSERT((valueWrite + 1) == valueRead);
    CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // unchanged
    CPPUNIT_ASSERT(-1 == client->InterfaceRequired1.GetValue()); // unchanged

    // test void event
    server->InterfaceProvided1.EventVoid();
    osaSleep(clientExecutionDelay);
    CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // unchanged
    CPPUNIT_ASSERT(0 == client->InterfaceRequired1.GetValue()); // reset by void event

    // test write event
    server->InterfaceProvided1.EventWrite(valueWrite);
    osaSleep(clientExecutionDelay);
    CPPUNIT_ASSERT((valueWrite + 1) == server->InterfaceProvided1.GetValue()); // unchanged
    CPPUNIT_ASSERT(valueWrite == client->InterfaceRequired1.GetValue()); // set by write event

    // stop all and cleanup
    manager->KillAll();
    CPPUNIT_ASSERT(manager->WaitForStateAll(mtsComponentState::FINISHED, TransitionDelay));
    manager->Disconnect(client->GetName(), "r1", server->GetName(), "p1");
    manager->RemoveComponent(client);
    manager->RemoveComponent(server);
}
#endif


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestDeviceDevice(void)
{
    StartAllComponents();
    PingAllComponents();
    VerifyProcesses();
    StopAllComponents();
    /*
    mtsTestDevice2<_elementType> * client = new mtsTestDevice2<_elementType>;
    mtsTestDevice3<_elementType> * server = new mtsTestDevice3<_elementType>;
    TestExecution(client, server, 0.0, 0.0);
    delete client;
    delete server;
    */
}
void mtsCommandAndEventNetworkedTest::TestDeviceDevice_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestDeviceDevice<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestDeviceDevice_int(void) {
    mtsCommandAndEventNetworkedTest::TestDeviceDevice<int>();
}


#if 0
template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestPeriodicPeriodic(void)
{
    mtsTestPeriodic1<_elementType> * client = new mtsTestPeriodic1<_elementType>("mtsTestPeriodic1Client");
    mtsTestPeriodic1<_elementType> * server = new mtsTestPeriodic1<_elementType>("mtsTestPeriodic1Server");
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay);
    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestPeriodicPeriodic_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestPeriodicPeriodic<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestPeriodicPeriodic_int(void) {
    mtsCommandAndEventNetworkedTest::TestPeriodicPeriodic<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestContinuousContinuous(void)
{
    mtsTestContinuous1<_elementType> * client = new mtsTestContinuous1<_elementType>("mtsTestContinuous1Client");
    mtsTestContinuous1<_elementType> * server = new mtsTestContinuous1<_elementType>("mtsTestContinuous1Server");
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay);
    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestContinuousContinuous_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestContinuousContinuous<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestContinuousContinuous_int(void) {
    mtsCommandAndEventNetworkedTest::TestContinuousContinuous<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallback(void)
{
    mtsTestFromCallback1<_elementType> * client = new mtsTestFromCallback1<_elementType>("mtsTestFromCallback1Client");
    mtsTestCallbackTrigger * clientTrigger = new mtsTestCallbackTrigger(client);
    mtsTestFromCallback1<_elementType> * server = new mtsTestFromCallback1<_elementType>("mtsTestFromCallback1Server");
    mtsTestCallbackTrigger * serverTrigger = new mtsTestCallbackTrigger(server);
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay);
    clientTrigger->Stop();
    delete clientTrigger;
    delete client;
    serverTrigger->Stop();
    delete serverTrigger;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallback_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallback<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallback_int(void) {
    mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallback<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestFromSignalFromSignal(void)
{
    mtsTestFromSignal1<_elementType> * client = new mtsTestFromSignal1<_elementType>("mtsTestFromSignal1Client");
    mtsTestFromSignal1<_elementType> * server = new mtsTestFromSignal1<_elementType>("mtsTestFromSignal1Server");
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay);
    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestFromSignalFromSignal_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestFromSignalFromSignal<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestFromSignalFromSignal_int(void) {
    mtsCommandAndEventNetworkedTest::TestFromSignalFromSignal<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestPeriodicPeriodicBlocking(void)
{
    const double blockingDelay = 0.5 * cmn_s;
    mtsTestPeriodic1<_elementType> * client = new mtsTestPeriodic1<_elementType>("mtsTestPeriodic1Client");
    mtsTestPeriodic1<_elementType> * server = new mtsTestPeriodic1<_elementType>("mtsTestPeriodic1Server", blockingDelay);
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay, blockingDelay);
    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestPeriodicPeriodicBlocking_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestPeriodicPeriodicBlocking<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestPeriodicPeriodicBlocking_int(void) {
    mtsCommandAndEventNetworkedTest::TestPeriodicPeriodicBlocking<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestContinuousContinuousBlocking(void)
{
    const double blockingDelay = 0.5 * cmn_s;
    mtsTestContinuous1<_elementType> * client = new mtsTestContinuous1<_elementType>("mtsTestContinuous1Client");
    mtsTestContinuous1<_elementType> * server = new mtsTestContinuous1<_elementType>("mtsTestContinuous1Server", blockingDelay);
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay, blockingDelay);
    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestContinuousContinuousBlocking_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestContinuousContinuousBlocking<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestContinuousContinuousBlocking_int(void) {
    mtsCommandAndEventNetworkedTest::TestContinuousContinuousBlocking<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallbackBlocking(void)
{
    const double blockingDelay = 0.5 * cmn_s;
    mtsTestFromCallback1<_elementType> * client = new mtsTestFromCallback1<_elementType>("mtsTestFromCallback1Client");
    mtsTestCallbackTrigger * clientTrigger = new mtsTestCallbackTrigger(client);
    mtsTestFromCallback1<_elementType> * server = new mtsTestFromCallback1<_elementType>("mtsTestFromCallback1Server", blockingDelay);
    mtsTestCallbackTrigger * serverTrigger = new mtsTestCallbackTrigger(server);
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay, blockingDelay);
    clientTrigger->Stop();
    delete clientTrigger;
    delete client;
    serverTrigger->Stop();
    delete serverTrigger;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallbackBlocking_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallbackBlocking<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallbackBlocking_int(void) {
    mtsCommandAndEventNetworkedTest::TestFromCallbackFromCallbackBlocking<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestFromSignalFromSignalBlocking(void)
{
    const double blockingDelay = 0.5 * cmn_s;
    mtsTestFromSignal1<_elementType> * client = new mtsTestFromSignal1<_elementType>("mtsTestFromSignal1Client");
    mtsTestFromSignal1<_elementType> * server = new mtsTestFromSignal1<_elementType>("mtsTestFromSignal1Server", blockingDelay);
    // these delays are OS dependent, we might need to increase them later
    const double clientExecutionDelay = 0.1 * cmn_s;
    const double serverExecutionDelay = 0.1 * cmn_s;
    TestExecution(client, server, clientExecutionDelay, serverExecutionDelay, blockingDelay);
    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestFromSignalFromSignalBlocking_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestFromSignalFromSignalBlocking<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestFromSignalFromSignalBlocking_int(void) {
    mtsCommandAndEventNetworkedTest::TestFromSignalFromSignalBlocking<int>();
}


template <class _elementType>
void mtsCommandAndEventNetworkedTest::TestArgumentPrototypes(void)
{
    typedef _elementType value_type;

    mtsTestContinuous1<value_type> * client = new mtsTestContinuous1<value_type>("mtsTestContinuous1Client");
    mtsTestContinuous1<value_type> * server = new mtsTestContinuous1<value_type>("mtsTestContinuous1Server");

    mtsComponentManager * manager = mtsComponentManager::GetInstance();

    // add to manager and start all
    manager->AddComponent(client);
    manager->AddComponent(server);
    manager->Connect(client->GetName(), "r1", server->GetName(), "p1");
    manager->CreateAll();
    CPPUNIT_ASSERT(manager->WaitForStateAll(mtsComponentState::READY, TransitionDelay));
    manager->StartAll();
    CPPUNIT_ASSERT(manager->WaitForStateAll(mtsComponentState::ACTIVE, TransitionDelay));

    // test that values are set properly for all argument prototypes
    const mtsGenericObject * argumentPrototypeGeneric;
    const mtsInt * argumentPrototype;
    int expected, actual;

    // test argument prototype for void with result command
    argumentPrototypeGeneric = client->InterfaceRequired1.FunctionVoidReturn.GetResultPrototype();
    CPPUNIT_ASSERT(argumentPrototypeGeneric != 0);
    argumentPrototype = dynamic_cast<const mtsInt *>(argumentPrototypeGeneric);
    CPPUNIT_ASSERT(argumentPrototype != 0);
    expected = mtsTestInterfaceProvided<mtsInt>::Argument2PrototypeDefault;
    actual = *argumentPrototype;
    CPPUNIT_ASSERT_EQUAL(expected, actual);

    // test argument prototype for write command
    argumentPrototypeGeneric = client->InterfaceRequired1.FunctionWrite.GetArgumentPrototype();
    CPPUNIT_ASSERT(argumentPrototypeGeneric != 0);
    argumentPrototype = dynamic_cast<const mtsInt *>(argumentPrototypeGeneric);
    CPPUNIT_ASSERT(argumentPrototype != 0);
    expected = mtsTestInterfaceProvided<mtsInt>::Argument1PrototypeDefault;
    actual = *argumentPrototype;
    CPPUNIT_ASSERT_EQUAL(expected, actual);

    // test argument prototype for read command
    argumentPrototypeGeneric = client->InterfaceRequired1.FunctionRead.GetArgumentPrototype();
    CPPUNIT_ASSERT(argumentPrototypeGeneric != 0);
    argumentPrototype = dynamic_cast<const mtsInt *>(argumentPrototypeGeneric);
    CPPUNIT_ASSERT(argumentPrototype != 0);
    expected = mtsTestInterfaceProvided<mtsInt>::Argument1PrototypeDefault;
    actual = *argumentPrototype;
    CPPUNIT_ASSERT_EQUAL(expected, actual);

    // test argument prototype for qualified read command
    argumentPrototypeGeneric = client->InterfaceRequired1.FunctionQualifiedRead.GetArgument1Prototype();
    CPPUNIT_ASSERT(argumentPrototypeGeneric != 0);
    argumentPrototype = dynamic_cast<const mtsInt *>(argumentPrototypeGeneric);
    CPPUNIT_ASSERT(argumentPrototype != 0);
    expected = mtsTestInterfaceProvided<mtsInt>::Argument1PrototypeDefault;
    actual = *argumentPrototype;
    CPPUNIT_ASSERT_EQUAL(expected, actual);

    argumentPrototypeGeneric = client->InterfaceRequired1.FunctionQualifiedRead.GetArgument2Prototype();
    CPPUNIT_ASSERT(argumentPrototypeGeneric != 0);
    argumentPrototype = dynamic_cast<const mtsInt *>(argumentPrototypeGeneric);
    CPPUNIT_ASSERT(argumentPrototype != 0);
    expected = mtsTestInterfaceProvided<mtsInt>::Argument2PrototypeDefault;
    actual = *argumentPrototype;
    CPPUNIT_ASSERT_EQUAL(expected, actual);

    // stop all and cleanup
    manager->KillAll();
    CPPUNIT_ASSERT(manager->WaitForStateAll(mtsComponentState::FINISHED, TransitionDelay));
    manager->Disconnect(client->GetName(), "r1", server->GetName(), "p1");
    manager->RemoveComponent(client);
    manager->RemoveComponent(server);

    delete client;
    delete server;
}
void mtsCommandAndEventNetworkedTest::TestArgumentPrototypes_mtsInt(void) {
    mtsCommandAndEventNetworkedTest::TestArgumentPrototypes<mtsInt>();
}
void mtsCommandAndEventNetworkedTest::TestArgumentPrototypes_int(void) {
    mtsCommandAndEventNetworkedTest::TestArgumentPrototypes<int>();
}
#endif

CPPUNIT_TEST_SUITE_REGISTRATION(mtsCommandAndEventNetworkedTest);
