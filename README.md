
[GSFS: GPGPU-based Secure File System](http://www.burge.pro/category-2/GSFS)

![GSFS](http://burge.pro/upload/cat-2-GSFS/slider_gsfs.jpg)

Burge Computer Lab

* Version: 0.51
* 16K LoC
* Tested on Linux kernel 2.6.34

##Features
GSFS is an open source (with GNU GPLv3 license) novel secure file system :
* GSFS provides integrity and confidentiality of data. This file system enables information sharing
for users by making inodes accessible for different users and also by
enabling users to expand/shrink the access level in each point of tree
structure of inodes.
* GSFS uses "Crust"[1] key revocation method for effective user revocation
and "Cryptographic Links" of "Cryptree"[2] to decrease the usage of time-consuming 
public key cryptography algorithm. It uses "Galois Counter Mode(GCM)"[3] 
to provide integrity and confidentiality services for regular secure inodes.
GSFS uses root user public key to make file system integrated and employs 
users public key for confidentiality. In this way it differentiates
confidentiality and integrity.
* GSFS uses GPGPU for encryption and decryption. It is implemented as a Linux 
kernel module and in current version, it uses one OpenCL user level program 
to encrypt/decrypt data in parallel with CPU and GPU. We map kernel memory pages 
on this process virtual memory and after the completion of the work, we use
the results in kernel. 

##Current Problems
* Context switch between kernel and user level.
* We have some problems with retreiving all allocated pages.

##Note
* Our "rsa.c" file is adopted from PolarSSL package "http://polarssl.org/" and we have only 
ported their work in to kernel mode for our usage.
* Our "skein512.c" file is adopted from "http://www.skein-hash.info/".
* "aes.c" is "Christophe Devine's AES" and we dont' use it. Instead we use kernel's aes, see cipher.c.

##References
[1] Erel Geron and Avishai Wool. Crust: cryptographic remote untrusted storage without 
public keys. Int. J. Inf. Sec., 8(5):357–377, 2009.

[2] Dominik Grolimund, Luzius Meisser, Stefan Schmid, and Roger Wattenhofer. 
Cryptree: A folder tree structure for cryptographic file systems. In SRDS, pages 189–198, 2006.

[3]David A. McGrew and John Viega. The galois/counter mode of operation. 
http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-spec.pdf, 2005.

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.