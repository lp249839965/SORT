/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.

    Copyright (c) 2011-2019 by Jiayin Cao - All rights reserved.

    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#pragma once

#ifdef AVX_ENABLED

#define OBVH_IMPLEMENTATION
#define Fbvh        Obvh
#define Fbvh_Node   Obvh_Node

#include "simd/avx_bbox.h"
#include "simd/avx_triangle.h"
//#include "simd/avx_line.h"
#include "fast_bvh.h"

#undef OBVH_IMPLEMENTATION
#undef Fbvh
#undef Fbvh_Node

#endif