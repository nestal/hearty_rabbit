/////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Thomson Reuters Limited
// This document contains information proprietary to Thomson Reuters Limited, and
// may not be reproduced, disclosed or used in whole or in part without
// the express written permission of Thomson Reuters Limited.
//
/////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>

namespace hrb {

static const std::size_t object_id_size  = 64;
static std::array<unsigned char, 5> object_redis_key_prefix = {'b','l','o','b', ':'};
using ObjectRedisKey = std::array<unsigned char, object_id_size + object_redis_key_prefix.size()>;

} // end of namespace
