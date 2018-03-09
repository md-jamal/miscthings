LOCAL_PATH := ${call my-dir}

kivvissl_src := \
./wolfcrypt/curve25519.c \
./wolfcrypt/chacha20_poly1305.c\
./wolfcrypt/ripemd.c\
./wolfcrypt/hc128.c\
./wolfcrypt/ecc_fp.c\
./wolfcrypt/ge_low_mem.c\
./wolfcrypt/memory.c\
./wolfcrypt/ed25519.c\
./wolfcrypt/asm.c\
./wolfcrypt/camellia.c\
./wolfcrypt/pkcs7.c\
./wolfcrypt/srp.c\
./wolfcrypt/integer.c\
./wolfcrypt/ecc.c\
./wolfcrypt/random.c\
./wolfcrypt/blake2b.c\
./wolfcrypt/misc.c\
./wolfcrypt/wc_encrypt.c\
./wolfcrypt/chacha.c\
./wolfcrypt/md2.c\
./wolfcrypt/sha256.c\
./wolfcrypt/ge_operations.c\
./wolfcrypt/fe_low_mem.c\
./wolfcrypt/hmac.c\
./wolfcrypt/poly1305.c\
./wolfcrypt/sha.c\
./wolfcrypt/rsa.c\
./wolfcrypt/des3.c\
./wolfcrypt/signature.c\
./wolfcrypt/sha512.c\
./wolfcrypt/tfm.c\
./wolfcrypt/hash.c\
./wolfcrypt/aes.c\
./wolfcrypt/rabbit.c\
./wolfcrypt/pwdbased.c\
./wolfcrypt/error.c\
./wolfcrypt/arc4.c\
./wolfcrypt/md5.c\
./wolfcrypt/wc_port.c\
./wolfcrypt/coding.c\
./wolfcrypt/compress.c\
./wolfcrypt/idea.c\
./wolfcrypt/dsa.c\
./wolfcrypt/dh.c\
./wolfcrypt/logging.c\
./wolfcrypt/md4.c\
./wolfcrypt/fe_operations.c\
./wolfcrypt/asn.c\
./wolfsrc/keys.c\
./wolfsrc/crl.c\
./wolfsrc/internal.c\
./wolfsrc/sniffer.c\
./wolfsrc/ocsp.c\
./wolfsrc/io.c\
./wolfsrc/tls.c\
./wolfsrc/ssl.c\
./cynovo/util.c\
./cynovo/wolf_cert_util.c \
./cynovo/wolf_rsa_util.c \
./cynovo/der.c \
./cynovo/der_cert.c \
./cynovo/der_rsa.c


include ${CLEAR_VARS}

LOCAL_LDLIBS := -llog -ldl 

LOCAL_MODULE := wolfssl

LOCAL_SRC_FILES :=  \
$(kivvissl_src) \
./kivvissl.c \
./kivvissl-jni.cpp \
../hlog.c

LOCAL_CFLAGS += -DWOLFSSL_KEY_GEN
LOCAL_CFLAGS += -DWOLFSSL_CERT_GEN
LOCAL_CFLAGS += -DWOLFSSL_CERT_EXT
LOCAL_CFLAGS += -DWOLFSSL_PUB_PEM_TO_DER
LOCAL_CFLAGS += -DWOLFSSL_ALT_NAMES
LOCAL_CFLAGS += -DWOLFSSL_CERT_REQ

LOCAL_C_INCLUDES := \
			$(LOCAL_PATH) \
			$(LOCAL_PATH)/../

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

include ${BUILD_SHARED_LIBRARY}



include ${CLEAR_VARS}

LOCAL_LDLIBS := -llog -ldl

LOCAL_MODULE := testwolfssl

LOCAL_SRC_FILES :=  \
$(kivvissl_src) \
./kivvissl.c \
./test_main.c \
../hlog.c

LOCAL_CFLAGS += -DWOLFSSL_KEY_GEN
LOCAL_CFLAGS += -DWOLFSSL_CERT_GEN
LOCAL_CFLAGS += -DWOLFSSL_CERT_EXT
LOCAL_CFLAGS += -DWOLFSSL_PUB_PEM_TO_DER
LOCAL_CFLAGS += -DWOLFSSL_ALT_NAMES
LOCAL_CFLAGS += -DWOLFSSL_CERT_REQ

LOCAL_C_INCLUDES := \
			$(LOCAL_PATH) \
			$(LOCAL_PATH)/../

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

include ${BUILD_EXECUTABLE}
