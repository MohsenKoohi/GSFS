#include "gsfs.h"

int aes_enc_dec(char* dest, char* src, char* key, int type){
	struct crypto_cipher *cc;
	
	cc=crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
	if(!cc)
		return -1;
	
	crypto_cipher_setkey(cc,key,gsfs_aes_keylen);
	
	if(type == encrypt_type)
		crypto_cipher_encrypt_one(cc, dest, src);
	else
		crypto_cipher_decrypt_one(cc, dest, src);
	
	crypto_free_cipher(cc);
	
	return 0;
}

int aes_ecb_enc_dec(char* dest, char* src, char* key, int type, int len){
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	struct scatterlist 	sgd,
				sgs;
	int ret;
	
	tfm = crypto_alloc_blkcipher("ecb(aes)", 0, 0);
	if(!tfm)
		return -1;
	
	desc.tfm = tfm;
	desc.flags = 0;

	ret=crypto_blkcipher_setkey(tfm, key, gsfs_aes_keylen);
	if(ret){
		//printk("<0>" "setkey_error\n");
		return -1;
	}
	
	sg_init_one(&sgd, dest, len);
	sg_init_one(&sgs, src, len);
	
	/*
	ret = crypto_blkcipher_ivsize(tfm);
	if(ret) 
		crypto_blkcipher_set_iv(tfm, key, gsfs_aes_keylen);
	printk("<0>" "ivsize:%d\n",ret);
	*/
	
	if(type==encrypt_type)
		ret = crypto_blkcipher_encrypt(&desc, &sgd, &sgs, len);
	else
		ret = crypto_blkcipher_decrypt(&desc, &sgd, &sgs, len);
	
	//printk("<0>" "ret from enc_dec:%d\n",ret);
	
	crypto_free_blkcipher(tfm);
	
	return 0;
}

int aes_enc_dec_with_crust_state(char* dest, char* src, struct crust_state* key_cs, crust_ver_type ver, 
				 int type, int len){
	unsigned char key[gsfs_aes_keylen];
	int ret;
	if(crust_get_key_of_state(key_cs, ver, key))
		return -1;
	
	ret=aes_ecb_enc_dec(dest, src, key, type, len);
	
	memset(key, 0, gsfs_aes_keylen);
	
	return ret;
}

inline int encrypt_crust_state(struct crust_state* dest_ct, struct crust_state* src_pt, struct crust_state* key_cs, crust_ver_type ver){
	int ret;
	
	ret=aes_enc_dec_with_crust_state((char*)dest_ct, (char*)src_pt, key_cs, ver, encrypt_type, 80);
	dest_ct->count=src_pt->count;
	
	return ret;
}

inline int decrypt_crust_state(struct crust_state* dest_pt, struct crust_state* src_ct, struct crust_state* key_cs, crust_ver_type ver){
	int ret;
	
	ret=aes_enc_dec_with_crust_state((char*)dest_pt, (char*)src_ct, key_cs, ver, decrypt_type, 80);
	dest_pt->count=src_ct->count;
	
	return ret;
}

inline int encrypt_owner_key(char* dest_link, char* src_pt, char* key){

	return aes_enc_dec(dest_link, src_pt, key, encrypt_type);
}

inline int decrypt_owner_key(char* dest_pt, char* src_link, char* key){
	
	return aes_enc_dec(dest_pt, src_link, key, decrypt_type);
}

inline int rsa_decrypt_crust_state_from_user_block(struct crust_state* dest, char* src, rsa_context* key){
	int 	ret,
		olen;
	char out[gsfs_rsalen];
	
	ret= rsa_1024_decrypt(key, RSA_PRIVATE, &olen, src, out, gsfs_rsalen);
	
	if(!ret)
		memcpy(dest, out, olen);
	
	memset(out, 0, gsfs_rsalen);
	
	//printk("<0>" "olen:%d ret:%d\n",olen,ret);
	
	return ret;
}

inline int rsa_encrypt_crust_state_for_user_block(char* dest, struct crust_state* src, rsa_context* key){
	int ret;
	
	ret=rsa_1024_encrypt(key, RSA_PUBLIC, sizeof(struct crust_state), (char*)src, dest);
	
	return ret;
}

inline int rsa_decrypt_owner_key_from_user_block(char* dest, char* src, rsa_context* key){
	int 	ret,
		olen;
	char out[gsfs_rsalen];
	
	ret= rsa_1024_decrypt(key, RSA_PRIVATE, &olen, src, out, gsfs_rsalen);

	if(!ret)
		memcpy(dest, out, olen);
	
	memset(out, 0, gsfs_rsalen);
	
	return ret;
}

inline int rsa_encrypt_owner_key_for_user_block(char* dest, char* src, rsa_context* key){
	int ret;
	
	ret=rsa_1024_encrypt(key, RSA_PUBLIC, gsfs_aes_keylen, (char*)src, dest);
	
	return ret;
}

inline int decrypt_inl(struct gdirent_inl* dest,struct gdirent_inl* src,char* key){
	
	return aes_ecb_enc_dec((char*)dest, (char*)src, key, decrypt_type, 64);
}

inline int encrypt_inl(struct gdirent_inl* dest,struct gdirent_inl* src,char* key){
	
	return aes_ecb_enc_dec((char*)dest, (char*)src, key, encrypt_type, 64);
}
