//
//  ios_predict-a.s
//  x264EncLib
//
//  Created by sensong on 13-5-16.
//  Copyright (c) 2013年 com.tencent.avteam. All rights reserved.
//

.macro function name
.global _\name
.align 2
_\name:
@ .hidden \name
@ .type \name, %function
@.func \name
@ \name:
.endm

constOne:
.int 0x01010101

.text
function x264_predict_4x4_h_armv6
ldrb	r1, [r0, #-1]
ldrb	r2, [r0, #31]
ldrb	r3, [r0, #63]
ldrb	ip, [r0, #95]
add	r1, r1, r1, lsl #8
add	r2, r2, r2, lsl #8
add	r3, r3, r3, lsl #8
add	ip, ip, ip, lsl #8
add	r1, r1, r1, lsl #16
str	r1, [r0]
add	r2, r2, r2, lsl #16
str	r2, [r0, #32]
add	r3, r3, r3, lsl #16
str	r3, [r0, #64]
add	ip, ip, ip, lsl #16
str	ip, [r0, #96]
bx	lr
function x264_predict_4x4_dc_armv6
mov	ip, #0	@ 0x0
ldr	r1, [r0, #-32]
ldrb	r2, [r0, #-1]
ldrb	r3, [r0, #31]
usad8	r1, r1, ip
add	r2, r2, #4	@ 0x4
ldrb	ip, [r0, #63]
add	r2, r2, r3
ldrb	r3, [r0, #95]
add	r2, r2, ip
add	r2, r2, r3
add	r1, r1, r2
lsr	r1, r1, #3
add	r1, r1, r1, lsl #8
add	r1, r1, r1, lsl #16
str	r1, [r0]
str	r1, [r0, #32]
str	r1, [r0, #64]
str	r1, [r0, #96]
bx	lr
function x264_predict_4x4_dc_top_neon
mov	ip, #32	@ 0x20
sub	r1, r0, #32	@ 0x20
vld1.32	{d1[]}, [r1, :32]
vpaddl.u8	d1, d1
vpadd.u16	d1, d1, d1
vrshr.u16	d1, d1, #2
vdup.8	d1, d1[0]
vst1.32	{d1[0]}, [r0, :32], ip
vst1.32	{d1[0]}, [r0, :32], ip
vst1.32	{d1[0]}, [r0, :32], ip
vst1.32	{d1[0]}, [r0, :32], ip
bx	lr
function x264_predict_4x4_ddr_armv6
ldr	r1, [r0, #-32]
ldrb	r2, [r0, #-33]
ldrb	r3, [r0, #-1]
push	{r4, r5, r6, lr}
add	r2, r2, r1, lsl #8
ldrb	r4, [r0, #31]
add	r3, r3, r2, lsl #8
ldrb	r5, [r0, #63]
ldrb	r6, [r0, #95]
add	r4, r4, r3, lsl #8
add	r5, r5, r4, lsl #8
add	r6, r6, r5, lsl #8
ldr	ip, constOne	@ 0x01010101
uhadd8	r1, r1, r3
uhadd8	r4, r4, r6
uhadd8	r3, r1, r2
uhadd8	r6, r4, r5
eor	r1, r1, r2
eor	r4, r4, r5
and	r1, r1, ip
and	r4, r4, ip
uadd8	r1, r1, r3
uadd8	r4, r4, r6
str	r1, [r0]
lsl	r2, r1, #8
lsl	r3, r1, #16
lsl	r4, r4, #8
lsl	r5, r1, #24
add	r2, r2, r4, lsr #24
str	r2, [r0, #32]
add	r3, r3, r4, lsr #16
str	r3, [r0, #64]
add	r5, r5, r4, lsr #8
str	r5, [r0, #96]
pop	{r4, r5, r6, pc}
function x264_predict_4x4_ddl_neon
sub	r0, r0, #32	@ 0x20
mov	ip, #32	@ 0x20
vld1.64	{d0}, [r0], ip
vdup.8	d3, d0[7]
vext.8	d1, d0, d0, #1
vext.8	d2, d0, d3, #2
vhadd.u8	d0, d0, d2
vrhadd.u8	d0, d0, d1
vst1.32	{d0[0]}, [r0, :32], ip
vext.8	d1, d0, d0, #1
vext.8	d2, d0, d0, #2
vst1.32	{d1[0]}, [r0, :32], ip
vext.8	d3, d0, d0, #3
vst1.32	{d2[0]}, [r0, :32], ip
vst1.32	{d3[0]}, [r0, :32], ip
bx	lr
function x264_predict_8x8_dc_neon
mov	ip, #0	@ 0x0
ldrd	r2,r3, [r1, #8]
push	{r4, r5, lr}
ldrd	r4,r5, [r1, #16]
lsl	r3, r3, #8
ldrb	lr, [r1, #7]
usad8	r2, r2, ip
usad8	r3, r3, ip
usada8	r2, r4, ip, r2
add	lr, lr, #8	@ 0x8
usada8	r3, r5, ip, r3
add	r2, r2, lr
mov	ip, #32	@ 0x20
add	r2, r2, r3
lsr	r2, r2, #4
vdup.8	d0, r2
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
pop	{r4, r5, pc}
function x264_predict_8x8_h_neon
add	r1, r1, #7	@ 0x7
mov	ip, #32	@ 0x20
vld1.64	{d16}, [r1]
vdup.8	d0, d16[7]
vdup.8	d1, d16[6]
vst1.64	{d0}, [r0, :64], ip
vdup.8	d2, d16[5]
vst1.64	{d1}, [r0, :64], ip
vdup.8	d3, d16[4]
vst1.64	{d2}, [r0, :64], ip
vdup.8	d4, d16[3]
vst1.64	{d3}, [r0, :64], ip
vdup.8	d5, d16[2]
vst1.64	{d4}, [r0, :64], ip
vdup.8	d6, d16[1]
vst1.64	{d5}, [r0, :64], ip
vdup.8	d7, d16[0]
vst1.64	{d6}, [r0, :64], ip
vst1.64	{d7}, [r0, :64], ip
bx	lr
function x264_predict_8x8_v_neon
add	r1, r1, #16	@ 0x10
mov	ip, #32	@ 0x20
vld1.8	{d0}, [r1, :64]
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d0}, [r0, :64], ip
bx	lr
function x264_predict_8x8_ddl_neon
add	r1, r1, #16	@ 0x10
vld1.8	{d0-d1}, [r1, :128]
vmov.i8	q3, #0	@ 0x00
vrev64.8	d2, d1
vext.8	q8, q3, q0, #15
vext.8	q2, q0, q1, #1
vhadd.u8	q8, q8, q2
mov	ip, #32	@ 0x20
vrhadd.u8	q0, q0, q8
vext.8	d2, d0, d1, #1
vext.8	d3, d0, d1, #2
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #3
vst1.8	{d3}, [r0, :64], ip
vext.8	d3, d0, d1, #4
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #5
vst1.8	{d3}, [r0, :64], ip
vext.8	d3, d0, d1, #6
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #7
vst1.8	{d3}, [r0, :64], ip
vst1.8	{d2}, [r0, :64], ip
vst1.8	{d1}, [r0, :64], ip
bx	lr
function x264_predict_8x8_ddr_neon
vld1.8	{d0-d3}, [r1, :128]
vext.8	q2, q0, q1, #7
vext.8	q3, q0, q1, #9
vhadd.u8	q2, q2, q3
vrhadd.u8	d0, d1, d4
vrhadd.u8	d1, d2, d5
add	r0, r0, #224	@ 0xe0
mvn	ip, #31	@ 0x1f
vext.8	d2, d0, d1, #1
vst1.8	{d0}, [r0, :64], ip
vext.8	d4, d0, d1, #2
vst1.8	{d2}, [r0, :64], ip
vext.8	d5, d0, d1, #3
vst1.8	{d4}, [r0, :64], ip
vext.8	d4, d0, d1, #4
vst1.8	{d5}, [r0, :64], ip
vext.8	d5, d0, d1, #5
vst1.8	{d4}, [r0, :64], ip
vext.8	d4, d0, d1, #6
vst1.8	{d5}, [r0, :64], ip
vext.8	d5, d0, d1, #7
vst1.8	{d4}, [r0, :64], ip
vst1.8	{d5}, [r0, :64], ip
bx	lr
function x264_predict_8x8_vl_neon
add	r1, r1, #16	@ 0x10
mov	ip, #32	@ 0x20
vld1.8	{d0-d1}, [r1, :128]
vext.8	q1, q1, q0, #15
vext.8	q2, q0, q2, #1
vrhadd.u8	q3, q0, q2
vhadd.u8	q1, q1, q2
vrhadd.u8	q0, q0, q1
vext.8	d2, d0, d1, #1
vst1.8	{d6}, [r0, :64], ip
vext.8	d3, d6, d7, #1
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #2
vst1.8	{d3}, [r0, :64], ip
vext.8	d3, d6, d7, #2
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #3
vst1.8	{d3}, [r0, :64], ip
vext.8	d3, d6, d7, #3
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #4
vst1.8	{d3}, [r0, :64], ip
vst1.8	{d2}, [r0, :64], ip
bx	lr
function x264_predict_8x8_vr_neon
add	r1, r1, #8	@ 0x8
mov	ip, #32	@ 0x20
vld1.8	{d4-d5}, [r1, :64]
vext.8	q1, q2, q2, #14
vext.8	q0, q2, q2, #15
vhadd.u8	q3, q2, q1
vrhadd.u8	q2, q2, q0
vrhadd.u8	q0, q0, q3
vorr	d2, d0, d0
vst1.8	{d5}, [r0, :64], ip
vuzp.8	d2, d0
vst1.8	{d1}, [r0, :64], ip
vext.8	d6, d0, d5, #7
vext.8	d3, d2, d1, #7
vst1.8	{d6}, [r0, :64], ip
vst1.8	{d3}, [r0, :64], ip
vext.8	d6, d0, d5, #6
vext.8	d3, d2, d1, #6
vst1.8	{d6}, [r0, :64], ip
vst1.8	{d3}, [r0, :64], ip
vext.8	d6, d0, d5, #5
vext.8	d3, d2, d1, #5
vst1.8	{d6}, [r0, :64], ip
vst1.8	{d3}, [r0, :64], ip
bx	lr
function x264_predict_8x8_hd_neon
mov	ip, #32	@ 0x20
add	r1, r1, #7	@ 0x7
vld1.8	{d2-d3}, [r1]
vext.8	q3, q1, q1, #1
vext.8	q2, q1, q1, #2
vrhadd.u8	q8, q1, q3
vhadd.u8	q1, q1, q2
vrhadd.u8	q0, q1, q3
vzip.8	d16, d0
vext.8	d2, d0, d1, #6
vext.8	d3, d0, d1, #4
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d0, d1, #2
vst1.8	{d3}, [r0, :64], ip
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d16, d0, #6
vst1.8	{d0}, [r0, :64], ip
vext.8	d3, d16, d0, #4
vst1.8	{d2}, [r0, :64], ip
vext.8	d2, d16, d0, #2
vst1.8	{d3}, [r0, :64], ip
vst1.8	{d2}, [r0, :64], ip
vst1.8	{d16}, [r0, :64], ip
bx	lr
function x264_predict_8x8_hu_neon
mov	ip, #32	@ 0x20
add	r1, r1, #7	@ 0x7
vld1.8	{d7}, [r1]
vdup.8	d6, d7[0]
vrev64.8	d7, d7
vext.8	d4, d7, d6, #2
vext.8	d2, d7, d6, #1
vhadd.u8	d16, d7, d4
vrhadd.u8	d0, d2, d7
vrhadd.u8	d1, d16, d2
vzip.8	d0, d1
vdup.16	q1, d1[3]
vext.8	q2, q0, q1, #2
vext.8	q3, q0, q1, #4
vext.8	q8, q0, q1, #6
vst1.8	{d0}, [r0, :64], ip
vst1.8	{d4}, [r0, :64], ip
vst1.8	{d6}, [r0, :64], ip
vst1.8	{d16}, [r0, :64], ip
vst1.8	{d1}, [r0, :64], ip
vst1.8	{d5}, [r0, :64], ip
vst1.8	{d7}, [r0, :64], ip
vst1.8	{d17}, [r0, :64]
bx	lr
function x264_predict_8x8c_dc_top_neon
sub	r2, r0, #32	@ 0x20
mov	r1, #32	@ 0x20
vld1.8	{d0}, [r2, :64]
vpaddl.u8	d0, d0
vpadd.i16	d0, d0, d0
vrshrn.i16	d0, q0, #2
vdup.8	d1, d0[1]
vdup.8	d0, d0[0]
vtrn.32	d0, d1
b	pred8x8_dc_end
function x264_predict_8x8c_dc_left_neon
mov	r1, #32	@ 0x20
sub	r2, r0, #1	@ 0x1
vld1.8	{d0[0]}, [r2], r1
vld1.8	{d0[1]}, [r2], r1
vld1.8	{d0[2]}, [r2], r1
vld1.8	{d0[3]}, [r2], r1
vld1.8	{d0[4]}, [r2], r1
vld1.8	{d0[5]}, [r2], r1
vld1.8	{d0[6]}, [r2], r1
vld1.8	{d0[7]}, [r2], r1
vpaddl.u8	d0, d0
vpadd.i16	d0, d0, d0
vrshrn.i16	d0, q0, #2
vdup.8	d1, d0[1]
vdup.8	d0, d0[0]
b	pred8x8_dc_end
function x264_predict_8x8c_dc_neon
sub	r2, r0, #32	@ 0x20
mov	r1, #32	@ 0x20
vld1.8	{d0}, [r2, :64]
sub	r2, r0, #1	@ 0x1
vld1.8	{d1[0]}, [r2], r1
vld1.8	{d1[1]}, [r2], r1
vld1.8	{d1[2]}, [r2], r1
vld1.8	{d1[3]}, [r2], r1
vld1.8	{d1[4]}, [r2], r1
vld1.8	{d1[5]}, [r2], r1
vld1.8	{d1[6]}, [r2], r1
vld1.8	{d1[7]}, [r2], r1
vtrn.32	d0, d1
vpaddl.u8	q0, q0
vpadd.i16	d0, d0, d1
vpadd.i16	d1, d0, d0
vrshrn.i16	d2, q0, #3
vrshrn.i16	d3, q0, #2
vdup.8	d0, d2[4]
vdup.8	d1, d3[3]
vdup.8	d4, d3[2]
vdup.8	d5, d2[5]
vtrn.32	q0, q2
pred8x8_dc_end:
add	r2, r0, r1, lsl #2
vst1.8	{d0}, [r0, :64], r1
vst1.8	{d1}, [r2, :64], r1
vst1.8	{d0}, [r0, :64], r1
vst1.8	{d1}, [r2, :64], r1
vst1.8	{d0}, [r0, :64], r1
vst1.8	{d1}, [r2, :64], r1
vst1.8	{d0}, [r0, :64], r1
vst1.8	{d1}, [r2, :64], r1
bx	lr
function x264_predict_8x8c_h_neon
sub	r1, r0, #1	@ 0x1
mov	ip, #32	@ 0x20
vld1.8	{d0[]}, [r1], ip
vld1.8	{d2[]}, [r1], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d2}, [r0, :64], ip
vld1.8	{d0[]}, [r1], ip
vld1.8	{d2[]}, [r1], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d2}, [r0, :64], ip
vld1.8	{d0[]}, [r1], ip
vld1.8	{d2[]}, [r1], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d2}, [r0, :64], ip
vld1.8	{d0[]}, [r1], ip
vld1.8	{d2[]}, [r1], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d2}, [r0, :64], ip
bx	lr
function x264_predict_8x8c_v_neon
sub	r0, r0, #32	@ 0x20
mov	ip, #32	@ 0x20
vld1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
vst1.64	{d0}, [r0, :64], ip
bx	lr

.align 4
p16weight:
.short 1,2,3,4,5,6,7,8
.text
function x264_predict_8x8c_p_neon
sub	r3, r0, #32	@ 0x20
mov	r1, #32	@ 0x20
add	r2, r3, #4	@ 0x4
sub	r3, r3, #1	@ 0x1
vld1.32	{d0[0]}, [r3]
vld1.32	{d2[0]}, [r2, :32], r1
vld1.8	{d0[4]}, [r3], r1
vld1.8	{d0[5]}, [r3], r1
vld1.8	{d0[6]}, [r3], r1
vld1.8	{d0[7]}, [r3], r1
add	r3, r3, r1
vld1.8	{d3[0]}, [r3], r1
vld1.8	{d3[1]}, [r3], r1
vld1.8	{d3[2]}, [r3], r1
vld1.8	{d3[3]}, [r3], r1
vaddl.u8	q8, d2, d3
vrev32.8	d0, d0
vtrn.32	d2, d3
vsubl.u8	q2, d2, d0
adr r3, p16weight
vld1.16	{d0-d1}, [r3, :128]
vmul.i16	d4, d4, d0
vmul.i16	d5, d5, d0
vpadd.i16	d4, d4, d5
vpaddl.s16	d4, d4
vshl.s32	d5, d4, #4
vadd.i32	d4, d4, d5
vrshrn.i32	d4, q2, #5
mov	r3, #0	@ 0x0
vtrn.16	d4, d5
vadd.i16	d2, d4, d5
vshl.s16	d3, d2, #2
vrev64.16	d16, d16
vsub.i16	d3, d3, d2
vadd.i16	d16, d16, d0
vshl.s16	d2, d16, #4
vsub.i16	d2, d2, d3
vshl.s16	d3, d4, #3
vext.8	q0, q0, q0, #14
vsub.i16	d6, d5, d3
vmov.16	d0[0], r3
vmul.i16	q0, q0, d4[0]
vdup.16	q1, d2[0]
vdup.16	q2, d4[0]
vdup.16	q3, d6[0]
vshl.s16	q2, q2, #3
vadd.i16	q1, q1, q0
vadd.i16	q3, q3, q2
mov	r3, #8	@ 0x8
flag6e8:
vqshrun.s16	d0, q1, #5
vadd.i16	q1, q1, q3
vst1.8	{d0}, [r0, :64], r1
subs	r3, r3, #1	@ 0x1
bne	flag6e8
bx	lr
function x264_predict_16x16_dc_top_neon
sub	r2, r0, #32	@ 0x20
mov	r1, #32	@ 0x20
vld1.8	{d0-d1}, [r2, :128]
vaddl.u8	q0, d0, d1
vadd.i16	d0, d0, d1
vpadd.i16	d0, d0, d0
vpadd.i16	d0, d0, d0
vrshrn.i16	d0, q0, #4
vdup.8	q0, d0[0]
b	pred16x16_dc_end
function x264_predict_16x16_dc_left_neon
mov	r1, #32	@ 0x20
sub	r2, r0, #1	@ 0x1
vld1.8	{d0[0]}, [r2], r1
vld1.8	{d0[1]}, [r2], r1
vld1.8	{d0[2]}, [r2], r1
vld1.8	{d0[3]}, [r2], r1
vld1.8	{d0[4]}, [r2], r1
vld1.8	{d0[5]}, [r2], r1
vld1.8	{d0[6]}, [r2], r1
vld1.8	{d0[7]}, [r2], r1
vld1.8	{d1[0]}, [r2], r1
vld1.8	{d1[1]}, [r2], r1
vld1.8	{d1[2]}, [r2], r1
vld1.8	{d1[3]}, [r2], r1
vld1.8	{d1[4]}, [r2], r1
vld1.8	{d1[5]}, [r2], r1
vld1.8	{d1[6]}, [r2], r1
vld1.8	{d1[7]}, [r2], r1
vaddl.u8	q0, d0, d1
vadd.i16	d0, d0, d1
vpadd.i16	d0, d0, d0
vpadd.i16	d0, d0, d0
vrshrn.i16	d0, q0, #4
vdup.8	q0, d0[0]
b	pred16x16_dc_end
function x264_predict_16x16_dc_neon
sub	r3, r0, #32	@ 0x20
sub	r0, r0, #1	@ 0x1
vld1.64	{d0-d1}, [r3, :128]
ldrb	ip, [r0], #32
vaddl.u8	q0, d0, d1
ldrb	r1, [r0], #32
vadd.i16	d0, d0, d1
vpadd.i16	d0, d0, d0
vpadd.i16	d0, d0, d0
ldrb	r2, [r0], #32
add	ip, ip, r1
ldrb	r3, [r0], #32
add	ip, ip, r2
ldrb	r1, [r0], #32
add	ip, ip, r3
ldrb	r2, [r0], #32
add	ip, ip, r1
ldrb	r3, [r0], #32
add	ip, ip, r2
ldrb	r1, [r0], #32
add	ip, ip, r3
ldrb	r2, [r0], #32
add	ip, ip, r1
ldrb	r3, [r0], #32
add	ip, ip, r2
ldrb	r1, [r0], #32
add	ip, ip, r3
ldrb	r2, [r0], #32
add	ip, ip, r1
ldrb	r3, [r0], #32
add	ip, ip, r2
ldrb	r1, [r0], #32
add	ip, ip, r3
ldrb	r2, [r0], #32
add	ip, ip, r1
ldrb	r3, [r0], #32
add	ip, ip, r2
sub	r0, r0, #512	@ 0x200
add	ip, ip, r3
vdup.16	d1, ip
vadd.i16	d0, d0, d1
mov	r1, #32	@ 0x20
add	r0, r0, #1	@ 0x1
vrshr.u16	d0, d0, #5
vdup.8	q0, d0[0]
pred16x16_dc_end:
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
vst1.64	{d0-d1}, [r0, :128], r1
bx	lr
function x264_predict_16x16_h_neon
sub	r1, r0, #1	@ 0x1
mov	ip, #32	@ 0x20
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
vld1.8	{d0[]}, [r1], ip
vorr	d1, d0, d0
vld1.8	{d2[]}, [r1], ip
vorr	d3, d2, d2
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d2-d3}, [r0, :128], ip
bx	lr
function x264_predict_16x16_v_neon
sub	r0, r0, #32	@ 0x20
mov	ip, #32	@ 0x20
vld1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
vst1.64	{d0-d1}, [r0, :128], ip
bx	lr
function x264_predict_16x16_p_neon
sub	r3, r0, #32	@ 0x20
mov	r1, #32	@ 0x20
add	r2, r3, #8	@ 0x8
sub	r3, r3, #1	@ 0x1
vld1.8	{d0}, [r3]
vld1.8	{d2}, [r2, :64], r1
vld1.8	{d1[0]}, [r3], r1
vld1.8	{d1[1]}, [r3], r1
vld1.8	{d1[2]}, [r3], r1
vld1.8	{d1[3]}, [r3], r1
vld1.8	{d1[4]}, [r3], r1
vld1.8	{d1[5]}, [r3], r1
vld1.8	{d1[6]}, [r3], r1
vld1.8	{d1[7]}, [r3], r1
add	r3, r3, r1
vld1.8	{d3[0]}, [r3], r1
vld1.8	{d3[1]}, [r3], r1
vld1.8	{d3[2]}, [r3], r1
vld1.8	{d3[3]}, [r3], r1
vld1.8	{d3[4]}, [r3], r1
vld1.8	{d3[5]}, [r3], r1
vld1.8	{d3[6]}, [r3], r1
vld1.8	{d3[7]}, [r3], r1
vrev64.8	q0, q0
vaddl.u8	q8, d2, d3
vsubl.u8	q2, d2, d0
vsubl.u8	q3, d3, d1
adr r3, p16weight
vld1.8	{d0-d1}, [r3, :128]
vmul.i16	q2, q2, q0
vmul.i16	q3, q3, q0
vadd.i16	d4, d4, d5
vadd.i16	d5, d6, d7
vpadd.i16	d4, d4, d5
vpadd.i16	d4, d4, d4
vshll.s16	q3, d4, #2
vaddw.s16	q2, q3, d4
vrshrn.i32	d4, q2, #6
mov	r3, #0	@ 0x0
vtrn.16	d4, d5
vadd.i16	d2, d4, d5
vshl.s16	d3, d2, #3
vrev64.16	d16, d17
vsub.i16	d3, d3, d2
vadd.i16	d16, d16, d0
vshl.s16	d2, d16, #4
vsub.i16	d2, d2, d3
vshl.s16	d3, d4, #4
vext.8	q0, q0, q0, #14
vsub.i16	d6, d5, d3
vmov.16	d0[0], r3
vmul.i16	q0, q0, d4[0]
vdup.16	q1, d2[0]
vdup.16	q2, d4[0]
vdup.16	q3, d6[0]
vshl.s16	q2, q2, #3
vadd.i16	q1, q1, q0
vadd.i16	q3, q3, q2
mov	r3, #16	@ 0x10
1:
vqshrun.s16	d0, q1, #5
vadd.i16	q1, q1, q2
vqshrun.s16	d1, q1, #5
vadd.i16	q1, q1, q3
vst1.8	{d0-d1}, [r0, :128], r1
subs	r3, r3, #1	@ 0x1
bne	1b
bx	lr

