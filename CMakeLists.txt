cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_BUILD_TYPE "Debug")

SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

add_library(cupti SHARED IMPORTED)
set_property(TARGET cupti PROPERTY IMPORTED_LOCATION "/usr/local/cuda/extras/CUPTI/lib64/libcupti.so")

add_library(nanomsg SHARED IMPORTED)
set_property(TARGET nanomsg PROPERTY IMPORTED_LOCATION "/opt/data1/share/users/anshuman/nanomsg/.libs/libnanomsg.so")

#add_library(anl SHARED IMPORTED)
#set_property(TARGET anl PROPERTY IMPORTED_LOCATION "/lib/x86_64-linux-gnu/libanl-2.19.so")

add_library(mysqlclient SHARED IMPORTED)
set_property(TARGET mysqlclient PROPERTY IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")

add_library(xercs-c SHARED IMPORTED)
set_property(TARGET xercs-c PROPERTY IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libxercs-c.so")

add_library(boost_thread SHARED IMPORTED)
set_property(TARGET boost_thread PROPERTY IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libboost_thread.so")

add_library(boost_system SHARED IMPORTED)
set_property(TARGET boost_system PROPERTY IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libboost_system.so")

project(evqueue)

add_executable(evqueue evqueue.cpp Configuration.cpp ConfigurationReader.cpp DB.cpp Exception.cpp Queue.cpp QueuePool.cpp WorkflowInstance.cpp WorkflowInstances.cpp Scheduler.cpp Retrier.cpp SocketQuerySAX2Handler.cpp ProcessManager.cpp WorkflowParameters.cpp handle_connection.cpp Statistics.cpp NetworkInputSource.cpp BinNetworkInputStream.cpp Schedule.cpp WorkflowSchedule.cpp WorkflowScheduler.cpp Tasks.cpp Task.cpp Workflow.cpp Workflows.cpp Logger.cpp GarbageCollector.cpp RetrySchedule.cpp RetrySchedules.cpp SequenceGenerator.cpp tools.cpp Messages.cpp Communicator.cpp Arbiter.cpp ReqRespDescriptor.cpp Reqresp.cpp RequestWindow.cpp PinfoListener.cpp)

include_directories(include /usr/include /usr/local/cuda/include /usr/local/cuda/extras/CUPTI/include /opt/data1/share/users/anshuman/nanomsg/src)

target_link_libraries(evqueue mysqlclient)
target_link_libraries(evqueue pthread)
target_link_libraries(evqueue xerces-c)
target_link_libraries(evqueue xqilla)
target_link_libraries(evqueue rt)
target_link_libraries(evqueue cupti)
target_link_libraries(evqueue nanomsg)
#target_link_libraries(evqueue anl)
target_link_libraries(evqueue boost_thread)
target_link_libraries(evqueue boost_system)

project(evqueue_monitor)

add_executable(evqueue_monitor evqueue_monitor.cpp)

include_directories(include /usr/include)

project(evqueue_client)

add_library(evqueue_client SHARED Configuration.cpp ConfigurationReader.cpp DB.cpp Exception.cpp Queue.cpp QueuePool.cpp WorkflowInstance.cpp WorkflowInstances.cpp Scheduler.cpp Retrier.cpp SocketQuerySAX2Handler.cpp ProcessManager.cpp WorkflowParameters.cpp handle_connection.cpp Statistics.cpp NetworkInputSource.cpp BinNetworkInputStream.cpp Schedule.cpp WorkflowSchedule.cpp WorkflowScheduler.cpp Tasks.cpp Task.cpp Workflow.cpp Workflows.cpp Logger.cpp GarbageCollector.cpp RetrySchedule.cpp RetrySchedules.cpp SequenceGenerator.cpp tools.cpp CuptiActivityProfiler.cpp EvqueueClient.cpp EvqueueManager.cpp Messages.cpp Interposer.cpp Communicator.cpp ReqRespDescriptor.cpp)

include_directories(include /usr/include /usr/local/cuda/include /usr/local/cuda/extras/CUPTI/include /opt/data1/share/users/anshuman/nanomsg/src)

target_link_libraries(evqueue_client mysqlclient)
target_link_libraries(evqueue_client pthread)
target_link_libraries(evqueue_client xerces-c)
target_link_libraries(evqueue_client xqilla)
target_link_libraries(evqueue_client rt)
target_link_libraries(evqueue_client cupti)
target_link_libraries(evqueue_client nanomsg)

install(TARGETS evqueue evqueue_monitor DESTINATION bin)
install(FILES etc/evqueue.conf DESTINATION /etc)
install(FILES etc/init.d/evqueue DESTINATION /etc/init.d PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
install(FILES etc/default/evqueue DESTINATION /etc/default PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(DIRECTORY var/run/evqueue DESTINATION /var/run DIRECTORY_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ)
install(FILES var/run/evqueue/evqueue.pid DESTINATION /var/run/evqueue)

set(CPACK_PACKAGE_FILE_NAME "evqueue-core_1.3_amd64")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "evQueue core module")
set(CPACK_PACKAGE_DESCRIPTION "evQueue is a fast event driven scheduler and queueing engine")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_NAME "evqueue-core")
set(CPACK_DEBIAN_PACKAGE_VERSION "1.3")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thibault Kummer <bob@coldsource.net>")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_GENERATOR "RPM")

include(CPack)
