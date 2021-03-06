/*
 *  ActionManager
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2017 EPAM Systems Inc.
 */

#include "ActionManager.hpp"

#include <algorithm>

#include "Exception.hpp"

using std::bind;
using std::dynamic_pointer_cast;
using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::to_string;
using std::toupper;
using std::transform;
using std::unique_lock;
using std::vector;

/*******************************************************************************
 * ActionManager
 ******************************************************************************/

ActionManager::ActionManager(ObjectManager& objects, ConfigPtr config)
    : mObjects(objects),
      mConfig(config),
      mTerminate(false),
      mLog("ActionManager")
{
    LOG(mLog, DEBUG) << "Create";

    init();

    mThread = thread(&ActionManager::run, this);
}

ActionManager::~ActionManager()
{
    {
        unique_lock<mutex> lock(mMutex);

        mTerminate = true;

        mCondVar.notify_all();
    }

    if (mThread.joinable()) {
        mThread.join();
    }

    LOG(mLog, DEBUG) << "Delete";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void ActionManager::createLayer(t_ilm_layer id)
{
    asyncCall(bind(&ActionManager::asyncCreateLayer, this, id));
}

void ActionManager::deleteLayer(t_ilm_layer id)
{
    asyncCall(bind(&ActionManager::asyncDeleteLayer, this, id));
}

void ActionManager::createSurface(t_ilm_surface id)
{
    asyncCall(bind(&ActionManager::asyncCreateSurface, this, id));
}

void ActionManager::deleteSurface(t_ilm_surface id)
{
    asyncCall(bind(&ActionManager::asyncDeleteSurface, this, id));
}

void ActionManager::userEvent(uint32_t id)
{
    asyncCall(bind(&ActionManager::asyncUserEvent, this, id));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

const vector<ActionManager::EventsTable> ActionManager::sEventsTable = {
    {EventType::CREATE, "CREATE"},
    {EventType::DESTROY, "DESTROY"},
    {EventType::USER, "USER"}};

const vector<ActionManager::ObjectsTable> ActionManager::sObjectsTable = {
    {ObjectType::LAYER, "LAYER"}, {ObjectType::SURFACE, "SURFACE"}};

const vector<ActionManager::ActionsTable> ActionManager::sActionsTable = {
    {ActionType::SOURCE, "SOURCE"},
    {ActionType::DESTINATION, "DESTINATION"},
    {ActionType::PARENT, "PARENT"},
    {ActionType::ORDER, "ORDER"},
    {ActionType::VISIBILITY, "VISIBILITY"},
    {ActionType::OPACITY, "OPACITY"}};

void ActionManager::init()
{
    for (int i = 0; i < mConfig->getEventsCount(); i++) {
        string name;

        mConfig->getEventName(i, name);

        EventPtr event;

        auto eventType = getType<EventType, EventsTable>(sEventsTable, name);

        switch (eventType) {
            case EventType::CREATE: {
                event = createEventCreate(i);

                break;
            }

            case EventType::DESTROY: {
                event = createEventDestroy(i);

                break;
            }

            case EventType::USER: {
                event = createEventUser(i);

                break;
            }

            default: {
                throw DmException("Not implemented event: " + name);

                break;
            }
        }

        createEventActions(i, event);

        mEvents.push_back(event);
    }
}

void ActionManager::asyncCall(AsyncCall f)
{
    unique_lock<mutex> lock(mMutex);

    mAsyncCalls.push_back(f);

    mCondVar.notify_one();
}

void ActionManager::run()
{
    unique_lock<mutex> lock(mMutex);

    while (!mTerminate) {
        mCondVar.wait(lock,
                      [this] { return mTerminate || !mAsyncCalls.empty(); });

        while (!mAsyncCalls.empty()) {
            auto asyncCall = mAsyncCalls.front();

            lock.unlock();

            try {
                asyncCall();
            }
            catch (const std::exception& e) {
                LOG(mLog, ERROR) << e.what();
            }

            lock.lock();

            mAsyncCalls.pop_front();
        }
    }
}

void ActionManager::asyncCreateLayer(t_ilm_layer id)
{
    for (int i = 0; i < mConfig->getLayersCount(); i++) {
        LayerConfig config;

        mConfig->getLayerConfig(i, config);

        if (id == config.id) {
            LOG(mLog, DEBUG) << "Create layer, id: " << id;

            auto layer = mObjects.createLayer(config);

            onCreateLayer(layer->getName());

            mObjects.update();

            return;
        }
    }

    LOG(mLog, WARNING) << "Unhandled layer " << id << " created";
}

void ActionManager::asyncDeleteLayer(t_ilm_layer id)
{
    auto layer = mObjects.getLayerByID(id);

    if (layer) {
        LOG(mLog, DEBUG) << "Delete layer, id: " << id;

        onDeleteLayer(layer->getName());

        mObjects.deleteLayerByName(layer->getName());

        layer.reset();

        mObjects.update();
    }
    else {
        LOG(mLog, WARNING) << "Unhandled layer " << id << " deleted";
    }
}

void ActionManager::asyncCreateSurface(t_ilm_surface id)
{
    for (int i = 0; i < mConfig->getSurfacesCount(); i++) {
        SurfaceConfig config;

        mConfig->getSurfaceConfig(i, config);

        if (id == config.id) {
            LOG(mLog, DEBUG) << "Create surface, id: " << id;

            auto surface = mObjects.createSurface(config);

            onCreateSurface(surface->getName());

            mObjects.update();

            return;
        }
    }

    LOG(mLog, WARNING) << "Unhandled surface " << id << " created";
}

void ActionManager::asyncDeleteSurface(t_ilm_surface id)
{
    auto surface = mObjects.getSurfaceByID(id);

    if (surface) {
        LOG(mLog, DEBUG) << "Delete surface, id: " << id;

        onDeleteSurface(surface->getName());

        mObjects.deleteSurfaceByName(surface->getName());

        surface.reset();

        mObjects.update();
    }
    else {
        LOG(mLog, WARNING) << "Unhandled surface " << id << " deleted";
    }
}

void ActionManager::asyncUserEvent(uint32_t id)
{
    onUserEvent(id);

    mObjects.update();
}

EventPtr ActionManager::createEventCreate(int eventIndex)
{
    EventCreateConfig createConfig;

    mConfig->getEventCreateConfig(eventIndex, createConfig);

    LOG(mLog, DEBUG) << "event create, object: " << createConfig.object
                     << ", name: " << createConfig.name;

    auto objectType =
        getType<ObjectType, ObjectsTable>(sObjectsTable, createConfig.object);

    return EventPtr(
        new EventObject(EventType::CREATE, objectType, createConfig.name));
}

EventPtr ActionManager::createEventDestroy(int eventIndex)
{
    EventDestroyConfig destroyConfig;

    mConfig->getEventDestroyConfig(eventIndex, destroyConfig);

    LOG(mLog, DEBUG) << "event destroy, object: " << destroyConfig.object
                     << ", name: " << destroyConfig.name;

    auto objectType =
        getType<ObjectType, ObjectsTable>(sObjectsTable, destroyConfig.object);

    return EventPtr(
        new EventObject(EventType::DESTROY, objectType, destroyConfig.name));
}

EventPtr ActionManager::createEventUser(int eventIndex)
{
    EventUserConfig userConfig;

    mConfig->getEventUserConfig(eventIndex, userConfig);

    LOG(mLog, DEBUG) << "event user, id: " << userConfig.id;

    return EventPtr(new EventUser(userConfig.id));
}

void ActionManager::createEventActions(int eventIndex, EventPtr event)
{
    auto count = mConfig->getActionsCount(eventIndex);

    for (int i = 0; i < count; i++) {
        string name;

        mConfig->getActionName(eventIndex, i, name);

        ActionPtr action;

        auto actionType =
            getType<ActionType, ActionsTable>(sActionsTable, name);

        switch (actionType) {
            case ActionType::SOURCE: {
                action = createActionSource(eventIndex, i);

                break;
            }

            case ActionType::DESTINATION: {
                action = createActionDestination(eventIndex, i);

                break;
            }

            case ActionType::PARENT: {
                action = createActionParent(eventIndex, i);

                break;
            }

            case ActionType::ORDER: {
                action = createActionOrder(eventIndex, i);

                break;
            }

            case ActionType::VISIBILITY: {
                action = createActionVisibility(eventIndex, i);

                break;
            }

            case ActionType::OPACITY: {
                action = createActionOpacity(eventIndex, i);

                break;
            }

            default: {
                throw DmException("Not implemented action: " + name);

                break;
            }
        }

        event->addAction(action);
    }
}

ActionPtr ActionManager::createActionSource(int eventIndex, int actionIndex)
{
    ActionSourceConfig sourceConfig;

    mConfig->getActionSourceConfig(eventIndex, actionIndex, sourceConfig);

    LOG(mLog, DEBUG) << "action source, index:" << eventIndex
                     << ", object: " << sourceConfig.object
                     << ", name: " << sourceConfig.name;

    auto objectType =
        getType<ObjectType, ObjectsTable>(sObjectsTable, sourceConfig.object);

    return ActionPtr(new ActionSource(mObjects, objectType, sourceConfig.name,
                                      sourceConfig.source));
}

ActionPtr ActionManager::createActionDestination(int eventIndex,
                                                 int actionIndex)
{
    ActionDestinationConfig destinationConfig;

    mConfig->getActionDestinationConfig(eventIndex, actionIndex,
                                        destinationConfig);

    LOG(mLog, DEBUG) << "action destination, index:" << eventIndex
                     << ", object: " << destinationConfig.object
                     << ", name: " << destinationConfig.name;

    auto objectType = getType<ObjectType, ObjectsTable>(
        sObjectsTable, destinationConfig.object);

    return ActionPtr(new ActionDestination(mObjects, objectType,
                                           destinationConfig.name,
                                           destinationConfig.destination));
}

ActionPtr ActionManager::createActionParent(int eventIndex, int actionIndex)
{
    ActionParentConfig parentConfig;

    mConfig->getActionParentConfig(eventIndex, actionIndex, parentConfig);

    LOG(mLog, DEBUG) << "action parent, index:" << eventIndex
                     << ", object: " << parentConfig.object
                     << ", name: " << parentConfig.name;

    auto objectType =
        getType<ObjectType, ObjectsTable>(sObjectsTable, parentConfig.object);

    return ActionPtr(new ActionParent(mObjects, objectType, parentConfig.name,
                                      parentConfig.parent));
}

ActionPtr ActionManager::createActionOrder(int eventIndex, int actionIndex)
{
    ActionOrderConfig orderConfig;

    mConfig->getActionOrderConfig(eventIndex, actionIndex, orderConfig);

    LOG(mLog, DEBUG) << "action order, index:" << eventIndex
                     << ", object: " << orderConfig.object
                     << ", name: " << orderConfig.name;

    auto objectType =
        getType<ObjectType, ObjectsTable>(sObjectsTable, orderConfig.object);

    return ActionPtr(new ActionOrder(mObjects, objectType, orderConfig.name,
                                     orderConfig.order));
}

ActionPtr ActionManager::createActionVisibility(int eventIndex, int actionIndex)
{
    ActionVisibilityConfig visibilityConfig;

    mConfig->getActionVisibilityConfig(eventIndex, actionIndex,
                                       visibilityConfig);

    LOG(mLog, DEBUG) << "action visibility, index:" << eventIndex
                     << ", object: " << visibilityConfig.object
                     << ", name: " << visibilityConfig.name;

    auto objectType = getType<ObjectType, ObjectsTable>(
        sObjectsTable, visibilityConfig.object);

    return ActionPtr(new ActionVisibility(mObjects, objectType,
                                          visibilityConfig.name,
                                          visibilityConfig.visibility));
}

ActionPtr ActionManager::createActionOpacity(int eventIndex, int actionIndex)
{
    ActionOpacityConfig opacityConfig;

    mConfig->getActionOpacityConfig(eventIndex, actionIndex, opacityConfig);

    LOG(mLog, DEBUG) << "action opacity, index:" << eventIndex
                     << ", object: " << opacityConfig.object
                     << ", name: " << opacityConfig.name;

    auto objectType =
        getType<ObjectType, ObjectsTable>(sObjectsTable, opacityConfig.object);

    return ActionPtr(new ActionOpacity(mObjects, objectType, opacityConfig.name,
                                       opacityConfig.opacity));
}

template <typename T, typename S>
T ActionManager::getType(const vector<S>& table, const string& name)
{
    string upperName = name;

    transform(upperName.begin(), upperName.end(), upperName.begin(),
              [](unsigned char c) { return toupper(c); });

    for (auto elem : table) {
        if (upperName == elem.name) {
            return elem.type;
        }
    }

    throw DmException("Unknown type " + name);
}

EventPtr ActionManager::getObjectEvent(EventType eventType,
                                       ObjectType objectType,
                                       const string& name)
{
    for (auto event : mEvents) {
        if (event->getEventType() == eventType) {
            auto eventObject = dynamic_pointer_cast<EventObject>(event);

            if (eventObject->getObjectType() == objectType &&
                eventObject->getName() == name) {
                return event;
            }
        }
    }

    return EventPtr();
}

EventPtr ActionManager::getUserEvent(uint32_t id)
{
    for (auto event : mEvents) {
        if (event->getEventType() == EventType::USER) {
            auto eventObject = dynamic_pointer_cast<EventUser>(event);

            if (eventObject->getID() == id) {
                return event;
            }
        }
    }

    return EventPtr();
}

void ActionManager::onCreateLayer(const std::string& name)
{
    auto event = getObjectEvent(EventType::CREATE, ObjectType::LAYER, name);

    if (event) {
        LOG(mLog, DEBUG) << "onCreateLayer, name: " << name;

        event->doActions();
    }
}

void ActionManager::onDeleteLayer(const std::string& name)
{
    auto event = getObjectEvent(EventType::DESTROY, ObjectType::LAYER, name);

    if (event) {
        LOG(mLog, DEBUG) << "onDeleteLayer, name: " << name;

        event->doActions();
    }
}

void ActionManager::onCreateSurface(const std::string& name)
{
    auto event = getObjectEvent(EventType::CREATE, ObjectType::SURFACE, name);

    if (event) {
        LOG(mLog, DEBUG) << "onCreateSurface, name: " << name;

        event->doActions();
    }
}

void ActionManager::onDeleteSurface(const std::string& name)
{
    auto event = getObjectEvent(EventType::DESTROY, ObjectType::SURFACE, name);

    if (event) {
        LOG(mLog, DEBUG) << "onDeleteSurface, name: " << name;

        event->doActions();
    }
}

void ActionManager::onUserEvent(uint32_t id)
{
    auto event = getUserEvent(id);

    if (event) {
        LOG(mLog, DEBUG) << "onUserEvent, id: " << id;

        event->doActions();
    }
}
