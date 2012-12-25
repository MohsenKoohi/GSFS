enum message_types{
	NEWUSER=0,
	LOGIN=1,
	LOGOUT=2,
	MAKESEC=3,
	ADDUSERS=4,
	REVOKEUSERS=5,
};

struct message{
	pid_t 		pid;
	
	unsigned int 	type;
	
	unsigned int	datalen,
			data2len,
			data3len;
			
	void		*data,
			*data2,
			*data3;
};

enum{
	add_user_response_no_pb_key=1,
	add_user_response_was_added_later=2,
	add_user_response_no_place=3,
	add_user_response_added_successfully=0,	
};

enum ocl_message_types{	
	//Kernel to User
	OCL_Get_Response=6,
	OCL_Exit=7,
	
	//User to Kernel
	OCL_Is_Ready=8,
	OCL_Response_Is_Ready=9,
	OCL_Is_Damaged=10,
};

struct ocl_message{
	unsigned int	type;
	
	unsigned int 	pages_count;
	
	void*		kernel_struct;
	unsigned long	pages_start_address;
	
	unsigned long	IVs_start_address;
	unsigned long	keys_start_address;
	unsigned long	results_start_address;
	
	//unsigned int	mapping_number;
};