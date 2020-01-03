﻿/*
 * MIT License
 *
 * Copyright (c) 2016-2019 xiongziliang <771730766@qq.com>
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
#ifndef SRC_HTTP_HTTPSESSION_H_
#define SRC_HTTP_HTTPSESSION_H_

#include <functional>
#include "Network/TcpSession.h"
#include "Rtmp/RtmpMediaSource.h"
#include "Rtmp/FlvMuxer.h"
#include "HttpRequestSplitter.h"
#include "WebSocketSplitter.h"
#include "HttpCookieManager.h"
#include "HttpFileManager.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

class HttpSession: public TcpSession,
                   public FlvMuxer,
                   public HttpRequestSplitter,
                   public WebSocketSplitter {
public:
	typedef StrCaseMap KeyValue;
	typedef HttpResponseInvokerImp HttpResponseInvoker;
	friend class AsyncSender;
	/**
	 * @param errMsg 如果为空，则代表鉴权通过，否则为错误提示
	 * @param accessPath 运行或禁止访问的根目录
	 * @param cookieLifeSecond 鉴权cookie有效期
	 **/
	typedef std::function<void(const string &errMsg,const string &accessPath, int cookieLifeSecond)> HttpAccessPathInvoker;

	HttpSession(const Socket::Ptr &pSock);
	virtual ~HttpSession();

	virtual void onRecv(const Buffer::Ptr &) override;
	virtual void onError(const SockException &err) override;
	virtual void onManager() override;
	static string urlDecode(const string &str);
protected:
	//FlvMuxer override
	void onWrite(const Buffer::Ptr &data) override ;
	void onDetach() override;
	std::shared_ptr<FlvMuxer> getSharedPtr() override;

	//HttpRequestSplitter override
	int64_t onRecvHeader(const char *data,uint64_t len) override;
	void onRecvContent(const char *data,uint64_t len) override;

	/**
	 * 重载之用于处理不定长度的content
	 * 这个函数可用于处理大文件上传、http-flv推流
	 * @param header http请求头
	 * @param data content分片数据
	 * @param len content分片数据大小
	 * @param totalSize content总大小,如果为0则是不限长度content
	 * @param recvedSize 已收数据大小
	 */
	virtual void onRecvUnlimitedContent(const Parser &header,
										const char *data,
										uint64_t len,
										uint64_t totalSize,
										uint64_t recvedSize){
        shutdown(SockException(Err_shutdown,"http post content is too huge,default closed"));
	}

	/**
     * websocket客户端连接上事件
     * @param header http头
     * @return true代表允许websocket连接，否则拒绝
     */
    virtual bool onWebSocketConnect(const Parser &header){
        WarnL << "http server do not support websocket default";
        return false;
    }

	//WebSocketSplitter override
	/**
     * 发送数据进行websocket协议打包后回调
     * @param buffer websocket协议数据
     */
	void onWebSocketEncodeData(const Buffer::Ptr &buffer) override;
private:
	void Handle_Req_GET(int64_t &content_len);
	void Handle_Req_POST(int64_t &content_len);
	bool checkLiveFlvStream(const function<void()> &cb = nullptr);
	bool checkWebSocket();
	bool emitHttpEvent(bool doInvoke);
	void urlDecode(Parser &parser);
	void sendNotFound(bool bClose);
	void sendResponse(const char *pcStatus, bool bClose, const char *pcContentType = nullptr,
					  const HttpSession::KeyValue &header = HttpSession::KeyValue(),
                      const HttpBody::Ptr &body = nullptr,bool is_http_flv = false);

	//设置socket标志
	void setSocketFlags();
private:
	string _origin;
    Parser _parser;
    Ticker _ticker;
    //消耗的总流量
    uint64_t _ui64TotalBytes = 0;
    //flv over http
    MediaInfo _mediaInfo;
    //处理content数据的callback
    function<bool (const char *data,uint64_t len) > _contentCallBack;
	bool _flv_over_websocket = false;
	bool _is_flv_stream = false;
};


typedef TcpSessionWithSSL<HttpSession> HttpsSession;

} /* namespace mediakit */

#endif /* SRC_HTTP_HTTPSESSION_H_ */
