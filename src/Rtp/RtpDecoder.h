﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xiongziliang/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_RTPDECODER_H
#define ZLMEDIAKIT_RTPDECODER_H

#if defined(ENABLE_RTPPROXY)
#include "Network/Buffer.h"
using namespace toolkit;

namespace mediakit{

class RtpDecoder {
public:
    RtpDecoder();
    virtual ~RtpDecoder();
protected:
    void decodeRtp(const void *data, int bytes);
    virtual void onRtpDecode(const uint8_t *packet, int bytes, uint32_t timestamp, int flags) = 0;
private:
    void *_rtp_decoder = nullptr;
    BufferRaw::Ptr _buffer;
};

}//namespace mediakit
#endif//defined(ENABLE_RTPPROXY)
#endif //ZLMEDIAKIT_RTPDECODER_H
