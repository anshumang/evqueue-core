/*
 * This file is part of evQueue
 * 
 * evQueue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * evQueue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with evQueue. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Thibault Kummer <bob@coldsource.net>, Anshuman Goswami <anshumang@gatech.edu>
 */

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>

#include <mysql/mysql.h>

#include <Logger.h>
#include <Queue.h>
#include <QueuePool.h>
#include <Workflows.h>
#include <WorkflowInstance.h>
#include <WorkflowInstances.h>
#include <ConfigurationReader.h>
#include <Exception.h>
#include <Retrier.h>
#include <WorkflowScheduler.h>
#include <WorkflowSchedule.h>
#include <global.h>
#include <ProcessManager.h>
#include <DB.h>
#include <Statistics.h>
#include <Tasks.h>
#include <RetrySchedules.h>
#include <GarbageCollector.h>
#include <SequenceGenerator.h>
#include <handle_connection.h>
#include <tools.h>

#include <xqilla/xqilla-dom3.hpp>

#include "Communicator.h"
#include "Arbiter.h"
#include "Reqresp.h"

//int listen_socket;//Moved definition to WorkflowInstance.cpp because this file is not built when generating the lib

void signal_callback_handler(int signum)
{
	if(signum==SIGINT || signum==SIGTERM)
	{
		// Shutdown requested
		// Close main listen socket, this will release accept() loop
		close(listen_socket);
	}
	else if(signum==SIGHUP)
	{
		Logger::Log(LOG_NOTICE,"Got SIGHUP, reloading scheduler configuration");
		
		WorkflowScheduler *scheduler = WorkflowScheduler::GetInstance();
		scheduler->Reload();
		
		Tasks *tasks = Tasks::GetInstance();
		tasks->Reload();
		
		RetrySchedules *retry_schedules = RetrySchedules::GetInstance();
		retry_schedules->Reload();
		
		Workflows *workflows = Workflows::GetInstance();
		workflows->Reload();
	}
	else if(signum==SIGUSR1)
	{
		Logger::Log(LOG_NOTICE,"Got SIGUSR1, flushing retrier");
		Retrier *retrier = Retrier::GetInstance();
		retrier->Flush();
	}
}

int main(int argc,const char **argv)
{
	// Check parameters
	const char *config_filename = 0;
	bool daemonize = false;
	bool daemonized = false;

/*	
	for(int i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"--daemon")==0)
			daemonize = true;
		else if(strcmp(argv[i],"--config")==0 && i+1<argc)
		{
			config_filename = argv[i+1];
			i++;
		}
		else if(strcmp(argv[i],"--ipcq-remove")==0)
			return tools_queue_destroy();
		else if(strcmp(argv[i],"--ipcq-stats")==0)
			return tools_queue_stats();
		else
		{
			fprintf(stderr,"Unknown option : %s\n",argv[i]);
			tools_print_usage();
			return -1;
		}
	}
	
	if(config_filename==0)
	{
		tools_print_usage();
		return -1;
	}
*/
	
	// Initialize external libraries
/*
	mysql_library_init(0,0,0);
	XQillaPlatformUtils::initialize();
*/
	
	openlog("evqueue",0,LOG_DAEMON);
	
	/*struct sigaction sa;
	sigset_t block_mask;
	
	sigemptyset(&block_mask);
	sigaddset(&block_mask, SIGINT);
	sigaddset(&block_mask, SIGTERM);
	sigaddset(&block_mask, SIGHUP);
	sigaddset(&block_mask, SIGUSR1);
	
	sa.sa_handler = signal_callback_handler;
	sa.sa_mask = block_mask;
	sa.sa_flags = 0;
	
	sigaction(SIGHUP,&sa,0);
	sigaction(SIGINT,&sa,0);
	sigaction(SIGTERM,&sa,0);
	sigaction(SIGUSR1,&sa,0);
	*/
	try
	{
		// Read configuration
		Configuration *config = new Configuration();
                /*
		config = ConfigurationReader::Read(config_filename);
		*/

		// Create logger as soon as possible
		Logger *logger = new Logger();
		
		// Open pid file before fork to eventually print errors
		//FILE *pidfile = fopen(config->Get("core.pidfile"),"w");
		//if(pidfile==0)
		//	throw Exception("core","Unable to open pid file");
		
		//int gid = atoi(config->Get("core.gid"));
		//if(gid!=0 && setgid(gid)!=0)
		//	throw Exception("core","Unable to set requested GID");
		
		// Set uid/gid if requested
		//int uid = atoi(config->Get("core.uid"));
		//if(uid!=0 && setuid(uid)!=0)
		//	throw Exception("core","Unable to set requested UID");
		
		// Check database connection
/*
		DB db;
		db.Ping();
*/
		
		if(daemonize)
		{
			daemon(1,0);
			daemonized = true;
		}
		
		// Write pid after daemonization
		//fprintf(pidfile,"%d\n",getpid());
		//fclose(pidfile);
		
		// Instanciate sequence generator, used for savepoint level 0 or 1
		//SequenceGenerator *seq = new SequenceGenerator();
		
		// Create statistics counter
		//Statistics *stats = new Statistics();
		
		// Start retrier
		//Retrier *retrier = new Retrier();
		
		// Start scheduler
		//WorkflowScheduler *scheduler = new WorkflowScheduler();
		
		// Create queue pool
		//QueuePool *pool = new QueuePool();
		
		// Instanciate workflow instances map
		//WorkflowInstances *workflow_instances = new WorkflowInstances();
		
		// Instanciate workflows list
		//Workflows *workflows = new Workflows();
		
		// Instanciate tasks list
		//Tasks *tasks = new Tasks();
		
		// Instanciate retry schedules list
		//RetrySchedules *retry_schedules = new RetrySchedules();
		
		// Check if workflows are to resume (we have to resume them before starting ProcessManager)
		//db.Query("SELECT workflow_instance_id, workflow_schedule_id FROM t_workflow_instance WHERE workflow_instance_status='EXECUTING'");

#if 0
		while(db.FetchRow())
		{
			Logger::Log(LOG_NOTICE,"[WID %d] Resuming",db.GetFieldInt(0));
			
			WorkflowInstance *workflow_instance = 0;
			bool workflow_terminated;
			try
			{
				workflow_instance = new WorkflowInstance(db.GetFieldInt(0));
				workflow_instance->Resume(&workflow_terminated);
				if(workflow_terminated)
					delete workflow_instance;
			}
			catch(Exception &e)
			{
				Logger::Log(LOG_NOTICE,"[WID %d] Unexpected exception trying to resume : [ %s ] %s\n",db.GetFieldInt(0),e.context,e.error);
				
				if(workflow_instance)
					delete workflow_instance;
			}
		}
		
		// On level 0 or 1, executing workflows are only stored during engine restart. Purge them since they are resumed
		if(Configuration::GetInstance()->GetInt("workflowinstance.savepoint.level")<=1)
			db.Query("DELETE FROM t_workflow_instance WHERE workflow_instance_status='EXECUTING'");
		
		// Load workflow schedules
		db.Query("SELECT ws.workflow_schedule_id, w.workflow_name, wi.workflow_instance_id FROM t_workflow_schedule ws LEFT JOIN t_workflow_instance wi ON(wi.workflow_schedule_id=ws.workflow_schedule_id AND wi.workflow_instance_status='EXECUTING') INNER JOIN t_workflow w ON(ws.workflow_id=w.workflow_id) WHERE ws.workflow_schedule_active=1");
		while(db.FetchRow())
		{
			WorkflowSchedule *workflow_schedule = 0;
			try
			{
				workflow_schedule = new WorkflowSchedule(db.GetFieldInt(0));
				scheduler->ScheduleWorkflow(workflow_schedule, db.GetFieldInt(2));
			}
			catch(Exception &e)
			{
				Logger::Log(LOG_NOTICE,"[WSID %d] Unexpected exception trying initialize workflow schedule : [ %s ] %s\n",db.GetFieldInt(0),e.context,e.error);
				
				if(workflow_schedule)
					delete workflow_schedule;
			}
		}
#endif
		
		// Start Process Manager (Forker & Gatherer)
		//ProcessManager *pm = new ProcessManager();
		
		// Start garbage GarbageCollector
		//GarbageCollector *gc = new GarbageCollector();
		
		Logger::Log(LOG_NOTICE,"evqueue core started");
		
		//int re,s,optval;
		//struct sockaddr_in local_addr,remote_addr;
		//socklen_t remote_addr_len;

                Arbiter arb(2);
                //arb.start();

                Reqresp tenant1("ipc:///tmp/req1.ipc" ,0, &arb);
                tenant1.start();
		
                Reqresp tenant2("ipc:///tmp/req2.ipc", 1, &arb);
                tenant2.start();
                
                tenant1.join();
		
		// Create listen socket
		//listen_socket=socket(PF_INET,SOCK_STREAM,0);
		
		// Configure socket
		//optval=1;
		//setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int));
		
		// Listen on socket
		//re=listen(listen_socket,config->GetInt("network.listen.backlog"));
		//if(re==-1)
		//	throw Exception("core","Unable to listen on socket");
		//Logger::Log(LOG_NOTICE,"Listen backlog set to %d",config->GetInt("network.listen.backlog"));
		
		//char *ptr,*parameters;
		
		//Logger::Log(LOG_NOTICE,"Accepting connection on port %s",config->Get("network.bind.port"));
		
		// Loop for incoming connections
		int len,*sp;
		//while(1)
		//{
			//remote_addr_len=sizeof(struct sockaddr);
			//s = accept(listen_socket,(struct sockaddr *)&remote_addr,&remote_addr_len);
                        #if 0
			if(s<0)
			{
				if(errno==EINTR)
					continue; // Interrupted by signal, continue
				
				// Shutdown requested
				Logger::Log(LOG_NOTICE,"Shutting down...");
				
				// Request shutdown on ProcessManager and wait
				pm->Shutdown();
				pm->WaitForShutdown();
				
				// Request shutdown on scheduler and wait
				scheduler->Shutdown();
				scheduler->WaitForShutdown();
				
				// Request shutdown on Retrier and wait
				retrier->Shutdown();
				retrier->WaitForShutdown();
				
				// Save current state in database
				workflow_instances->RecordSavepoint();
				
				// Request shutdown on GarbageCollector and wait
				gc->Shutdown();
				gc->WaitForShutdown();
				
				// All threads have exited, we can cleanly exit
				delete stats;
				delete retrier;
				delete scheduler;
				delete pool;
				delete workflow_instances;
				delete workflows;
				delete tasks;
				delete retry_schedules;
				delete pm;
				delete gc;
				delete seq;
				
				XQillaPlatformUtils::terminate();
				mysql_library_end();
				
				
				unlink(config->Get("core.pidfile"));
				Logger::Log(LOG_NOTICE,"Clean exit");
				delete logger;
				
				return 0;
			}
                        #endif
			
			/*sp = new int;
			*sp = s;
			pthread_t thread;
			pthread_attr_t thread_attr;
			pthread_attr_init(&thread_attr);
			pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
			pthread_create(&thread, &thread_attr, handle_connection, sp);
                        */
		//}
	}
	catch(Exception &e)
	{
		// We have to use only syslog here because the logger might not be instanciated yet
		syslog(LOG_CRIT,"Unexpected exception : [ %s ] %s\n",e.context,e.error);
		
		if(!daemonized)
			fprintf(stderr,"Unexpected exception : [ %s ] %s\n",e.context,e.error);
		
		return -1;
	}
	
	return 0;
}
