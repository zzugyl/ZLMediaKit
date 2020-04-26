﻿/*
* Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
*
* This file is part of ZLMediaKit(https://github.com/xiongziliang/ZLMediaKit).
*
* Use of this source code is governed by MIT license that can be found in the
* LICENSE file in the root of the source tree. All contributing project authors
* may be found in the AUTHORS file in the root of the source tree.
*/

#include "MultiMediaSourceMuxer.h"
namespace mediakit {

MultiMuxerPrivate::~MultiMuxerPrivate() {}
MultiMuxerPrivate::MultiMuxerPrivate(const string &vhost,
                                     const string &app,
                                     const string &stream,
                                     float dur_sec,
                                     bool enable_rtsp,
                                     bool enable_rtmp,
                                     bool enable_hls,
                                     bool enable_mp4) {
    if (enable_rtmp) {
        _rtmp = std::make_shared<RtmpMediaSourceMuxer>(vhost, app, stream, std::make_shared<TitleMeta>(dur_sec));
    }
    if (enable_rtsp) {
        _rtsp = std::make_shared<RtspMediaSourceMuxer>(vhost, app, stream, std::make_shared<TitleSdp>(dur_sec));
    }

    if (enable_hls) {
        _hls = Recorder::createRecorder(Recorder::type_hls, vhost, app, stream);
    }

    if (enable_mp4) {
        _mp4 = Recorder::createRecorder(Recorder::type_mp4, vhost, app, stream);
    }
}

void MultiMuxerPrivate::resetTracks() {
    if (_rtmp) {
        _rtmp->resetTracks();
    }
    if (_rtsp) {
        _rtsp->resetTracks();
    }

    //拷贝智能指针，目的是为了防止跨线程调用设置录像相关api导致的线程竞争问题
    auto hls = _hls;
    if (hls) {
        hls->resetTracks();
    }

    auto mp4 = _mp4;
    if (mp4) {
        mp4->resetTracks();
    }
}

void MultiMuxerPrivate::setMediaListener(const std::weak_ptr<MediaSourceEvent> &listener) {
    if (_rtmp) {
        _rtmp->setListener(listener);
    }

    if (_rtsp) {
        _rtsp->setListener(listener);
    }

    auto hls_src = getHlsMediaSource();
    if (hls_src) {
        hls_src->setListener(listener);
    }
    _meida_listener = listener;
}

int MultiMuxerPrivate::totalReaderCount() const {
    auto hls_src = getHlsMediaSource();
    return (_rtsp ? _rtsp->readerCount() : 0) + (_rtmp ? _rtmp->readerCount() : 0) + (hls_src ? hls_src->readerCount() : 0);
}

static std::shared_ptr<MediaSinkInterface> makeRecorder(const vector<Track::Ptr> &tracks, Recorder::type type, MediaSource &sender){
    auto recorder = Recorder::createRecorder(type, sender.getVhost(), sender.getApp(), sender.getId());
    for (auto &track : tracks) {
        recorder->addTrack(track);
    }
    return recorder;
}

//此函数可能跨线程调用
bool MultiMuxerPrivate::setupRecord(MediaSource &sender, Recorder::type type, bool start, const string &custom_path){
    switch (type) {
        case Recorder::type_hls : {
            if (start && !_hls) {
                //开始录制
                _hls = makeRecorder(getTracks(true), type, sender);
                auto hls_src = getHlsMediaSource();
                if (hls_src) {
                    //设置HlsMediaSource的事件监听器
                    hls_src->setListener(_meida_listener);
                    hls_src->setTrackSource(shared_from_this());
                }
            } else if (!start && _hls) {
                //停止录制
                _hls = nullptr;
            }
            return true;
        }
        case Recorder::type_mp4 : {
            if (start && !_mp4) {
                //开始录制
                _mp4 = makeRecorder(getTracks(true), type, sender);;
            } else if (!start && _mp4) {
                //停止录制
                _mp4 = nullptr;
            }
            return true;
        }
        default:
            return false;
    }
}

//此函数可能跨线程调用
bool MultiMuxerPrivate::isRecording(MediaSource &sender, Recorder::type type){
    switch (type){
        case Recorder::type_hls :
            return _hls ? true : false;
        case Recorder::type_mp4 :
            return _mp4 ? true : false;
        default:
            return false;
    }
}

void MultiMuxerPrivate::setTimeStamp(uint32_t stamp) {
    if (_rtmp) {
        _rtmp->setTimeStamp(stamp);
    }
    if (_rtsp) {
        _rtsp->setTimeStamp(stamp);
    }
}

void MultiMuxerPrivate::setTrackListener(Listener *listener) {
    _listener = listener;
}

void MultiMuxerPrivate::onTrackReady(const Track::Ptr &track) {
    if (_rtmp) {
        _rtmp->addTrack(track);
    }
    if (_rtsp) {
        _rtsp->addTrack(track);
    }

    //拷贝智能指针，目的是为了防止跨线程调用设置录像相关api导致的线程竞争问题
    auto hls = _hls;
    if (hls) {
        hls->addTrack(track);
    }
    auto mp4 = _mp4;
    if (mp4) {
        mp4->addTrack(track);
    }
}

void MultiMuxerPrivate::onTrackFrame(const Frame::Ptr &frame) {
    if (_rtmp) {
        _rtmp->inputFrame(frame);
    }
    if (_rtsp) {
        _rtsp->inputFrame(frame);
    }
    //拷贝智能指针，目的是为了防止跨线程调用设置录像相关api导致的线程竞争问题
    //此处使用智能指针拷贝来确保线程安全，比互斥锁性能更优
    auto hls = _hls;
    if (hls) {
        hls->inputFrame(frame);
    }
    auto mp4 = _mp4;
    if (mp4) {
        mp4->inputFrame(frame);
    }
}

void MultiMuxerPrivate::onAllTrackReady() {
    if (_rtmp) {
        _rtmp->setTrackSource(shared_from_this());
        _rtmp->onAllTrackReady();
    }
    if (_rtsp) {
        _rtsp->setTrackSource(shared_from_this());
        _rtsp->onAllTrackReady();
    }

    auto hls_src = getHlsMediaSource();
    if (hls_src) {
        hls_src->setTrackSource(shared_from_this());
    }

    if (_listener) {
        _listener->onAllTrackReady();
    }
}

MediaSource::Ptr MultiMuxerPrivate::getHlsMediaSource() const {
    auto recorder = dynamic_pointer_cast<HlsRecorder>(_hls);
    if (recorder) {
        return recorder->getMediaSource();
    }
    return nullptr;
}

/////////////////////////////////////////////////////////////////

MultiMediaSourceMuxer::~MultiMediaSourceMuxer() {}
MultiMediaSourceMuxer::MultiMediaSourceMuxer(const string &vhost,
                                             const string &app,
                                             const string &stream,
                                             float dur_sec,
                                             bool enable_rtsp,
                                             bool enable_rtmp,
                                             bool enable_hls,
                                             bool enable_mp4) {
    _muxer.reset(new MultiMuxerPrivate(vhost, app, stream, dur_sec, enable_rtsp, enable_rtmp, enable_hls, enable_mp4));
}

void MultiMediaSourceMuxer::setMediaListener(const std::weak_ptr<MediaSourceEvent> &listener) {
    _muxer->setMediaListener(shared_from_this());
    _listener = listener;
}

int MultiMediaSourceMuxer::totalReaderCount() const {
    return _muxer->totalReaderCount();
}

void MultiMediaSourceMuxer::setTimeStamp(uint32_t stamp) {
    _muxer->setTimeStamp(stamp);
}

void MultiMediaSourceMuxer::setTrackListener(Listener *listener) {
    _muxer->setTrackListener(listener);
}

vector<Track::Ptr> MultiMediaSourceMuxer::getTracks(bool trackReady) const {
    return _muxer->getTracks(trackReady);
}

bool MultiMediaSourceMuxer::seekTo(MediaSource &sender, uint32_t ui32Stamp) {
    auto listener = _listener.lock();
    if (!listener) {
        return false;
    }
    return listener->seekTo(sender, ui32Stamp);
}

bool MultiMediaSourceMuxer::close(MediaSource &sender, bool force) {
    auto listener = _listener.lock();
    if (!listener) {
        return false;
    }
    return listener->close(sender, force);
}

int MultiMediaSourceMuxer::totalReaderCount(MediaSource &sender) {
    auto listener = _listener.lock();
    if (!listener) {
        return _muxer->totalReaderCount();
    }
    return listener->totalReaderCount(sender);
}

bool MultiMediaSourceMuxer::setupRecord(MediaSource &sender, Recorder::type type, bool start, const string &custom_path) {
    return _muxer->setupRecord(sender,type,start,custom_path);
}

bool MultiMediaSourceMuxer::isRecording(MediaSource &sender, Recorder::type type) {
    return _muxer->isRecording(sender,type);
}

void MultiMediaSourceMuxer::addTrack(const Track::Ptr &track) {
    _muxer->addTrack(track);
}

void MultiMediaSourceMuxer::addTrackCompleted() {
    _muxer->addTrackCompleted();
}

void MultiMediaSourceMuxer::resetTracks() {
    _muxer->resetTracks();
}

void MultiMediaSourceMuxer::inputFrame(const Frame::Ptr &frame) {
    _muxer->inputFrame(frame);
}

}//namespace mediakit