/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Ankur Kapoor, Peter Kazanzides, Anton Deguet
  Created on: 2004-04-30

  (C) Copyright 2004-2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstCommon/cmnPortability.h>
#include <cisstOSAbstraction/osaThread.h>
#include <cisstOSAbstraction/osaThreadBuddy.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>
#include <cisstMultiTask/mtsCommandQueuedVoid.h>
#include <cisstMultiTask/mtsCommandQueuedVoidReturn.h>
#include <cisstMultiTask/mtsCommandQueuedWrite.h>
#include <cisstMultiTask/mtsCommandQueuedWriteReturn.h>
#include <cisstMultiTask/mtsCommandFilteredWrite.h>
#include <cisstMultiTask/mtsCommandFilteredQueuedWrite.h>
#include <cisstMultiTask/mtsComponent.h>

#include <iostream>
#include <string>


mtsInterfaceProvided::mtsInterfaceProvided(const std::string & name, mtsComponent * component,
                                           mtsInterfaceQueueingPolicy queueingPolicy,
                                           mtsCallableVoidBase * postCommandQueuedCallable):
    BaseType(name, component),
    MailBox(0),
    QueueingPolicy(queueingPolicy),
    MailBoxSize(DEFAULT_MAIL_BOX_AND_ARGUMENT_QUEUES_SIZE),
    ArgumentQueuesSize(DEFAULT_MAIL_BOX_AND_ARGUMENT_QUEUES_SIZE),
    OriginalInterface(0),
    UserCounter(0),
    CommandsVoid("CommandsVoid", true),
    CommandsVoidReturn("CommandsVoidReturn", true),
    CommandsWrite("CommandsWrite", true),
    CommandsRead("CommandsRead", true),
    CommandsQualifiedRead("CommandsQualifiedRead", true),
    EventVoidGenerators("EventVoidGenerators", true),
    EventWriteGenerators("EventWriteGenerators", true),
    CommandsInternal("CommandsInternal", true),
    PostCommandQueuedCallable(postCommandQueuedCallable)
{
    // make sure queueing policy is set
    if (queueingPolicy == MTS_COMPONENT_POLICY) {
        CMN_LOG_CLASS_INIT_ERROR << "constructor: interface queueing policy has not been set correctly for component \""
                                 << name << "\"" << std::endl;
    }
    // by default, if queueing is required we assume this is not an end
    // user interface but a factory.  this setting can only be changed
    // by the factory interface using the private method SetAsEndUser.
    if (queueingPolicy == MTS_COMMANDS_SHOULD_BE_QUEUED) {
        EndUserInterface = false;
    } else {
        EndUserInterface = true;
    }
    // consistency check
    if (postCommandQueuedCallable && (queueingPolicy == MTS_COMMANDS_SHOULD_NOT_BE_QUEUED)) {
        CMN_LOG_CLASS_INIT_ERROR << "constructor: a post command queued callable has been provided while queueing is turned off for component \""
                                 << name << "\"" << std::endl;
    }
    // set owner for all maps (to make logs more readable)
    CommandsVoid.SetOwner(*this);
    CommandsVoidReturn.SetOwner(*this);
    CommandsWrite.SetOwner(*this);
    CommandsRead.SetOwner(*this);
    CommandsQualifiedRead.SetOwner(*this);
    EventVoidGenerators.SetOwner(*this);
    EventWriteGenerators.SetOwner(*this);
    CommandsInternal.SetOwner(*this);
}


mtsInterfaceProvided::mtsInterfaceProvided(mtsInterfaceProvided * originalInterface,
                                           const std::string & userName,
                                           size_t mailBoxSize,
                                           size_t argumentQueuesSize):
    BaseType(mtsInterfaceProvided::GetEndUserInterfaceName(originalInterface, userName),
             originalInterface->Component),
    MailBox(0),
    QueueingPolicy(MTS_COMMANDS_SHOULD_BE_QUEUED),
    MailBoxSize(mailBoxSize),
    ArgumentQueuesSize(argumentQueuesSize),
    OriginalInterface(originalInterface),
    EndUserInterface(true),
    UserCounter(0),
    CommandsVoid("CommandsVoid", true),
    CommandsVoidReturn("CommandsVoidReturn", true),
    CommandsWrite("CommandsWrite", true),
    CommandsRead("CommandsRead", true),
    CommandsQualifiedRead("CommandsQualifiedRead", true),
    EventVoidGenerators("EventVoidGenerators", true),
    EventWriteGenerators("EventWriteGenerators", true),
    CommandsInternal("CommandsInternal", true),
    PostCommandQueuedCallable(originalInterface->PostCommandQueuedCallable)
{
    // set owner for all maps (to make logs more readable)
    CommandsVoid.SetOwner(*this);
    CommandsVoidReturn.SetOwner(*this);
    CommandsWrite.SetOwner(*this);
    CommandsRead.SetOwner(*this);
    CommandsQualifiedRead.SetOwner(*this);
    EventVoidGenerators.SetOwner(*this);
    EventWriteGenerators.SetOwner(*this);
    CommandsInternal.SetOwner(*this);

    // duplicate what needs to be duplicated (i.e. void and write
    // commands)
    MailBox = new mtsMailBox(this->GetName(),
                             mailBoxSize,
                             this->PostCommandQueuedCallable);

    // clone void commands
    CommandVoidMapType::const_iterator iterVoid = originalInterface->CommandsVoid.begin();
    const CommandVoidMapType::const_iterator endVoid = originalInterface->CommandsVoid.end();
    mtsCommandVoid * commandVoid;
    mtsCommandQueuedVoid * commandQueuedVoid;
    for (;
         iterVoid != endVoid;
         iterVoid++) {
        commandQueuedVoid = dynamic_cast<mtsCommandQueuedVoid *>(iterVoid->second);
        if (commandQueuedVoid) {
            commandVoid = commandQueuedVoid->Clone(this->MailBox, argumentQueuesSize);
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: cloned queued void command \"" << iterVoid->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        } else {
            commandVoid = iterVoid->second;
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: using existing pointer on void command \"" << iterVoid->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        }
        CommandsVoid.AddItem(iterVoid->first, commandVoid, CMN_LOG_LEVEL_INIT_ERROR);

    }
    // clone void return commands
    CommandVoidReturnMapType::const_iterator iterVoidReturn = originalInterface->CommandsVoidReturn.begin();
    const CommandVoidReturnMapType::const_iterator endVoidReturn = originalInterface->CommandsVoidReturn.end();
    mtsCommandVoidReturn * commandVoidReturn;
    mtsCommandQueuedVoidReturn * commandQueuedVoidReturn;
    for (;
         iterVoidReturn != endVoidReturn;
         iterVoidReturn++) {
        commandQueuedVoidReturn = dynamic_cast<mtsCommandQueuedVoidReturn *>(iterVoidReturn->second);
        if (commandQueuedVoidReturn) {
            commandVoidReturn = commandQueuedVoidReturn->Clone(this->MailBox);
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: cloned queued void return command \"" << iterVoidReturn->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        } else {
            commandVoidReturn = iterVoidReturn->second;
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: using existing pointer on void return command \"" << iterVoidReturn->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        }
        CommandsVoidReturn.AddItem(iterVoidReturn->first, commandVoidReturn, CMN_LOG_LEVEL_INIT_ERROR);

    }
    // clone write commands
    CommandWriteMapType::const_iterator iterWrite = originalInterface->CommandsWrite.begin();
    const CommandWriteMapType::const_iterator endWrite = originalInterface->CommandsWrite.end();
    mtsCommandWriteBase * commandWrite;
    mtsCommandQueuedWriteBase * commandQueuedWrite;
    for (;
         iterWrite != endWrite;
         iterWrite++) {
        commandQueuedWrite = dynamic_cast<mtsCommandQueuedWriteBase *>(iterWrite->second);
        if (commandQueuedWrite) {
            commandWrite = commandQueuedWrite->Clone(this->MailBox, argumentQueuesSize);
            CMN_LOG_CLASS_INIT_VERBOSE << "constructor: cloned queued write command " << iterWrite->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        } else {
            commandWrite = iterWrite->second;
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: using existing pointer on write command \"" << iterWrite->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        }
        CommandsWrite.AddItem(iterWrite->first, commandWrite, CMN_LOG_LEVEL_INIT_ERROR);
    }
    // clone write return commands
    CommandWriteReturnMapType::const_iterator iterWriteReturn = originalInterface->CommandsWriteReturn.begin();
    const CommandWriteReturnMapType::const_iterator endWriteReturn = originalInterface->CommandsWriteReturn.end();
    mtsCommandWriteReturn * commandWriteReturn;
    mtsCommandQueuedWriteReturn * commandQueuedWriteReturn;
    for (;
         iterWriteReturn != endWriteReturn;
         iterWriteReturn++) {
        commandQueuedWriteReturn = dynamic_cast<mtsCommandQueuedWriteReturn *>(iterWriteReturn->second);
        if (commandQueuedWriteReturn) {
            commandWriteReturn = commandQueuedWriteReturn->Clone(this->MailBox);
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: cloned queued write return command \"" << iterWriteReturn->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        } else {
            commandWriteReturn = iterWriteReturn->second;
            CMN_LOG_CLASS_INIT_VERBOSE << "factory constructor: using existing pointer on write return command \"" << iterWriteReturn->first
                                       << "\" for \"" << this->GetName() << "\"" << std::endl;
        }
        CommandsWriteReturn.AddItem(iterWriteReturn->first, commandWriteReturn, CMN_LOG_LEVEL_INIT_ERROR);

    }

}


mtsInterfaceProvided::~mtsInterfaceProvided()
{
	CMN_LOG_CLASS_INIT_VERBOSE << "Class mtsInterfaceProvided: Class destructor" << std::endl;
    // ADV: Need to add all cleanup, i.e. make sure all mailboxes are
    // properly deleted.
}


void mtsInterfaceProvided::Cleanup(void)
{
    CMN_LOG_CLASS_INIT_ERROR << "Cleanup: need to cleanup all created interfaces ... (not implemented yet)" << std::endl;
#if 0 // adeguet1, adv
    InterfacesProvidedCreatedType::iterator op;
    for (op = QueuedCommands.begin(); op != QueuedCommands.end(); op++) {
        delete op->second->GetMailBox();
        delete op->second;
    }
    QueuedCommands.erase(QueuedCommands.begin(), QueuedCommands.end());
#endif
	CMN_LOG_CLASS_INIT_VERBOSE << "Done base class Cleanup " << Name << std::endl;
}


void mtsInterfaceProvided::SetMailBoxSize(size_t desiredSize)
{
    if (this->QueueingPolicy == MTS_COMMANDS_SHOULD_NOT_BE_QUEUED) {
        CMN_LOG_CLASS_INIT_WARNING << "SetMailBoxSize: interface \"" << this->GetName()
                                   << "\" is not queuing commands, calling SetMailBoxSize has no effect"
                                   << std::endl;
    }
    this->MailBoxSize = desiredSize;
}


void mtsInterfaceProvided::SetArgumentQueuesSize(size_t desiredSize)
{
    if (this->QueueingPolicy == MTS_COMMANDS_SHOULD_NOT_BE_QUEUED) {
        CMN_LOG_CLASS_INIT_WARNING << "SetArgumentQueuesSize: interface \"" << this->GetName()
                                   << "\" is not queuing commands, calling SetArgumentQueuesSize has no effect"
                                   << std::endl;
    }
    if (desiredSize > this->MailBoxSize) {
        CMN_LOG_CLASS_INIT_WARNING << "SetArgumentQueuesSize: interface \"" << this->GetName()
                                   << "\" new size (" << desiredSize
                                   << ") is smaller than command mail box size (" << this->MailBoxSize
                                   << "), the extra space won't be used" << std::endl;
    }
    this->ArgumentQueuesSize = desiredSize;
}


void mtsInterfaceProvided::SetMailBoxAndArgumentQueuesSize(size_t desiredSize)
{
    this->SetMailBoxSize(desiredSize);
    this->SetArgumentQueuesSize(desiredSize);
}



// Execute all commands in the mailbox.  This is just a temporary implementation, where
// all commands in a mailbox are executed before moving on the next mailbox.  The final
// implementation will probably look at timestamps.  We may also want to pass in a
// parameter (enum) to set the mailbox processing policy.
size_t mtsInterfaceProvided::ProcessMailBoxes(void)
{
    if (!this->EndUserInterface) {
        size_t numberOfCommands = 0;
        InterfaceProvidedCreatedListType::iterator iterator = InterfacesProvidedCreated.begin();
        //const InterfaceProvidedCreatedVectorType::iterator end = InterfacesProvidedCreated.end();
        mtsMailBox * mailBox;
        for (;
             //iterator != end;
             iterator != InterfacesProvidedCreated.end();
             ++iterator) {
            mailBox = iterator->second->GetMailBox();
            while (mailBox->ExecuteNext()) {
                numberOfCommands++;
            }
        }
        return numberOfCommands;
    }
    CMN_LOG_CLASS_RUN_ERROR << "ProcessMailBoxes: called on end user interface for " << this->GetName() << std::endl;
    return 0;
}


void mtsInterfaceProvided::ToStream(std::ostream & outputStream) const
{
    outputStream << "Provided Interface \"" << Name << "\"" << std::endl;
    CommandsVoid.ToStream(outputStream);
    CommandsVoidReturn.ToStream(outputStream);
    CommandsWrite.ToStream(outputStream);
    CommandsWriteReturn.ToStream(outputStream);
    CommandsRead.ToStream(outputStream);
    CommandsQualifiedRead.ToStream(outputStream);
    EventVoidGenerators.ToStream(outputStream);
    EventWriteGenerators.ToStream(outputStream);
}



mtsMailBox * mtsInterfaceProvided::GetMailBox(void)
{
    return this->MailBox;
}


bool mtsInterfaceProvided::UseQueueBasedOnInterfacePolicy(mtsCommandQueueingPolicy queueingPolicy,
                                                          const std::string & methodName,
                                                          const std::string & commandName)
{
    if (queueingPolicy == MTS_INTERFACE_COMMAND_POLICY) {
        if (this->QueueingPolicy == MTS_COMMANDS_SHOULD_BE_QUEUED) {
            return true;
        } else {
            return false;
        }
    }
    if (queueingPolicy == MTS_COMMAND_NOT_QUEUED) {
        // send warning if queueing is "disabled"
        if (this->QueueingPolicy == MTS_COMMANDS_SHOULD_BE_QUEUED) {
            CMN_LOG_CLASS_INIT_WARNING << methodName << ": adding non queued void command \""
                                       << commandName << "\" to provided interface \""
                                       << this->GetName()
                                       << "\" which has beed created with policy MTS_COMMANDS_SHOULD_BE_QUEUED, thread safety has to be provided by the underlying method"
                                       << std::endl;
        } else {
            // send message to tell explicit queueing policy is useless
            CMN_LOG_CLASS_INIT_DEBUG << methodName << ": adding non queued void command \""
                                     << commandName << "\" to provided interface \""
                                     << this->GetName()
                                     << "\" which has beed created with policy MTS_COMMANDS_SHOULD_NOT_BE_QUEUED, this is the default therefore there is no need to explicitely define the queueing policy"
                                     << std::endl;
        }
        return false;
    }
    if (queueingPolicy == MTS_COMMAND_QUEUED) {
        // send error if the interface has no mailbox, can not queue
        if (this->QueueingPolicy == MTS_COMMANDS_SHOULD_BE_QUEUED) {
            // send message to tell explicit queueing policy is useless
            CMN_LOG_CLASS_INIT_DEBUG << methodName << ": adding queued void command \""
                                     << commandName << "\" to provided interface \""
                                     << this->GetName()
                                     << "\" which has beed created with policy MTS_COMMANDS_SHOULD_BE_QUEUED, this is the default therefore there is no need to explicitely define the queueing policy"
                                     << std::endl;
            return true;
        } else {
            // this is a case we can not handle
            CMN_LOG_CLASS_INIT_ERROR << methodName << ": adding queued void command \""
                                     << commandName << "\" to provided interface \""
                                     << this->GetName()
                                     << "\" which has beed created with policy MTS_COMMANDS_SHOULD_NOT_BE_QUEUED is not possible.  The command will NOT be queued"
                                     << std::endl;
            return false;
        }
    }
    CMN_LOG_CLASS_INIT_ERROR << "UseQueueBasedOnInterfacePolicy: this case should nerver happen" << std::endl;
    return false;
}


mtsCommandVoid * mtsInterfaceProvided::AddCommandVoid(mtsCallableVoidBase * callable,
                                                      const std::string & name,
                                                      mtsCommandQueueingPolicy queueingPolicy)
{
    // check that the input is valid
    if (callable) {
        // determine if this should be a queued command or not
        bool queued = this->UseQueueBasedOnInterfacePolicy(queueingPolicy, "AddCommandVoid", name);
        if (!queued) {
            mtsCommandVoid * command = new mtsCommandVoid(callable, name);
            if (!CommandsVoid.AddItem(name, command, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete command;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoid: unable to add command \""
                                         << command->GetName() << "\"" << std::endl;
            }
            return command;
        } else {
            // create with no mailbox
            mtsCommandQueuedVoid * queuedCommand = new mtsCommandQueuedVoid(callable, name, 0, 0);
            if (!CommandsVoid.AddItem(name, queuedCommand, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete queuedCommand;
                queuedCommand = 0;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoid: unable to add queued command \""
                                         << queuedCommand->GetName() << "\"" << std::endl;
            }
            return queuedCommand;
        }
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoid: attempt to add undefined command (null callable pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandVoid * mtsInterfaceProvided::AddCommandVoid(mtsCommandVoid * command)
{
    // check that the input is valid
    if (command) {
        if (!CommandsVoid.AddItem(command->GetName(), command, CMN_LOG_LEVEL_INIT_ERROR)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoid: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
        }
        return command;
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoid: attempt to add undefined command (null command pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandVoidReturn * mtsInterfaceProvided::AddCommandVoidReturn(mtsCallableVoidReturnBase * callable,
                                                                  const std::string & name,
                                                                  mtsGenericObject * resultPrototype,
                                                                  mtsCommandQueueingPolicy queueingPolicy)
{
    // check that the input is valid
    if (callable) {
        // determine if this should be a queued command or not
        bool queued = this->UseQueueBasedOnInterfacePolicy(queueingPolicy, "AddCommandVoidReturn", name);
        if (!queued) {
            mtsCommandVoidReturn * command = new mtsCommandVoidReturn(callable, name, resultPrototype);
            if (!CommandsVoidReturn.AddItem(name, command, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete command;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoidReturn: unable to add command \""
                                         << command->GetName() << "\"" << std::endl;
            }
            return command;
        } else {
            // create with no mailbox
            mtsCommandQueuedVoidReturn * queuedCommand = new mtsCommandQueuedVoidReturn(callable, name, resultPrototype, 0);
            if (!CommandsVoidReturn.AddItem(name, queuedCommand, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete queuedCommand;
                queuedCommand = 0;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoidReturn: unable to add queued command \""
                                         << queuedCommand->GetName() << "\"" << std::endl;
            }
            return queuedCommand;
        }
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoidReturn: attempt to add undefined command (null callable pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandVoidReturn * mtsInterfaceProvided::AddCommandVoidReturn(mtsCommandVoidReturn * command)
{
    // check that the input is valid
    if (command) {
        if (!CommandsVoidReturn.AddItem(command->GetName(), command, CMN_LOG_LEVEL_INIT_ERROR)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoidReturn: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
        }
        return command;
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandVoidReturn: attempt to add undefined command (null command pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandWriteBase * mtsInterfaceProvided::AddCommandWrite(mtsCommandWriteBase * command, mtsCommandQueueingPolicy queueingPolicy)
{
    // check that the input is valid
    if (command) {
        // determine if this should be a queued command or not
        bool queued = this->UseQueueBasedOnInterfacePolicy(queueingPolicy, "AddCommandWrite", command->GetName());
        if (!queued) {
            if (!CommandsWrite.AddItem(command->GetName(), command, CMN_LOG_LEVEL_INIT_ERROR)) {
                command = 0;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandWrite: unable to add command \""
                                         << command->GetName() << "\"" << std::endl;
            }
            return command;
        } else {
            // create with no mailbox
            mtsCommandQueuedWriteBase * queuedCommand = new mtsCommandQueuedWriteGeneric(0, command, 0);
            if (!CommandsWrite.AddItem(command->GetName(), queuedCommand, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete queuedCommand;
                queuedCommand = 0;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandWrite: unable to add queued command \""
                                         << command->GetName() << "\"" << std::endl;
            }
            return queuedCommand;
        }
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandWrite: attempt to add undefined command (null pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandWriteReturn * mtsInterfaceProvided::AddCommandWriteReturn(mtsCallableWriteReturnBase * callable,
                                                                    const std::string & name,
                                                                    mtsGenericObject * argumentPrototype,
                                                                    mtsGenericObject * resultPrototype,
                                                                    mtsCommandQueueingPolicy queueingPolicy)
{
    // check that the input is valid
    if (callable) {
        // determine if this should be a queued command or not
        bool queued = this->UseQueueBasedOnInterfacePolicy(queueingPolicy, "AddCommandWriteReturn", name);
        if (!queued) {
            mtsCommandWriteReturn * command = new mtsCommandWriteReturn(callable, name, argumentPrototype, resultPrototype);
            if (!CommandsWriteReturn.AddItem(name, command, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete command;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandWriteReturn: unable to add command \""
                                         << command->GetName() << "\"" << std::endl;
            }
            return command;
        } else {
            // create with no mailbox
            mtsCommandQueuedWriteReturn * queuedCommand = new mtsCommandQueuedWriteReturn(callable, name, argumentPrototype, resultPrototype, 0);
            if (!CommandsWriteReturn.AddItem(name, queuedCommand, CMN_LOG_LEVEL_INIT_ERROR)) {
                delete queuedCommand;
                queuedCommand = 0;
                CMN_LOG_CLASS_INIT_ERROR << "AddCommandWriteReturn: unable to add queued command \""
                                         << queuedCommand->GetName() << "\"" << std::endl;
            }
            return queuedCommand;
        }
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandWriteReturn: attempt to add undefined command (null callable pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandRead * mtsInterfaceProvided::AddCommandRead(mtsCallableReadBase * callable,
                                                      const std::string & name,
                                                      mtsGenericObject * argumentPrototype)
{
    // check that the input is valid
    if (callable) {
        mtsCommandRead * command = new mtsCommandRead(callable, name, argumentPrototype);
        if (!CommandsRead.AddItem(name, command, CMN_LOG_LEVEL_INIT_ERROR)) {
            delete command;
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandRead: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
        }
        return command;
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandRead: attempt to add undefined command (null callable pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandRead * mtsInterfaceProvided::AddCommandRead(mtsCommandRead * command)
{
    if (command) {
        if (!CommandsRead.AddItem(command->GetName(), command, CMN_LOG_LEVEL_INIT_ERROR)) {
            command = 0;
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandRead: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
        }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddCommandRead: unable to create command \""
                                 << command->GetName() << "\"" << std::endl;
    }
    return command;
}


mtsCommandQualifiedRead * mtsInterfaceProvided::AddCommandQualifiedRead(mtsCallableQualifiedReadBase * callable,
                                                                        const std::string & name,
                                                                        mtsGenericObject * argument1Prototype,
                                                                        mtsGenericObject * argument2Prototype)
{
    // check that the input is valid
    if (callable) {
        mtsCommandQualifiedRead * command =
            new mtsCommandQualifiedRead(callable, name, argument1Prototype, argument2Prototype);
        if (!CommandsQualifiedRead.AddItem(name, command, CMN_LOG_LEVEL_INIT_ERROR)) {
            delete command;
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandQualifiedRead: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
        }
        return command;
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddCommandQualifiedRead: attempt to add undefined command (null callable pointer) to interface \""
                             << this->GetName() << "\"" << std::endl;
    return 0;
}


mtsCommandQualifiedRead * mtsInterfaceProvided::AddCommandQualifiedRead(mtsCommandQualifiedRead * command)
{
    if (command) {
        if (!CommandsQualifiedRead.AddItem(command->GetName(), command, CMN_LOG_LEVEL_INIT_ERROR)) {
            command = 0;
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandQualifiedRead: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
        }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddCommandQualifiedRead: unable to create command \""
                                 << command->GetName() << "\"" << std::endl;
    }
    return command;
}


mtsCommandWriteBase * mtsInterfaceProvided::AddCommandFilteredWrite(mtsCommandQualifiedRead * filter,
                                                                    mtsCommandWriteBase * command,
                                                                    mtsCommandQueueingPolicy queueingPolicy)
{
    if (filter && command) {
        if (!CommandsInternal.AddItem(filter->GetName(), filter, CMN_LOG_LEVEL_INIT_ERROR)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandFilteredWrite: unable to add filter \""
                                     << command->GetName() << "\"" << std::endl;
            return 0;
        }
        // The mtsCommandWrite is called commandName because that name will be used by mtsCommandFilteredQueuedWrite.
        //  For clarity, we store it in the internal map under the name commandName+"Write".
        if (!CommandsInternal.AddItem(command->GetName()+"Write", command, CMN_LOG_LEVEL_INIT_ERROR)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandFilteredWrite: unable to add command \""
                                     << command->GetName() << "\"" << std::endl;
            CommandsInternal.RemoveItem(filter->GetName(), CMN_LOG_LEVEL_INIT_ERROR);
            return 0;
        }
        bool queued = this->UseQueueBasedOnInterfacePolicy(queueingPolicy, "AddCommandFilteredWrite", command->GetName());
        mtsCommandWriteBase * filteredCommand;
        if (!queued) {
            filteredCommand = new mtsCommandFilteredWrite(filter, command);
        } else {
            filteredCommand = new mtsCommandFilteredQueuedWrite(filter, command);
        }
        if (filteredCommand && CommandsWrite.AddItem(command->GetName(), filteredCommand, CMN_LOG_LEVEL_INIT_ERROR)) {
            return filteredCommand;
        } else {
            CommandsInternal.RemoveItem(filter->GetName(), CMN_LOG_LEVEL_INIT_ERROR);
            CommandsInternal.RemoveItem(command->GetName(), CMN_LOG_LEVEL_INIT_ERROR);
            if (filteredCommand) {
                delete filteredCommand;
            }
            CMN_LOG_CLASS_INIT_ERROR << "AddCommandFilteredWrite: unable to add queued command \""
                                     << command->GetName() << "\"" << std::endl;
            return 0;
        }
    }
    return 0;
}


std::string mtsInterfaceProvided::GetEndUserInterfaceName(const mtsInterfaceProvided * originalInterface,
                                                          const std::string &userName)
{
    return originalInterface->GetName() + "For" + userName;
}

// Protected function, should only be called from mtsComponent
mtsInterfaceProvided * mtsInterfaceProvided::GetEndUserInterface(const std::string & userName)
{
    // check if this is already an end user interface
    if (this->EndUserInterface) {
        return this;
    }
    // else we need to duplicate this interface.

    // Note that we don't check for duplicate user names (don't need to care whether there are duplicates)
    this->UserCounter++;
    CMN_LOG_CLASS_INIT_VERBOSE << "GetEndUserInterface: component \"" << Component->GetName()
                               << "\" interface \"" << this->Name
                               << "\" creating new copy (#" << this->UserCounter
                               << ") for user \"" << userName << "\"" << std::endl;
    // new end user interface created with default size for mailbox
    mtsInterfaceProvided * interfaceProvided = new mtsInterfaceProvided(this,
                                                                        userName,
                                                                        this->MailBoxSize,
                                                                        this->ArgumentQueuesSize);
    InterfacesProvidedCreated.push_back(InterfaceProvidedCreatedPairType(this->UserCounter, interfaceProvided));

    return interfaceProvided;
}


// Remove the end-user interface specified by the parameter interfaceProvided.
// Note that there are two mtsInterfaceProvided objects:  (1) the interfaceProvided parameter, which should be a
// pointer to the end-user interface to be removed and (2) the "this" pointer, which should point to the original interface.
mtsInterfaceProvided * mtsInterfaceProvided::RemoveEndUserInterface(mtsInterfaceProvided *interfaceProvided,
                                                                    const std::string &userName)
{
    // First, do some error checking
    // 1) Make sure interfaceProvided is non-zero
    if (!interfaceProvided) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveEndUserInterface: component \"" << Component->GetName()
                                << "\" interface \"" << this->Name
                                << "\": null provided interface" << std::endl;
        return interfaceProvided;
    }
    // 2) The interfaceProvided parameter should be an end-user interface
    if (!interfaceProvided->EndUserInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveEndUserInterface: component \"" << Component->GetName()
                                << "\" interface \"" << this->Name
                                << "\": parameter not an end-user interface" << std::endl;
        return interfaceProvided;
    }
    // 3) This object should be an original interface (i.e., the OriginalInterface pointer should be 0)
    if (this->OriginalInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveEndUserInterface: component \"" << Component->GetName()
                                << "\" interface \"" << this->Name
                                << "\": called on object that is not an original interface" << std::endl;
        return (interfaceProvided ? interfaceProvided : this->OriginalInterface);
    }

    // Now, handle the case where this object is also an end-user interface, which would occur when
    // there are no queued commands.
    if (this->EndUserInterface) {
        if (interfaceProvided && (interfaceProvided != this))
            CMN_LOG_CLASS_RUN_WARNING << "RemoveEndUserInterface: component \"" << Component->GetName()
                                      << "\" interface \"" << this->Name
                                      << "\": original interface inconsistent with provided end-user interface" << std::endl;
        CMN_LOG_CLASS_RUN_VERBOSE << "RemoveEndUserInterface: component \"" << Component->GetName()
                                  << "\" interface \"" << this->Name
                                  << "\": original interface is also the end-user interface" << std::endl;
        return 0;
    }

    // Finally, remove the end-user interface from the list of end-user interfaces (InterfacesProvidedCreated).
    InterfaceProvidedCreatedListType::iterator it;
    for (it = InterfacesProvidedCreated.begin(); it != InterfacesProvidedCreated.end(); it++) {
        if (it->second == interfaceProvided) {
            CMN_LOG_CLASS_RUN_VERBOSE << "RemoveEndUserInterface: component \"" << Component->GetName()
                                      << "\" interface \"" << this->Name
                                      << "\" removing copy (#" << it->first
                                      << ") for user \"" << userName << "\"" << std::endl;
            InterfacesProvidedCreated.erase(it);
            delete interfaceProvided;
            return 0;
        }
    }

    CMN_LOG_CLASS_RUN_ERROR << "RemoveEndUserInterface: component \"" << Component->GetName()
                            << "\" interface \"" << this->Name
                            << "\" could not find end-user interface for user \""
                            << userName << "\"" << std::endl;
    return interfaceProvided;
}


mtsInterfaceProvided * mtsInterfaceProvided::GetOriginalInterface(void) const
{
    return this->OriginalInterface;
}

mtsInterfaceProvided * mtsInterfaceProvided::FindEndUserInterfaceByName(const std::string &userName)
{
    // First, check if there is just a single provided interface (i.e., no queued commands)
    if ((this->OriginalInterface == 0) && this->EndUserInterface)
        return this;
    std::string interfaceName = mtsInterfaceProvided::GetEndUserInterfaceName(this, userName);
    InterfaceProvidedCreatedListType::iterator it;
    for (it = InterfacesProvidedCreated.begin(); it != InterfacesProvidedCreated.end(); it++) {
        if (it->second->GetName() == interfaceName) {
            return it->second;
        }
    }
    return 0;
}

mtsCommandVoid * mtsInterfaceProvided::AddEventVoid(const std::string & eventName)
{
    mtsMulticastCommandVoid * eventMulticastCommand = new mtsMulticastCommandVoid(eventName);
    if (eventMulticastCommand) {
        if (AddEvent(eventName, eventMulticastCommand)) {
            return eventMulticastCommand;
        }
        delete eventMulticastCommand;
        CMN_LOG_CLASS_INIT_ERROR << "AddEventVoid: unable to add event \""
                                 << eventName << "\"" << std::endl;
        return 0;
    }
    CMN_LOG_CLASS_INIT_ERROR << "AddEventVoid: unable to create multi-cast command for event \""
                             << eventName << "\"" << std::endl;
    return 0;
}


bool mtsInterfaceProvided::AddEventVoid(mtsFunctionVoid & eventTrigger,
                                      const std::string eventName)
{
    mtsCommandVoid * command;
    command = this->AddEventVoid(eventName);
    if (command) {
        eventTrigger.Bind(command);
        return true;
    }
    return false;
}


bool mtsInterfaceProvided::AddEvent(const std::string & name, mtsMulticastCommandVoid * generator)
{
    if (EventWriteGenerators.GetItem(name, CMN_LOG_LEVEL_NONE)) {
        // Is this check really needed?
        CMN_LOG_CLASS_INIT_VERBOSE << "AddEvent (void): event " << name << " already exists as write event, ignored." << std::endl;
        return false;
    }
    return EventVoidGenerators.AddItem(name, generator, CMN_LOG_LEVEL_INIT_ERROR);
}


bool mtsInterfaceProvided::AddEvent(const std::string & name, mtsMulticastCommandWriteBase * generator)
{
    if (EventVoidGenerators.GetItem(name, CMN_LOG_LEVEL_NONE)) {
        // Is this check really needed?
        CMN_LOG_CLASS_INIT_VERBOSE << "AddEvent (write): event " << name << " already exists as void event, ignored." << std::endl;
        return false;
    }
    return EventWriteGenerators.AddItem(name, generator, CMN_LOG_LEVEL_INIT_ERROR);
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommands(void) const
{
    std::vector<std::string> commands = GetNamesOfCommandsVoid();
    std::vector<std::string> tmp = GetNamesOfCommandsVoidReturn();
    commands.insert(commands.begin(), tmp.begin(), tmp.end());
    tmp.clear();
    tmp = GetNamesOfCommandsWrite();
    commands.insert(commands.begin(), tmp.begin(), tmp.end());
    tmp.clear();
    tmp = GetNamesOfCommandsWriteReturn();
    commands.insert(commands.begin(), tmp.begin(), tmp.end());
    tmp.clear();
    tmp = GetNamesOfCommandsRead();
    commands.insert(commands.begin(), tmp.begin(), tmp.end());
    tmp.clear();
    tmp = GetNamesOfCommandsQualifiedRead();
    commands.insert(commands.begin(), tmp.begin(), tmp.end());
    return commands;
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommandsVoid(void) const
{
    return CommandsVoid.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommandsVoidReturn(void) const
{
    return CommandsVoidReturn.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommandsWrite(void) const
{
    return CommandsWrite.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommandsWriteReturn(void) const
{
    return CommandsWriteReturn.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommandsRead(void) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->CommandsRead.GetNames();
    }
    return CommandsRead.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfCommandsQualifiedRead(void) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->CommandsQualifiedRead.GetNames();
    }
    return CommandsQualifiedRead.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfEventsVoid(void) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->EventVoidGenerators.GetNames();
    }
    return EventVoidGenerators.GetNames();
}


std::vector<std::string> mtsInterfaceProvided::GetNamesOfEventsWrite(void) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->EventWriteGenerators.GetNames();
    }
    return EventWriteGenerators.GetNames();
}


mtsCommandVoid * mtsInterfaceProvided::GetCommandVoid(const std::string & commandName) const
{
    if (this->EndUserInterface) {
        return CommandsVoid.GetItem(commandName, CMN_LOG_LEVEL_INIT_ERROR);
    }
    CMN_LOG_CLASS_INIT_ERROR << "GetCommandVoid: cannot retrieve command " << commandName << " from \"factory\" interface \""
                             << this->GetName()
                             << "\", you must call GetEndUserInterface to make sure you are using an end-user interface"
                             << std::endl;
    return 0;
}


mtsCommandVoidReturn * mtsInterfaceProvided::GetCommandVoidReturn(const std::string & commandName) const
{
    if (this->EndUserInterface) {
        return CommandsVoidReturn.GetItem(commandName, CMN_LOG_LEVEL_INIT_ERROR);
    }
    CMN_LOG_CLASS_INIT_ERROR << "GetCommandVoidReturn: cannot retrieve command " << commandName << " from \"factory\" interface \""
                             << this->GetName()
                             << "\", you must call GetEndUserInterface to make sure you are using an end-user interface"
                             << std::endl;
    return 0;
}


mtsCommandWriteBase * mtsInterfaceProvided::GetCommandWrite(const std::string & commandName) const
{
    if (this->EndUserInterface) {
        return CommandsWrite.GetItem(commandName, CMN_LOG_LEVEL_INIT_ERROR);
    }
    CMN_LOG_CLASS_INIT_ERROR << "GetCommandWrite: cannot retrieve command " << commandName << " from \"factory\" interface \""
                             << this->GetName()
                             << "\", you must call GetEndUserInterface to make sure you are using an end-user interface"
                             << std::endl;
    return 0;
}


mtsCommandWriteReturn * mtsInterfaceProvided::GetCommandWriteReturn(const std::string & commandName) const
{
    if (this->EndUserInterface) {
        return CommandsWriteReturn.GetItem(commandName, CMN_LOG_LEVEL_INIT_ERROR);
    }
    CMN_LOG_CLASS_INIT_ERROR << "GetCommandWriteReturn: cannot retrieve command " << commandName << " from \"factory\" interface \""
                             << this->GetName()
                             << "\", you must call GetEndUserInterface to make sure you are using an end-user interface"
                             << std::endl;
    return 0;
}


mtsCommandRead * mtsInterfaceProvided::GetCommandRead(const std::string & commandName) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->GetCommandRead(commandName);
    }
    return CommandsRead.GetItem(commandName, CMN_LOG_LEVEL_INIT_ERROR);
}


mtsCommandQualifiedRead * mtsInterfaceProvided::GetCommandQualifiedRead(const std::string & commandName) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->GetCommandQualifiedRead(commandName);
    }
    return CommandsQualifiedRead.GetItem(commandName, CMN_LOG_LEVEL_INIT_ERROR);
}


mtsMulticastCommandVoid * mtsInterfaceProvided::GetEventVoid(const std::string & eventName) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->GetEventVoid(eventName);
    }
    return EventVoidGenerators.GetItem(eventName, CMN_LOG_LEVEL_INIT_ERROR);
}


mtsMulticastCommandWriteBase * mtsInterfaceProvided::GetEventWrite(const std::string & eventName) const
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->GetEventWrite(eventName);
    }
    return EventWriteGenerators.GetItem(eventName, CMN_LOG_LEVEL_INIT_ERROR);
}


bool mtsInterfaceProvided::AddObserver(const std::string & eventName, mtsCommandVoid * handler)
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->AddObserver(eventName, handler);
    }
    mtsMulticastCommandVoid * multicastCommand = EventVoidGenerators.GetItem(eventName);
    if (multicastCommand) {
        // should probably check for duplicates (have AddCommand return bool?)
        multicastCommand->AddCommand(handler);
        return true;
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddObserver (void): cannot find event named \"" << eventName << "\"" << std::endl;
        return false;
    }
}


bool mtsInterfaceProvided::AddObserver(const std::string & eventName, mtsCommandWriteBase * handler)
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->AddObserver(eventName, handler);
    }
    mtsMulticastCommandWriteBase * multicastCommand = EventWriteGenerators.GetItem(eventName);
    if (multicastCommand) {
        // should probably check for duplicates (have AddCommand return bool?)
        multicastCommand->AddCommand(handler);
        return true;
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddObserver (write): cannot find event named \"" << eventName << "\"" << std::endl;
        return false;
    }
}


void mtsInterfaceProvided::AddObserverList(const mtsEventHandlerList &argin, mtsEventHandlerList &argout)
{
    argout = argin;
    unsigned int i;
    for (i = 0; i < argin.VoidEvents.size(); i++)
        argout.VoidEvents[i].Result = argin.Provided->AddObserver(argin.VoidEvents[i].EventName, argin.VoidEvents[i].HandlerPtr);
    for (i = 0; i < argin.WriteEvents.size(); i++)
        argout.WriteEvents[i].Result = argin.Provided->AddObserver(argin.WriteEvents[i].EventName, argin.WriteEvents[i].HandlerPtr);
}

bool mtsInterfaceProvided::RemoveObserver(const std::string & eventName, mtsCommandVoid * handler)
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->RemoveObserver(eventName, handler);
    }
    mtsMulticastCommandVoid * multicastCommand = EventVoidGenerators.GetItem(eventName);
    if (multicastCommand) {
        if (!multicastCommand->RemoveCommand(handler)) {
            CMN_LOG_CLASS_INIT_ERROR << "RemoveObserver (void): did not find handler for event " << eventName << std::endl;
            return false;
        }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "RemoveObserver (void): cannot find event named \"" << eventName << "\"" << std::endl;
        return false;
    }
    return true;
}

bool mtsInterfaceProvided::RemoveObserver(const std::string & eventName, mtsCommandWriteBase * handler)
{
    if (this->OriginalInterface) {
        return this->OriginalInterface->RemoveObserver(eventName, handler);
    }
    mtsMulticastCommandWriteBase * multicastCommand = EventWriteGenerators.GetItem(eventName);
    if (multicastCommand) {
        if (!multicastCommand->RemoveCommand(handler)) {
            CMN_LOG_CLASS_INIT_ERROR << "RemoveObserver (write): did not find handler for event " << eventName << std::endl;
            return false;
        }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "RemoveObserver (write): cannot find event named \"" << eventName << "\"" << std::endl;
        return false;
    }
    return true;
}

void mtsInterfaceProvided::RemoveObserverList(const mtsEventHandlerList &argin, mtsEventHandlerList &argout)
{
    argout = argin;
    unsigned int i;
    for (i = 0; i < argin.VoidEvents.size(); i++)
        argout.VoidEvents[i].Result = argin.Provided->RemoveObserver(argin.VoidEvents[i].EventName, argin.VoidEvents[i].HandlerPtr);
    for (i = 0; i < argin.WriteEvents.size(); i++)
        argout.WriteEvents[i].Result = argin.Provided->RemoveObserver(argin.WriteEvents[i].EventName, argin.WriteEvents[i].HandlerPtr);
}

