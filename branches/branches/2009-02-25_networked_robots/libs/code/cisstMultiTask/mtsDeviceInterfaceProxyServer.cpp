/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsDeviceInterfaceProxyServer.cpp 145 2009-03-18 23:32:40Z mjung5 $

  Author(s):  Min Yang Jung
  Created on: 2009-04-24

  (C) Copyright 2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstCommon/cmnAssert.h>
#include <cisstMultiTask/mtsDeviceInterfaceProxyServer.h>
#include <cisstMultiTask/mtsDeviceInterface.h>
#include <cisstMultiTask/mtsTaskManager.h>
#include <cisstMultiTask/mtsDeviceProxy.h>
#include <cisstMultiTask/mtsTask.h>

CMN_IMPLEMENT_SERVICES(mtsDeviceInterfaceProxyServer);

#define DeviceInterfaceProxyServerLogger(_log) Logger->trace("mtsDeviceInterfaceProxyServer", _log)
#define DeviceInterfaceProxyServerLoggerError(_log1, _log2) \
        std::stringstream s;\
        s << "mtsDeviceInterfaceProxyServer: " << _log1 << _log2;\
        Logger->error(s.str());

mtsDeviceInterfaceProxyServer::mtsDeviceInterfaceProxyServer(
    const std::string& adapterName, const std::string& endpointInfo,
    const std::string& communicatorID) 
    : BaseType(adapterName, endpointInfo, communicatorID), ConnectedTask(0)
{
    Serializer = new cmnSerializer(SerializationBuffer);
    DeSerializer = new cmnDeSerializer(DeSerializationBuffer);
}

mtsDeviceInterfaceProxyServer::~mtsDeviceInterfaceProxyServer()
{
    OnClose();
}

void mtsDeviceInterfaceProxyServer::OnClose()
{
    delete Serializer;
    delete DeSerializer;

    FunctionVoidProxyMap.DeleteAll();
    FunctionWriteProxyMap.DeleteAll();
    FunctionReadProxyMap.DeleteAll();
    FunctionQualifiedReadProxyMap.DeleteAll();
}

void mtsDeviceInterfaceProxyServer::Start(mtsTask * callingTask)
{
    // Initialize Ice object.
    // Notice that a worker thread is not created right now.
    Init();
    
    if (InitSuccessFlag) {
        // Create a worker thread here and returns immediately.
        ThreadArgumentsInfo.argument = callingTask;
        ThreadArgumentsInfo.proxy = this;
        ThreadArgumentsInfo.Runner = mtsDeviceInterfaceProxyServer::Runner;

        WorkerThread.Create<ProxyWorker<mtsTask>, ThreadArguments<mtsTask>*>(
            &ProxyWorkerInfo, &ProxyWorker<mtsTask>::Run, &ThreadArgumentsInfo, "C-PRX");
    }
}

void mtsDeviceInterfaceProxyServer::StartServer()
{
    Sender->Start();

    // This is a blocking call that should run in a different thread.
    IceCommunicator->waitForShutdown();
}

void mtsDeviceInterfaceProxyServer::Runner(ThreadArguments<mtsTask> * arguments)
{
    mtsDeviceInterfaceProxyServer * ProxyServer = 
        dynamic_cast<mtsDeviceInterfaceProxyServer*>(arguments->proxy);
    
    ProxyServer->SetConnectedTask(arguments->argument);
    
    //!!!!!!!!!!!!
    //mtsDeviceInterfaceProxy::ProvidedInterfaceSequence providedInterfaces;
    //ProxyServer->GetProvidedInterface(providedInterfaces);

    ProxyServer->GetLogger()->trace("mtsDeviceInterfaceProxyServer", "Proxy server starts.");

    try {
        ProxyServer->StartServer();
    } catch (const Ice::Exception& e) {
        ProxyServer->GetLogger()->trace("mtsDeviceInterfaceProxyServer error: ", e.what());
    } catch (const char * msg) {
        ProxyServer->GetLogger()->trace("mtsDeviceInterfaceProxyServer error: ", msg);
    }

    ProxyServer->OnThreadEnd();
}

void mtsDeviceInterfaceProxyServer::OnThreadEnd()
{
    DeviceInterfaceProxyServerLogger("Proxy server ends.");

    BaseType::OnThreadEnd();

    Sender->Destroy();
}

mtsProvidedInterface * mtsDeviceInterfaceProxyServer::GetProvidedInterface(
    const std::string resourceDeviceName, const std::string providedInterfaceName) const
{
    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();
    
    mtsProvidedInterface * providedInterface = NULL;
    mtsTask * resourceTask = NULL;
    mtsDevice * resourceDevice = NULL;

    // Get an original resource device or task
    resourceDevice = taskManager->GetDevice(resourceDeviceName);
    if (!resourceDevice) {
        resourceTask = taskManager->GetTask(resourceDeviceName);
        if (!resourceTask) {
            DeviceInterfaceProxyServerLoggerError("GetProvidedInterface: ", 
                "Cannot find an original resource device or task at server side: " + resourceDeviceName);
            return 0;
        }
        DeviceInterfaceProxyServerLogger("GetProvidedInterface: found an original resource TASK at server side: " + resourceDeviceName);
    } else {
        // Look for the task in the task map (i.e., if a resource task is of mtsTask type)
        DeviceInterfaceProxyServerLogger("GetProvidedInterface: found an original resource DEVICE at server side: " + resourceDeviceName);
    }

    // Get the provided interface
    if (resourceDevice) {
        providedInterface = resourceDevice->GetProvidedInterface(providedInterfaceName);        
    } else {
        CMN_ASSERT(resourceTask);
        providedInterface = resourceTask->GetProvidedInterface(providedInterfaceName);
    }

    CMN_ASSERT(providedInterface);

    return providedInterface;
}

bool mtsDeviceInterfaceProxyServer::PopulateRequiredInterfaceProxy(
    mtsRequiredInterface * requiredInterfaceProxy, mtsProvidedInterface * providedInterface)
{
    /*
    mtsRequiredInterface Robot("Robot");

    mtsFunctionRead GetPositionJoint;
    mtsFunctionWrite MovePositionJoint;

    // mts events callbacks, in this example started event is void,
    // end event is write
    void CallBackStarted(void);
    mtsCommandVoidBase * CallBackStartedCommand; 
    void CallBackFinished(const PositionJointType &);
    mtsCommandWriteBase * CallBackFinishedCommand; 



    Robot.AddFunction("GetPositionJoint", GetPositionJoint);
    Robot.AddFunction("MovePositionJoint", MovePositionJoint);
    // false --> Event handlers are not queued
    Robot.AddEventHandlerVoid(&userInterface::CallBackStarted, this,
                              "MotionStarted", false);
    Robot.AddEventHandlerWrite(&userInterface::CallBackFinished, this,
                  "MotionFinished", PositionJointType(NB_JOINTS), false);
    */

    // Get the list of commands
    mtsFunctionVoid  * functionVoidProxy = NULL;
    mtsFunctionWrite * functionWriteProxy = NULL;
    mtsFunctionRead  * functionReadProxy = NULL;
    mtsFunctionQualifiedRead * functionQualifiedReadProxy = NULL;

    //std::vector<std::string> namesOfCommandsVoid = providedInterface->GetNamesOfCommandsVoid();
    //for (unsigned int i = 0; i < namesOfCommandsVoid.size(); ++i) {
    //    functionVoidProxy = new mtsFunctionVoid(providedInterface, namesOfCommandsVoid[i]);
    //    CMN_ASSERT(FunctionVoidProxyMap.AddItem(namesOfCommandsVoid[i], functionVoidProxy));
    //    CMN_ASSERT(requiredInterfaceProxy->AddFunction(namesOfCommandsVoid[i], *functionVoidProxy));
    //}
#define ADD_FUNCTION_PROXY_BEGIN(_commandType)\
    std::vector<std::string> namesOfCommands##_commandType = providedInterface->GetNamesOfCommands##_commandType##();\
    for (unsigned int i = 0; i < namesOfCommands##_commandType.size(); ++i) {\
        function##_commandType##Proxy = new mtsFunction##_commandType##(providedInterface, namesOfCommands##_commandType##[i]);\
        CMN_ASSERT(Function##_commandType##ProxyMap.AddItem(namesOfCommands##_commandType[i], function##_commandType##Proxy));\
        CMN_ASSERT(requiredInterfaceProxy->AddFunction(namesOfCommands##_commandType##[i], *function##_commandType##Proxy));
#define ADD_FUNCTION_PROXY_END\
    }

    ADD_FUNCTION_PROXY_BEGIN(Void);
    ADD_FUNCTION_PROXY_END;

    ADD_FUNCTION_PROXY_BEGIN(Write);
    ADD_FUNCTION_PROXY_END;

    ADD_FUNCTION_PROXY_BEGIN(Read);
    ADD_FUNCTION_PROXY_END;

    ADD_FUNCTION_PROXY_BEGIN(QualifiedRead);
    ADD_FUNCTION_PROXY_END;

    // Using AllocateResources(), get pointers allocated for this required interface.
    unsigned int userId;
    userId = providedInterface->AllocateResources(requiredInterfaceProxy->GetName() + "Proxy");

    // Connect to the original device or task that provides allocated resources.
    requiredInterfaceProxy->ConnectTo(providedInterface);
    if (!requiredInterfaceProxy->BindCommandsAndEvents(userId)) {
        DeviceInterfaceProxyServerLoggerError(
            "PopulateRequiredInterfaceProxy", "BindCommandsAndEvents failed.");
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------
//  Task Processing
//-------------------------------------------------------------------------
const bool mtsDeviceInterfaceProxyServer::ReceiveGetProvidedInterfaces(
    ::mtsDeviceInterfaceProxy::ProvidedInterfaceSequence & providedInterfaces)
{
    CMN_ASSERT(ConnectedTask);

    // 1) Iterate all provided interfaces
    mtsDeviceInterface * providedInterface = NULL;    

    std::vector<std::string> namesOfProvidedInterfaces = 
        ConnectedTask->GetNamesOfProvidedInterfaces();
    std::vector<std::string>::const_iterator it = namesOfProvidedInterfaces.begin();
    for (; it != namesOfProvidedInterfaces.end(); ++it) {
        mtsDeviceInterfaceProxy::ProvidedInterface providedInterfaceSpec;

        // 1) Get a provided interface object
        providedInterface = ConnectedTask->GetProvidedInterface(*it);
        CMN_ASSERT(providedInterface);

        // 2) Get a provided interface name.
        providedInterfaceSpec.interfaceName = providedInterface->GetName();

        // TODO: Maybe I can just assume that only mtsDeviceInterface is used.
        // Determine the type of the provided interface: is it mtsDeviceInterface or 
        // mtsDeviceInterface?
        //if (dynamic_cast<mtsDeviceInterface*>(providedInterface)) {
        //    providedInterfaceSpec.providedInterfaceForTask = true;
        //} else {
            providedInterfaceSpec.providedInterfaceForTask = false;
        //}
            
        // 3) Extract all the information on registered command objects, events, and so on.
#define ITERATE_INTERFACE_BEGIN( _commandType ) \
        mtsDeviceInterface::Command##_commandType##MapType::MapType::const_iterator iterator##_commandType = \
            providedInterface->Commands##_commandType.GetMap().begin();\
        mtsDeviceInterface::Command##_commandType##MapType::MapType::const_iterator iterator##_commandType##End = \
            providedInterface->Commands##_commandType.GetMap().end();\
        for (; iterator##_commandType != iterator##_commandType##End; ++( iterator##_commandType ) ) {\
            mtsDeviceInterfaceProxy::Command##_commandType##Info info;\
            info.Name = iterator##_commandType->second->GetName();\
            info.CommandId = reinterpret_cast<int>(iterator##_commandType->second);

#define ITERATE_INTERFACE_END( _commandType ) \
            providedInterfaceSpec.commands##_commandType.push_back(info);\
        }

        // 3-1) Command: Void
        ITERATE_INTERFACE_BEGIN(Void);            
        ITERATE_INTERFACE_END(Void);

        // 3-2) Command: Write
        ITERATE_INTERFACE_BEGIN(Write);
            info.ArgumentTypeName = iteratorWrite->second->GetArgumentClassServices()->GetName();
        ITERATE_INTERFACE_END(Write);

        // 3-3) Command: Read
        ITERATE_INTERFACE_BEGIN(Read);
            info.ArgumentTypeName = iteratorRead->second->GetArgumentClassServices()->GetName();
        ITERATE_INTERFACE_END(Read);

        // 3-4) Command: QualifiedRead
        ITERATE_INTERFACE_BEGIN(QualifiedRead);
            info.Argument1TypeName = iteratorQualifiedRead->second->GetArgument1Prototype()->Services()->GetName();
            info.Argument2TypeName = iteratorQualifiedRead->second->GetArgument2Prototype()->Services()->GetName();
        ITERATE_INTERFACE_END(QualifiedRead);

#undef ITERATE_INTERFACE_BEGIN
#undef ITERATE_INTERFACE_END

        // TODO: 
        // 4) Extract events information (void, write)

        providedInterfaces.push_back(providedInterfaceSpec);
    }

    return true;
}

bool mtsDeviceInterfaceProxyServer::ReceiveConnectServerSide(
    const std::string & userTaskName, const std::string & requiredInterfaceName,
    const std::string & resourceTaskName, const std::string & providedInterfaceName)
{
    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();

    const std::string clientDeviceProxyName = mtsDeviceProxy::GetClientTaskProxyName(
        resourceTaskName, providedInterfaceName, userTaskName, requiredInterfaceName);

    // Get an original provided interface.
    mtsProvidedInterface * providedInterface = GetProvidedInterface(
        resourceTaskName, providedInterfaceName);
    if (!providedInterface) {
        DeviceInterfaceProxyServerLoggerError("ReceiveConnectServerSide: cannot find a provided interface: ", providedInterface);
        return false;
    }

    // Create a client task proxy (mtsDevice) and a required Interface proxy (mtsRequiredInterface)
    mtsDeviceProxy * clientTaskProxy = new mtsDeviceProxy(clientDeviceProxyName);
    if (!taskManager->AddDevice(clientTaskProxy)) {
        DeviceInterfaceProxyServerLoggerError("ReceiveConnectServerSide: cannot add a device proxy: ", clientDeviceProxyName);
        return false;
    }

    mtsRequiredInterface * requiredInterfaceProxy = 
        clientTaskProxy->AddRequiredInterface(requiredInterfaceName);
    if (!requiredInterfaceProxy) {
        DeviceInterfaceProxyServerLoggerError("ReceiveConnectServerSide: cannot add required interface: ", requiredInterfaceName);
        return false;
    }

    // Populate a required Interface proxy
    if (!PopulateRequiredInterfaceProxy(requiredInterfaceProxy, providedInterface)) {
        DeviceInterfaceProxyServerLoggerError("ReceiveConnectServerSide: failed to populate a required interface proxy: ", requiredInterfaceName);
        return false;
    }

    // Connect() locally    
    if (!taskManager->Connect(
        clientDeviceProxyName, requiredInterfaceName, resourceTaskName, providedInterfaceName)) 
    {
        DeviceInterfaceProxyServerLoggerError("ReceiveConnectServerSide: failed to connect: ", 
            userTaskName + " : " + requiredInterfaceName + " - " + 
            resourceTaskName + " : " +  providedInterfaceName);
        return false;
    }

    // After Connect() succeeds at server side, update command id of command proxies 
    // at client side.
    //

    DeviceInterfaceProxyServerLogger("Connect() at server side succeeded: " +
        userTaskName + " : " + requiredInterfaceName + " - " + 
        resourceTaskName + " : " +  providedInterfaceName);

    return true;
}

void mtsDeviceInterfaceProxyServer::ReceiveExecuteCommandVoid(const int commandId) const
{    
    mtsCommandVoidBase * commandVoid = reinterpret_cast<mtsCommandVoidBase *>(commandId);
    CMN_ASSERT(commandVoid);

    commandVoid->Execute();
}

void mtsDeviceInterfaceProxyServer::ReceiveExecuteCommandWriteSerialized(const int commandId, const std::string argument)
{
    mtsCommandWriteBase * commandWrite = reinterpret_cast<mtsCommandWriteBase *>(commandId);
    CMN_ASSERT(commandWrite);

    static char buf[100];
    sprintf(buf, "ExecuteCommandWriteSerialized: %d bytes received", argument.size());
    Logger->trace("TIServer", buf);

    // Deserialization
    DeSerializationBuffer.str("");
    DeSerializationBuffer << argument;
    
    mtsGenericObject * obj = dynamic_cast<mtsGenericObject *>(DeSerializer->DeSerialize());
    CMN_ASSERT(obj);
    //!!!!!!!!!!! FIX THIS
    //commandWrite->Execute(*obj);

    //std::cout << *obj << std::endl;
}

void mtsDeviceInterfaceProxyServer::ReceiveExecuteCommandReadSerialized(const int commandId, std::string & argument)
{
    mtsCommandReadBase * commandRead = reinterpret_cast<mtsCommandReadBase *>(commandId);    
    CMN_ASSERT(commandRead);

    // Create a placeholder
    mtsGenericObject * placeHolder = dynamic_cast<mtsGenericObject *>(commandRead->GetArgumentClassServices()->Create());
    CMN_ASSERT(placeHolder);
    {
        commandRead->Execute(*placeHolder);

        // Serialization
        SerializationBuffer.str("");
        Serializer->Serialize(*placeHolder);
        std::string s = SerializationBuffer.str();

        argument = s;
    }
    delete placeHolder;    
}

void mtsDeviceInterfaceProxyServer::ReceiveExecuteCommandQualifiedReadSerialized(const int commandId, const std::string argument1, std::string & argument2)
{
}

//-------------------------------------------------------------------------
//  Definition by mtsTaskManagerProxy.ice
//-------------------------------------------------------------------------
mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::DeviceInterfaceServerI(
    const Ice::CommunicatorPtr& communicator,
    const Ice::LoggerPtr& logger,
    mtsDeviceInterfaceProxyServer * DeviceInterfaceServer) 
    : Communicator(communicator), Logger(logger),
      DeviceInterfaceServer(DeviceInterfaceServer),
      Runnable(true),
      Sender(new SendThread<DeviceInterfaceServerIPtr>(this))
{
}

void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::Start()
{
    DeviceInterfaceProxyServerLogger("Send thread starts");

    Sender->start();
}

void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::Run()
{
    int num = 0;
    while(true)
    {
        std::set<mtsDeviceInterfaceProxy::DeviceInterfaceClientPrx> clients;
        {
            IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);
            timedWait(IceUtil::Time::seconds(2));

            if(!Runnable)
            {
                break;
            }

            clients = _clients;
        }

#ifdef _COMMUNICATION_TEST_
        if(!clients.empty())
        {
            ++num;
            for(std::set<mtsDeviceInterfaceProxy::DeviceInterfaceClientPrx>::iterator p 
                = clients.begin(); p != clients.end(); ++p)
            {
                try
                {
                    std::cout << "server sends: " << num << std::endl;
                    (*p)->ReceiveData(num);
                }
                catch(const IceUtil::Exception& ex)
                {
                    std::cerr << "removing client `" << Communicator->identityToString((*p)->ice_getIdentity()) << "':\n"
                        << ex << std::endl;

                    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);
                    _clients.erase(*p);
                }
            }
        }
#endif
    }
}

void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::Destroy()
{
    DeviceInterfaceProxyServerLogger("Send thread is terminating.");

    IceUtil::ThreadPtr callbackSenderThread;

    {
        IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

        DeviceInterfaceProxyServerLogger("Destroying sender.");
        Runnable = false;

        notify();

        callbackSenderThread = Sender;
        Sender = 0; // Resolve cyclic dependency.
    }

    callbackSenderThread->getThreadControl().join();
}

//-----------------------------------------------------------------------------
//  Proxy Server Implementation
//-----------------------------------------------------------------------------
void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::AddClient(
    const ::Ice::Identity& ident, const ::Ice::Current& current)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

    Logger->trace("TIServer", "<<<<< RECV: AddClient: " + Communicator->identityToString(ident));

    mtsDeviceInterfaceProxy::DeviceInterfaceClientPrx client = 
        mtsDeviceInterfaceProxy::DeviceInterfaceClientPrx::uncheckedCast(current.con->createProxy(ident));
    
    _clients.insert(client);
}

bool mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::GetProvidedInterfaces(
    ::mtsDeviceInterfaceProxy::ProvidedInterfaceSequence & providedInterfaces, 
    const ::Ice::Current& current) const
{
    Logger->trace("TIServer", "<<<<< RECV: GetProvidedInterface");

    return DeviceInterfaceServer->ReceiveGetProvidedInterfaces(providedInterfaces);
}

bool mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::ConnectServerSide(
    const std::string & userTaskName, const std::string & requiredInterfaceName,
    const std::string & resourceTaskName, const std::string & providedInterfaceName,
    const ::Ice::Current&)
{
    Logger->trace("TIServer", "<<<<< RECV: ConnectServerSide");
    
    return DeviceInterfaceServer->ReceiveConnectServerSide(
        userTaskName, requiredInterfaceName, resourceTaskName, providedInterfaceName);
}
            
void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::ExecuteCommandVoid(
    ::Ice::Int sid, const ::Ice::Current&)
{
    //Logger->trace("TIServer", "<<<<< RECV: ExecuteCommandVoid");

    DeviceInterfaceServer->ReceiveExecuteCommandVoid(sid);
}

void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::ExecuteCommandWriteSerialized(
    ::Ice::Int sid, const ::std::string& argument, const ::Ice::Current&)
{
    //Logger->trace("TIServer", "<<<<< RECV: ExecuteCommandWriteSerialized");

    DeviceInterfaceServer->ReceiveExecuteCommandWriteSerialized(sid, argument);
}

void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::ExecuteCommandReadSerialized(
    ::Ice::Int sid, ::std::string& argument, const ::Ice::Current&)
{
    //Logger->trace("TIServer", "<<<<< RECV: ExecuteCommandReadSerialized");

    DeviceInterfaceServer->ReceiveExecuteCommandReadSerialized(sid, argument);
}

void mtsDeviceInterfaceProxyServer::DeviceInterfaceServerI::ExecuteCommandQualifiedReadSerialized(
    ::Ice::Int, const ::std::string&, ::std::string&, const ::Ice::Current&)
{
}
