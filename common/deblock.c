/*****************************************************************************
 * deblock.c: deblocking
 *****************************************************************************
 * Copyright (C) 2003-2012 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
 *          Henrik Gramner <hengar-6@student.ltu.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/
#include "common.h"

#if HAVE_ARMV6
#include <arm_neon.h>
#endif


/* Deblocking filter */
static const uint8_t i_alpha_table[52+12*3] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
   255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,
};
static const uint8_t i_beta_table[52+12*3] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
};
static const int8_t i_tc0_table[52+12*3][4] =
{
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 1 },
    {-1, 0, 0, 1 }, {-1, 0, 0, 1 }, {-1, 0, 0, 1 }, {-1, 0, 1, 1 }, {-1, 0, 1, 1 }, {-1, 1, 1, 1 },
    {-1, 1, 1, 1 }, {-1, 1, 1, 1 }, {-1, 1, 1, 1 }, {-1, 1, 1, 2 }, {-1, 1, 1, 2 }, {-1, 1, 1, 2 },
    {-1, 1, 1, 2 }, {-1, 1, 2, 3 }, {-1, 1, 2, 3 }, {-1, 2, 2, 3 }, {-1, 2, 2, 4 }, {-1, 2, 3, 4 },
    {-1, 2, 3, 4 }, {-1, 3, 3, 5 }, {-1, 3, 4, 6 }, {-1, 3, 4, 6 }, {-1, 4, 5, 7 }, {-1, 4, 5, 8 },
    {-1, 4, 6, 9 }, {-1, 5, 7,10 }, {-1, 6, 8,11 }, {-1, 6, 8,13 }, {-1, 7,10,14 }, {-1, 8,11,16 },
    {-1, 9,12,18 }, {-1,10,13,20 }, {-1,11,15,23 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
};
#define alpha_table(x) i_alpha_table[(x)+24]
#define beta_table(x)  i_beta_table[(x)+24]
#define tc0_table(x)   i_tc0_table[(x)+24]

/* From ffmpeg */
static ALWAYS_INLINE void deblock_edge_luma_c( pixel *pix, intptr_t xstride, int alpha, int beta, int8_t tc0 )
{
    int p2 = pix[-3*xstride];
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];
    int q2 = pix[ 2*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        int tc = tc0;
        int delta;
        if( abs( p2 - p0 ) < beta )
        {
            if( tc0 )
                pix[-2*xstride] = p1 + x264_clip3( (( p2 + ((p0 + q0 + 1) >> 1)) >> 1) - p1, -tc0, tc0 );
            tc++;
        }
        if( abs( q2 - q0 ) < beta )
        {
            if( tc0 )
                pix[ 1*xstride] = q1 + x264_clip3( (( q2 + ((p0 + q0 + 1) >> 1)) >> 1) - q1, -tc0, tc0 );
            tc++;
        }

        delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
        pix[-1*xstride] = x264_clip_pixel( p0 + delta );    /* p0' */
        pix[ 0*xstride] = x264_clip_pixel( q0 - delta );    /* q0' */
    }
}
static inline void deblock_luma_c( pixel *pix, intptr_t xstride, intptr_t ystride, int alpha, int beta, int8_t *tc0 )
{
    for( int i = 0; i < 4; i++ )
    {
        if( tc0[i] < 0 )
        {
            pix += 4*ystride;
            continue;
        }
        for( int d = 0; d < 4; d++, pix += ystride )
            deblock_edge_luma_c( pix, xstride, alpha, beta, tc0[i] );
    }
}
static void deblock_h_luma_mbaff_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    for( int d = 0; d < 8; d++, pix += stride )
        deblock_edge_luma_c( pix, 1, alpha, beta, tc0[d>>1] );
}
static void deblock_v_luma_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_luma_c( pix, stride, 1, alpha, beta, tc0 );
}
static void deblock_h_luma_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_luma_c( pix, 1, stride, alpha, beta, tc0 );
}

static ALWAYS_INLINE void deblock_edge_chroma_c( pixel *pix, intptr_t xstride, int alpha, int beta, int8_t tc )
{
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        int delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
        pix[-1*xstride] = x264_clip_pixel( p0 + delta );    /* p0' */
        pix[ 0*xstride] = x264_clip_pixel( q0 - delta );    /* q0' */
    }
}
static ALWAYS_INLINE void deblock_chroma_c( pixel *pix, int height, intptr_t xstride, intptr_t ystride, int alpha, int beta, int8_t *tc0 )
{
    for( int i = 0; i < 4; i++ )
    {
        int tc = tc0[i];
        if( tc <= 0 )
        {
            pix += height*ystride;
            continue;
        }
        for( int d = 0; d < height; d++, pix += ystride-2 )
            for( int e = 0; e < 2; e++, pix++ )
                deblock_edge_chroma_c( pix, xstride, alpha, beta, tc0[i] );
    }
}
static void deblock_h_chroma_mbaff_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_chroma_c( pix, 1, 2, stride, alpha, beta, tc0 );
}
static void deblock_v_chroma_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_chroma_c( pix, 2, stride, 2, alpha, beta, tc0 );
}
static void deblock_h_chroma_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_chroma_c( pix, 2, 2, stride, alpha, beta, tc0 );
}
static void deblock_h_chroma_422_c( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_chroma_c( pix, 4, 2, stride, alpha, beta, tc0 );
}

static ALWAYS_INLINE void deblock_edge_luma_intra_c( pixel *pix, intptr_t xstride, int alpha, int beta )
{
    int p2 = pix[-3*xstride];
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];
    int q2 = pix[ 2*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        if( abs( p0 - q0 ) < ((alpha >> 2) + 2) )
        {
            if( abs( p2 - p0 ) < beta ) /* p0', p1', p2' */
            {
                const int p3 = pix[-4*xstride];
                pix[-1*xstride] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                pix[-2*xstride] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                pix[-3*xstride] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
            }
            else /* p0' */
                pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
            if( abs( q2 - q0 ) < beta ) /* q0', q1', q2' */
            {
                const int q3 = pix[3*xstride];
                pix[0*xstride] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                pix[1*xstride] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                pix[2*xstride] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
            }
            else /* q0' */
                pix[0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
        }
        else /* p0', q0' */
        {
            pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
            pix[ 0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
        }
    }
}
static inline void deblock_luma_intra_c( pixel *pix, intptr_t xstride, intptr_t ystride, int alpha, int beta )
{
    for( int d = 0; d < 16; d++, pix += ystride )
        deblock_edge_luma_intra_c( pix, xstride, alpha, beta );
}
static void deblock_h_luma_intra_mbaff_c( pixel *pix, intptr_t ystride, int alpha, int beta )
{
    for( int d = 0; d < 8; d++, pix += ystride )
        deblock_edge_luma_intra_c( pix, 1, alpha, beta );
}
static void deblock_v_luma_intra_c( pixel *pix, intptr_t stride, int alpha, int beta )
{
    deblock_luma_intra_c( pix, stride, 1, alpha, beta );
}
static void deblock_h_luma_intra_c( pixel *pix, intptr_t stride, int alpha, int beta )
{
    deblock_luma_intra_c( pix, 1, stride, alpha, beta );
}

static ALWAYS_INLINE void deblock_edge_chroma_intra_c( pixel *pix, intptr_t xstride, int alpha, int beta )
{
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        pix[-1*xstride] = (2*p1 + p0 + q1 + 2) >> 2;   /* p0' */
        pix[ 0*xstride] = (2*q1 + q0 + p1 + 2) >> 2;   /* q0' */
    }
}
static ALWAYS_INLINE void deblock_chroma_intra_c( pixel *pix, int width, int height, intptr_t xstride, intptr_t ystride, int alpha, int beta )
{
    for( int d = 0; d < height; d++, pix += ystride-2 )
        for( int e = 0; e < width; e++, pix++ )
            deblock_edge_chroma_intra_c( pix, xstride, alpha, beta );
}
static void deblock_h_chroma_intra_mbaff_c( pixel *pix, intptr_t stride, int alpha, int beta )
{
    deblock_chroma_intra_c( pix, 2, 4, 2, stride, alpha, beta );
}
static void deblock_v_chroma_intra_c( pixel *pix, intptr_t stride, int alpha, int beta )
{
    deblock_chroma_intra_c( pix, 1, 16, stride, 2, alpha, beta );
}
static void deblock_h_chroma_intra_c( pixel *pix, intptr_t stride, int alpha, int beta )
{
    deblock_chroma_intra_c( pix, 2, 8, 2, stride, alpha, beta );
}
static void deblock_h_chroma_422_intra_c( pixel *pix, intptr_t stride, int alpha, int beta )
{
    deblock_chroma_intra_c( pix, 2, 16, 2, stride, alpha, beta );
}

static void deblock_strength_c( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4], int mvy_limit,
                                int bframe )
{
    for( int dir = 0; dir < 2; dir++ )
    {
        int s1 = dir ? 1 : 8;
        int s2 = dir ? 8 : 1;
        for( int edge = 0; edge < 4; edge++ )
            for( int i = 0, loc = X264_SCAN8_0+edge*s2; i < 4; i++, loc += s1 )
            {
                int locn = loc - s2;
                if( nnz[loc] || nnz[locn] )
                    bs[dir][edge][i] = 2;
                else if( ref[0][loc] != ref[0][locn] ||
                         abs( mv[0][loc][0] - mv[0][locn][0] ) >= 4 ||
                         abs( mv[0][loc][1] - mv[0][locn][1] ) >= mvy_limit ||
                        (bframe && (ref[1][loc] != ref[1][locn] ||
                         abs( mv[1][loc][0] - mv[1][locn][0] ) >= 4 ||
                         abs( mv[1][loc][1] - mv[1][locn][1] ) >= mvy_limit )))
                {
                    bs[dir][edge][i] = 1;
                }
                else
                    bs[dir][edge][i] = 0;
            }
    }
}

static ALWAYS_INLINE void deblock_edge( x264_t *h, pixel *pix, intptr_t i_stride, uint8_t bS[4], int i_qp,
                                        int a, int b, int b_chroma, x264_deblock_inter_t pf_inter )
{
    int index_a = i_qp + a;
    int index_b = i_qp + b;
    int alpha = alpha_table(index_a) << (BIT_DEPTH-8);
    int beta  = beta_table(index_b) << (BIT_DEPTH-8);
    int8_t tc[4];

    if( !M32(bS) || !alpha || !beta )
        return;

    tc[0] = (tc0_table(index_a)[bS[0]] << (BIT_DEPTH-8)) + b_chroma;
    tc[1] = (tc0_table(index_a)[bS[1]] << (BIT_DEPTH-8)) + b_chroma;
    tc[2] = (tc0_table(index_a)[bS[2]] << (BIT_DEPTH-8)) + b_chroma;
    tc[3] = (tc0_table(index_a)[bS[3]] << (BIT_DEPTH-8)) + b_chroma;

    pf_inter( pix, i_stride, alpha, beta, tc );
}

static ALWAYS_INLINE void deblock_edge_intra( x264_t *h, pixel *pix, intptr_t i_stride, uint8_t bS[4], int i_qp,
                                              int a, int b, int b_chroma, x264_deblock_intra_t pf_intra )
{
    int index_a = i_qp + a;
    int index_b = i_qp + b;
    int alpha = alpha_table(index_a) << (BIT_DEPTH-8);
    int beta  = beta_table(index_b) << (BIT_DEPTH-8);

    if( !alpha || !beta )
        return;

    pf_intra( pix, i_stride, alpha, beta );
}

static ALWAYS_INLINE void x264_macroblock_cache_load_neighbours_deblock( x264_t *h, int mb_x, int mb_y )
{
    int deblock_on_slice_edges = h->sh.i_disable_deblocking_filter_idc != 2;

    h->mb.i_neighbour = 0;
    h->mb.i_mb_xy = mb_y * h->mb.i_mb_stride + mb_x;
    h->mb.b_interlaced = PARAM_INTERLACED && h->mb.field[h->mb.i_mb_xy];
    h->mb.i_mb_top_y = mb_y - (1 << MB_INTERLACED);
    h->mb.i_mb_top_xy = mb_x + h->mb.i_mb_stride*h->mb.i_mb_top_y;
    h->mb.i_mb_left_xy[1] =
    h->mb.i_mb_left_xy[0] = h->mb.i_mb_xy - 1;
    if( SLICE_MBAFF )
    {
        if( mb_y&1 )
        {
            if( mb_x && h->mb.field[h->mb.i_mb_xy - 1] != MB_INTERLACED )
                h->mb.i_mb_left_xy[0] -= h->mb.i_mb_stride;
        }
        else
        {
            if( h->mb.i_mb_top_xy >= 0 && MB_INTERLACED && !h->mb.field[h->mb.i_mb_top_xy] )
            {
                h->mb.i_mb_top_xy += h->mb.i_mb_stride;
                h->mb.i_mb_top_y++;
            }
            if( mb_x && h->mb.field[h->mb.i_mb_xy - 1] != MB_INTERLACED )
                h->mb.i_mb_left_xy[1] += h->mb.i_mb_stride;
        }
    }

    if( mb_x > 0 && (deblock_on_slice_edges ||
        h->mb.slice_table[h->mb.i_mb_left_xy[0]] == h->mb.slice_table[h->mb.i_mb_xy]) )
        h->mb.i_neighbour |= MB_LEFT;
    if( mb_y > MB_INTERLACED && (deblock_on_slice_edges
        || h->mb.slice_table[h->mb.i_mb_top_xy] == h->mb.slice_table[h->mb.i_mb_xy]) )
        h->mb.i_neighbour |= MB_TOP;
}

void x264_frame_deblock_row( x264_t *h, int mb_y )
{
    int b_interlaced = SLICE_MBAFF;
    int a = h->sh.i_alpha_c0_offset - QP_BD_OFFSET;
    int b = h->sh.i_beta_offset - QP_BD_OFFSET;
    int qp_thresh = 15 - X264_MIN( a, b ) - X264_MAX( 0, h->pps->i_chroma_qp_index_offset );
    int stridey   = h->fdec->i_stride[0];
    int strideuv  = h->fdec->i_stride[1];
    int chroma444 = CHROMA444;
    int chroma_height = 16 >> CHROMA_V_SHIFT;
    intptr_t uvdiff = chroma444 ? h->fdec->plane[2] - h->fdec->plane[1] : 1;

    for( int mb_x = 0; mb_x < h->mb.i_mb_width; mb_x += (~b_interlaced | mb_y)&1, mb_y ^= b_interlaced )
    {
        x264_prefetch_fenc( h, h->fdec, mb_x, mb_y );
        x264_macroblock_cache_load_neighbours_deblock( h, mb_x, mb_y );

        int mb_xy = h->mb.i_mb_xy;
        int transform_8x8 = h->mb.mb_transform_size[h->mb.i_mb_xy];
        int intra_cur = IS_INTRA( h->mb.type[mb_xy] );
        uint8_t (*bs)[8][4] = h->deblock_strength[mb_y&1][h->param.b_sliced_threads?mb_xy:mb_x];

        pixel *pixy = h->fdec->plane[0] + 16*mb_y*stridey  + 16*mb_x;
        pixel *pixuv = h->fdec->plane[1] + chroma_height*mb_y*strideuv + 16*mb_x;

        if( mb_y & MB_INTERLACED )
        {
            pixy -= 15*stridey;
            pixuv -= (chroma_height-1)*strideuv;
        }

        int stride2y  = stridey << MB_INTERLACED;
        int stride2uv = strideuv << MB_INTERLACED;
        int qp = h->mb.qp[mb_xy];
        int qpc = h->chroma_qp_table[qp];
        int first_edge_only = (h->mb.partition[mb_xy] == D_16x16 && !h->mb.cbp[mb_xy] && !intra_cur) || qp <= qp_thresh;

        #define FILTER( intra, dir, edge, qp, chroma_qp )\
        do\
        {\
            if( !(edge & 1) || !transform_8x8 )\
            {\
                deblock_edge##intra( h, pixy + 4*edge*(dir?stride2y:1),\
                                     stride2y, bs[dir][edge], qp, a, b, 0,\
                                     h->loopf.deblock_luma##intra[dir] );\
                if( CHROMA_FORMAT == CHROMA_444 )\
                {\
                    deblock_edge##intra( h, pixuv          + 4*edge*(dir?stride2uv:1),\
                                         stride2uv, bs[dir][edge], chroma_qp, a, b, 0,\
                                         h->loopf.deblock_luma##intra[dir] );\
                    deblock_edge##intra( h, pixuv + uvdiff + 4*edge*(dir?stride2uv:1),\
                                         stride2uv, bs[dir][edge], chroma_qp, a, b, 0,\
                                         h->loopf.deblock_luma##intra[dir] );\
                }\
                else if( CHROMA_FORMAT == CHROMA_420 && !(edge & 1) )\
                {\
                    deblock_edge##intra( h, pixuv + edge*(dir?2*stride2uv:4),\
                                         stride2uv, bs[dir][edge], chroma_qp, a, b, 1,\
                                         h->loopf.deblock_chroma##intra[dir] );\
                }\
            }\
            if( CHROMA_FORMAT == CHROMA_422 && (dir || !(edge & 1)) )\
            {\
                deblock_edge##intra( h, pixuv + edge*(dir?4*stride2uv:4),\
                                     stride2uv, bs[dir][edge], chroma_qp, a, b, 1,\
                                     h->loopf.deblock_chroma##intra[dir] );\
            }\
        } while(0)

        if( h->mb.i_neighbour & MB_LEFT )
        {
            if( b_interlaced && h->mb.field[h->mb.i_mb_left_xy[0]] != MB_INTERLACED )
            {
                int luma_qp[2];
                int chroma_qp[2];
                int left_qp[2];
                x264_deblock_inter_t luma_deblock = h->loopf.deblock_luma_mbaff;
                x264_deblock_inter_t chroma_deblock = h->loopf.deblock_chroma_mbaff;
                x264_deblock_intra_t luma_intra_deblock = h->loopf.deblock_luma_intra_mbaff;
                x264_deblock_intra_t chroma_intra_deblock = h->loopf.deblock_chroma_intra_mbaff;
                int c = chroma444 ? 0 : 1;

                left_qp[0] = h->mb.qp[h->mb.i_mb_left_xy[0]];
                luma_qp[0] = (qp + left_qp[0] + 1) >> 1;
                chroma_qp[0] = (qpc + h->chroma_qp_table[left_qp[0]] + 1) >> 1;
                if( intra_cur || IS_INTRA( h->mb.type[h->mb.i_mb_left_xy[0]] ) )
                {
                    deblock_edge_intra( h, pixy,           2*stridey,  bs[0][0], luma_qp[0],   a, b, 0, luma_intra_deblock );
                    deblock_edge_intra( h, pixuv,          2*strideuv, bs[0][0], chroma_qp[0], a, b, c, chroma_intra_deblock );
                    if( chroma444 )
                        deblock_edge_intra( h, pixuv + uvdiff, 2*strideuv, bs[0][0], chroma_qp[0], a, b, c, chroma_intra_deblock );
                }
                else
                {
                    deblock_edge( h, pixy,           2*stridey,  bs[0][0], luma_qp[0],   a, b, 0, luma_deblock );
                    deblock_edge( h, pixuv,          2*strideuv, bs[0][0], chroma_qp[0], a, b, c, chroma_deblock );
                    if( chroma444 )
                        deblock_edge( h, pixuv + uvdiff, 2*strideuv, bs[0][0], chroma_qp[0], a, b, c, chroma_deblock );
                }

                int offy = MB_INTERLACED ? 4 : 0;
                int offuv = MB_INTERLACED ? 4-CHROMA_V_SHIFT : 0;
                left_qp[1] = h->mb.qp[h->mb.i_mb_left_xy[1]];
                luma_qp[1] = (qp + left_qp[1] + 1) >> 1;
                chroma_qp[1] = (qpc + h->chroma_qp_table[left_qp[1]] + 1) >> 1;
                if( intra_cur || IS_INTRA( h->mb.type[h->mb.i_mb_left_xy[1]] ) )
                {
                    deblock_edge_intra( h, pixy           + (stridey<<offy),   2*stridey,  bs[0][4], luma_qp[1],   a, b, 0, luma_intra_deblock );
                    deblock_edge_intra( h, pixuv          + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], a, b, c, chroma_intra_deblock );
                    if( chroma444 )
                        deblock_edge_intra( h, pixuv + uvdiff + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], a, b, c, chroma_intra_deblock );
                }
                else
                {
                    deblock_edge( h, pixy           + (stridey<<offy),   2*stridey,  bs[0][4], luma_qp[1],   a, b, 0, luma_deblock );
                    deblock_edge( h, pixuv          + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], a, b, c, chroma_deblock );
                    if( chroma444 )
                        deblock_edge( h, pixuv + uvdiff + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], a, b, c, chroma_deblock );
                }
            }
            else
            {
                int qpl = h->mb.qp[h->mb.i_mb_xy-1];
                int qp_left = (qp + qpl + 1) >> 1;
                int qpc_left = (qpc + h->chroma_qp_table[qpl] + 1) >> 1;
                int intra_left = IS_INTRA( h->mb.type[h->mb.i_mb_xy-1] );

                if( intra_cur || intra_left )
                    FILTER( _intra, 0, 0, qp_left, qpc_left );
                else
                    FILTER(       , 0, 0, qp_left, qpc_left );
            }
        }
        if( !first_edge_only )
        {
            FILTER( , 0, 1, qp, qpc );
            FILTER( , 0, 2, qp, qpc );
            FILTER( , 0, 3, qp, qpc );
        }

        if( h->mb.i_neighbour & MB_TOP )
        {
            if( b_interlaced && !(mb_y&1) && !MB_INTERLACED && h->mb.field[h->mb.i_mb_top_xy] )
            {
                int mbn_xy = mb_xy - 2 * h->mb.i_mb_stride;

                for( int j = 0; j < 2; j++, mbn_xy += h->mb.i_mb_stride )
                {
                    int qpt = h->mb.qp[mbn_xy];
                    int qp_top = (qp + qpt + 1) >> 1;
                    int qpc_top = (qpc + h->chroma_qp_table[qpt] + 1) >> 1;
                    int intra_top = IS_INTRA( h->mb.type[mbn_xy] );
                    if( intra_cur || intra_top )
                        M32( bs[1][4*j] ) = 0x03030303;

                    // deblock the first horizontal edge of the even rows, then the first horizontal edge of the odd rows
                    deblock_edge( h, pixy      + j*stridey,  2* stridey, bs[1][4*j], qp_top, a, b, 0, h->loopf.deblock_luma[1] );
                    if( chroma444 )
                    {
                        deblock_edge( h, pixuv          + j*strideuv, 2*strideuv, bs[1][4*j], qpc_top, a, b, 0, h->loopf.deblock_luma[1] );
                        deblock_edge( h, pixuv + uvdiff + j*strideuv, 2*strideuv, bs[1][4*j], qpc_top, a, b, 0, h->loopf.deblock_luma[1] );
                    }
                    else
                        deblock_edge( h, pixuv          + j*strideuv, 2*strideuv, bs[1][4*j], qpc_top, a, b, 1, h->loopf.deblock_chroma[1] );
                }
            }
            else
            {
                int qpt = h->mb.qp[h->mb.i_mb_top_xy];
                int qp_top = (qp + qpt + 1) >> 1;
                int qpc_top = (qpc + h->chroma_qp_table[qpt] + 1) >> 1;
                int intra_top = IS_INTRA( h->mb.type[h->mb.i_mb_top_xy] );

                if( (!b_interlaced || (!MB_INTERLACED && !h->mb.field[h->mb.i_mb_top_xy]))
                    && (intra_cur || intra_top) )
                {
                    FILTER( _intra, 1, 0, qp_top, qpc_top );
                }
                else
                {
                    if( intra_cur || intra_top )
                        M32( bs[1][0] ) = 0x03030303;
                    FILTER(       , 1, 0, qp_top, qpc_top );
                }
            }
        }

        if( !first_edge_only )
        {
            FILTER( , 1, 1, qp, qpc );
            FILTER( , 1, 2, qp, qpc );
            FILTER( , 1, 3, qp, qpc );
        }

        #undef FILTER
    }
}

/* For deblock-aware RD.
 * TODO:
 *  deblock macroblock edges
 *  support analysis partitions smaller than 16x16
 *  deblock chroma for 4:2:0/4:2:2
 *  handle duplicate refs correctly
 *  handle cavlc+8x8dct correctly
 */
void x264_macroblock_deblock( x264_t *h )
{
    int a = h->sh.i_alpha_c0_offset - QP_BD_OFFSET;
    int b = h->sh.i_beta_offset - QP_BD_OFFSET;
    int qp_thresh = 15 - X264_MIN( a, b ) - X264_MAX( 0, h->pps->i_chroma_qp_index_offset );
    int intra_cur = IS_INTRA( h->mb.i_type );
    int qp = h->mb.i_qp;
    int qpc = h->mb.i_chroma_qp;
    if( (h->mb.i_partition == D_16x16 && !h->mb.i_cbp_luma && !intra_cur) || qp <= qp_thresh )
        return;

    uint8_t (*bs)[8][4] = h->mb.cache.deblock_strength;
    if( intra_cur )
    {
        memset( &bs[0][1], 3, 3*4*sizeof(uint8_t) );
        memset( &bs[1][1], 3, 3*4*sizeof(uint8_t) );
    }
    else
        h->loopf.deblock_strength( h->mb.cache.non_zero_count, h->mb.cache.ref, h->mb.cache.mv,
                                   bs, 4 >> MB_INTERLACED, h->sh.i_type == SLICE_TYPE_B );

    int transform_8x8 = h->mb.b_transform_8x8;

    #define FILTER( dir, edge )\
    do\
    {\
        deblock_edge( h, h->mb.pic.p_fdec[0] + 4*edge*(dir?FDEC_STRIDE:1),\
                      FDEC_STRIDE, bs[dir][edge], qp, a, b, 0,\
                      h->loopf.deblock_luma[dir] );\
        if( CHROMA444 )\
        {\
            deblock_edge( h, h->mb.pic.p_fdec[1] + 4*edge*(dir?FDEC_STRIDE:1),\
                          FDEC_STRIDE, bs[dir][edge], qpc, a, b, 0,\
                          h->loopf.deblock_luma[dir] );\
            deblock_edge( h, h->mb.pic.p_fdec[2] + 4*edge*(dir?FDEC_STRIDE:1),\
                          FDEC_STRIDE, bs[dir][edge], qpc, a, b, 0,\
                          h->loopf.deblock_luma[dir] );\
        }\
    } while(0)

    if( !transform_8x8 ) FILTER( 0, 1 );
                         FILTER( 0, 2 );
    if( !transform_8x8 ) FILTER( 0, 3 );

    if( !transform_8x8 ) FILTER( 1, 1 );
                         FILTER( 1, 2 );
    if( !transform_8x8 ) FILTER( 1, 3 );

    #undef FILTER
}

#if HAVE_MMX
EXTERNFLAG void x264_deblock_v_luma_sse2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_v_luma_avx ( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_luma_sse2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_luma_avx ( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_v_chroma_sse2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_v_chroma_avx ( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_sse2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_avx ( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_mbaff_sse2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_mbaff_avx ( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_422_mmx2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_422_sse2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_422_avx ( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_v_luma_intra_sse2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_v_luma_intra_avx ( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_luma_intra_sse2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_luma_intra_avx ( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_v_chroma_intra_sse2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_v_chroma_intra_avx ( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_intra_sse2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_intra_avx ( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_422_intra_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_422_intra_sse2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_422_intra_avx ( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_strength_mmx2 ( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                  int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                  int mvy_limit, int bframe );
EXTERNFLAG void x264_deblock_strength_sse2 ( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                  int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                  int mvy_limit, int bframe );
EXTERNFLAG void x264_deblock_strength_ssse3( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                  int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                  int mvy_limit, int bframe );
EXTERNFLAG void x264_deblock_strength_avx  ( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                  int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                  int mvy_limit, int bframe );

EXTERNFLAG void x264_deblock_h_chroma_intra_mbaff_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_intra_mbaff_sse2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_intra_mbaff_avx ( pixel *pix, intptr_t stride, int alpha, int beta );
#if ARCH_X86
EXTERNFLAG void x264_deblock_h_luma_mmx2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_v8_luma_mmx2( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_v_chroma_mmx2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_mmx2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_chroma_mbaff_mmx2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
EXTERNFLAG void x264_deblock_h_luma_intra_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_v8_luma_intra_mmx2( uint8_t *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_v_chroma_intra_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_intra_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );
EXTERNFLAG void x264_deblock_h_chroma_intra_mbaff_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );

#if HIGH_BIT_DEPTH
void x264_deblock_v_luma_mmx2( pixel *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_luma_intra_mmx2( pixel *pix, intptr_t stride, int alpha, int beta );
#else
// FIXME this wrapper has a significant cpu cost
static void x264_deblock_v_luma_mmx2( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 )
{
    x264_deblock_v8_luma_mmx2( pix,   stride, alpha, beta, tc0   );
    x264_deblock_v8_luma_mmx2( pix+8, stride, alpha, beta, tc0+2 );
}
static void x264_deblock_v_luma_intra_mmx2( uint8_t *pix, intptr_t stride, int alpha, int beta )
{
    x264_deblock_v8_luma_intra_mmx2( pix,   stride, alpha, beta );
    x264_deblock_v8_luma_intra_mmx2( pix+8, stride, alpha, beta );
}
#endif // HIGH_BIT_DEPTH
#endif
#endif

#if ARCH_PPC
void x264_deblock_v_luma_altivec( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_altivec( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
#endif // ARCH_PPC

#if HAVE_ARMV6
void x264_deblock_v_luma_neon  ( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_neon  ( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_neon( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_neon( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );

//added by sensong 201305
static void deblock_v_luma_intra_neon( pixel *pix, intptr_t stride, int alpha, int beta )
{
    asm volatile(
                 "sub           %0,%0,%1,lsl #2  \n" //pix = pix -4*stride;
                
                 "vld1.8        d0,[%0],%1      \n" //d0 p3
                 "vld1.8        d1,[%0],%1      \n" //d1 p2
                 "vld1.8        d2,[%0],%1      \n" //d2 p1
                 "vld1.8        d3,[%0],%1      \n" //d3 p0
                 "vld1.8        d4,[%0],%1      \n" //d4 q0
                 "vld1.8        d5,[%0],%1      \n" //d5 q1
                 "vld1.8        d6,[%0],%1      \n" //d6 q2
                 "vld1.8        d7,[%0],%1      \n" //d7 q3
                 
                 "vaddl.u8      q4,d3,d4        \n" // q4: A p0+q0
                 "vaddl.u8      q5,d1,d2        \n" // q5: B p2+p1
                 "vaddl.u8      q6,d0,d1        \n" // q6: C p3+p2
                 "vaddl.u8      q7,d5,d6        \n" // q7: D q1+q2
                 "vdup.u8      d31,%2           \n" //d31 alpha
                 "vaddl.u8      q8,d6,d7        \n" // q8: E q2+q3
                 "vaddl.u8      q9,d2,d5        \n" // q9: F p1+q1
                 "vaddl.u8     q10,d2,d3        \n" //q10: G p1+p0
                 "vaddl.u8     q11,d4,d5        \n" //q11: H q0+q1
                 
                 "vdup.u8     d30, %3           \n" //d30 beta
                 "vadd.u16     q5, q5, q4       \n" // q5:A+B
                 "vadd.u16     q7, q7, q4       \n" // q7:A+D
                 "vadd.u16    q10,q10, q9       \n" //q10:G+F
                 "vshr.u8     d29,d31,2         \n" // d29: (alpha>>2)
                 "vadd.u16    q11,q11, q9       \n" //q11:H+F
                 "vadd.u16     q4, q4, q9       \n" // q4:A+F
                 "vadd.u16     q6, q6, q6       \n" // q6:2C
                 "vadd.u16     q8, q8, q8       \n" // q8:2E
                 "vmov.u8     d28,#2            \n" // d28:2

                 "vadd.u16     q9, q4, q5       \n" // q9:A+B+A+F
                 "vadd.u16     q6, q6, q5       \n" // q6:A+B+2C
                 "vadd.u16     q8, q8, q7       \n" // q8:A+D+2E
                 "vadd.u16     q4, q4, q7       \n" // q4:A+F+A+D
                 "vadd.u8     d29,d29,d28       \n" //

                 "vrshrn.u16    d10, q5,#2       \n" //d10:p1'   ( p2 + p1 + p0 + q0 + 2 ) >> 2
                 "vrshrn.u16    d12, q6,#3       \n" //d12:p2'   ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3
                 "vrshrn.u16    d14, q7,#2       \n" //d14:q1'   ( p0 + q0 + q1 + q2 + 2 ) >> 2
                 "vrshrn.u16    d16, q8,#3       \n" //d16:q2'   ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                 "vrshrn.u16    d18, q9,#3       \n" //d18:p0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                 "vrshrn.u16    d20,q10,#2       \n" //d20:p0_F  ( 2*p1 + p0 + q1 + 2 ) >> 2;
                 "vrshrn.u16    d22,q11,#2       \n" //d22:q0_F  ( 2*q1 + q0 + p1 + 2 ) >> 2;
                 "vrshrn.u16     d8, q4,#3       \n" // d8:q0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3
                 
                 "vabd.u8      d11,d3,d4        \n"//d11:abs(p0-q0)
                 "vabd.u8      d17,d1,d3        \n"//d17:abs(p2-p0)
                 "vabd.u8      d19,d6,d4        \n"//d19:abs(q2-q0)
                 "vabd.u8      d13,d2,d3        \n"//d13:abs(p1-p0)
                 "vabd.u8      d15,d5,d4        \n"//d15:abs(q1-q0)
                 "sub           %0,%0,%1,lsl #3 \n" //pix = pix -4*stride;

                 
                 "vclt.u8      d28,d11,d29      \n"//d28:abs(p0-q0)<((alpha>>2)+2)  cond1
                 "vclt.u8      d17,d17,d30      \n"//d17:abs(p2-p0)<beta            cond2
                 "vclt.u8      d19,d19,d30      \n"//d19:abs(q2-q0)<beta            cond3
                 
                 "add          r12,%0,#8        \n"//r12->pix+8

                 
                 "vclt.u8      d11,d11,d31      \n"//d11:abs(p0-q0)<alpha
                 "vclt.u8      d13,d13,d30      \n"//d13:abs(p1-p0)<beta
                 "vclt.u8      d15,d15,d30      \n"//d15:abs(q1-q0)<beta
                 "add          %0,%0,%1         \n"//%0-> pix-3*stride
                 
                 "vand.u8      d17,d17,d28      \n"//d17:cond1&&cond2
                 "vand.u8      d11,d11,d13      \n"
                 "vand.u8      d19,d19,d28      \n"//d19:cond1&&cond3
                 "vand.u8      d11,d11,d15      \n"//d11:cond0
                 
                 
                 "vbit.u8      d20,d18,d17      \n" // if(cond1&&cond2) {B+2A+F;}else {G+F}
                 "vbit.u8      d22, d8,d19      \n" // if(cond1&&cond3) {F+2A+D;}else {H+F}
                 
                 "vand.u8      d17,d17,d11      \n" // cond0 && cond1 && cond2
                 "vand.u8      d19,d19,d11      \n" // cond0 && cond1 && cond3
                 
                 "vbit.u8      d3,d20,d11       \n" //p0
                 "vbit.u8      d4,d22,d11       \n" //q0
                 "vbit.u8      d2,d10,d17       \n" //p1
                 "vbit.u8      d1,d12,d17       \n" //p2
                 "vbit.u8      d5,d14,d19       \n" //q1
                 "vbit.u8      d6,d16,d19       \n" //q2

                 "vst1.8        d1,[%0],%1      \n" //d1 p2
                 "vst1.8        d2,[%0],%1      \n" //d2 p1
                 "vst1.8        d3,[%0],%1      \n" //d3 p0
                 "vst1.8        d4,[%0],%1      \n" //d4 q0
                 "vst1.8        d5,[%0],%1      \n" //d5 q1
                 "vst1.8        d6,[%0],%1      \n" //d6 q2

                 
                 // Next 8 bytes
                 
                 "vld1.8        d0,[r12],%1      \n" //d0 p3
                 "vld1.8        d1,[r12],%1      \n" //d1 p2
                 "vld1.8        d2,[r12],%1      \n" //d2 p1
                 "vld1.8        d3,[r12],%1      \n" //d3 p0
                 "vld1.8        d4,[r12],%1      \n" //d4 q0
                 "vld1.8        d5,[r12],%1      \n" //d5 q1
                 "vld1.8        d6,[r12],%1      \n" //d6 q2
                 "vld1.8        d7,[r12],%1      \n" //d7 q3
                 
                 "vaddl.u8      q4,d3,d4        \n" // q4: A p0+q0
                 "vaddl.u8      q5,d1,d2        \n" // q5: B p2+p1
                 "vaddl.u8      q6,d0,d1        \n" // q6: C p3+p2
                 "vaddl.u8      q7,d5,d6        \n" // q7: D q1+q2

                 "vaddl.u8      q8,d6,d7        \n" // q8: E q2+q3
                 "vaddl.u8      q9,d2,d5        \n" // q9: F p1+q1
                 "vaddl.u8     q10,d2,d3        \n" //q10: G p1+p0
                 "vaddl.u8     q11,d4,d5        \n" //q11: H q0+q1
                 

                 "vadd.u16     q5, q5, q4       \n" // q5:A+B
                 "vadd.u16     q7, q7, q4       \n" // q7:A+D
                 "vadd.u16    q10,q10, q9       \n" //q10:G+F

                 "vadd.u16    q11,q11, q9       \n" //q11:H+F
                 "vadd.u16     q4, q4, q9       \n" // q4:A+F
                 "vadd.u16     q6, q6, q6       \n" // q6:2C
                 "vadd.u16     q8, q8, q8       \n" // q8:2E

                 
                 "vadd.u16     q9, q4, q5       \n" // q9:A+B+A+F
                 "vadd.u16     q6, q6, q5       \n" // q6:A+B+2C
                 "vadd.u16     q8, q8, q7       \n" // q8:A+D+2E
                 "vadd.u16     q4, q4, q7       \n" // q4:A+F+A+D

                 
                 "vrshrn.u16    d10, q5,#2       \n" //d10:p1'   ( p2 + p1 + p0 + q0 + 2 ) >> 2
                 "vrshrn.u16    d12, q6,#3       \n" //d12:p2'   ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3
                 "vrshrn.u16    d14, q7,#2       \n" //d14:q1'   ( p0 + q0 + q1 + q2 + 2 ) >> 2
                 "vrshrn.u16    d16, q8,#3       \n" //d16:q2'   ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                 "vrshrn.u16    d18, q9,#3       \n" //d18:p0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                 "vrshrn.u16    d20,q10,#2       \n" //d20:p0_F  ( 2*p1 + p0 + q1 + 2 ) >> 2;
                 "vrshrn.u16    d22,q11,#2       \n" //d22:q0_F  ( 2*q1 + q0 + p1 + 2 ) >> 2;
                 "vrshrn.u16     d8, q4,#3       \n" // d8:q0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3
                 
                 "vabd.u8      d11, d3,d4        \n"//d11:abs(p0-q0)
                 "vabd.u8      d17, d1,d3        \n"//d17:abs(p2-p0)
                 "vabd.u8      d19, d6,d4        \n"//d19:abs(q2-q0)
                 "vabd.u8      d13, d2,d3        \n"//d13:abs(p1-p0)
                 "vabd.u8      d15, d5,d4        \n"//d15:abs(q1-q0)
                 "sub          r12,r12,%1,lsl #3 \n" //pix = pix -4*stride;
                 
                 
                 "vclt.u8      d28,d11,d29      \n"//d28:abs(p0-q0)<((alpha>>2)+2)  cond1
                 "vclt.u8      d17,d17,d30      \n"//d17:abs(p2-p0)<beta            cond2
                 "vclt.u8      d19,d19,d30      \n"//d19:abs(q2-q0)<beta            cond3
                 

                 
                 
                 "vclt.u8      d11,d11,d31      \n"//d11:abs(p0-q0)<alpha
                 "vclt.u8      d13,d13,d30      \n"//d13:abs(p1-p0)<beta
                 "vclt.u8      d15,d15,d30      \n"//d15:abs(q1-q0)<beta
                 "add          r12,r12,%1       \n"//%0-> pix-3*stride
                 
                 "vand.u8      d17,d17,d28      \n"//d17:cond1&&cond2
                 "vand.u8      d11,d11,d13      \n"
                 "vand.u8      d19,d19,d28      \n"//d19:cond1&&cond3
                 "vand.u8      d11,d11,d15      \n"//d11:cond0
                 
                 
                 "vbit.u8      d20,d18,d17      \n" // if(cond1&&cond2) {B+2A+F;}else {G+F}
                 "vbit.u8      d22, d8,d19      \n" // if(cond1&&cond3) {F+2A+D;}else {H+F}
                 
                 "vand.u8      d17,d17,d11      \n" // cond0 && cond1 && cond2
                 "vand.u8      d19,d19,d11      \n" // cond0 && cond1 && cond3
                 
                 "vbit.u8      d3,d20,d11       \n" //p0
                 "vbit.u8      d4,d22,d11       \n" //q0
                 "vbit.u8      d2,d10,d17       \n" //p1
                 "vbit.u8      d1,d12,d17       \n" //p2
                 "vbit.u8      d5,d14,d19       \n" //q1
                 "vbit.u8      d6,d16,d19       \n" //q2
                 
                 "vst1.8        d1,[r12],%1      \n" //d1 p2
                 "vst1.8        d2,[r12],%1      \n" //d2 p1
                 "vst1.8        d3,[r12],%1      \n" //d3 p0
                 "vst1.8        d4,[r12],%1      \n" //d4 q0
                 "vst1.8        d5,[r12],%1      \n" //d5 q1
                 "vst1.8        d6,[r12],%1      \n" //d6 q2

                 
                 :"+r"(pix),"+r"(stride),"+r"(alpha),"+r"(beta)::"r12","q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","q13","q14","q15","memory");
    
    
}
static void deblock_h_luma_intra_neon( pixel *pix, intptr_t stride, int alpha, int beta )
{
    asm volatile(
                 "sub           %0,%0,#4  \n" //pix = pix -4
                 
                 
                 "vld1.8        d0,[%0],%1      \n" //d0 line p3
                 "vld1.8        d1,[%0],%1      \n" //d1 line p2
                 "vld1.8        d2,[%0],%1      \n" //d2 line p1
                 "vld1.8        d3,[%0],%1      \n" //d3 line p0
                 "vld1.8        d4,[%0],%1      \n" //d4 line q0
                 "vld1.8        d5,[%0],%1      \n" //d5 line q1
                 "vld1.8        d6,[%0],%1      \n" //d6 line q2
                 "vld1.8        d7,[%0],%1      \n" //d7 line q3
                 
                 
                 "vtrn.u8 d0,d1                 \n" // d0 high< 16,06,14,04,12,02,10,00>low
                                                    // d1 high< 17,07,15,05,13,03,11,01>low
                 "vtrn.u8 d2,d3                 \n" // d2 high< 36,26,34,24,32,22,30,20>low
                                                    // d3 high< 37,27,35,25,33,23,31,21>low
                 "vtrn.u8 d4,d5                 \n" // d4 high< 56,46,54,44,52,42,50,40>low
                                                    // d5 high< 57,47,55,45,53,43,51,41>low
                 "vtrn.u8 d6,d7                 \n" // d6 high< 76,66,74,64,72,62,70,60>low
                                                    // d7 high< 77,67,75,65,73,63,71,61>low
                 
                 //-------------------------------------------------------------------------------
                 "vtrn.u16 q0,q1                \n" // q0 high< 35,25,15,05,31,21,11,01,34,24,14,04,30,20,10,00>low
                                                    // q1 high< 37,27,17,07,33,23,13,03,36,26,16,06,32,22,12,02>low
                 
                 "vtrn.u16 q2,q3                \n" // q2 high< 75,65,55,45,71,61,51,41,74,64,54,44,70,60,50,40>low
                                                    // q3 high< 77,67,57,47,73,63,53,53,76,66,56,46,72,62,52,42>low
                 //-------------------------------------------------------------------------------
                 "vtrn.u32 q0,q2                \n" // q0 high< 71,61,51,41,31,21,11,01,70,60,50,40,30,20,10,00>low
                                                    // q2 high< 75,65,55,45,35,25,15,05,74,64,54,44,34,24,14,04>low
                 "vtrn.u32 q1,q3                \n" // q1 high< 73,63,53,53,33,23,13,03,72,62,52,42,32,22,12,02>low
                                                    // q3 high< 77,67,57,47,37,27,17,07,76,66,56,46,36,26,16,06>low
                 
                 "vaddl.u8      q4,d3,d4        \n" // q4: A p0+q0
                 "vaddl.u8      q5,d1,d2        \n" // q5: B p2+p1
                 "vaddl.u8      q6,d0,d1        \n" // q6: C p3+p2
                 "vaddl.u8      q7,d5,d6        \n" // q7: D q1+q2
                 "vdup.u8      d31,%2           \n" //d31 alpha
                 "vaddl.u8      q8,d6,d7        \n" // q8: E q2+q3
                 "vaddl.u8      q9,d2,d5        \n" // q9: F p1+q1
                 "vaddl.u8     q10,d2,d3        \n" //q10: G p1+p0
                 "vaddl.u8     q11,d4,d5        \n" //q11: H q0+q1
                 
                 "vdup.u8     d30, %3           \n" //d30 beta
                 "vadd.u16     q5, q5, q4       \n" // q5:A+B
                 "vadd.u16     q7, q7, q4       \n" // q7:A+D
                 "vadd.u16    q10,q10, q9       \n" //q10:G+F
                 "vshr.u8     d29,d31,2         \n" // d29: (alpha>>2)
                 "vadd.u16    q11,q11, q9       \n" //q11:H+F
                 "vadd.u16     q4, q4, q9       \n" // q4:A+F
                 "vadd.u16     q6, q6, q6       \n" // q6:2C
                 "vadd.u16     q8, q8, q8       \n" // q8:2E
                 "vmov.u8     d28,#2            \n" // d28:2
                 
                 "vadd.u16     q9, q4, q5       \n" // q9:A+B+A+F
                 "vadd.u16     q6, q6, q5       \n" // q6:A+B+2C
                 "vadd.u16     q8, q8, q7       \n" // q8:A+D+2E
                 "vadd.u16     q4, q4, q7       \n" // q4:A+F+A+D
                 "vadd.u8     d29,d29,d28       \n" //
                 
                 "vrshrn.u16    d10, q5,#2       \n" //d10:p1'   ( p2 + p1 + p0 + q0 + 2 ) >> 2
                 "vrshrn.u16    d12, q6,#3       \n" //d12:p2'   ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3
                 "vrshrn.u16    d14, q7,#2       \n" //d14:q1'   ( p0 + q0 + q1 + q2 + 2 ) >> 2
                 "vrshrn.u16    d16, q8,#3       \n" //d16:q2'   ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                 "vrshrn.u16    d18, q9,#3       \n" //d18:p0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                 "vrshrn.u16    d20,q10,#2       \n" //d20:p0_F  ( 2*p1 + p0 + q1 + 2 ) >> 2;
                 "vrshrn.u16    d22,q11,#2       \n" //d22:q0_F  ( 2*q1 + q0 + p1 + 2 ) >> 2;
                 "vrshrn.u16     d8, q4,#3       \n" // d8:q0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3
                 
                 "vabd.u8      d11,d3,d4        \n"//d11:abs(p0-q0)
                 "vabd.u8      d17,d1,d3        \n"//d17:abs(p2-p0)
                 "vabd.u8      d19,d6,d4        \n"//d19:abs(q2-q0)
                 "vabd.u8      d13,d2,d3        \n"//d13:abs(p1-p0)
                 "vabd.u8      d15,d5,d4        \n"//d15:abs(q1-q0)
                 "sub           %0,%0,%1,lsl #3 \n" //pix = pix -4;
                 
                 
                 "vclt.u8      d28,d11,d29      \n"//d28:abs(p0-q0)<((alpha>>2)+2)  cond1
                 "vclt.u8      d17,d17,d30      \n"//d17:abs(p2-p0)<beta            cond2
                 "vclt.u8      d19,d19,d30      \n"//d19:abs(q2-q0)<beta            cond3
                 
                 
                 "vclt.u8      d11,d11,d31      \n"//d11:abs(p0-q0)<alpha
                 "vclt.u8      d13,d13,d30      \n"//d13:abs(p1-p0)<beta
                 "vclt.u8      d15,d15,d30      \n"//d15:abs(q1-q0)<beta

                 
                 "vand.u8      d17,d17,d28      \n"//d17:cond1&&cond2
                 "vand.u8      d11,d11,d13      \n"
                 "vand.u8      d19,d19,d28      \n"//d19:cond1&&cond3
                 "vand.u8      d11,d11,d15      \n"//d11:cond0
                 
                 
                 "vbit.u8      d20,d18,d17      \n" // if(cond1&&cond2) {B+2A+F;}else {G+F}
                 "vbit.u8      d22, d8,d19      \n" // if(cond1&&cond3) {F+2A+D;}else {H+F}
                 
                 "vand.u8      d17,d17,d11      \n" // cond0 && cond1 && cond2
                 "vand.u8      d19,d19,d11      \n" // cond0 && cond1 && cond3
                 
                 "vbit.u8      d3,d20,d11       \n" //p0
                 "vbit.u8      d4,d22,d11       \n" //q0
                 "vbit.u8      d2,d10,d17       \n" //p1
                 "vbit.u8      d1,d12,d17       \n" //p2
                 "vbit.u8      d5,d14,d19       \n" //q1
                 "vbit.u8      d6,d16,d19       \n" //q2

                 "vtrn.u8 d0,d1                 \n"
                 "vtrn.u8 d2,d3                 \n"
                 "vtrn.u8 d4,d5                 \n"
                 "vtrn.u8 d6,d7                 \n"
                 "vtrn.u16 q0,q1                \n"
                 "vtrn.u16 q2,q3                \n"
                 "vtrn.u32 q0,q2                \n"
                 "vtrn.u32 q1,q3                \n"
                 
                 "vst1.8        d0,[%0],%1      \n" //d0 line p3
                 "vst1.8        d1,[%0],%1      \n" //d1 line p2
                 "vst1.8        d2,[%0],%1      \n" //d2 line p1
                 "vst1.8        d3,[%0],%1      \n" //d3 line p0
                 "vst1.8        d4,[%0],%1      \n" //d4 line q0
                 "vst1.8        d5,[%0],%1      \n" //d5 line q1
                 "vst1.8        d6,[%0],%1      \n" //d6 line q2
                 "vst1.8        d7,[%0],%1      \n" //d7 line q3

                 
                 // Next 8 bytes
                 "vld1.8        d0,[%0],%1      \n" //d0 line p3
                 "vld1.8        d1,[%0],%1      \n" //d1 line p2
                 "vld1.8        d2,[%0],%1      \n" //d2 line p1
                 "vld1.8        d3,[%0],%1      \n" //d3 line p0
                 "vld1.8        d4,[%0],%1      \n" //d4 line q0
                 "vld1.8        d5,[%0],%1      \n" //d5 line q1
                 "vld1.8        d6,[%0],%1      \n" //d6 line q2
                 "vld1.8        d7,[%0],%1      \n" //d7 line q3
                 
                 
                 "vtrn.u8 d0,d1                 \n" // d0 high< 16,06,14,04,12,02,10,00>low
                 // d1 high< 17,07,15,05,13,03,11,01>low
                 "vtrn.u8 d2,d3                 \n" // d2 high< 36,26,34,24,32,22,30,20>low
                 // d3 high< 37,27,35,25,33,23,31,21>low
                 "vtrn.u8 d4,d5                 \n" // d4 high< 56,46,54,44,52,42,50,40>low
                 // d5 high< 57,47,55,45,53,43,51,41>low
                 "vtrn.u8 d6,d7                 \n" // d6 high< 76,66,74,64,72,62,70,60>low
                 // d7 high< 77,67,75,65,73,63,71,61>low
                 
                 //-------------------------------------------------------------------------------
                 "vtrn.u16 q0,q1                \n" // q0 high< 35,25,15,05,31,21,11,01,34,24,14,04,30,20,10,00>low
                 // q1 high< 37,27,17,07,33,23,13,03,36,26,16,06,32,22,12,02>low
                 
                 "vtrn.u16 q2,q3                \n" // q2 high< 75,65,55,45,71,61,51,41,74,64,54,44,70,60,50,40>low
                 // q3 high< 77,67,57,47,73,63,53,53,76,66,56,46,72,62,52,42>low
                 //-------------------------------------------------------------------------------
                 "vtrn.u32 q0,q2                \n" // q0 high< 71,61,51,41,31,21,11,01,70,60,50,40,30,20,10,00>low
                 // q2 high< 75,65,55,45,35,25,15,05,74,64,54,44,34,24,14,04>low
                 "vtrn.u32 q1,q3                \n" // q1 high< 73,63,53,53,33,23,13,03,72,62,52,42,32,22,12,02>low
                 // q3 high< 77,67,57,47,37,27,17,07,76,66,56,46,36,26,16,06>low
                 
                 "vaddl.u8      q4,d3,d4        \n" // q4: A p0+q0
                 "vaddl.u8      q5,d1,d2        \n" // q5: B p2+p1
                 "vaddl.u8      q6,d0,d1        \n" // q6: C p3+p2
                 "vaddl.u8      q7,d5,d6        \n" // q7: D q1+q2

                 "vaddl.u8      q8,d6,d7        \n" // q8: E q2+q3
                 "vaddl.u8      q9,d2,d5        \n" // q9: F p1+q1
                 "vaddl.u8     q10,d2,d3        \n" //q10: G p1+p0
                 "vaddl.u8     q11,d4,d5        \n" //q11: H q0+q1
                 

                 "vadd.u16     q5, q5, q4       \n" // q5:A+B
                 "vadd.u16     q7, q7, q4       \n" // q7:A+D
                 "vadd.u16    q10,q10, q9       \n" //q10:G+F

                 "vadd.u16    q11,q11, q9       \n" //q11:H+F
                 "vadd.u16     q4, q4, q9       \n" // q4:A+F
                 "vadd.u16     q6, q6, q6       \n" // q6:2C
                 "vadd.u16     q8, q8, q8       \n" // q8:2E

                 
                 "vadd.u16     q9, q4, q5       \n" // q9:A+B+A+F
                 "vadd.u16     q6, q6, q5       \n" // q6:A+B+2C
                 "vadd.u16     q8, q8, q7       \n" // q8:A+D+2E
                 "vadd.u16     q4, q4, q7       \n" // q4:A+F+A+D

                 
                 "vrshrn.u16    d10, q5,#2       \n" //d10:p1'   ( p2 + p1 + p0 + q0 + 2 ) >> 2
                 "vrshrn.u16    d12, q6,#3       \n" //d12:p2'   ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3
                 "vrshrn.u16    d14, q7,#2       \n" //d14:q1'   ( p0 + q0 + q1 + q2 + 2 ) >> 2
                 "vrshrn.u16    d16, q8,#3       \n" //d16:q2'   ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                 "vrshrn.u16    d18, q9,#3       \n" //d18:p0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                 "vrshrn.u16    d20,q10,#2       \n" //d20:p0_F  ( 2*p1 + p0 + q1 + 2 ) >> 2;
                 "vrshrn.u16    d22,q11,#2       \n" //d22:q0_F  ( 2*q1 + q0 + p1 + 2 ) >> 2;
                 "vrshrn.u16     d8, q4,#3       \n" // d8:q0_T  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3
                 
                 "vabd.u8      d11,d3,d4        \n"//d11:abs(p0-q0)
                 "vabd.u8      d17,d1,d3        \n"//d17:abs(p2-p0)
                 "vabd.u8      d19,d6,d4        \n"//d19:abs(q2-q0)
                 "vabd.u8      d13,d2,d3        \n"//d13:abs(p1-p0)
                 "vabd.u8      d15,d5,d4        \n"//d15:abs(q1-q0)
                 
                 "vclt.u8      d28,d11,d29      \n"//d28:abs(p0-q0)<((alpha>>2)+2)  cond1
                 "vclt.u8      d17,d17,d30      \n"//d17:abs(p2-p0)<beta            cond2
                 "vclt.u8      d19,d19,d30      \n"//d19:abs(q2-q0)<beta            cond3
                 
                 
                 "vclt.u8      d11,d11,d31      \n"//d11:abs(p0-q0)<alpha
                 "vclt.u8      d13,d13,d30      \n"//d13:abs(p1-p0)<beta
                 "vclt.u8      d15,d15,d30      \n"//d15:abs(q1-q0)<beta
                 
                 
                 "vand.u8      d17,d17,d28      \n"//d17:cond1&&cond2
                 "vand.u8      d11,d11,d13      \n"
                 "vand.u8      d19,d19,d28      \n"//d19:cond1&&cond3
                 "vand.u8      d11,d11,d15      \n"//d11:cond0
                 
                 
                 "vbit.u8      d20,d18,d17      \n" // if(cond1&&cond2) {B+2A+F;}else {G+F}
                 "vbit.u8      d22, d8,d19      \n" // if(cond1&&cond3) {F+2A+D;}else {H+F}
                 
                 "vand.u8      d17,d17,d11      \n" // cond0 && cond1 && cond2
                 "vand.u8      d19,d19,d11      \n" // cond0 && cond1 && cond3
                 
                 "vbit.u8      d3,d20,d11       \n" //p0
                 "vbit.u8      d4,d22,d11       \n" //q0
                 "vbit.u8      d2,d10,d17       \n" //p1
                 "vbit.u8      d1,d12,d17       \n" //p2
                 "vbit.u8      d5,d14,d19       \n" //q1
                 "vbit.u8      d6,d16,d19       \n" //q2
                 
                 "sub           %0,%0,%1,lsl #3 \n" //pix = pix -4;
                 
                 "vtrn.u8 d0,d1                 \n"
                 "vtrn.u8 d2,d3                 \n"
                 "vtrn.u8 d4,d5                 \n"
                 "vtrn.u8 d6,d7                 \n"
                 "vtrn.u16 q0,q1                \n"
                 "vtrn.u16 q2,q3                \n"
                 "vtrn.u32 q0,q2                \n"
                 "vtrn.u32 q1,q3                \n"
                 
                 "vst1.8        d0,[%0],%1      \n" //d0 line p3
                 "vst1.8        d1,[%0],%1      \n" //d1 line p2
                 "vst1.8        d2,[%0],%1      \n" //d2 line p1
                 "vst1.8        d3,[%0],%1      \n" //d3 line p0
                 "vst1.8        d4,[%0],%1      \n" //d4 line q0
                 "vst1.8        d5,[%0],%1      \n" //d5 line q1
                 "vst1.8        d6,[%0],%1      \n" //d6 line q2
                 "vst1.8        d7,[%0],%1      \n" //d7 line q3
                 
                 :"+r"(pix),"+r"(stride),"+r"(alpha),"+r"(beta)::"q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","q13","q14","q15","memory");
    

}


// Neon uv interlaced intra chroma deblocking, added by sensong 201305/
// u0 v0 u1 v1 u2 v2 u3 v3 u4 v4 u5 v5 u6 v6 u7 v7
static void deblock_v_chroma_intra_neon( pixel *pix, intptr_t stride, int alpha, int beta )
{
    asm volatile(
                 "sub           %0,%0,%1,lsl #1 \n" //pix = pix -2*stride;
                 "vld1.8        {q0},[%0],%1      \n" //q0 A1
                 "vld1.8        {q1},[%0],%1      \n" //q1 A0
                 "vld1.8        {q2},[%0],%1      \n" //q2 B0
                 "vld1.8        {q3},[%0]         \n" //q3 B1
                 
                 "vabd.u8       q4,q1,q2        \n" //q4 abs(A0-B0)
                 "vaddl.u8      q8,d0,d6        \n" //q8 (A1+B1) low
                 "vdup.u8       q15,%2          \n" //q15 alpha
                 "vaddl.u8      q9,d1,d7        \n" //q9 (A1+B1) high
                 
                 "vabd.u8       q5,q0,q1        \n" //q5 abs(A1-A0)
                 "vaddl.u8     q10,d0,d2        \n" //q10 (A1+A0) low
                 "vdup.u8       q14,%3          \n" //q14 beta
                 "vaddl.u8     q11,d1,d3        \n" //q11 (A1+A0) high
                 
                 "vabd.u8       q6,q3,q2        \n" //q6 abs(B1-B0)
                 "vaddl.u8     q12,d6,d4        \n" //q12 (B1 + B0) low
                 "vaddl.u8     q13,d7,d5        \n" //q13 (B1 + B0) high
                 "vclt.u8      q4,q4,q15        \n" //abs(A0-B0)<alpha
                 "vclt.u8      q5,q5,q14        \n" //abs(A1-A0)<beta
                 "vclt.u8      q6,q6,q14        \n" //abs(B1-B0)<beta
                 "vadd.u16     q10,q10,q8       \n" //2*A1+A0+B1
                 "vadd.u16     q11,q11,q9       \n"
                 "vand.u8      q4,q4,q5         \n" // q4 :abs(A0-B0)&&abs(A1-A0)
                 "vadd.u16     q12,q12,q8       \n" //2*B1+B0+A1
                 "vadd.u16     q13,q13,q9       \n"
                 "vand.u8      q4,q4,q6         \n" // q4 :abs(A0-B0)&&abs(A1-A0)&&abs(B1-B0)
                 "vrshrn.u16    d16,q10,#2       \n" //(2*p1+p0+q1+2)>>2 p0'
                 "vrshrn.u16    d17,q11,#2       \n"
                 "vrshrn.u16    d18,q12,#2       \n" //(2*q1+q0+p1+2)>>2 q0'
                 "vrshrn.u16    d19,q13,#2       \n"
                 "sub          %0,%0,%1,lsl #1  \n" // pix =pix-stride
                 "vbit.u8      q1,q8,q4         \n"
                 "vbit.u8      q2,q9,q4         \n"
                 
                 "vst1.8        {q1},[%0],%1      \n" //d0 high<B01,B00,A00,A01>low
                 "vst1.8        {q2},[%0]         \n" //d2 high<B11,B10,A10,A11>low

                 :"+r"(pix),"+r"(stride),"+r"(alpha),"+r"(beta)::"q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","q13","q14","q15","memory");   
}



// A1 A1 A0 A0 | B0 B0 B1 B1
// u0 v0 u1 v1 | u2 v2 u3 v3
static void deblock_h_chroma_intra_neon( pixel *pix, intptr_t stride, int alpha, int beta )
{
    asm volatile(
    "sub           %0,%0, #4       \n" //pix = pix -4;
    "vld1.8        d0,[%0],%1      \n" //d0 high<B01,B00,A00,A01>low
    "vld1.8        d2,[%0],%1      \n" //d2 high<B11,B10,A10,A11>low
    "vld1.8        d4,[%0],%1      \n" //d4 high<B21,B20,A20,A21>low
    "vld1.8        d6,[%0],%1      \n" //d6 high<B31,B30,A30,A31>low
    "vld1.8        d1,[%0],%1      \n" //d1 high<B41,B40,A40,A41>low
    "vld1.8        d3,[%0],%1      \n" //d3 high<B51,B50,A50,A51>low
    "vld1.8        d5,[%0],%1      \n" //d5 high<B61,B60,A60,A61>low
    "vld1.8        d7,[%0],%1      \n" //d7 high<B71,B70,A70,A71>low
    
    "vtrn.u16      q0,q1           \n" //q0 high<B50,B40,A51,A41,B10,B00,A11,A01>low
                                       //q1 high<B51,B41,A50,A40,B11,B01,A10,A00>low
    "vtrn.u16      q2,q3           \n" //q2 high<B70,B60,A71,A61,B30,B20,A31,A21>low
                                       //q3 high<B71,B61,A70,A60,B31,B21,A30,A20>low
                 
    "vtrn.u32      q0,q2           \n" //q0 high<A71,A61,A51,A41,A31,A21,A11,A01>low  A1
                                       //q2 high<B70,B60,B50,B40,B30,B20,B10,B00>low  B0
    "vtrn.u32      q1,q3           \n" //q1 high<A70,A60,A50,A40,A30,A20,A10,A00>low  A0
                                       //q3 high<B71,B61,B51,B41,B31,B21,B11,B01>low  B1

    "vabd.u8       q4,q1,q2        \n" //q4 abs(A0-B0)
    "vaddl.u8      q8,d0,d6        \n" //q8 (A1+B1) low
    "vdup.u8       q14,%3          \n" //q14 beta
    "vaddl.u8      q9,d1,d7        \n" //q9 (A1+B1) high
                 
    "vabd.u8       q5,q0,q1        \n" //q5 abs(A1-A0)
    "vaddl.u8     q10,d0,d2        \n" //q10 (A1+A0) low
    "vdup.u8       q15,%2          \n" //q15 alpha
    "vaddl.u8     q11,d1,d3        \n" //q11 (A1+A0) high
                 
    "vabd.u8       q6,q3,q2        \n" //q6 abs(B1-B0)
    "vaddl.u8     q12,d6,d4        \n" //q12 (B1 + B0) low
    "vaddl.u8     q13,d7,d5        \n" //q13 (B1 + B0) high
    "vclt.u8      q4,q4,q15        \n" //abs(A0-B0)<alpha
    "vclt.u8      q5,q5,q14        \n" //abs(A1-A0)<beta
    "vclt.u8      q6,q6,q14        \n" //abs(B1-B0)<beta
    "vadd.u16     q10,q10,q8       \n" //2*A1+A0+B1
    "vadd.u16     q11,q11,q9       \n"
    "vand.u8      q4,q4,q5         \n" // q4 :abs(A0-B0)&&abs(A1-A0)
    "vadd.u16     q12,q12,q8       \n" //2*B1+B0+A1
    "vadd.u16     q13,q13,q9       \n"
    "vand.u8      q4,q4,q6         \n" // q4 :abs(A0-B0)&&abs(A1-A0)&&abs(B1-B0)
    "vrshrn.u16    d16,q10,#2       \n" //(2*A1+A0+B1+2)>>2 A0'
    "vrshrn.u16    d17,q11,#2       \n"
    "vrshrn.u16    d18,q12,#2       \n" //(2*B1+B0+A1+2)>>2 B0'
    "vrshrn.u16    d19,q13,#2       \n"
    "sub          %0,%0,%1,lsl #3  \n"
    "vbit.u8      q1,q8,q4         \n"
    "vbit.u8      q2,q9,q4         \n"
                 
    "vtrn.u32     q0,q2            \n"
    "vtrn.u32     q1,q3            \n"
    "vtrn.u16     q0,q1            \n"
    "vtrn.u16     q2,q3            \n"
    
    "vst1.8        d0,[%0],%1      \n" //d0 high<B01,B00,A00,A01>low
    "vst1.8        d2,[%0],%1      \n" //d2 high<B11,B10,A10,A11>low
    "vst1.8        d4,[%0],%1      \n" //d4 high<B21,B20,A20,A21>low
    "vst1.8        d6,[%0],%1      \n" //d6 high<B31,B30,A30,A31>low
    "vst1.8        d1,[%0],%1      \n" //d1 high<B41,B40,A40,A41>low
    "vst1.8        d3,[%0],%1      \n" //d3 high<B51,B50,A50,A51>low
    "vst1.8        d5,[%0],%1      \n" //d5 high<B61,B60,A60,A61>low
    "vst1.8        d7,[%0]         \n" //d7 high<B71,B70,A70,A71>low
    
    :"+r"(pix),"+r"(stride),"+r"(alpha),"+r"(beta)::"q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","q13","q14","q15","memory");

}


//End





#endif


#if HAVE_ARMV6 || ARCH_AARCH64
void x264_deblock_v_luma_neon  ( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_neon  ( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_neon( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_neon( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_strength_neon( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                 int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                 int mvy_limit, int bframe );
#if ARCH_AARCH64
void x264_deblock_h_chroma_422_neon( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_mbaff_neon( uint8_t *pix, intptr_t stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_intra_mbaff_neon( uint8_t *pix, intptr_t stride, int alpha, int beta );
void x264_deblock_h_chroma_intra_neon( uint8_t *pix, intptr_t stride, int alpha, int beta );
void x264_deblock_h_chroma_422_intra_neon( uint8_t *pix, intptr_t stride, int alpha, int beta );
void x264_deblock_v_chroma_intra_neon( uint8_t *pix, intptr_t stride, int alpha, int beta );
void x264_deblock_h_luma_intra_neon( uint8_t *pix, intptr_t stride, int alpha, int beta );
void x264_deblock_v_luma_intra_neon( uint8_t *pix, intptr_t stride, int alpha, int beta );
#endif
#endif


void x264_deblock_init( int cpu, x264_deblock_function_t *pf, int b_mbaff )
{
    pf->deblock_luma[1] = deblock_v_luma_c;
    pf->deblock_luma[0] = deblock_h_luma_c;
    pf->deblock_chroma[1] = deblock_v_chroma_c;
    pf->deblock_h_chroma_420 = deblock_h_chroma_c;
    pf->deblock_h_chroma_422 = deblock_h_chroma_422_c;
    pf->deblock_luma_intra[1] = deblock_v_luma_intra_c;
    pf->deblock_luma_intra[0] = deblock_h_luma_intra_c;
    pf->deblock_chroma_intra[1] = deblock_v_chroma_intra_c;
    pf->deblock_h_chroma_420_intra = deblock_h_chroma_intra_c;
    pf->deblock_h_chroma_422_intra = deblock_h_chroma_422_intra_c;
    pf->deblock_luma_mbaff = deblock_h_luma_mbaff_c;
    pf->deblock_chroma_420_mbaff = deblock_h_chroma_mbaff_c;
    pf->deblock_luma_intra_mbaff = deblock_h_luma_intra_mbaff_c;
    pf->deblock_chroma_420_intra_mbaff = deblock_h_chroma_intra_mbaff_c;
    pf->deblock_strength = deblock_strength_c;

#if HAVE_MMX
    if( cpu&X264_CPU_MMX2 )
    {
#if ARCH_X86
        pf->deblock_luma[1] = x264_deblock_v_luma_mmx2;
        pf->deblock_luma[0] = x264_deblock_h_luma_mmx2;
        pf->deblock_chroma[1] = x264_deblock_v_chroma_mmx2;
        pf->deblock_h_chroma_420 = x264_deblock_h_chroma_mmx2;
        pf->deblock_chroma_420_mbaff = x264_deblock_h_chroma_mbaff_mmx2;
        pf->deblock_h_chroma_422 = x264_deblock_h_chroma_422_mmx2;
        pf->deblock_h_chroma_422_intra = x264_deblock_h_chroma_422_intra_mmx2;
        pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_mmx2;
        pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_mmx2;
        pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_mmx2;
        pf->deblock_h_chroma_420_intra = x264_deblock_h_chroma_intra_mmx2;
        pf->deblock_chroma_420_intra_mbaff = x264_deblock_h_chroma_intra_mbaff_mmx2;
#endif
#if !HIGH_BIT_DEPTH
        pf->deblock_chroma_420_intra_mbaff = x264_deblock_h_chroma_intra_mbaff_mmx2;
#endif
        pf->deblock_strength = x264_deblock_strength_mmx2;
        if( cpu&X264_CPU_SSE2 )
        {
            pf->deblock_strength = x264_deblock_strength_sse2;
            pf->deblock_h_chroma_420 = x264_deblock_h_chroma_sse2;
            pf->deblock_h_chroma_422 = x264_deblock_h_chroma_422_sse2;
            pf->deblock_h_chroma_422_intra = x264_deblock_h_chroma_422_intra_sse2;
            pf->deblock_chroma_420_mbaff = x264_deblock_h_chroma_mbaff_sse2;
            if( !(cpu&X264_CPU_STACK_MOD4) )
            {
                pf->deblock_luma[1] = x264_deblock_v_luma_sse2;
                pf->deblock_luma[0] = x264_deblock_h_luma_sse2;
                pf->deblock_chroma[1] = x264_deblock_v_chroma_sse2;
                pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_sse2;
                pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_sse2;
                pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_sse2;
                pf->deblock_h_chroma_420_intra = x264_deblock_h_chroma_intra_sse2;
#if HIGH_BIT_DEPTH
                pf->deblock_chroma_420_intra_mbaff= x264_deblock_h_chroma_intra_mbaff_sse2;
#endif
            }
        }
        if( cpu&X264_CPU_SSSE3 )
            pf->deblock_strength = x264_deblock_strength_ssse3;
        if( cpu&X264_CPU_AVX )
        {
            pf->deblock_strength = x264_deblock_strength_avx;
            pf->deblock_h_chroma_420 = x264_deblock_h_chroma_avx;
            pf->deblock_h_chroma_422 = x264_deblock_h_chroma_422_avx;
            pf->deblock_h_chroma_422_intra = x264_deblock_h_chroma_422_intra_avx;
            if( !(cpu&X264_CPU_STACK_MOD4) )
            {
                pf->deblock_luma[1] = x264_deblock_v_luma_avx;
                pf->deblock_luma[0] = x264_deblock_h_luma_avx;
                pf->deblock_chroma[1] = x264_deblock_v_chroma_avx;
                pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_avx;
                pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_avx;
                pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_avx;
                pf->deblock_h_chroma_420_intra = x264_deblock_h_chroma_intra_avx;
#if HIGH_BIT_DEPTH
                pf->deblock_chroma_420_mbaff = x264_deblock_h_chroma_mbaff_avx;
                pf->deblock_chroma_420_intra_mbaff = x264_deblock_h_chroma_intra_mbaff_avx;
#endif
            }
        }
    }
#endif

#if !HIGH_BIT_DEPTH
#if HAVE_ALTIVEC
    if( cpu&X264_CPU_ALTIVEC )
    {
        pf->deblock_luma[1] = x264_deblock_v_luma_altivec;
        pf->deblock_luma[0] = x264_deblock_h_luma_altivec;
   }
#endif // HAVE_ALTIVEC

#if HAVE_ARMV6
   if( cpu&X264_CPU_NEON )
   {
        pf->deblock_luma[1] = x264_deblock_v_luma_neon;
        pf->deblock_luma[0] = x264_deblock_h_luma_neon;
        pf->deblock_chroma[1] = x264_deblock_v_chroma_neon;
       pf->deblock_h_chroma_420 = x264_deblock_h_chroma_neon;
       // added by sensong 201305
       pf->deblock_h_chroma_420_intra = deblock_h_chroma_intra_neon;
       pf->deblock_chroma_intra[1] = deblock_v_chroma_intra_neon;
       pf->deblock_luma_intra[1] = deblock_v_luma_intra_neon;
       pf->deblock_luma_intra[0] = deblock_h_luma_intra_neon;
       // end
   }
#endif
#if HAVE_ARMV6 || ARCH_AARCH64
    if( cpu&X264_CPU_NEON )
    {
        pf->deblock_luma[1] = x264_deblock_v_luma_neon;
        pf->deblock_luma[0] = x264_deblock_h_luma_neon;
        pf->deblock_chroma[1] = x264_deblock_v_chroma_neon;
        pf->deblock_h_chroma_420 = x264_deblock_h_chroma_neon;
#if ARCH_AARCH64
        pf->deblock_chroma_420_mbaff = x264_deblock_h_chroma_mbaff_neon;
        pf->deblock_chroma_420_intra_mbaff = x264_deblock_h_chroma_intra_mbaff_neon;
        pf->deblock_h_chroma_420_intra = x264_deblock_h_chroma_intra_neon;
        pf->deblock_h_chroma_422 = x264_deblock_h_chroma_422_neon;
        pf->deblock_h_chroma_422_intra = x264_deblock_h_chroma_422_intra_neon;
        pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_neon;
        pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_neon;
        pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_neon;
#endif
        pf->deblock_strength     = x264_deblock_strength_neon;
    }
#endif


#endif // !HIGH_BIT_DEPTH

    /* These functions are equivalent, so don't duplicate them. */
    pf->deblock_chroma_422_mbaff = pf->deblock_h_chroma_420;
    pf->deblock_chroma_422_intra_mbaff = pf->deblock_h_chroma_420_intra;
}
