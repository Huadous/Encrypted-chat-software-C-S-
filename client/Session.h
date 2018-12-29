//
//  Session.h
//  RSA
//
//  Created by 石雁航 on 2018/12/1.
//  Copyright © 2018 石雁航. All rights reserved.
//

#ifndef Session_h
#define Session_h
void SWAP(unsigned char *a, unsigned char *b) {
    unsigned char temp = *a;
    *a = *b;
    *b = temp;
}
//对称加密
unsigned char * initS(unsigned char * k) {
    int len = 20;
    unsigned char *S = (unsigned char *)malloc(256 * sizeof(unsigned char));
    unsigned char *T = (unsigned char *)malloc(256 * sizeof(unsigned char));
    for (int i = 0; i < 256; i++) {
        S[i] = i;
        T[i] = k[i % len];
    }
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + T[i]) % 256;
        SWAP(S+i, T+j);
    }
    return S;
}
char * RC4(char * M, unsigned char * S) {
    unsigned char k = 0;
    int i = 0, t = 0, j = 0;
    int len = (int)strlen(M);
    char * C = (char *)malloc((len + 1)*sizeof(char));
    char temp;
    for (int index = 0; index != len; index ++) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        SWAP(S+i, S+j);
        t = (S[i] + S[j]) % 256;
        k = S[t];
        temp = M[index] ^ k;
        C[index] = temp;
    }
    C[len] = '\0';
    return C;
}
//生成回话密钥生成器
int createSessionNumber() {
    srand(time(NULL)%100000);
    return rand()%128 + 128;
}
//生成回话密钥
unsigned char * createSessionKey(int randN, Key eKey) {
    unsigned char * ret = (unsigned char *)malloc(21*sizeof(unsigned char));
//    printf("ret:");
    for (int i = 0; i < 20; i++ ) {
        ret[i] = (unsigned char)quick((eKey.k+eKey.n)%128, (randN-i)%128, 372)%127;
//        printf("[%c]",*(ret+i));
    }
//    printf(".\n");
    ret[20] = '\0';
    return ret;
}
//加密回话密钥生成数
int encodeSessionNumber(int randN, Key eKey) {
    return quick(randN, eKey.k, eKey.n);
}
//解密回话密钥生成数
int decodeSessionNumber(int N, Key myKey) {
    return quick(N, myKey.k, myKey.n);
}
#endif /* Session_h */
