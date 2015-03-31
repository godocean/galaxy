// Copyright (c) 2015, Galaxy Authors. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include "galaxy.h"

#include "proto/master.pb.h"
#include "rpc/rpc_client.h"

namespace galaxy {
class GalaxyImpl : public Galaxy {
public:
    GalaxyImpl(const std::string& master_addr)
      : master_(NULL) {
        rpc_client_ = new RpcClient();
        rpc_client_->GetStub(master_addr, &master_);
    }
    virtual ~GalaxyImpl() {}
    bool NewJob(const JobDescription& job);
    bool UpdateJob(const JobDescription& job);
    bool ListJob(std::vector<JobInstanceDescription>* jobs);
    bool TerminateJob(int64_t job_id);
    bool ListTask(int64_t job_id,std::vector<TaskDescription>* tasks);
    bool KillTask(int64_t task_id);
private:
    RpcClient* rpc_client_;
    Master_Stub* master_;
};

bool GalaxyImpl::KillTask(int64_t task_id){
    TerminateTaskRequest request;
    request.set_task_id(task_id);
    TerminateTaskResponse response;
    rpc_client_->SendRequest(master_,
            &Master_Stub::TerminateTask,
            &request, &response, 5, 1);
    if (response.has_status()
            && response.status() == 0) {
        fprintf(stdout, "SUCCESS\n");
    }
    else {
        if (response.has_status()) {
            fprintf(stdout, "FAIL %d\n", response.status());
        }
        else {
            fprintf(stdout, "FAIL unkown\n");
        }
    }
    return true;
}
bool GalaxyImpl::TerminateJob(int64_t job_id) {
    KillJobRequest request;
    KillJobResponse response;
    request.set_job_id(job_id);
    rpc_client_->SendRequest(master_, &Master_Stub::KillJob,
                             &request,&response,5,1);

    return true;
}

bool GalaxyImpl::NewJob(const JobDescription& job) {
    NewJobRequest request;
    NewJobResponse response;
    request.set_job_name(job.job_name);
    request.set_job_raw(job.pkg.source);
    request.set_cmd_line(job.cmd_line);
    request.set_replica_num(job.replicate_count);
    rpc_client_->SendRequest(master_, &Master_Stub::NewJob,
                             &request,&response,5,1);
    return true;
}

bool GalaxyImpl::UpdateJob(const JobDescription& job) {
    UpdateJobRequest request;
    UpdateJobResponse response;
    request.set_job_id(job.job_id);
    request.set_replica_num(job.replicate_count);
    rpc_client_->SendRequest(master_, &Master_Stub::UpdateJob,
                             &request, &response, 5, 1);
    return true;
}

bool GalaxyImpl::ListJob(std::vector<JobInstanceDescription>* jobs) {
    ListJobRequest request;
    ListJobResponse response;
    rpc_client_->SendRequest(master_, &Master_Stub::ListJob,
                             &request,&response,5,1);
    int job_num = response.jobs_size();
    for(int i = 0; i< job_num;i++){
        const JobInstance& job = response.jobs(i);
        JobInstanceDescription job_instance ;
        job_instance.job_id = job.job_id();
        job_instance.job_name = job.job_name();
        job_instance.running_task_num = job.running_task_num();
        job_instance.replicate_count = job.replica_num();
        jobs->push_back(job_instance);
    }
    return true;
}

bool GalaxyImpl::ListTask(int64_t job_id, std::vector<TaskDescription>* /*job*/) {
    ListTaskRequest request;
    if (job_id != -1) {
        request.set_task_id(job_id);
    }
    ListTaskResponse response;
    rpc_client_->SendRequest(master_, &Master_Stub::ListTask,
                             &request, &response, 5, 1);
    fprintf(stdout, "================================\n");
    int task_size = response.tasks_size();
    for (int i = 0; i < task_size; i++) {
        if (!response.tasks(i).has_info() ||
                !response.tasks(i).info().has_task_id()) {
            continue;
        }
        int64_t task_id = response.tasks(i).info().task_id();
        std::string task_name;
        std::string agent_addr;
        std::string state;
        if (response.tasks(i).has_info()){
            if (response.tasks(i).info().has_task_name()) {
                task_name = response.tasks(i).info().task_name();
            }
        }
        if (response.tasks(i).has_status()) {
            int task_state = response.tasks(i).status();
            if (TaskState_IsValid(task_state)) {
                state = TaskState_Name((TaskState)task_state);
            }
        }
        if (response.tasks(i).has_agent_addr()) {
            agent_addr = response.tasks(i).agent_addr();
        }
        fprintf(stdout, "%ld\t%s\t%s\t%s\n", task_id, task_name.c_str(), state.c_str(), agent_addr.c_str());
    }
    fprintf(stdout, "================================\n");
    return true;
}



Galaxy* Galaxy::ConnectGalaxy(const std::string& master_addr) {
    return new GalaxyImpl(master_addr);
}

}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
