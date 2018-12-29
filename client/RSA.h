//
//  RSA.h
//  RSA
//
//  Created by 石雁航 on 2018/11/30.
//  Copyright © 2018 石雁航. All rights reserved.
//

#ifndef RSA_h
#define RSA_h
//(a^b)%c
int quick(int a,int b,int c)
{
        int ans = 1;
        a = a % c;
        while(b>0) {
            if(b % 2 == 1)
                ans = (ans * a) % c;
            b = b/2;
            a = (a * a) % c;
        }
    return ans;
}
int * RSA_Encode(char * msg, int len, int k, int n) {
    int *secret = (int *)malloc(len*sizeof(int));
    for (int i = 0; i < len; i++ ) {
        secret[i] = quick((int) msg[i], k, n);
    }
    return secret;
}
char * RSA_Decode(int * msg, const int len, int k, int n) {
    char * ming = (char *)malloc((len+1)*sizeof(char));
    for (int i = 0; i < len; i++ ) {
        ming[i] = (char)quick(msg[i], k, n);
    }
    ming[len] = '\0';
    return ming;
}
#endif /* RSA_h */
