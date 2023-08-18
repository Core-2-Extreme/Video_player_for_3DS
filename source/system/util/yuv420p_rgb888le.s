.data
.align 4
.global yuv420p_to_rgb888le_asm
.type yuv420p_to_rgb888le_asm, "function"

//#define C(Y) ((Y) - 16)
//#define D(U) ((U) - 128)
//#define E(V) ((V) - 128)

//#define YUV2R(Y, V)		CLIP((298 * C(Y) + 409 * E(V) + 128) >> 8)
//#define YUV2G(Y, U, V)	CLIP((298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
//#define YUV2B(Y, U)		CLIP((298 * C(Y) + 516 * D(U) + 128) >> 8)

/*
	r0 yuv420p pointer
	r1 bgr pointer
	r2 width
	r3 height
*/
.text
yuv420p_to_rgb888le_asm:
	push { r4-r12 }

	/*
	r0 -> free
	r1 -> bgr pointer
	r2 -> width
	r3 -> height
	r4 -> y pointer
	r5 -> u pointer
	r6 -> v pointer
	r7 -> div cache, free
	r8 -> div cache, free
	r9 -> mul cache, free
	r10-r12 -> free
	*/
	mul r9, r2, r3
	//r6 = y pointer
	mov r4, r0
	//r5 = u pointer + (width * height)
	add r5, r0, r9
	//r4 = v pointer + (width * height / 4)
	add r6, r5, r9, asr #2

	/*
	r0 -> bgr pointer
	r1 -> width
	r2 -> height
	r3 -> loop counter x
	r4 -> y pointer -> loop counter y
	r5 -> u pointer -> cache -> free
	r6 -> v pointer -> free
	r7-r12 -> free
	*/
	push { r4-r6 }
	mov r0, r1
	mov r1, r2
	mov r2, r3
	mov r3, #0
	mov r4, #0

	conversion_loop:

	load_yuv:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r7 -> yuv pointer cache -> free
		r8 -> cache -> free
		r9 -> u,v, offset cache -> free
		r10 -> y value
		r11 -> u value
		r12 -> v value
		y pos :
		------
		|....|
		|●...|
		|....|
		|....|
		------
		*/
		pop { r5-r7 }

		//(y / 2) * (width / 2) + (x / 2)
		calc_uv_offset:
			//r8 = width / 2
			asr r8, r1, #1
			//r9 = y / 2
			asr r9, r4, #1
			//r8 = r9 * r8
			mul r8, r9, r8
			ldrb r10, [r5, r1]
			//r9 = r8 + x / 2
			push { r5-r7 }
			add r9, r8, r3, asr #1

		ldrb r11, [r6, r9]
		ldrb r12, [r7, r9]

		ldr r7, =298
		sub r10, #16
		sub r11, #128
		sub r12, #128

	//r10 = r7 * C(Y)
	mul r10, r7, r10

	convert_yub_b:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5 -> b value
		r6-r8 -> cache -> free
		r9 -> result for 516 * D(U) + 128
		r10 -> result for 298 * C(Y)
		r11 -> u value
		r12 -> v value
		YUV2B(Y, U) CLIP((298 * C(Y) + 516 * D(U) + 128) >> 8)
		*/


	convert_yub_g:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5 -> b value -> free
		r6 -> g value -> free
		r7 -> cache -> free
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> result for 298 * C(Y)
		r11 -> u value -> bgr pointer + (width * 3)
		r12 -> v value
		YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
		*/
		//r9 = 516 * D(U) + 128
		ldr r8, =516
		ldr r5, =128
		mla r9, r8, r11, r5
		ldr r7, =100
		ldr r8, =208
		//r5 = (r10 + r9) >> 8
		add r5, r10, r9

		//r6 = 100 * D(U)
		mul r6, r7, r11

		asrs r5, r5, #8
		//clamp
		movmi r5, #0

		//r6 = 208 * E(V) + r6
		mla r6, r8, r12, r6

		cmp r5, #255
		movgt r5, #255

		//r8 = r6 - 128
		sub r8, r6, #128
		//r6 = (r10 - r8) >> 8
		sub r6, r10, r8
		add r11, r0, r1, lsl #1
		asrs r6, r6, #8
		add r11, r11, r1

		//clamp
		movmi r6, #0
		cmp r6, #255
		movgt r6, #255

		//store
		strb r5, [r11], #1
		strb r6, [r11], #1

	convert_yub_r:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> cache -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> result for 298 * C(Y)
		r11 -> bgr pointer + width
		r12 -> v value -> free
		YUV2R(Y, V) CLIP(( 298 * C(Y) + 409 * E(V) + 128) >> 8)
		*/
		//r6 = 409 * E(V) + 128
		ldr r6, =409
		ldr r5, =128
		mla r7, r6, r12, r5
		//r6 = (r10 + r7) >> 8
		add r6, r10, r7
		asrs r5, r6, #8

		//clamp
		movmi r5, #0
		cmp r5, #255
		movgt r5, #255

		//store
		strb r5, [r11], #1


	load_y_1:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> result for 298 * C(Y) -> y pointer -> free
		r11 -> bgr pointer + width
		r12 -> result for 298 * C(Y)
		y pos :
		------
		|●...|
		|....|
		|....|
		|....|
		------
		*/
		pop { r10 }
		ldrb r12, [r10], #1

		ldr r5, =298
		sub r12, #16
		push { r10 }

	//r12 = r5 * C(Y)
	mul r12, r5, r12

	convert_yub_bgr_1:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> cache -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> cache -> free
		r11 -> bgr pointer + width
		r12 -> result for 298 * C(Y)
		YUV2B(Y, U) CLIP((298 * C(Y) + 516 * D(U) + 128) >> 8)
		YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
		YUV2R(Y, V) CLIP(( 298 * C(Y) + 409 * E(V) + 128) >> 8)
		stack : y.u.v pointer
		*/
		add r5, r12, r9

		asrs r5, r5, #8//clamp
		movmi r5, #0
		cmp r5, #255
		movgt r5, #255
		strb r5, [r0], #1//store

		sub r6, r12, r8

		asrs r6, r6, #8//clamp
		movmi r6, #0
		cmp r6, #255
		movgt r6, #255
		strb r6, [r0], #1//store

		add r10, r12, r7

		asrs r10, r10, #8//clamp
		movmi r10, #0
		cmp r10, #255
		movgt r10, #255
		strb r10, [r0], #1//store


	load_y_2:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> result for 298 * C(Y) -> y pointer -> free
		r11 -> bgr pointer + width
		r12 -> result for 298 * C(Y)
		y pos :
		------
		|....|
		|.●..|
		|....|
		|....|
		------
		*/
		pop { r10 }
		ldrb r12, [r10, r1]

		ldr r5, =298
		sub r12, #16
		push { r10 }

	//r12 = r5 * C(Y)
	mul r12, r5, r12
	add r3, r3, #2//update loop counter x

	convert_yub_bgr_2:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> cache -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> cache -> free
		r11 -> bgr pointer + width
		r12 -> result for 298 * C(Y)
		YUV2B(Y, U) CLIP((298 * C(Y) + 516 * D(U) + 128) >> 8)
		YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
		YUV2R(Y, V) CLIP(( 298 * C(Y) + 409 * E(V) + 128) >> 8)
		stack : y.u.v pointer
		*/
		add r5, r12, r9

		asrs r5, r5, #8//clamp
		movmi r5, #0
		cmp r5, #255
		movgt r5, #255
		strb r5, [r11], #1//store

		sub r6, r12, r8

		asrs r6, r6, #8//clamp
		movmi r6, #0
		cmp r6, #255
		movgt r6, #255
		strb r6, [r11], #1//store

		add r10, r12, r7

		asrs r10, r10, #8//clamp
		movmi r10, #0
		cmp r10, #255
		movgt r10, #255
		strb r10, [r11], #1//store


	load_y_3:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> result for 298 * C(Y) -> y pointer -> free
		r11 -> bgr pointer + width
		r12 -> result for 298 * C(Y)
		y pos :
		------
		|.●..|
		|....|
		|....|
		|....|
		------
		*/
		pop { r10 }
		ldrb r12, [r10], #1

		ldr r5, =298
		sub r12, #16
		push { r10 }

	//r12 = r5 * C(Y)
	mul r12, r5, r12

	convert_yub_bgr_3:
		/*
		r0 -> bgr pointer
		r1 -> width
		r2 -> height
		r3 -> loop counter x
		r4 -> loop counter y
		r5-r6 -> cache -> free
		r7 -> result for 409 * E(V) + 128
		r8 -> result for 100 * D(U) - 208 * E(V) + 128
		r9 -> result for 516 * D(U) + 128
		r10 -> cache -> free
		r11 -> bgr pointer + width
		r12 -> result for 298 * C(Y)
		YUV2B(Y, U) CLIP((298 * C(Y) + 516 * D(U) + 128) >> 8)
		YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
		YUV2R(Y, V) CLIP(( 298 * C(Y) + 409 * E(V) + 128) >> 8)
		stack : y.u.v pointer
		*/
		add r5, r12, r9

		asrs r5, r5, #8//clamp
		movmi r5, #0
		cmp r5, #255
		movgt r5, #255
		strb r5, [r0], #1//store

		sub r6, r12, r8

		asrs r6, r6, #8//clamp
		movmi r6, #0
		cmp r6, #255
		movgt r6, #255
		strb r6, [r0], #1//store

		add r10, r12, r7

		asrs r10, r10, #8//clamp
		movmi r10, #0
		cmp r10, #255
		movgt r10, #255
		strb r10, [r0], #1//store


	cmp r3, r1
	blt conversion_loop
	add r4, r4, #2
	ldr r3, =0
	cmp r4, r2

	//y pointer += width
	//bgr pointer += width * 3
	pop { r10 }
	add r10, r10, r1
	add r0, r0, r1, lsl #1
	push { r10 }
	add r0, r0, r1
	blt conversion_loop

	pop { r5-r7 }

	pop { r4-r12 }
	bx lr
