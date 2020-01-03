﻿/*
 * MIT License
 *
 * Copyright (c) 2019 xiongziliang <771730766@qq.com>
 *
 * This file is part of ZLMediaKit(https://github.com/xiongziliang/ZLMediaKit).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MK_PROXY_PLAYER_H_
#define MK_PROXY_PLAYER_H_

#include "mk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *mk_proxy_player;

/**
 * 创建一个代理播放器
 * @param vhost 虚拟主机名，一般为__defaultVhost__
 * @param app 应用名
 * @param stream 流名
 * @param rtp_type rtsp播放方式:RTP_TCP = 0, RTP_UDP = 1, RTP_MULTICAST = 2
 * @param hls_enabled 是否生成hls
 * @param mp4_enabled 是否生成mp4
 * @return 对象指针
 */
API_EXPORT mk_proxy_player API_CALL mk_proxy_player_create(const char *vhost, const char *app, const char *stream, int hls_enabled, int mp4_enabled);

/**
 * 销毁代理播放器
 * @param ctx 对象指针
 */
API_EXPORT void API_CALL mk_proxy_player_release(mk_proxy_player ctx);

/**
 * 设置代理播放器配置选项
 * @param ctx 代理播放器指针
 * @param key 配置项键,支持 net_adapter/rtp_type/rtsp_user/rtsp_pwd/protocol_timeout_ms/media_timeout_ms/beat_interval_ms/max_analysis_ms
 * @param val 配置项值,如果是整形，需要转换成统一转换成string
 */
API_EXPORT void API_CALL mk_proxy_player_set_option(mk_proxy_player ctx, const char *key, const char *val);

/**
 * 开始播放
 * @param ctx 对象指针
 * @param url 播放url,支持rtsp/rtmp
 */
API_EXPORT void API_CALL mk_proxy_player_play(mk_proxy_player ctx, const char *url);

#ifdef __cplusplus
}
#endif

#endif /* MK_PROXY_PLAYER_H_ */
