/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ISSREVENTLISTENER_HPP
#define ISSREVENTLISTENER_HPP

#include "AudioDefinesInternal.hpp"

namespace telux {
namespace audio {

class ISSREventListener {

 public:
    virtual void onSSREvent(SSREvent event) = 0;
};

}  // end of namespace audio
}  // end of namespace telux

#endif  // ISSREVENTLISTENER_HPP