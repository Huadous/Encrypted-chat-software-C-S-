//
//  createKEY.h
//  authentic
//
//  Created by 石雁航 on 2018/11/30.
//  Copyright © 2018 石雁航. All rights reserved.
//

#ifndef createKEY_h
#define createKEY_h
//模拟RSA算法
typedef struct Keytype {
    int k; //指数
    int n; //模数
} Key;

long long findS(int n) {
    int i = 0;
    
    long long k = 0;
    long long j;
    for ( j = (long long)n; j > 2; j--) {
        k = (long long)sqrt((double)j);
        for (i = 2; i <= k; i++) {
            if (j % i == 0) {
                break;
            }
        }
        if(i>k)
            return j;
    }
    return 0;
}
long long findD(long long n_, long long e) {
    long long l;
    long long i;
    for ( i = 1; ; i++) {
        l = (n_ * i + 1)%e;
        if (l==0) {
            return (n_ * i + 1)/e;
        }
    }
    return 0;
}
//用户用来产生私钥【模数加指数
Key CreatePrKey(char * id_s, char * pw_s) {
    unsigned char xo_r[6];
    int i;
    for ( i = 0; i < 5; i++) {
        xo_r[i] = id_s[i] ^ pw_s[i];
    }
    unsigned lt = (unsigned)((xo_r[0]+id_s[1]+pw_s[3])%37+13);
    unsigned rt = (unsigned)((xo_r[1]+id_s[2]+pw_s[4])%37+13);
    long long p = findS(lt);
    long long q = findS(rt);
    long long n = p * q;
    long long n_ = (p-1) * (q-1);
    long long e = 373;
    long long d = findD(n_, e);
    Key myKey;
    myKey.k = (int)d;
    myKey.n = (int)n;
    return myKey;
}
//服务器用来产生模数，公钥指数定为3389
Key createPuKey(char *id_s, char *pw_s) {
    unsigned char xo_r[6];
    int i;
    for ( i = 0; i < 5; i++) {
        xo_r[i] = id_s[i] ^ pw_s[i];
    }
    unsigned lt = (unsigned)((xo_r[0]+id_s[1]+pw_s[3])%37+13);
    unsigned rt = (unsigned)((xo_r[1]+id_s[2]+pw_s[4])%37+13);
    long long p = findS(lt);
    long long q = findS(rt);
    long long n = p * q;
    Key myKey;
    myKey.k = 373;
    myKey.n = (int)n;
    return myKey;
}
//use DH
//unsigned char * CreatePuKey(char * id_s, char * pw_s) {
//
//}


#endif /* createKEY_h */
