// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "brpc/log.h"
#include "butil/strings/string_split.h"
#include "brpc/policy/nacos_naming_service.h"
#include "nacos/Properties.h"
#include "nacos/ResourceGuard.h"
#include "nacos/constant/PropertyKeyConst.h"
#include "nacos/factory/NacosFactoryFactory.h"

namespace brpc {
namespace policy {

void nacosInstances2ServerNodes(const std::list<nacos::Instance>& instances,
                                std::vector<ServerNode>& nodes) {
  for (auto& instance : instances) {
    if (!instance.healthy || !instance.enabled) {
      continue;
    }

    butil::EndPoint point;
    if (str2endpoint(instance.ip.c_str(), instance.port, &point) != 0 &&
        hostname2endpoint(instance.ip.c_str(), instance.port, &point) != 0) {
        LOG(ERROR) << "Invalid address : " << instance.ip << ":" << instance.port;
        continue;
    }

    ServerNode node{point};
    nodes.push_back(node);
  }
}

NacosNamingServiceListener::NacosNamingServiceListener(
    NamingServiceActions* actions)
    : _actions(actions) {}

void NacosNamingServiceListener::receiveNamingInfo(
    const nacos::ServiceInfo& serviceInfo) {
    LOG(INFO) << "Watched nacos service UPDATED: "
                << serviceInfo.toInstanceString();
    std::cout << "Wathched nacos update " << serviceInfo.toInstanceString();
    nacos::ServiceInfo info = serviceInfo;
    std::vector<ServerNode> servers;
    nacosInstances2ServerNodes(info.getHosts(), servers);

    _actions->ResetServers(servers);
}

NacosNamingService::~NacosNamingService() {
    if (_nacosNs) {
        _nacosNs->unsubscribe(_serviceName, _groupName, {_clusterId}, _listener);
        delete _nacosNs;
    }

    // The listenser_ will be freed in nacos::EventDispatcher::removeListenerHelper
}

int NacosNamingService::RunNamingService(const char* service_name,
                                         NamingServiceActions* actions) {
    _serviceDefination = std::string(service_name);
    _pActions = actions;
    _listener = new NacosNamingServiceListener(actions);
    initNacosNamingService();

    auto instances = _nacosNs->getAllInstances(_serviceName, _groupName, {_clusterId});
    std::vector<ServerNode> servers;
    nacosInstances2ServerNodes(instances, servers);
    actions->ResetServers(servers);

    _nacosNs->subscribe(_serviceName, _groupName, {_clusterId}, _listener);
    return 0;
}

void NacosNamingService::Describe(std::ostream& os,
                                  const DescribeOptions&) const {
    os << "nacos";
    return;
}

NamingService* NacosNamingService::New() const {
    return new NacosNamingService;
}

void NacosNamingService::Destroy() { delete this; }

void NacosNamingService::initNacosNamingService() {
    parseService();

    nacos::Properties configProps;
    configProps[nacos::PropertyKeyConst::NAMESPACE] = _nameSpace;
    configProps[nacos::PropertyKeyConst::SERVER_ADDR] = _serverAddr;

    nacos::INacosServiceFactory* factory =
        nacos::NacosFactoryFactory::getNacosFactory(configProps);
    nacos::ResourceGuard<nacos::INacosServiceFactory> _guardFactory(factory);
    _nacosNs = factory->CreateNamingService();
}

void NacosNamingService::parseService() {
    // here we define a protocl:
    // nacos://serverAddress/namespace/groupName/serviceName/clusterId
    std::vector<butil::StringPiece> serviceProperites;
    butil::SplitString(_serviceDefination, '/', &serviceProperites);

    LOG_ASSERT(serviceProperites.size() == 5);
    _serverAddr = serviceProperites[0].as_string();
    _nameSpace = serviceProperites[1].as_string();
    _groupName = serviceProperites[2].as_string();
    _serviceName = serviceProperites[3].as_string();
    _clusterId = serviceProperites[4].as_string();
}

} // namespace policy
} // namespace brpc
