In His Sublime Name

GSFS: GPGPU-based Secure File System
Version: 0.51
Tested on linux kernel 2.6.34

Mohsen Koohi Esfahani (koohi@burge.ir)
Burge Computer Lab


GSFS is an open source novel secure filesystem in two cases:
1) GSFS provides integrity and confidentiality of data. This file system enables information sharing
for users by making inodes accessible for different users and also by
enabling users to expand/shrink the access level in each point of tree
structure of inodes.
GSFS uses Crust key revocation method for effective user revocation
and Cryptographic Links of Cryptree to decrease the usage of time-consuming 
public key cryptography algorithm. It uses Galois Counter
Mode(GCM) to provide integrity and confidentiality services for regular secure inodes.
GSFS uses root user public key to make file system integrated and employs 
users public key for confidentiality. In this way it differentiates
confidentiality and integrity.

2) GSFS uses GPGPU for encryption and decryption. It is implemented as a Linux 
kernel module and in the current version, it uses one OpenCL user level program 
to encrypt/decrypt data in parallel with CPU and GPU. We map kernel memory pages 
on this process virtual memory and after the completion of the work, we use
the results in kernel.

This work has been done during my master thesis in IUT under supervision
Dr. Mohammad Ali Montazeri and Dr. Mehdi Berenjkoub.

You can download GSFS using:
git clone git://github.com/MohsenKoohi/GSFS.git

Current Problems:
1)Current context switch between kernel and user level.
2)Currently we have some problems with retreiving all allocated pages.