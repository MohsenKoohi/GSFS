//          In The Name Of GOD
//         GSFS: GPGPU based Secure File System

#include <linux/init.h> 
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/current.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/swap.h>
#include <linux/types.h>
#include <linux/time.h>  
#include <linux/buffer_head.h>
#include <linux/namei.h>
#include <linux/sort.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/vfs.h>
#include <linux/moduleparam.h>
#include <linux/crypto.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/smp_lock.h>
#include <linux/kthread.h>

#define 	GSFS_MAGIC 			0x7a15d4eb95c2f5d9UL
#define 	Block_Size_Bits 		(12)
#define 	Block_Size 			(1UL<<(Block_Size_Bits))
#define 	Block_Size_in_Bits 		(Block_Size<<3)
#define 	Max_File_Size 			(1UL<<36)
#define 	Max_Disk_Size 			(1UL<<44)
#define 	Max_Block_Number 		(1UL<<32)

#define 	Blocks_per_BAT_Block 		(Block_Size<<3)
#define 	Blocks_per_BAT_Block_Bits	(Block_Size_Bits+3)

#define 	Blocks_per_IAT_Block 		(Block_Size>>2)

#define 	Indecis_per_SAT_Block 		(Block_Size<<3)
#define 	Indecis_per_SAT_Block_Bits	(Block_Size_Bits+3)

#define 	Inode_hashes_per_IHP_Block 	(Block_Size>>4)
#define 	Inode_hashes_per_IHP_Block_Bits	(Block_Size_Bits-4)

#define		Sector_Size			(512)
#define		Sector_Size_Bits		(9)
#define		Block_to_Sector(block)		((block)<<(Block_Size_Bits-Sector_Size_Bits))
#define		Sectors_Per_Block		(1<<(Block_Size_Bits-Sector_Size_Bits))

#define		gsfs_hashlen_shift	(4)
#define		gsfs_hashlen		(1<<gsfs_hashlen_shift)
#define		gsfs_hashlen_bits	(gsfs_hashlen<<3)
#define		gsfs_aes_keylen		(16)
#define		gsfs_aes_keylen_bits	(gsfs_aes_keylen<<3)
#define		gsfs_rsalen		(128)
#define		gsfs_rsalen_bits	(gsfs_rsalen<<3)

#define		hash_levels	(3)

#define		fl_hg		(9)			//first-level-hash-groups
#define		sl_hg		(5)
#define		tl_hg		(5)

#define		fl_cn		(fl_hg*sl_hg*tl_hg)	//first-level-child-number
#define		sl_cn		(sl_hg*tl_hg)
#define		tl_cn		(tl_hg)

#define		fl_off		(0)			//first-level-offset
#define		sl_off		(fl_cn)
#define		tl_off		(fl_cn+sl_cn)
#define		root_off	(fl_cn+sl_cn+tl_cn)

#define		hash_root_offset	(root_off<<gsfs_hashlen_shift)

//#define gsfs_test

//#define development
//#define development_super
//#define development_inode

#ifdef gsfs_test
	#define gt(mesg) mesg
#else
	#define gt(mesg)
#endif

#ifdef development
	#define gw(mesg) mesg
#else
	#define gw(mesg) 
#endif

#ifdef development_super
	#define gws(mesg) mesg
#else
	#define gws(mesg) 
#endif

#ifdef development_inode
	#define gwi(mesg) mesg
#else
	#define gwi(mesg) 
#endif

struct GSFS_sb_ondisk{
	unsigned int	total_blocks,
			total_inodes,
			total_sec_indecis,
			
			free_blocks,
			free_inodes,
			free_sec_indecis,
			
			revocation_num;
	
			
	unsigned long	max_file_size,
			disk_size;
	
	unsigned int		last_inode,
				last_block,
				last_sec_index,
				
				bat_start,		//block allocation table start block number
				bat_end,		//block allocation table end block number
				
				iat_start,		//inode block number start block number
				iat_end,		//inode block number end block number
				
				sat_start,		//secure index allocation table start block number
				sat_end,		//secure index allocation table end block number
			
				ihp_start,		//inodes hash pages start
				ihp_end;		//inodes hash pages end

	unsigned int	pb_page;		//public keys page number
	unsigned short	pb_num,			//number of pbkeys
			pb_last;		//last index used in pbkeys page		
			
	unsigned char	root_inode_has_secure_child;
	
	char		public_keys_hash[gsfs_hashlen],
			IAT_hash[gsfs_hashlen],
			BAT_hash[gsfs_hashlen],
			SAT_hash[gsfs_hashlen],
			IHP_hash[gsfs_hashlen],
			sb_hash[gsfs_hashlen];
			
	char		root_signed_super_block_hash[gsfs_rsalen];
};

enum{
	BAT_LRU=0,	//block allocation table
	IAT_LRU=1,	//inode allocation table
	SAT_LRU=2,	//secure index allocation table
	IHP_LRU=3,	//inode hash pages
	LRU_COUNT=4,
};

#define LRU_BH_MAX 30
#define 	bat_bn(block) 		((block)>>Blocks_per_BAT_Block_Bits)	//bat block of the (block)
#define 	bat_offset(block) 	((block)&(Blocks_per_BAT_Block-1))

#define 	iat_bn(block) 		((block)>>(Block_Size_Bits-2)) 		//iat block of the (block)
#define 	iat_offset(block) 	((block)&(Blocks_per_IAT_Block-1))

#define 	sat_bn(in) 		((in)>>Indecis_per_SAT_Block_Bits)	//sat block of the (in)
#define 	sat_offset(in) 		((in)&(Indecis_per_SAT_Block-1))

#define 	ihp_bn(in) 		((in)>>Inode_hashes_per_IHP_Block_Bits)	//ihp block of the (in)
#define 	ihp_offset(in) 		((in)&(Inode_hashes_per_IHP_Block-1))

struct rsa_key;
struct rsacontext;
typedef struct rsacontext rsa_context;

#define MAX_INCOM_INODES 100
struct incom_inodes{
	struct inode* inodes[MAX_INCOM_INODES];
	struct incom_inodes* next;
	int count;
};

#define sgflag_sb_ondisk	(1<<0)
#define sgflag_pb_page		(1<<1)
#define sgflag_IAT		(1<<2)
#define sgflag_BAT		(1<<3)
#define sgflag_SAT		(1<<4)
#define sgflag_IHP		(1<<5)

struct GSFS_sb{
	struct GSFS_sb_ondisk 		gsb_disk;
	
	struct super_block		*sb;
	struct inode			*root_inode;
	unsigned int			sgflags;
	
	//struct rw_semaphore		write_rwsem;
	
	struct rw_semaphore		lru_rwsem[LRU_COUNT];
	struct buffer_head      	*lru_bh[LRU_COUNT][LRU_BH_MAX];
	unsigned long 			lru_time[LRU_COUNT][LRU_BH_MAX];
	sector_t 			lru_bh_number[LRU_COUNT][LRU_BH_MAX];
	short 				lru_count[LRU_COUNT];
	
	struct rw_semaphore		rsa_keys_rwsem;
	struct rsa_key			*first_rsa_key,
					*last_rsa_key;
	
	struct incom_inodes		*incom_inodes;
	struct rw_semaphore		incom_inodes_rwsem;
	
	struct{
		pid_t			gum_pid;
		struct vm_area_struct*	gum_vma;
		unsigned long		gum_start_address;
		void*			gum_ocl_mes;
		
		struct semaphore	gum_struct_sem;
		struct semaphore 	gum_is_ready_sem;
		bool			gum_is_initialized;
		
	} gum_struct;			//gsfs user module struct
	
	#ifdef gsfs_test
		#define max_inode_sems 20
		char inode_sems_test[max_inode_sems];
	#endif
};

#define GSFS_MAX_GROUP 			(448)	//512 byte for other inode fields and 448=(4096-512)/(4+4)
#define GSFS_MAX_INODES_PER_DIRECTORY	(225)
#define GSFS_DEDICATION_ARRAY_LEN	(30)	//>=1+[GSFS_MAX_INODES_PER_DIRECTORY/8]
#define GSFS_DEDICATION_ARRAY_LEN_BITS	(GSFS_DEDICATION_ARRAY_LEN<<3)

//crust.c starts
#define crust_ver_type	unsigned short

#define 	crust_m			(16)		//change m and 4 below parameter concurrently
#define 	crust_m_is_pow_of_2 	(1)
#define 	crust_m_log2		(4)
#define 	crust_d			(4)
#define 	crust_maxver		((1<<16)-1)	//((unsigned int)ppow(crust_m,crust_d)-1)
#define 	crust_keylenbit		(128)
#define 	crust_keylen		(crust_keylenbit>>3)

struct crust_state{
	unsigned char	versions	[crust_d][crust_d];
	unsigned char	keys		[crust_d][crust_keylen];
	unsigned char	count;
};

int crust_get_key_of_state(struct crust_state* ,unsigned int ,unsigned char* );
int crust_get_next_state(struct crust_state*, unsigned int, unsigned char* );
void printkey(unsigned char*);
void printhexstring(unsigned char* hs, char* dest, int len );
void printver(unsigned char*);
//crust.c ends

#define 	gsfs_IV_len		(12)
#define 	gsfs_IV_len_bits	(gsfs_IV_len<<3)
#define 	gsfs_bnh_hash_len	(12)
#define 	gsfs_bnh_hash_len_bits	(gsfs_bnh_hash_len<<3)
#define 	l0_offset(bn)		(bn&127)
#define 	l1_offset(bn)		((bn>>7)&255)
#define 	l2_offset(bn)		((bn>>15)&255)
#define 	l3_offset(bn)		((bn>>23)&1)
#define 	via_len			(sizeof(struct ver_IV_AT))
#define 	vias_per_block		(Block_Size/via_len)

struct ver_IV_AT{
	char 		IV[gsfs_IV_len];
	crust_ver_type 	ver;
	char		ext[2];
	char		AT[gsfs_hashlen];
};

struct bnh{
	unsigned int	blocknumber;
	unsigned char	hash[gsfs_bnh_hash_len];
};

#define 	l0_vias_len	(8)
#define 	l1_bnhs_len	(6)
#define 	l2_bnhs_len	(4)
#define 	l3_bnhs_len	(2)

struct GSFS_inode_disk_inf{
	unsigned int 	ino,
			inlink,
			iblocks,
			ibytes,
			iuid;
	unsigned char	igflags;
			
	umode_t		imode;		//unsigned short
	unsigned long	isize,
			ictime,		//time of last inode change
			imtime;		//time of last file write
			
	unsigned int	parent_ino,
			SAT_index;
	unsigned short	index_in_parent;
	
	union{
		struct{
			struct ver_IV_AT	l0_vias[l0_vias_len];
			struct bnh 		l1_bnhs[l1_bnhs_len];
			struct bnh 		l2_bnhs[l2_bnhs_len];
			struct bnh		l3_bnhs[l3_bnhs_len];
		} reg_inode_security;
		
		struct {
			unsigned int	gdirent_hash_block,
					user_block;
			
			unsigned char 	dir_inode_child_index_dedication_array [GSFS_DEDICATION_ARRAY_LEN],
					dir_inode_sec_has_sec_child_array [GSFS_DEDICATION_ARRAY_LEN],
					//dir_inode_first_level_child_array [GSFS_DEDICATION_ARRAY_LEN],
					child_num,
					sec_has_sec_child_num;
			
			char		inode_user_block_hash[gsfs_hashlen],
					inode_gdirent_hash[gsfs_hashlen];
		} dir_inode_security;
	};
	
	unsigned int	grps_num,
			grps[2*GSFS_MAX_GROUP];
};

struct users{
	spinlock_t	lock;
	unsigned int	*users;
	unsigned char	users_num,
			*writability;
	int 		count;
};

enum{
	GDirent_Hash_Changed_Event=0,
	Crust_Struct_Set_VEvent=1,
	Users_Set_VEvent=2,
	Events_Num=3,
};

#define event_flag_is_present	 (1<<0)
#define event_flag_from_disk	 (1<<1)

struct event{
	unsigned char	flags;
	unsigned int	datalen;
	void		*data;
};

struct child{
	struct inode	*inode;
	unsigned short	index;
	struct event	events[Events_Num];
};

//avl_tree.c	starts
#define __AVL_SETTINGS
	
#define avl_data_type		struct child*
#define avl_data_compare(x,y)	((y->index)-(x->index))
#define avl_data_free(x)	free_child(x)	
#define avl_search_compare(x,y)	((y)-(x->index))
#define	avl_search_input_type	unsigned short

struct avl_tree_node{
	struct avl_tree_node	//*parent,
				*left,
				*right;
	avl_data_type data;
};

typedef struct avl_tree_node atn;
	
atn* avl_tree_search(atn* node,avl_search_input_type data);
atn* avl_tree_insert(atn* node,avl_data_type data);
void avl_tree_free(atn*);
void print_avl_tree(atn*);
int avl_tree_get_size(atn*);
int avl_tree_get_all_nodes(atn* root,atn** res,int res_len);
//avl_tree.c	ends

#define		inode_integrity_array_len_bits	(1<<(Block_Size_Bits-gsfs_hashlen_shift))
#define		inode_integrity_array_len_bytes	(inode_integrity_array_len_bits>>3)

enum{
	//Metadata_Integrity=0,
	GDirent_Integrity=1,
};

typedef struct{
	spinlock_t 		lock;
	
	struct crust_state 	crust_state;
	crust_ver_type		max_ver;
	
	unsigned int		user_block;
	char*			owner_key;
	
	unsigned int 		count;
}crust_struct;

#define 	key_lru_num		(10)

#define		sri_flag_bnh_on_bh	(1<<0)		//for    l1,l2 active bnh
#define		sri_flag_bh_changed	(1<<1)		//for l0,l1,l2 bh

typedef struct{
	unsigned int	l0_bh_num,
			l1_bh_num,
			l2_bh_num;
			
	int		active_l1,
			active_l2,
			active_l3;
	
	struct buffer_head	*l0_bh,
				*l1_bh,
				*l2_bh;
	
	struct bnh	*l1_active_bnh,
			*l2_active_bnh,
			*l3_active_bnh;
	
	char		l0_flags,
			l1_flags,
			l2_flags;
			
	struct rw_semaphore	sri_via_rwsem;
			
	unsigned long		key_lru_time[key_lru_num];
	char			key_lru_key[key_lru_num][gsfs_aes_keylen];
	crust_ver_type		key_lru_ver[key_lru_num];
	
}sri_struct;			//Secure Regular Inode 

#define sri_len		(sizeof(sri_struct))

struct GSFS_inode{
	struct GSFS_inode_disk_inf 	disk_info;
				
	int			current_grp;
	unsigned int		current_start,
				current_end,
				start_bn,
				inode_bnr;
	
	struct 	users		*users;
	unsigned short		igflags;
	
	crust_struct		*inode_crust_struct;	
	
	struct avl_tree_node	*children;
	
	//when number of children levels increases kernel cann't track their hierarchy
	//therefore we remove parent field of inode and we use GSFS_get_inode beyond this field
	//struct inode		*parent;
	
	struct rw_semaphore	inode_rwsem;
				
	char			children_gdirent_hash_integrity[inode_integrity_array_len_bytes];
	
	int			(*add_event_to_parent)(struct inode*, unsigned char type, void* data, unsigned int datalen, char par_get_sem);
	
	struct buffer_head	*inode_bh;
	
	sri_struct		*sri;
};

#define		igflag_dir			(1<<0)
#define		igflag_secure			(1<<1)
#define		igflag_has_sec_child		(1<<2)
#define		igflag_first_level_sec_inode	(1<<3)
#define		igflag_encrypted_inl		(1<<4)
#define		igflag_active_parent_link	(1<<5)

#define		igflag_active_uid		igflag_secure
#define		igflag_active_user_block	igflag_first_level_sec_inode

#define 	igflag_inode_metadata_changed	(1<<8)
//#define 	igflag_present_owner_key	(1<<9)
#define 	igflag_incomplete_inode		(1<<9)

#define	igflag_ondisk(p)	(p&0xff)

#define 	gsfs_dirent_len 	sizeof(struct GSFS_dirent)
#define 	GSFS_MAX_NAME_LEN 	(58)
#define		gsfs_inl_len		(4+GSFS_MAX_NAME_LEN+1+1)

struct gdirent_inl{
	unsigned int 	ino;
	char		name [GSFS_MAX_NAME_LEN+1];
	unsigned char	len;
};

struct GSFS_dirent{
	struct gdirent_inl	gd_inl;
	
	#define		gd_ino 		gd_inl.ino
	#define 	gd_name 	gd_inl.name
	#define		gd_len		gd_inl.len
	
	unsigned char	gd_flags;
			
	crust_ver_type		gd_dirent_inl_ver;		//inl= ino - name - len
			
	struct{
		unsigned int	gd_user_block;
		char		gd_user_block_hash[gsfs_hashlen];
	} gd_first_dir_security_fields;
};

struct inode_user_key{
	uid_t	uid;
	char	writability;
	char	rsa_encrypted_key[gsfs_rsalen];
};

#define GSFS_MAX_USERS_PER_INODE (28)	//(4096-4-16)/(4+4+128)-1

struct inode_user_page{
	unsigned int 		num;
	crust_ver_type		max_ver;
	
	struct inode_user_key	owner_key;
	char			crust_hash[gsfs_hashlen];
	
	struct crust_state	crust_state_link;
	crust_ver_type		parent_cs_ver_for_cs_link;
	
	struct inode_user_key 	users_key[GSFS_MAX_USERS_PER_INODE];
};

#define		spflag_aux_page_is_ready			(1<<0)
#define		spflag_key_is_ready				(1<<1)
#define 	spflag_page_is_ready_for_read			(1<<2)
#define 	spflag_page_is_authenticated			(1<<3)
#define		spflag_page_is_refused_in_first_round_of_read	(1<<4)

struct sec_inode_page{
	struct ver_IV_AT 	*via;
	
	struct page		*aux_page;
	
	unsigned char		spflags;
	
	char			key[gsfs_aes_keylen];
};

#define sip_len (sizeof(struct sec_inode_page))

#define pflag_page_is_added_to_pagecache	(1<<0)

struct GSFS_page{
	union{
		struct 	sec_inode_page	*sip;			//for origin_page
		struct 	page		*origin_page;		//for aux_page
	};
	
	unsigned int	disk_bnr;
	
	unsigned char	flags;
};

typedef struct{
	struct semaphore	sem;
	int 			ret;
	
	unsigned int		waiters_num;
	atomic_t		waiters_returned;
	struct semaphore	wanu_sem;
	
	struct page		**results,
				**IVs,
				**keys;
	unsigned int		results_count,
				IVs_pages_count;
}OCL_kernel_struct;

#define gp_len	sizeof(struct GSFS_page)

//super.c
int GSFS_sync_fs(struct super_block* , int );
void GSFS_put_super(struct super_block*);
void GSFS_write_super(struct super_block*);
int GSFS_statfs (struct dentry * , struct kstatfs* );
int GSFS_create_disk(struct super_block* );
int GSFS_fill_sb(struct super_block *, void* , int );
int GSFS_get_sb( struct file_system_type *, int , const char* ,  void* , struct vfsmount *);
int write_one_bh_dev(struct buffer_head*);
struct buffer_head* read_one_bh_dev(struct block_device* , sector_t );
struct page * process_virt_to_page(struct mm_struct* , unsigned long );
extern struct super_operations GSFS_super_operations;

#ifdef gsfs_test
	void printsemkeys(char* dest, struct GSFS_sb* gsb);
	//#define gsfs_down_write(p, i, gsb)	{down_write(p); if(i<max_inode_sems) gsb->inode_sems_test[i]=1;}
	//#define gsfs_up_write(p, i, gsb)	{up_write(p); if(i<max_inode_sems) gsb->inode_sems_test[i]=0;}
#else
	//#define gsfs_down_write(p, i, gsb)	down_write(p)
	//#define gsfs_up_write(p, i, gsb)	up_write(p)
#endif

//users.c
int GSFS_add_new_user(struct super_block* ,rsa_context*,uid_t );
int GSFS_user_logout(struct super_block*,uid_t);
int GSFS_make_sec(struct super_block*,char*);
int GSFS_user_login(struct super_block* sb,rsa_context*rsa,uid_t uid, char* pt, int ptlen, 
		    char* ct, int ctlen );
int GSFS_add_users_to_inode(struct super_block* sb, char* dest, unsigned int* uids, 
				unsigned int* writes, int num);
int GSFS_revoke_users(struct super_block* sb, char* dest, unsigned int* uids, 
				int* rets, int num);

//lru.c
void initialize_lru(struct GSFS_sb *);
struct buffer_head* get_lru_bh(struct GSFS_sb* ,unsigned char,sector_t );
void exit_lru(struct GSFS_sb *);
void sync_lru(struct GSFS_sb *);

void set_BAT(struct GSFS_sb* , sector_t from , sector_t to);
void clear_BAT(struct GSFS_sb* , sector_t from, sector_t to);
int BAT_clear_one_block(struct GSFS_sb* gsb, unsigned int bn);
int BAT_get_some_blocks(struct GSFS_sb* ,unsigned int ,unsigned int* );

void set_IAT(struct GSFS_sb* , unsigned int , sector_t );
void clear_IAT(struct GSFS_sb* , unsigned int);
unsigned int IAT_get_one_inode(struct GSFS_sb*, unsigned int );

inline unsigned int SAT_get_one_index(struct GSFS_sb* gsb);
int SAT_clear_one_index(struct GSFS_sb* gsb, unsigned int index);
int test_one_SAT_index(struct GSFS_sb* gsb, unsigned int index);

inline int set_IHP(struct GSFS_sb* gsb, unsigned int index, char* hash);
inline int get_IHP(struct GSFS_sb* gsb, unsigned int index, char* hash);

int GSFS_add_new_private_key(struct super_block* sb,rsa_context* user_rsa,uid_t uid, char* pt, 
			     int ptlen, char* ct, int ctlen);
int add_new_public_key(struct GSFS_sb*,rsa_context*,uid_t);
struct rsa_key* get_rsa_key(struct GSFS_sb*,uid_t, char private);
int GSFS_remove_private_key(struct super_block*,uid_t);
void free_all_rsa_keys(struct GSFS_sb*);

//inode.c
inline int set_one_index(unsigned char* array, unsigned int index, unsigned int max);
inline int is_set_one_index(unsigned char* array, unsigned int index, unsigned int max);
int set_and_get_one_index(unsigned char *array, unsigned int max);
int get_all_set_indecis(unsigned char* array, unsigned int* res, unsigned int max);
struct users* get_users(struct users* u);
void add_child_to_parent(struct inode* pin, struct inode* in);
void put_users(struct users*u);
crust_struct* get_crust_struct(crust_struct *cs);
void put_crust_struct(crust_struct *cs);
void set_inode_parameters(struct inode* ,unsigned short );
int GSFS_write_inode(struct inode *, struct writeback_control *);
int write_inode_to_disk(struct inode *in, int do_sync);
void GSFS_clear_inode(struct inode* );
void GSFS_destroy_inode(struct inode* );
void GSFS_delete_inode(struct inode* );
void GSFS_truncate(struct inode* );
struct inode* GSFS_get_inode(struct GSFS_sb* , unsigned int );
int set_and_get_one_index(unsigned char *, unsigned int);
sector_t get_dp_bn_of_in(struct inode*, unsigned int);
unsigned int get_inode_by_name(struct inode* , char* name,int );
unsigned int get_block_number_of_inode(struct GSFS_sb* , unsigned int );
extern struct inode_operations GSFS_inode_operations;
struct inode* GSFS_get_new_locked_inode_and_add_its_link(struct GSFS_sb* gsb, struct inode* parent,
							 struct dentry* dent, unsigned char flags);
int add_some_blocks_to_inode(struct inode* ,int );
int verify_and_get_integrity_for_child(unsigned char type,struct inode*in, unsigned short index, char* dest);
int get_inl_key_for_gdirent(struct inode* dir,struct GSFS_dirent* gd,unsigned short index,char* destkey);
int traverse_all_gdirents_for_gibn_or_rd(struct inode* in, 
					 loff_t* fpos, void* dirent, filldir_t filldir, 
					 char* name, int len);
int read_owner_key_for_crust_struct(crust_struct* cs, struct super_block* sb, char* ubhash);
void add_inode_to_incom_inodes(struct GSFS_sb*, struct inode*);
void free_incom_inodes(struct GSFS_sb*);
void get_inode_for_incom_inodes_of_uid(struct GSFS_sb*, uid_t);
void delete_inode_from_incom_inodes(struct GSFS_sb* , struct inode* );
int get_inode_users_and_or_crust_state_from_parent(struct inode	* pin,		unsigned short childindex, 
						crust_struct	** dest_crust,	struct users** dest_users, 
						unsigned int user_block_num,	char* user_block_hash, 
						unsigned char flags, 		int input_ret1,
						int crust_can_get_event, int users_can_get_event);
int get_users_and_decrypt_inl_of_gdirent(struct inode* dir,struct GSFS_dirent* gd,unsigned short index,
					struct users** users, struct gdirent_inl *inl, 
					char crust_can_get_event, char users_can_get_event);

#ifdef gsfs_test
	void clear_test_indecis(void);
#endif

//file.c
int GSFS_readdir (struct file * , void * , filldir_t );
extern struct file_operations GSFS_dir_fops;
extern struct file_operations GSFS_file_fops;
int GSFS_file_open(struct inode *, struct file *);
ssize_t GSFS_file_read (struct file *, char __user *, size_t , loff_t *);

//pagecache.c
unsigned int get_some_pages(unsigned int ,struct page **);
int GSFS_readpages(struct file *, struct address_space *,struct list_head*, unsigned);
void GSFS_put_data_page_of_inode(struct inode*, struct page*);
struct page* GSFS_get_data_page_of_inode_with_read(struct inode* , unsigned int );
int GSFS_get_data_pages_of_inode(struct inode*, unsigned int[],unsigned int ,struct page**, int odirect );
struct page* GSFS_get_locked_data_page_of_inode_without_read(struct inode* ,unsigned int);
extern struct address_space_operations GSFS_address_space_operations;
int GSFS_writepages(struct address_space *mapping, struct writeback_control *wbc);

//skein512.c
int skein512(size_t hashbit_len ,const unsigned char* data,size_t databit_len,unsigned char *hashval);

//aes.c
typedef struct
{
    unsigned int erk[64];     /* encryption round keys */
    unsigned int drk[64];     /* decryption round keys */
    int nr;             /* number of rounds */
}
aes_context;

int  aes_set_key( aes_context *, unsigned char *, int  );
void aes_encrypt( aes_context *, unsigned char [16], unsigned char [16] );
void aes_decrypt( aes_context *, unsigned char [16], unsigned char [16] );

//rsa.c
typedef unsigned long t_int;
typedef unsigned int t_dbl __attribute__((mode(TI)));

typedef struct{
    int s;              /*!<  int sign      */
    int n;              /*!<  total # of limbs  */
    t_int *p;           /*!<  pointer to limbs  */
} mpi;

void mpi_init( mpi *, ... );
void mpi_free( mpi *, ... );
int mpi_copy( mpi *, const mpi * );
int mpi_copy_fu( mpi *, const mpi * );
int mpi_copy_fu2( mpi *, const mpi * );

struct rsacontext{
    int ver;                    /*!<  always 0          */
    int len;                    /*!<  size(N) in char s  */

    mpi N;                      /*!<  public modulus    */
    mpi E;                      /*!<  public exponent   */

    mpi D;                      /*!<  private exponent  */
    mpi P;                      /*!<  1st prime factor  */
    mpi Q;                      /*!<  2nd prime factor  */
    mpi DP;                     /*!<  D % (P - 1)       */
    mpi DQ;                     /*!<  D % (Q - 1)       */
    mpi QP;                     /*!<  1 / (Q % P)       */

    mpi RN;                     /*!<  cached R^2 mod N  */
    mpi RP;                     /*!<  cached R^2 mod P  */
    mpi RQ;                     /*!<  cached R^2 mod Q  */

    int padding;                /*!<  1.5 or OAEP/PSS   */
    int hash_id;                /*!<  hash identifier   */
};

void rsa_1024_init(void);
void rsa_init( rsa_context *);
void rsa_free( rsa_context *);

#define RSA_PUBLIC      0
#define RSA_PRIVATE     1
//mode = RSA_PRIVATE or RSA_PUBLIC
int rsa_1024_encrypt(rsa_context* ,int mode, int ilen,  const unsigned char* input,unsigned char* output);
int rsa_1024_decrypt(rsa_context* ,int mode, int* olen, const unsigned char* input,unsigned char *output,int max_olen);

typedef struct{
	int s;
	int n;
	t_int p[16];
}mpi_on_disk;

struct  on_disk_public_key{
	int 	len;
	mpi_on_disk	N,
			E;
};

struct on_disk_user_info{
	uid_t	uid;
	struct	on_disk_public_key user_pbk;
};

struct rsa_key{
	spinlock_t	lock;
	rsa_context*	key;
	struct rsa_key*	next;
	uid_t		uid;
	unsigned char	is_private;
};

struct pb_page_entry{
	uid_t uid;
	unsigned int block;
};

#define total_users (Block_Size/(sizeof(struct pb_page_entry)))

//cdev.c
int GSFS_chardev_init(struct super_block*);
void GSFS_chardev_exit(void);
void GSFS_gum_exit(void);
OCL_kernel_struct* gum_get_gctr_pages(struct super_block* sb, struct page ** IVs, 
				     struct page ** keys, struct page ** res, unsigned int count);

//hash.c
inline int get_IAT_hash(struct GSFS_sb* gsb, char* hashval);
inline int get_BAT_hash(struct GSFS_sb* gsb, char* hashval);
inline int get_SAT_hash(struct GSFS_sb* gsb, char* hashval);
inline int get_IHP_hash(struct GSFS_sb* gsb, char* hashval);
int get_pb_page_hash(struct GSFS_sb* gsb,char* hashval);
inline int get_sb_hash(struct GSFS_sb* gsb,char* hashval);
int get_hash_of_sequential_pages(struct super_block *sb, unsigned int start, unsigned int end,char* hashval);
int get_hash_of_non_sequential_pages(struct super_block *sb, unsigned int * blocks, unsigned int num,char* hashval);
int get_gdirent_hash(char* dest,struct GSFS_dirent* gd);
int update_hash_block_to_root(char* page, unsigned short *changes,unsigned short ch_len);
int get_user_block_hash(char* dest, char* page);
int get_inode_metadata_hash_for_parent(struct inode* in, char* dest);
int verify_hash_integrity(char* page,char* integ_arr,unsigned short index,char* dest,char* verifid_root);
inline int get_crust_hash(char* dest, struct crust_state* crust);
inline int get_bnh_page_hash(char* dest, char* bnh_page);

//gcm.c
int inc_IV(char *);
inline int get_j0(char* j0, char* IV);
int get_gctr_page(char* apd, char* key, char* IV);
int get_gctr_page_and_j0(char* apd, char* j0, char* key, char* IV);
int get_AT(char* AT, char* apd, char* key, char* j0);

//reg_inode.c
int update_sec_reg_inode_hash_blocks(struct inode* in,unsigned int start_bn,unsigned int len);
void delete_sec_reg_inode_hash_blocks(struct inode* in);
inline int get_blocks_via(struct inode *in, unsigned int *sorted_blocks, struct ver_IV_AT **vias,
			  int *res, int len);
inline int set_blocks_via(struct inode *in, unsigned int *sorted_blocks, struct ver_IV_AT **vias,
			  int *res, int len);
//cipher.c
#define encrypt_type	1
#define decrypt_type	2

int aes_ecb_enc_dec(char* dest, char* src, char* key, int type, int len);
int encrypt_crust_state(struct crust_state* dest_ct, struct crust_state* src_pt, struct crust_state* key_cs, crust_ver_type ver);
int decrypt_crust_state(struct crust_state* dest_pt, struct crust_state* src_ct, struct crust_state* key_cs, crust_ver_type ver);
int encrypt_owner_key(char* dest_link, char* src_pt, char* key);
int decrypt_owner_key(char* dest_pt, char* src_link, char* key);
int rsa_encrypt_crust_state_for_user_block(char* dest, struct crust_state* src, rsa_context* key);
int rsa_decrypt_crust_state_from_user_block(struct crust_state* dest, char* src, rsa_context* key);
int rsa_encrypt_owner_key_for_user_block(char* dest, char* src, rsa_context* key);
int rsa_decrypt_owner_key_from_user_block(char* dest, char* src, rsa_context* key);
int encrypt_inl(struct gdirent_inl* dest,struct gdirent_inl* src,char* key);
int decrypt_inl(struct gdirent_inl* dest,struct gdirent_inl* src,char* key);

//events.c
void free_child(struct child* );
int add_event_to_inode( struct inode* inode, unsigned short child_index, unsigned char type, void* data, unsigned int datalen, unsigned char flags);
int general_add_event_to_parent(struct inode* in, unsigned char type, void* data, unsigned int len, char get_sem);
int get_event(struct GSFS_inode* inf, unsigned short child_index,unsigned char type, void* data, unsigned int* len);
inline void free_one_present_event(struct event* ev, unsigned char type);