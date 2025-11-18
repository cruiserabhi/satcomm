/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef EVENT_SERVICEHELPER_HPP
#define EVENT_SERVICEHELPER_HPP

#include <iostream>
#include <string>
#include <mutex>
#include <queue>
#include <vector>
#include <algorithm>
#include <condition_variable>
#include <grpcpp/grpcpp.h>

#include "libs/common/Logger.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "ServerEventManager.hpp"
#include "protos/proto-src/event_simulation.grpc.pb.h"

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

template <typename T>
class EventServiceHelper: public T::Service {
public:
    /**
     * @brief This API gets the count of clients registered for a particular filter.
     * The count is useful in knowing if a filter is being registered by the first client
     * or deregistered by the last client.
     */
    inline int getClientsForFilter(std::string filter) {
        std::lock_guard<std::mutex> lck(clientMtx_);
        int count = 0;
        for(auto &client: clients_) {
            if(std::find(client.second.filters.begin(), client.second.filters.end(), filter)
                != client.second.filters.end()) {
                ++count;
            }
        }
        return count;
    }

protected:
    EventServiceHelper() {
        LOG(DEBUG, __FUNCTION__);
        init();
        exit_ = false;
    }

    virtual ~EventServiceHelper() {
        LOG(DEBUG, __FUNCTION__, ": Shutting down");
        {
            std::lock_guard<std::mutex> lck(exitMtx_);
            exit_ = true;
        }
        exitCv_.notify_all();
        taskQ_.shutdown();
        LOG(DEBUG, __FUNCTION__, ": Shutdown complete");
    }

private:
    /**
     * @brief This API dispatch an event from the queue & pass it to eventWriter.
     */
    void eventDispatcher() {
        LOG(DEBUG, __FUNCTION__);
        typename eventService::EventResponse eventResponse;

        while(true) {
            {
                std::unique_lock<std::mutex> lck(qMtx_);
                if (eventQ_.empty()) {
                    qCv_.wait(lck, [this] {return !(eventQ_.empty());});
                }

                eventResponse = eventQ_.front();
                eventQ_.pop();
            }

            auto f = std::async(std::launch::deferred, [this, eventResponse]() {
                    eventWriter(eventResponse);}).share();
            taskQ_.add(f);
        }
    }

    /**
     * @brief This API initialize an event dispatcher thread.
     */
    void init() {
        LOG(DEBUG, __FUNCTION__);
        auto f = std::async(std::launch::async, [this]() { eventDispatcher(); }).share();
        taskQ_.add(f);
    }

    /**
     * @brief This API writes the event to the stream based on the filters
     * set by the client.
     */
    void eventWriter(typename eventService::EventResponse eventResponse) {
        std::lock_guard<std::mutex> lck(clientMtx_);
        {
            for(auto& client : clients_) {
                const auto& clientObj = client.second;
                if(find(clientObj.filters.begin(), clientObj.filters.end(),
                eventResponse.filter()) != clientObj.filters.end()) {
                    if (clientObj.clientWriter) {
                        LOG(DEBUG, __FUNCTION__, ":: writing for filter:",
                            eventResponse.filter(), ", clientId: ",
                            clientObj.clientId);
                        clientObj.clientWriter->Write(eventResponse);
                    }
                }
            }
        }
    }

public:
    /**
     * @brief This is an gRPC RPC call invoked by the client & is responsible for
     * storing the writer instance.
     */
    grpc::Status registerForEvents(ServerContext* context,
        const typename eventService::EventRequest* request,
        ServerWriter<typename eventService::EventResponse>* writer) override {
        LOG(DEBUG, __FUNCTION__, ":: clientId: ", request->client_id());

        typename eventService::EventResponse eventResponse;
        std::vector<std::string> filters;

        {
            std::lock_guard<std::mutex> lck(clientMtx_);
            Client client;
            /*
            * Since filters are not available during ClientEventManager
            * initialization so we are updating only client_id & writer
            * instance here. Filters would be updated by call to updateFilter
            * from the client side.
            */
            client.clientId = request->client_id();
            client.clientWriter = writer;
            clients_[request->client_id()] = client;
        }

        std::unique_lock<std::mutex> lk(exitMtx_);
        exitCv_.wait(lk, [this] { return (exit_ == true); });
        return grpc::Status::OK;
    }

    /**
     * @brief This is an gRPC RPC call invoked by event_injector & is used to forward
     * events to the server.
     */
    grpc::Status InjectEvent(ServerContext* context,
        const typename eventService::UnsolicitedEvent* request,
        google::protobuf::Empty* response) override {
        LOG(DEBUG, __FUNCTION__);

        typename eventService::UnsolicitedEvent msg;
        msg.set_filter(request->filter());
        msg.set_event(request->event());

        auto f = std::async(std::launch::deferred, [this, msg]() {
            auto &serverEventManager = ServerEventManager::getInstance();
            serverEventManager.handleEventNotifications(msg);}).share();
        taskQ_.add(f);

        return grpc::Status::OK;
    }

    /**
     * @brief This is an gRPC RPC call invoked by client & is used by client to update
     * filter list maintained by server.
     */
    grpc::Status updateFilter(ServerContext* context,
        const typename eventService::EventRequest* request,
        google::protobuf::Empty* response) override {
        LOG(DEBUG, __FUNCTION__);

        std::lock_guard<std::mutex> lck(clientMtx_);
        {
            /**
             * For every updateFilter called by a client, it sends the list of filters the client
             * is currently interested in. Hence we clear the stale list maintained at the server
             * and update the filter for the client with the updated list.
             */
            clients_[request->client_id()].filters.clear();
            for (auto filter : request->filters()) {
                LOG(DEBUG, __FUNCTION__, ":: putting filter: ", filter,
                    ", clientId: ", request->client_id());
                clients_[request->client_id()].filters.push_back(filter);
            }
        }
        return grpc::Status::OK;
    }

    grpc::Status isServiceAvailable(ServerContext* context,
        const google::protobuf::Empty* request,
        google::protobuf::Empty* response) override {
        LOG(DEBUG, __FUNCTION__);

        return grpc::Status::OK;
    }

    void updateEventQueue(const typename eventService::EventResponse& event) {
        LOG(DEBUG, __FUNCTION__);
        {
            std::lock_guard<std::mutex> lck(qMtx_);
            LOG(DEBUG, __FUNCTION__, " pushing event in queue for filter::", event.filter());
            eventQ_.push(event);
            qCv_.notify_all();
        }
    }

    grpc::Status cleanup(ServerContext* context,
        const typename eventService::CleanupRequest* request,
        google::protobuf::Empty* response) override {
        LOG(DEBUG, __FUNCTION__, " erasing obsolete client::", request->client_id());

        std::lock_guard<std::mutex> lck(clientMtx_);
        clients_.erase(request->client_id());
        return grpc::Status::OK;
    }

private:
    /**
     * @brief gRPC client containing event filters and event writer.
     */
    struct Client {
        int clientId;
        std::vector<std::string> filters;
        ServerWriter<typename eventService::EventResponse>* clientWriter;

        bool operator ==(Client &rHl) {
            if (clientId == rHl.clientId) {
                return true;
            }
            return false;
        }
    };

    std::mutex clientMtx_;
    std::unordered_map<int, Client> clients_;

    std::mutex exitMtx_;
    std::condition_variable exitCv_;
    std::atomic<bool> exit_;

    std::mutex qMtx_;
    std::condition_variable qCv_;
    std::queue<typename eventService::EventResponse> eventQ_;

    telux::common::AsyncTaskQueue<void> taskQ_;
};

#endif // EVENT_SERVICEHELPER_HPP
