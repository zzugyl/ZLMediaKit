﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xiongziliang/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_RTPPROXY)
#include "RtpSelector.h"

namespace mediakit{

INSTANCE_IMP(RtpSelector);

bool RtpSelector::inputRtp(const Socket::Ptr &sock, const char *data, int data_len,const struct sockaddr *addr,uint32_t *dts_out) {
    uint32_t ssrc = 0;
    if(!getSSRC(data,data_len,ssrc)){
        WarnL << "get ssrc from rtp failed:" << data_len;
        return false;
    }
    auto process = getProcess(ssrc, true);
    if(process){
        return process->inputRtp(sock, data,data_len, addr,dts_out);
    }
    return false;
}

bool RtpSelector::getSSRC(const char *data,int data_len, uint32_t &ssrc){
    if(data_len < 12){
        return false;
    }
    uint32_t *ssrc_ptr = (uint32_t *)(data + 8);
    ssrc = ntohl(*ssrc_ptr);
    return true;
}

RtpProcess::Ptr RtpSelector::getProcess(uint32_t ssrc,bool makeNew) {
    lock_guard<decltype(_mtx_map)> lck(_mtx_map);
    auto it = _map_rtp_process.find(ssrc);
    if(it == _map_rtp_process.end() && !makeNew){
        return nullptr;
    }
    RtpProcessHelper::Ptr &ref = _map_rtp_process[ssrc];
    if(!ref){
        ref = std::make_shared<RtpProcessHelper>(ssrc,shared_from_this());
        ref->attachEvent();
        createTimer();
    }
    return ref->getProcess();
}

void RtpSelector::createTimer() {
    if (!_timer) {
        //创建超时管理定时器
        weak_ptr<RtpSelector> weakSelf = shared_from_this();
        _timer = std::make_shared<Timer>(3.0, [weakSelf] {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return false;
            }
            strongSelf->onManager();
            return true;
        }, EventPollerPool::Instance().getPoller());
    }
}

void RtpSelector::delProcess(uint32_t ssrc,const RtpProcess *ptr) {
    lock_guard<decltype(_mtx_map)> lck(_mtx_map);
    auto it = _map_rtp_process.find(ssrc);
    if(it == _map_rtp_process.end()){
        return;
    }

    if(it->second->getProcess().get() != ptr){
        return;
    }

    _map_rtp_process.erase(it);
}

void RtpSelector::onManager() {
    lock_guard<decltype(_mtx_map)> lck(_mtx_map);
    for (auto it = _map_rtp_process.begin(); it != _map_rtp_process.end();) {
        if (it->second->getProcess()->alive()) {
            ++it;
            continue;
        }
        WarnL << "RtpProcess timeout:" << printSSRC(it->first);
        it = _map_rtp_process.erase(it);
    }
}

RtpSelector::RtpSelector() {
}

RtpSelector::~RtpSelector() {
}

RtpProcessHelper::RtpProcessHelper(uint32_t ssrc, const weak_ptr<RtpSelector> &parent) {
    _ssrc = ssrc;
    _parent = parent;
    _process = std::make_shared<RtpProcess>(_ssrc);
}

RtpProcessHelper::~RtpProcessHelper() {
}

void RtpProcessHelper::attachEvent() {
    _process->setListener(shared_from_this());
}

bool RtpProcessHelper::close(MediaSource &sender, bool force) {
    //此回调在其他线程触发
    if(!_process || (!force && _process->totalReaderCount())){
        return false;
    }
    auto parent = _parent.lock();
    if(!parent){
        return false;
    }
    parent->delProcess(_ssrc,_process.get());
    WarnL << "close media:" << sender.getSchema() << "/" << sender.getVhost() << "/" << sender.getApp() << "/" << sender.getId() << " " << force;
    return true;
}

int RtpProcessHelper::totalReaderCount(MediaSource &sender) {
    return _process ? _process->totalReaderCount() : sender.totalReaderCount();
}

RtpProcess::Ptr &RtpProcessHelper::getProcess() {
    return _process;
}

}//namespace mediakit
#endif//defined(ENABLE_RTPPROXY)