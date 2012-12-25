//#define Page_Size	(4096)  //it will be set during build
//#define rounds		(10)	 //it will be set during build
#define Block_Size		(16)
#define Blocks_per_Page (Page_Size/Block_Size)

__constant uchar SBox[256]=
 { 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76 //0
   , 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0 //1
   , 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15 //2
   , 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75 //3
   , 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84 //4
   , 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf //5
   , 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8 //6
   , 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2 //7
   , 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73 //8
   , 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb //9
   , 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79 //A
   , 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08 //B
   , 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a //C
   , 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e //D
   , 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf //E
   , 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};//F
    //0      1    2      3     4    5     6     7      8    9     A      B    C     D     E     F


unsigned char galoisMultiplication(unsigned char a, unsigned char b){
	unsigned char p = 0; 
	
	for(unsigned int i=0; i < 8; ++i){
		if((b&1) == 1)
			p^=a;
	
		unsigned char hiBitSet = (a & 0x80);
		a <<= 1;
	
		if(hiBitSet == 0x80)
			a ^= 0x1b;
		
		b >>= 1;
	}
	
	return p;
}

inline uchar4 sbox(uchar4 block){
    return (uchar4)(SBox[block.x], SBox[block.y], SBox[block.z], SBox[block.w]);
}

void mixColumns(uchar4* dest, uchar4 * block, __private uchar4 * galiosCoeff){
	for(int i=0; i<4 ; i++){
		dest[i].x = galoisMultiplication(galiosCoeff[0].x, block[i].x);
		dest[i].x^= galoisMultiplication(galiosCoeff[0].y, block[i].y);
		dest[i].x^= galoisMultiplication(galiosCoeff[0].z, block[i].z);
		dest[i].x^= galoisMultiplication(galiosCoeff[0].w, block[i].w);
		
		dest[i].y = galoisMultiplication(galiosCoeff[1].x, block[i].x);
		dest[i].y^= galoisMultiplication(galiosCoeff[1].y, block[i].y);
		dest[i].y^= galoisMultiplication(galiosCoeff[1].z, block[i].z);
		dest[i].y^= galoisMultiplication(galiosCoeff[1].w, block[i].w);
		
		dest[i].z = galoisMultiplication(galiosCoeff[2].x, block[i].x);
		dest[i].z^= galoisMultiplication(galiosCoeff[2].y, block[i].y);
		dest[i].z^= galoisMultiplication(galiosCoeff[2].z, block[i].z);
		dest[i].z^= galoisMultiplication(galiosCoeff[2].w, block[i].w);
		
		dest[i].w = galoisMultiplication(galiosCoeff[3].x, block[i].x);
		dest[i].w^= galoisMultiplication(galiosCoeff[3].y, block[i].y);
		dest[i].w^= galoisMultiplication(galiosCoeff[3].z, block[i].z);
		dest[i].w^= galoisMultiplication(galiosCoeff[3].w, block[i].w);
	}
	
	return ;
}

uchar4 shiftRows(uchar4 row, unsigned int j){
	uchar4 r = row;
	for(uint i=0; i < j; ++i)  
		r = r.yzwx;
	return r;
}

__kernel void aes_enc(__global  uchar4* output, __global  uchar4* IVs, __global  uchar4* roundKeys){
	
	__private unsigned int index=get_global_id(0);
	__private unsigned int output_index=index*4;
	__private unsigned int page_index=index/Blocks_per_Page;
	__private unsigned int offset_in_page=index%Blocks_per_Page;
	__private unsigned int iv_index=page_index*4;
	__private unsigned int rk_index=page_index*4*(rounds+1);
	
	__private uchar4 input[4];
	
	input[3]=(uchar4)(0,0,0,offset_in_page+2);
	if(offset_in_page>=254)
		input[3]=(uchar4)(0,0,1,offset_in_page-254);
	input[2]=IVs[iv_index+2];
	input[1]=IVs[iv_index+1];
	input[0]=IVs[iv_index];
	
	//for(int i=0;i<4;i++)
	//	output[output_index+i]=input[i];
	//return;
	
	__private uchar4 block1[4];
	
	__private uchar4 galiosCoeff[4];
	
	/*
	galiosCoeff[0] = (uchar4)(2, 1, 1, 3);
	galiosCoeff[1] = (uchar4)(3, 2, 1, 1);
	galiosCoeff[2] = (uchar4)(1, 3, 2, 1);
	galiosCoeff[3] = (uchar4)(1, 1, 3, 2);
	*/
	
	galiosCoeff[0] = (uchar4)(2, 3, 1, 1);
	galiosCoeff[1] = (uchar4)(1, 2, 3, 1);
	galiosCoeff[2] = (uchar4)(1, 1, 2, 3);
	galiosCoeff[3] = (uchar4)(3, 1, 1, 2);
	
	for(int i=0; i<4; i++)
		input[i] ^= roundKeys[rk_index+i];
	
	for(unsigned int r=1; r < rounds; ++r){
		
		for (int i=0; i<4; i++){
			input[i] = sbox(input[i]);
			//input[i] = shiftRows(input[i], i); 
		}
		
		block1[0].xyzw=(uchar4)(input[0].x,input[1].y,input[2].z,input[3].w);
		block1[1].xyzw=(uchar4)(input[1].x,input[2].y,input[3].z,input[0].w);
		block1[2].xyzw=(uchar4)(input[2].x,input[3].y,input[0].z,input[1].w);
		block1[3].xyzw=(uchar4)(input[3].x,input[0].y,input[1].z,input[2].w);
		
		mixColumns(input,block1, galiosCoeff); 
		
		for(int i=0; i<4; i++)
			input[i] = input[i] ^ roundKeys[rk_index+r*4 + i];
		
	}  
	
	for(int i=0; i<4; i++)
		input[i] = sbox(input[i]);
		
	block1[0].xyzw=(uchar4)(input[0].x,input[1].y,input[2].z,input[3].w);
	block1[1].xyzw=(uchar4)(input[1].x,input[2].y,input[3].z,input[0].w);
	block1[2].xyzw=(uchar4)(input[2].x,input[3].y,input[0].z,input[1].w);
	block1[3].xyzw=(uchar4)(input[3].x,input[0].y,input[1].z,input[2].w);

	for(int i=0;i<4;i++)
		output[output_index+i] =  block1[i] ^ roundKeys[rk_index+(rounds)*4 + i];
	
	return;
}
