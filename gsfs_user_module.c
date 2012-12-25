#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <CL/cl.h>
#include <pthread.h>
#include "gsfs_user_module.h"
#include <sys/mman.h>
#include "message.h"

#define CPU_GPU_Thresh	(4096*0000)
//#define CPU_GPU_Thresh	(4096*00)

#define PAGES_LENGTH		(4096*1000*100)

#define Max_CL_File_Size	(40960)

#define warning
#ifdef warning
	#define warn(p)	p
#else
	#define warn(p)
#endif

void CL_CALLBACK context_notify(const char* errinfo, const void* private, size_t cb, void* user_data){
	printf("\n!!! Context Error: %s\n",errinfo);
	return;
}

typedef struct{
	cl_context 		context;
	cl_device_id		dev;
	cl_program		pr;
	cl_device_type		type;
} xpu_struct;

typedef struct{
	cl_platform_id	platform;
	xpu_struct*		gpu;
	xpu_struct*		cpu;
	char			kernelname[100];
} cl_struct;

int setup_cl_platform(cl_struct* cl){
	cl_uint	 num;
	cl_int 	ret;
	size_t 	s;
	char 		a[100];
	
	cl_context_properties props[3];
	
	ret=clGetPlatformIDs(1, &cl->platform, &num);
	printf("\nret: %d, Platform num: %u ** ", ret==CL_SUCCESS, num);
	if(ret!=CL_SUCCESS)
		return -1;
	
	ret=clGetPlatformInfo(cl->platform, CL_PLATFORM_PROFILE, 100, a, &s);
	printf("ret: %d, s: %u, Platform Profile: %s ** ", ret==CL_SUCCESS, s, a);
	
	clGetPlatformInfo(cl->platform, CL_PLATFORM_VERSION, 100, a, &s);
	printf("ret: %d, s: %u, Platform Version: %s \n", ret==CL_SUCCESS, s, a);
	
	return 0;
}

xpu_struct* setup_cl_context(cl_struct* cl, cl_device_type xpu_type){
	cl_uint	 num,
			count;
	cl_int 	ret,
			erc;
	size_t 	s;
	cl_device_id	devs[10];
	char 		a[100];
	int 	i;
	cl_context_properties props[3];
	xpu_struct* xpu;
	
	xpu=malloc(sizeof(xpu_struct));
	memset(xpu, 0, sizeof(xpu_struct));
	
	xpu->type=xpu_type;
	
	printf("Setup CL Context, for %s ** ",(xpu_type==CL_DEVICE_TYPE_CPU)?"CPU":"GPU");
		
	ret=clGetDeviceIDs(cl->platform, xpu_type, 10, devs, &num);
	printf("GetDeviceIDs: ret: %d, num: %u **", ret==CL_SUCCESS, num);
	
	if(num==0)
		return 0;
	xpu->dev=devs[0];
	printf("\n");
	
	for(i=0;i<num;i++){
		cl_device_id dev=devs[i];
		
		cl_device_type t=0;
				
		ret=clGetDeviceInfo(dev, CL_DEVICE_TYPE, 100, &t, 0);
		if(ret==CL_SUCCESS)
			printf("##i: %u ====>>>> type: %lu ** ", i, t);
		
		cl_uint mcu=0;
		ret=clGetDeviceInfo(dev,CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &mcu, 0);
		if(ret==CL_SUCCESS)
			printf("Max Compute Units: %u ** " ,mcu);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &mcu, 0);
		if(ret==CL_SUCCESS)
			printf("Max Work Item Dimensions: %u ** " ,mcu);
		
		size_t s[4];
		memset(s, 0, sizeof(s));
		ret=clGetDeviceInfo(dev,CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(s), s , 0);
		if(ret==CL_SUCCESS)
			printf("Max Work Item Sizes: %lu %lu %lu %lu ** " , s[0], s[1], s[2], s[3]);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(size_t), s, 0);
		if(ret==CL_SUCCESS)
			printf("Device Profiling Timer Resolution: %lu ** " ,s[0]);
		
		cl_device_exec_capabilities cc;
		ret=clGetDeviceInfo(dev,CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cc), &cc, 0);
		if(ret==CL_SUCCESS)
			printf("Device Execution Capabilities: Kernel: %d, Native Kernel: %d  ** " ,
			       cc&CL_EXEC_KERNEL, cc&CL_EXEC_NATIVE_KERNEL);
		
		size_t ss;
		ret=clGetDeviceInfo(dev,CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(ss), &ss, 0);
		if(ret==CL_SUCCESS)
			printf("Max Work Group Size: %lu ** " , ss);
		else
			printf("No Max Work Group Size: %d ** " , ret);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_MAX_CLOCK_FREQUENCY, 100, &mcu, 0);
		if(ret==CL_SUCCESS)
			printf("Max Clock Frequency: %u MHz** " ,mcu);
		
		cl_ulong ul;
		ret=clGetDeviceInfo(dev,CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &ul, 0);
		if(ret==CL_SUCCESS)
			printf("Max Mem Alloc Mem Size: %ld MB ** " ,ul>>20);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &ul, 0);
		if(ret==CL_SUCCESS)
			printf("Global Mem Cache Size: 0x%lx B ** " ,ul);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &ul, 0);
		if(ret==CL_SUCCESS)
			printf("Global Mem Size: %ld MB ** " ,ul>>20);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &ul, 0);
		if(ret==CL_SUCCESS)
			printf("Local Mem Size: %ld B** " ,ul);
		
		cl_bool bo;
		ret=clGetDeviceInfo(dev,CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &bo, 0);
		if(ret==CL_SUCCESS)
			printf("Host Unified Memory: %d ** " ,(int)bo);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &bo, 0);
		if(ret==CL_SUCCESS)
			printf("Endian Little: %d ** " ,(int)bo);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_AVAILABLE, sizeof(cl_bool), &bo, 0);
		if(ret==CL_SUCCESS)
			printf("Device Available: %d ** " ,(int)bo);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_ADDRESS_BITS, sizeof(mcu), &mcu, 0);
		if(ret==CL_SUCCESS)
			printf("Device Address Bits: %u ** " ,mcu);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_NAME, 100, a, 0);
		if(ret==CL_SUCCESS)
			printf("Name: %s ** " ,a);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_VERSION, 100, a, 0);
		if(ret==CL_SUCCESS)
			printf("Version: %s ** " ,a);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_PROFILE, 100, a, 0);
		if(ret==CL_SUCCESS)
			printf("Profile: %s ** " ,a);
		
		ret=clGetDeviceInfo(dev,CL_DEVICE_EXTENSIONS, 100, a, 0);
		if(ret==CL_SUCCESS)
			printf("Extensions: %s **  " ,a);
		
		printf("\n");
	}
	printf("\n");
	
	erc=0;
	props[0]=CL_CONTEXT_PLATFORM;
	props[1]=(cl_context_properties)cl->platform;
	props[2]=0;
	xpu->context=clCreateContext(props, 1, &xpu->dev, context_notify, 0, &erc);
	if(erc!=CL_SUCCESS){
		printf("error for creating context: %d\n", erc);
		free(xpu);
		return 0;
	}
			
	ret=clGetContextInfo(xpu->context, CL_CONTEXT_REFERENCE_COUNT, sizeof(cl_uint), &count, &s);
	if(ret==CL_SUCCESS)
		printf("Context Count: %u ** ",count);
	
	ret=clGetContextInfo(xpu->context, CL_CONTEXT_PROPERTIES, sizeof(cl_context_properties)*3, props, &s);
	if(ret==CL_SUCCESS)
		printf("Context Properties: %lx %lx %lx %lx ** ", props[0]==CL_CONTEXT_PLATFORM, props[1]==(unsigned long)cl->platform, props[2], s);
	
	printf("\n");
		
	return xpu;
}

int build_kernel(xpu_struct* xpu, char* filename){
	size_t ws;
	
	char *source=malloc(Max_CL_File_Size);
	int clfd=open(filename, O_RDONLY);
	int ret=read(clfd, source, Max_CL_File_Size);
	close(clfd);
	
	if(ret<=0)
		return -1;
	source[ret]=0;
	printf("Reading %d bytes of file %s  ** ", ret, filename);
	
	cl_int erc;
	xpu->pr=clCreateProgramWithSource(xpu->context, 1,(const char**)&source, 0, &erc);
	if(erc!=CL_SUCCESS){
		printf("Unable to create program \n ");
		return -1;
	}
	printf("Program: %lx ** ",xpu->pr);
	
	free(source);
	source=0;
	
	char options[100];
	sprintf(options,"-D Page_Size=%u -D rounds=%u", Block_Size, Rounds);
	erc=clBuildProgram(xpu->pr, 1, &xpu->dev, options, 0, 0);
	if(erc!=CL_SUCCESS){
		printf("Unable to build program \n");
		return -1;
	}
	printf("Program Built %d ** ", erc==CL_BUILD_PROGRAM_FAILURE);

	cl_build_status bs;
	erc=clGetProgramBuildInfo(xpu->pr, xpu->dev, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &bs, 0);
	if(erc==CL_SUCCESS)
		printf("Program Build Status: success:%d error: %d ** ", bs==CL_BUILD_SUCCESS, bs==CL_BUILD_ERROR);
	
	char a[10000];
	erc=clGetProgramBuildInfo(xpu->pr, xpu->dev, CL_PROGRAM_BUILD_LOG, 10000, a, &ws);
	a[ws]=0;
	printf("Program Build Log: %s  (%lu) ** \n", a, ws);
	
	return 0;
}

void release_cl(cl_struct* cl){
	int i;
	for(i=0;i<2;i++){
		xpu_struct* xpu;
		if(i==0)
			xpu=cl->cpu;
		else
			xpu=cl->gpu;
		
		if(xpu==0)
			continue;
		
		if(xpu->pr)
			clReleaseProgram(xpu->pr);
		
		if(xpu->context)
			clReleaseContext(xpu->context);
		
		free(xpu);
	}
	
	return;
}

void rotate(cl_uchar * word){
	cl_uchar c = word[0];
	
	for(cl_uint i=0; i<3; ++i)
		word[i] = word[i+1];
	
	word[3] = c;
	
	return;
}

void core(cl_uchar * word, cl_uint iter){
	rotate(word);

	for(cl_uint i=0; i < 4; ++i)
		word[i] = sbox[word[i]];//getSBoxValue(word[i]);
	
	word[0] = word[0]^Rcon[iter];
	
	return;
}

void keyExpansion(cl_uchar * key, cl_uchar * expandedKey, cl_uint keySize, cl_uint explandedKeySize){
	cl_uint currentSize    = 0;
	cl_uint rConIteration = 1;
	cl_uchar temp[4]      = {0};

	for(cl_uint i=0; i < keySize; ++i)
		expandedKey[i] = key[i];

	currentSize += keySize;

	while(currentSize < explandedKeySize){
		for(cl_uint i=0; i < 4; ++i)
			temp[i] = expandedKey[(currentSize - 4) + i];
	
		if(currentSize%keySize == 0)
			core(temp, rConIteration++);
		
		for(cl_uint i=0; i < 4; ++i){
			expandedKey[currentSize] = expandedKey[currentSize - keySize]^temp[i];
			currentSize++;
		}
	}
	
	return;
}

void createRoundKey(cl_uchar * eKey, cl_uchar * rKey){
	for(cl_uint i=0; i < 4; ++i)
		for(cl_uint j=0; j < 4; ++j)
		    rKey[i+ j*4] = eKey[i*4 + j];
		 
	return;
}

int get_gctr_page(cl_struct* cl,unsigned char* IVs, unsigned char* keys, unsigned char* pages, int count){
	int ret=0;
	xpu_struct *xpu=0;
	
	if(!IVs || !keys || !pages || !count)
		return -1;

	cl_int erc=0;
	
	/*
	for(int i=0; i<count; i++){
		memset(pages, i, Block_Size);
		printf("i: %u, pages[i]: %lx\n",i,(unsigned long)pages);
		fflush(0);
		pages+=Block_Size;
	}
	return 0;
	*/
	
	if(!cl->gpu && !cl->cpu)
		return -1;
	
	if(!cl->gpu){
		xpu=cl->cpu;
		warn(printf("\nSelecting CPU for get_gctr_page ** ");)
		goto gctr_cont;
	}
	
	if(!cl->cpu){
		xpu=cl->gpu;
		warn(printf("\nSelecting GPU for get_gctr_page ** ");)
		goto gctr_cont;
	}
	
	//cl->cpu && cl->gpu
	if(count*Block_Size<CPU_GPU_Thresh) {
		xpu=cl->cpu;
		warn(printf("\nSelecting CPU for get_gctr_page ** ");)
	}
	else{
		xpu=cl->gpu;
		warn(printf("\nSelecting GPU for get_gctr_page ** ");)
	}
	
gctr_cont:
	if(xpu==0){
		warn(printf("\nCant GPU or CPU for get_gctr_page **\n ");)
		return -1;
	}
	
	cl_command_queue cq=clCreateCommandQueue(xpu->context, xpu->dev, 0, &erc);
	if(erc!=CL_SUCCESS){
		warn(printf("Can't create command queue **\n ");)
		return -1;
	}
	
	//key_expansion
	unsigned long expandedKeySize = (Rounds+1)*Key_Size;
	unsigned long expandedKeysSize = expandedKeySize*count;  
	unsigned char* expandedKeys = malloc(expandedKeysSize);
	unsigned char* roundKeys=expandedKeys;
	int i;
	for(i=0; i<count; i++)
		keyExpansion(keys+Key_Size*i, expandedKeys+i*expandedKeySize, Key_Size, expandedKeySize);
	
	int j;
	
	//getting a kernel 
	cl_kernel kernel=clCreateKernel(xpu->pr, cl->kernelname, &erc);
	if(erc!=CL_SUCCESS){
		warn(printf("Unable to create kernel: %s **\n ", cl->kernelname));
		ret=-1;
		goto rel_cq;
	}
	warn(printf("Kernel: %s is created successfully ** ", cl->kernelname);)
	warn(fflush(0));
	
	#ifdef warning
	size_t mwg=0;
	cl_int wgi=clGetKernelWorkGroupInfo(kernel, xpu->dev, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &mwg, 0);
	if(wgi==CL_SUCCESS)
		warn(printf("CL_KERNEL_WORK_GROUP_SIZE : %lu ** ", mwg));
	#endif
	
	//creating buffers
	cl_int status;
	cl_mem IVsbuffer = clCreateBuffer(xpu->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 
					IV_Size*count, IVs, &status);
	if(status!=CL_SUCCESS){
		warn(printf("clCreateBuffer failed. for IVsbuffer");)
		ret=-1;
		goto rel_ker;
	}
	warn(printf("IVsbuffer created. ** ");)
	warn(fflush(0));
	
	cl_mem outputBuffer;
	if(xpu->type == CL_DEVICE_TYPE_CPU)
		outputBuffer = clCreateBuffer(xpu->context,  CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
						Block_Size*count, pages, &status);
	else
		outputBuffer = clCreateBuffer(xpu->context,  CL_MEM_WRITE_ONLY ,
						Block_Size*count, 0, &status);
	if(status!=CL_SUCCESS){
		warn(printf("clCreateBuffer failed. for outputBuffer");)
		ret=-1;
		goto rel_ivbuf;
	}
	warn(printf("outputBuffer created. ** ");)
	warn(fflush(0));
	
	cl_mem rKeyBuffer = clCreateBuffer(xpu->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					expandedKeysSize, roundKeys, &status);	
	if(status!=CL_SUCCESS){
		printf("clCreateBuffer failed. for rKeyBuffer");
		ret=-1;
		goto rel_opbuf;
	}
	warn(printf("rKeyBuffer created. ** ");)
	warn(fflush(0));
	
	/*
	cl_mem sboxBuffer=clCreateBuffer(xpu->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 256, sbox, &status);
	if(status!=CL_SUCCESS){
		printf("clCreateBuffer failed. for sboxBuffer");
		ret=-1;
		goto rel_rkbuf;
	}
	warn(printf("sBoxBuffer created. **\n ");)
	*/
	
	//setting kernel args
	status=clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&outputBuffer);
	if(status!=CL_SUCCESS){
		ret=-1;
		warn(printf("Unable to Setting Kernel Arg 0");)
		goto rel_sbbuf;
	}
	
	status=clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&IVsbuffer);
	if(status!=CL_SUCCESS){
		ret=-1;
		warn(printf("Unable to Setting Kernel Arg 1");)
		goto rel_sbbuf;
	}
	
	status=clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&rKeyBuffer);
	if(status!=CL_SUCCESS){
		ret=-1;
		warn(printf("Unable to Setting Kernel Arg 2");)
		goto rel_sbbuf;
	}
	
	//status=clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&sboxBuffer);
	//if(status!=CL_SUCCESS){
// 		ret=-1;
// 		printf("Unable to Setting Kernel Arg 3");
// 		goto rel_sbbuf;
// 	}
// 	
	size_t wis=count*Block_Size/Key_Size;
	
	status=clEnqueueNDRangeKernel(cq, kernel, 1, 0, &wis, 0, 0, 0, 0);
	if(status !=CL_SUCCESS){
		ret=-1;
		warn(printf(" Unable to EnqueNDRange: %d  ", ret);)
		goto rel_sbbuf;
	}
		
	warn(printf(" clEnqueueNDRangeKernel: %d, wis: %lu ** ", ret, wis);)
	warn(fflush(0));
	
	clFinish(cq);
	
	if(xpu->type != CL_DEVICE_TYPE_CPU){
		unsigned char* output=clEnqueueMapBuffer(cq, outputBuffer, CL_TRUE, CL_MAP_READ, 0, Block_Size*count, 0, 0, 0, &status);
		
		warn(printf("\nReading buffer: success  %d, output= %lx **\n",erc==CL_SUCCESS, output);)
		if(status!=CL_SUCCESS){
			ret=-1;
			
			goto rel_sbbuf;
		}
		
		char* current=pages;
		for(i=0; i<count; i++){
			memcpy(current, output, Block_Size);
			output+=Block_Size;
			current+=Block_Size;
		}
		
		clEnqueueUnmapMemObject(cq, outputBuffer, output, 0, 0, 0);
	}
	
rel_sbbuf:
	//clReleaseMemObject(sboxBuffer);
	
rel_rkbuf:
	clReleaseMemObject(rKeyBuffer);
	
rel_opbuf:
	clReleaseMemObject(outputBuffer);
	
rel_ivbuf:
	clReleaseMemObject(IVsbuffer);
		    
rel_ker:	
	clReleaseKernel(kernel);

rel_cq:
	clReleaseCommandQueue(cq);
	memset(roundKeys, 0, expandedKeysSize);
	free(roundKeys);
	
	printf("\n");
	warn(fflush(0));
	
	return ret;
}

cl_struct cl;
int fid;

void* enc(void * input){
	struct ocl_message* mes=(struct ocl_message*)input;
	
	warn(printf("enc: pages_count: %u, IVs: %lx, Keys: %lx, results: %lx **\n ",mes->pages_count, 
		    mes->IVs_start_address , mes->keys_start_address, mes->results_start_address));
	
	int ret=get_gctr_page(&cl, (void*)mes->IVs_start_address , (void*)mes->keys_start_address, 
			      (void*)mes->results_start_address, mes->pages_count);
	
	warn(printf("enc: ret:%d ** ",ret));
	warn(fflush(0);)
	if(ret)
		mes->type=OCL_Is_Damaged;
	else
		mes->type=OCL_Response_Is_Ready;
	
	warn(printf("start to read ** "));
	warn(fflush(0);)
	
	read(fid, mes, sizeof(struct ocl_message));
	
	warn(printf("Going to unmap: %lx ** ",mes->pages_start_address));
	warn(fflush(0));
	warn(int k);
	if(mes->pages_start_address)
		warn(k=)munmap((void*)mes->pages_start_address, PAGES_LENGTH);
	
	warn(printf("Exiting thread with, k: %d ** ", k));
	warn(fflush(0));
	
	memset(mes, 0, sizeof(struct ocl_message));
	free(mes);
	
	return (void*)((unsigned long)ret);
}

int main(int argc, char** argv){
	int 	ret,
		i;
	if(argc>2){
		int fid2=open(argv[1],O_RDWR);
	
		struct ocl_message mes;
		mes.type=110;
		mes.kernel_struct=0;
		
		read(fid2, &mes, sizeof(&mes));
	
		return 0;
	}
	
	printf("\nIn The Name of God ** GSFS User Module ** \n");

	memset(&cl, 0, sizeof(cl_struct));
	
	ret=setup_cl_platform(&cl);
	if(ret)
		return -1;
	
	cl.cpu=setup_cl_context(&cl, CL_DEVICE_TYPE_CPU);
	cl.gpu=setup_cl_context(&cl, CL_DEVICE_TYPE_GPU);
	if(cl.cpu==0 && cl.gpu==0)
		return -1;
	
	char* kernelname= "aes_enc";
	if(cl.cpu){
		printf("\nBuilding for CPU ** ");
		ret=build_kernel(cl.cpu, "gsfs_aes_cl.c");
		if(ret)
			goto ret_rel;
	}
	
	if(cl.gpu){
		printf("\nBuilding for GPU ** "); 
		ret=build_kernel(cl.gpu, "gsfs_aes_cl.c");
		if(ret)
			goto ret_rel;
	}
	
	clUnloadCompiler();
	memcpy(cl.kernelname, kernelname, strlen(kernelname));
	cl.kernelname[strlen(kernelname)]=0;

	fid=open(argv[1],O_RDWR);
	
	if(fid<=0){
		printf("\n\n\t\tWrong argument: %s\n\n\n",argv[1]);
		sleep(1);
		warn(fflush(0));
		return -1;
	}
	printf("fid: %d \n", fid);
	
	while(1){
		warn(fflush(0));
		
		void *pages_start=mmap(0, PAGES_LENGTH, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0);
		if(pages_start==MAP_FAILED){
			warn(printf("Unable to mmap, errno: %s\n\n ", strerror(errno)));
			warn(fflush(0));
			
			struct ocl_message mes;
			mes.type=OCL_Is_Damaged;
			mes.kernel_struct=0;
			
			read(fid, &mes, sizeof(&mes));
			
			ret=-1;
			goto func_ret;
		}
		warn(printf("pages_start: %lx ** ", pages_start));
		warn(fflush(0));
		
		struct ocl_message *mes=malloc(sizeof(struct ocl_message));
		memset(mes, 0, sizeof(struct ocl_message));
			
		mes->type=OCL_Is_Ready;
		mes->pages_start_address=(unsigned long)pages_start;
		
		read(fid, mes, sizeof(mes));
		
		if(mes->type==OCL_Exit){
			warn(printf("\nmes->type == OCL_Exit ** "));
			warn(fflush(0));
			
			munmap(pages_start, PAGES_LENGTH);
			
			warn(printf("Exiting "));
			warn(fflush(0));
			
			free(mes);
			
			break;
		}
		
		if(mes->type==OCL_Get_Response){
			warn(printf("new message from kernel with mes->type=OCL_Get_Response"));
			
			pthread_t pth;
			int ptret=pthread_create(&pth, 0, enc, mes);
			
			warn(printf("pth ret: %d\n",ptret));
			
			if(ptret){
				mes->type=OCL_Is_Damaged;
				
				read(fid, mes, sizeof(struct ocl_message));
				
				memset(mes, 0, sizeof(struct ocl_message));
				free(mes);
			}
			
			continue;
		}
		
		warn(printf(" message from kernel with mes->type=%u\n",mes->type));
	}
	
	ret=0;
	
func_ret:	
	close(fid);
	
	printf("\n\n");
	
ret_rel:	
	release_cl(&cl);
	
	warn(fflush(0));
	return ret;
}
