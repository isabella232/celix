# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

add_celix_bundle(rsa_topology_manager
    SOURCES
        src/topology_manager.c
        src/scope.c
        src/activator.c
    VERSION 0.9.0
    SYMBOLIC_NAME "apache_celix_rs_topology_manager"
    GROUP "Celix/RSA"
    NAME "Apache Celix RS Topology Manager"
)
target_include_directories(rsa_topology_manager PRIVATE src)
target_include_directories(rsa_topology_manager PRIVATE include)
target_link_libraries(rsa_topology_manager PRIVATE Celix::log_helper Celix::deprecated_rsa_spi)

if (ENABLE_TESTING)
    find_package(Jansson REQUIRED)
    add_subdirectory(tms_tst)
endif (ENABLE_TESTING)

install_celix_bundle(rsa_topology_manager EXPORT celix COMPONENT rsa)
#Setup target aliases to match external usage
add_library(Celix::rsa_topology_manager ALIAS rsa_topology_manager)
