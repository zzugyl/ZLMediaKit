﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xiongziliang/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef SRC_RTSPPLAYER_RTSPPLAYER_H_TXT_
#define SRC_RTSPPLAYER_RTSPPLAYER_H_TXT_

#include <string>
#include <memory>
#include "RtspSession.h"
#include "RtspMediaSource.h"
#include "Player/PlayerBase.h"
#include "Util/util.h"
#include "Util/logger.h"
#include "Util/TimeTicker.h"
#include "Poller/Timer.h"
#include "Network/Socket.h"
#include "Network/TcpClient.h"
#include "RtspSplitter.h"
#include "RtpReceiver.h"
#include "Common/Stamp.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

//实现了rtsp播放器协议部分的功能，及数据接收功能
class RtspPlayer: public PlayerBase,public TcpClient, public RtspSplitter, public RtpReceiver {
public:
    typedef std::shared_ptr<RtspPlayer> Ptr;

    RtspPlayer(const EventPoller::Ptr &poller) ;
    virtual ~RtspPlayer(void);
    void play(const string &strUrl) override;
    void pause(bool bPause) override;
    void teardown() override;
    float getPacketLossRate(TrackType type) const override;
protected:
    //派生类回调函数
    virtual bool onCheckSDP(const string &strSdp) = 0;
    virtual void onRecvRTP(const RtpPacket::Ptr &pRtppt, const SdpTrack::Ptr &track) = 0;
    uint32_t getProgressMilliSecond() const;
    void seekToMilliSecond(uint32_t ms);

    /**
     * 收到完整的rtsp包回调，包括sdp等content数据
     * @param parser rtsp包
     */
    void onWholeRtspPacket(Parser &parser) override ;

    /**
     * 收到rtp包回调
     * @param data
     * @param len
     */
    void onRtpPacket(const char *data,uint64_t len) override ;

    /**
     * rtp数据包排序后输出
     * @param rtppt rtp数据包
     * @param trackidx track索引
     */
    void onRtpSorted(const RtpPacket::Ptr &rtppt, int trackidx) override;


    /**
     * 收到RTCP包回调
     * @param iTrackidx
     * @param track
     * @param pucData
     * @param uiLen
     */
    virtual void onRtcpPacket(int iTrackidx, SdpTrack::Ptr &track, unsigned char *pucData, unsigned int uiLen);

    /////////////TcpClient override/////////////
    void onConnect(const SockException &err) override;
    void onRecv(const Buffer::Ptr &pBuf) override;
    void onErr(const SockException &ex) override;
private:
    void onRecvRTP_l(const RtpPacket::Ptr &pRtppt, const SdpTrack::Ptr &track);
    void onPlayResult_l(const SockException &ex , bool handshakeCompleted);

    int getTrackIndexByInterleaved(int interleaved) const;
    int getTrackIndexByTrackType(TrackType trackType) const;

    void handleResSETUP(const Parser &parser, unsigned int uiTrackIndex);
    void handleResDESCRIBE(const Parser &parser);
    bool handleAuthenticationFailure(const string &wwwAuthenticateParamsStr);
    void handleResPAUSE(const Parser &parser, int type);

    //发送SETUP命令
    void sendSetup(unsigned int uiTrackIndex);
    void sendPause(int type , uint32_t ms);
    void sendDescribe();

    void sendRtspRequest(const string &cmd, const string &url ,const StrCaseMap &header = StrCaseMap());
    void sendRtspRequest(const string &cmd, const string &url ,const std::initializer_list<string> &header);
    void sendReceiverReport(bool overTcp,int iTrackIndex);
    void createUdpSockIfNecessary(int track_idx);
private:
    string _strUrl;
    vector<SdpTrack::Ptr> _aTrackInfo;
    function<void(const Parser&)> _onHandshake;
    Socket::Ptr _apRtpSock[2]; //RTP端口,trackid idx 为数组下标
    Socket::Ptr _apRtcpSock[2];//RTCP端口,trackid idx 为数组下标

    //rtsp鉴权相关
    string _rtspMd5Nonce;
    string _rtspRealm;
    //rtsp info
    string _strSession;
    unsigned int _uiCseq = 1;
    string _strContentBase;
    Rtsp::eRtpType _eType = Rtsp::RTP_TCP;

    /* 丢包率统计需要用到的参数 */
    uint16_t _aui16FirstSeq[2] = { 0 , 0};
    uint16_t _aui16NowSeq[2] = { 0 , 0 };
    uint64_t _aui64RtpRecv[2] = { 0 , 0};

    //超时功能实现
    Ticker _rtpTicker;
    std::shared_ptr<Timer> _pPlayTimer;
    std::shared_ptr<Timer> _pRtpTimer;

    //时间戳
    Stamp _stamp[2];

    //rtcp相关
    RtcpCounter _aRtcpCnt[2]; //rtcp统计,trackid idx 为数组下标
    Ticker _aRtcpTicker[2]; //rtcp发送时间,trackid idx 为数组下标

    //是否为rtsp点播
    bool _is_play_back;
    //是否为性能测试模式
    bool _benchmark_mode = false;
};

} /* namespace mediakit */

#endif /* SRC_RTSPPLAYER_RTSPPLAYER_H_TXT_ */
