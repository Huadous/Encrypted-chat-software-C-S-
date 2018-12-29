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
    int ans=1;   //记录结果
    a=a%c;   //预处理，使得a处于c的数据范围之下
    while(b!=0)
    {
        if(b&1) ans=(ans*a)%c;   //如果b的二进制位不是0，那么我们的结果是要参与运算的
        b>>=1;    //二进制的移位操作，相当于每次除以2，用二进制看，就是我们不断的遍历b的二进制位
        a=(a*a)%c;   //不断的加倍
    }
    return ans;
}
int * RSA_Encode(char * msg, int len, int k, int n) {
    int *secret = (int *)malloc(len*sizeof(int));
//    printf("-------------------encode-----------------------\n");
    int i;
    for ( i = 0; i < len; i++ ) {
//        printf("{%c}",msg[i]);
//        printf(":");
        secret[i] = quick((int) msg[i], k, n);
//        printf("{%d}",secret[i]);
//        printf("\n");
    }
//    printf("\n");
//    printf("encode:|%s|\n", (char *) secret);
//    printf("len:%d\n",(int)strlen((char *)secret));
//    printf("-------------------encode-----------------------\n");
//    char * clr = (char *)malloc(len);
//    for (int j = 0; j < len; j++) {
//        clr[j] = quick(secret[j], 3389, 6319);
//    }
//    printf("%s\n", clr)
    return secret;
}
char * RSA_Decode(int * msg, const int len, int k, int n) {
//    printf("-------------------decode-----------------------\n");
    char * ming = (char *)malloc((len+1)*sizeof(char));
    int i;
    for (i = 0; i < len; i++ ) {
//        printf("{%d}",msg[i]);
//        printf(":");
        ming[i] = (char)quick(msg[i], k, n);
//        printf("{%c}",ming[i]);
//        printf("\n");
    }
//    printf("\n");
    ming[len] = '\0';
//    printf("decode:|%s|\n", ming);
//    printf("len:%d\n",(int)strlen(ming));
    //    char * clr = (char *)malloc(len);
    //    for (int j = 0; j < len; j++) {
    //        clr[j] = quick(secret[j], 3389, 6319);
    //    }
    //    printf("%s\n", clr);
//    printf("-------------------decode-----------------------\n");
    return ming;
}
#endif /* RSA_h */
