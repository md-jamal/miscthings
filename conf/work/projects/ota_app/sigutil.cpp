/*************************************************************************
	> File Name: createsig.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: 2018年06月11日 星期一 21时29分38秒
 ************************************************************************/

// need to be careful of memory new and free when handle error

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef __cplusplus
}
#endif

#include "sigutil.h"
#include "debug.h"

#define READSIZ 1*1024*1024

/*
 * readin a file content, calculate sha256sum
 * fname: input file name
 * result: output sha256sum result
 * return value:
 *		0 success
 *		-1 fail
 */
int SigUtil::get_sha256_from_file(const char *fname, unsigned char* result, unsigned long start, unsigned long end) {
	SHA256_CTX ctx;
	SHA256_Init(&ctx);

	int fd = open(fname, O_RDONLY);
	if (fd == -1) {
		pr_info("can not open file %s\n", fname);
		return -1;
	}

	unsigned char tmpbuf[READSIZ];
    int count = 0;
    unsigned long sum, alreadydone = 0;

    if (end == 0L) {
        sum = lseek(fd, 0L, SEEK_END);
    } else {
        sum = end - start;
    }

    lseek(fd, start, SEEK_SET);
    while (alreadydone < sum ) {
        unsigned long  need_read = std::min((unsigned long)READSIZ, sum - alreadydone);
        if ((unsigned long)read(fd, tmpbuf, need_read) == need_read) {
            SHA256_Update(&ctx, tmpbuf, count);
        } else {
            pr_info("can not calculate sha\n");
            return -1;
        }
        alreadydone += need_read;
    }
	SHA256_Final(result, &ctx);
	close(fd);
	return 0;
}


// caller should free buffer allocated inside this function
int SigUtil::readkeyfromfile(const char* fname, char** bufptr) {
	int fd, count;
	char *buf = NULL;
	// read privkey and sign
	if ((fd = open(fname, O_RDONLY)) == -1) {
		pr_info("can not open file %s for key read\n", fname);
		return -1;
	}
	count = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	buf = (char*)malloc(count);
	if (buf == NULL) {
		pr_info("can not alloc mem for file %s key read\n", fname);
		return -1;
	}
	if (read(fd, buf, count) != count) {
		pr_info("can not read from file %s\n", fname);
		return -1;
	}
	*bufptr = buf;
	return count;
}

int SigUtil::readpriveckey(EC_KEY **priveckeyptr, const char *privkeypath, const char*pubkeypath) {
	EC_KEY *priveckey = NULL;
	EC_POINT *pubkey = NULL;
	char *keybuf = NULL;

	pr_info("begin read privkey\n");
	priveckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (priveckey == NULL) {
		pr_info("can not create curve for priveckey\n");
		return -1;
	}

	// read privkey and sign
	if (readkeyfromfile(privkeypath, &keybuf) == -1) {
		pr_info("can not read privkey\n");
		return -1;
	}
	BIGNUM *privbignum = NULL;
	if (BN_hex2bn(&privbignum, keybuf) == 0) {
		pr_info("create big num fail\n");
		return 0;
	}
	free(keybuf);


	if (EC_KEY_set_private_key(priveckey, privbignum) != 1) {
		pr_info("can not set private key\n");
		return -1;
	}

	// also read pubkey, set pubkey into privkey
	if (readkeyfromfile(pubkeypath, &keybuf) == -1) {
		pr_info("can not read pubkey\n");
		return -1;
	}
	if ((pubkey = EC_POINT_hex2point(EC_KEY_get0_group(priveckey), keybuf, pubkey, NULL)) == NULL) {
		pr_info("can not convert from hex to ecpoint\n");
		return -1;
	}
	free(keybuf);

	if (EC_KEY_set_public_key(priveckey, pubkey) != 1) {
		pr_info("can not set public eckey into privkey\n");
		return -1;
	}

	*priveckeyptr = priveckey;
	return 0;
}

int SigUtil::readpubeckey(EC_KEY **pubeckeyptr, const char* pubkeypath) {
	EC_POINT *pubkey = NULL;
	EC_KEY *pubeckey = NULL;
	char *keybuf = NULL;

	// read pubkey and verify
	pr_info("begin read pubkey\n");
	pubeckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (pubeckey == NULL) {
		pr_info("can not create curve for pubeckey\n");
		return -1;
	}
	// also read pubkey, set pubkey into privkey
	if (readkeyfromfile(pubkeypath, &keybuf) == -1) {
		pr_info("can not read pubkey\n");
		return -1;
	}
	if ((pubkey = EC_POINT_hex2point(EC_KEY_get0_group(pubeckey), keybuf, pubkey, NULL)) == NULL) {
		pr_info("can not convert from hex to ecpoint\n");
		return -1;
	}
	free(keybuf);

	if (EC_KEY_set_public_key(pubeckey, pubkey) != 1) {
		pr_info("can not set public eckey\n");
		return -1;
	}
	*pubeckeyptr = pubeckey;
	return 0;
}

/*
 * passin NULL siglen poiner and return size of signature needed
 *
 * if success, return 0
 *	  error, return -1
 */
int SigUtil::ecdsa_sign(unsigned char *content, int contentlen, unsigned char* sig, unsigned int *siglenptr,
		const char *priveckeyname, const char *pubeckeyname) {

	EC_KEY *priveckey;

	// if private or public key file not exist, regenerate keys and store into file
	if (access((const char*)priveckeyname, F_OK) != 0 || access((const char*)pubeckeyname, F_OK) != 0) {
		if (generate_new_keypairs(priveckeyname, pubeckeyname) != 0) {
			pr_info("can not generate key pair\n");
			return -1;
		}
	}

	if (readpriveckey(&priveckey, priveckeyname, pubeckeyname) != 0) {
		pr_info("can not read priveckey\n");
		return -1;
	}

	// return size needed
	if (siglenptr == NULL || sig == NULL) {
		return ECDSA_size(priveckey);
	}

	//sign
	if (!ECDSA_sign(0, content, contentlen, sig, siglenptr, priveckey)) {
		pr_info("can not generate sig\n");
		return -1;
	}
	pr_info("sig length is %d, contentlen is %d\n", *siglenptr, contentlen);

	EC_KEY_free(priveckey);
	return 0;
}


/*
 * verify
 * return:
 *		-1 error, 0 verify fail, 1 success
 */
int SigUtil::ecdsa_verify(unsigned char *content, int contentlen, unsigned char* sig, unsigned int siglen, const char *pubeckeyname) {
	EC_KEY *pubeckey;
	int ret;

	// do a checkarg here
	if (sig == NULL) {
		pr_info("can not get sig\n");
		return -1;
	}

	// read public key
	if (readpubeckey(&pubeckey, pubeckeyname) != 0) {
		pr_info("can not read pubeckey\n");
		return -1;
	}

	// do verify
	ret = ECDSA_verify(0, content, contentlen, sig, siglen, pubeckey);
	if (ret == -1) {
		pr_info("verify error\n");
	} else if (ret == 0) {
		pr_info("verify fail, wrong sig\n");
	} else {
		pr_info("verify success\n");
	}
	EC_KEY_free(pubeckey);
	return ret;
}

// we preread pubkey into buffer ,so just passin the buffer here
bool SigUtil::verify(const char * fname, const char *pubkeypath) {
	unsigned char content[SHA256_DIGEST_LENGTH];
	int res = 0;
	bool ret = true;
    unsigned int siglen;
    unsigned char sig[128];
    unsigned long datastart, datalen;

    if (split_and_getsig(fname, &datastart, &datalen, sig, &siglen) != 0) {
        pr_info("can not split and get sig and data offset\n");
        return false;
    }
    pr_info("datastart %lu, len %lu, siglen %u\n", datastart, datalen, siglen);

    //split file into two membuf, second one is sig, siglen, firs one is databuf, datalen
    if (get_sha256_from_file(fname, content, datastart, datastart + datalen) == -1) {
		pr_info("can not get sha\n");
        return false;
	}
	
    res = ecdsa_verify(content, SHA256_DIGEST_LENGTH, sig, siglen, pubkeypath);
	switch(res) {
	case -1:
	  ret = false;
	  break;
	case 0:
	  ret = false;
	  break;
	default:
	  ret = true;
	  break;
	}

    return ret;
}

 int SigUtil::sign(const char* fname, unsigned char* sig, unsigned int *siglenptr,
                 const char*privkeyfname, const char *pubkeyfname) {
    unsigned char content[SHA256_DIGEST_LENGTH];
    int res = 0;

    if (get_sha256_from_file(fname, content, 0L, 0L) == -1) {
        pr_info("can not get sha\n");
        return -1;
    }

    res = ecdsa_sign(content, SHA256_DIGEST_LENGTH, sig, siglenptr, privkeyfname, pubkeyfname);
    if (res != 0) {
        pr_info("can not sign successfully\n");
        return -1;
    } else {
        pr_info("sign successfully\n");
    }
    return 0;
}

 int SigUtil::generate_new_keypairs(const char *privkeypath, const char* pubkeypath) {

     int fd, count;
     EC_KEY *eckey;
     char *privkeystr = NULL , *pubkeystr = NULL;

     eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
     if (eckey == NULL) {
         pr_info("can not create curve for eckey\n");
         return -1;
     }

     pr_info("begin generate key\n");
     if (!EC_KEY_generate_key(eckey)) {
         pr_info("cannot generate key from curve group\n");
         return -1;
     }

     pr_info("begin save privkey\n");
     // save priv key
     privkeystr = BN_bn2hex(EC_KEY_get0_private_key(eckey));
     if ((fd = open(privkeypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1) {
         pr_info("can not open file for priv key store %s\n", privkeypath);
         return -1;
     }
     count = strlen((char*)privkeystr);
     if (write(fd, privkeystr, count) != count) {
         pr_info("error write to file of privkey\n");
         return -1;
     }
     free(privkeystr);
     close(fd);

     pr_info("begin save pubkey\n");
     // save pub key
     pubkeystr = EC_POINT_point2hex(EC_KEY_get0_group(eckey), EC_KEY_get0_public_key(eckey), POINT_CONVERSION_COMPRESSED, NULL);
     if ((fd = open(pubkeypath, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1) {
         pr_info("can not open file for pub key store %s\n", pubkeypath);
         return -1;
     }
     count = strlen(pubkeystr);
     if (write(fd, pubkeystr, count) != count) {
         pr_info("error write to file of pubkey\n");
         return -1;
     }
     free(pubkeystr);
     close(fd);
     EC_KEY_free(eckey);
     return 0;
 }

 int SigUtil::split_and_getsig(const char *fname, unsigned long *datastart, unsigned long *datalen,
                      unsigned char* sig, unsigned int *siglen) {
     *datastart = 0L;
     int fd = open(fname, O_RDONLY);
     if (fd == -1) {
         pr_info("in split: can not open file %s for read\n", fname);
         return -1;
     }

     unsigned char ch = 0;
     lseek(fd, -1L, SEEK_END);
     if (read(fd, &ch, 1) < 0) {
         pr_info("can not get sig len\n");
         return -1;
     }
     pr_info("sig len is %d\n", ch);

     unsigned long dataend = lseek(fd, -1 * ch -1, SEEK_END);
     *datalen = dataend - *datastart;
     pr_info("get datalen value: %lu\n", *datalen);
     *siglen = (unsigned long)ch;
     if (read(fd, sig, *siglen) != *siglen) {
         pr_info("can not read sig correctly\n");
         return -1;
     }
     return 0;
 }
