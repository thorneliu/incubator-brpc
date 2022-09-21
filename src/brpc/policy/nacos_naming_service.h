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

#ifndef BPRC_POLICY_NACOS_NAMING_SERVICE
#define BPRC_POLICY_NACOS_NAMING_SERVICE

#include "brpc/naming_service.h"
#include "nacos/naming/NamingService.h"
#include "nacos/naming/ServiceInfo.h"
#include "nacos/naming/subscribe/EventListener.h"

namespace brpc {
namespace policy {

class NacosNamingServiceListener : public nacos::EventListener {
 public:
   NacosNamingServiceListener(NamingServiceActions* actions);
   ~NacosNamingServiceListener() = default;
   void receiveNamingInfo(const nacos::ServiceInfo& serviceInfo) override;

 private:
   NamingServiceActions* _actions;
};

class NacosNamingService : public NamingService {
 public:
   ~NacosNamingService();

 private:
   int RunNamingService(const char* service_name,
                       NamingServiceActions* actions) override;

   void Describe(std::ostream& os, const DescribeOptions&) const override;

   NamingService* New() const override;

   void Destroy() override;

   void initNacosNamingService();
   void parseService();

   NamingServiceActions* _pActions;
   nacos::NamingService* _nacosNs;
   NacosNamingServiceListener* _listener;

   std::string _serviceDefination;
   std::string _serverAddr;
   std::string _nameSpace;
   std::string _groupName;
   std::string _serviceName;
   std::string _clusterId;
};

}  // namespace policy
}  // namespace brpc

#endif   // BPRC_POLICY_NACOS_NAMING_SERVICE