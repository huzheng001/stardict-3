#include <stdio.h>
#include <string.h>
#include <stdlib.h>   
#include <time.h>  
#include <math.h>
#include <malloc.h>

#include "rsa.h"

#include <list>

#define LEN sizeof(struct slink)


void sub(int a[RSA_MAX],int b[RSA_MAX] ,int c[RSA_MAX] );

struct slink
{ 
	int  bignum[RSA_MAX];
	/*bignum[98]用来标记正负号，1正，0负。bignum[99]来标记实际长度*/
	struct slink *next;
};

void do_free_slink(struct slink *head)
{
	struct slink *p, *p_old;
	p = head;
	while (p != NULL) {
		p_old = p;
		p = p->next;
		free(p_old);
	};

	/*std::list<struct slink *> p;
	while (head) {
		p.push_back(head);
		head = head->next;
	}
	for (std::list<struct slink *>::iterator i = p.begin(); i != p.end(); ++i) {
		free(*i);
	}*/
}

/*/--------------------------------------自己建立的大数运算库-------------------------------------*/


/*
void  print(  int a[RSA_MAX] )
   {
	   int i;
	   for(i=0;i<a[99];i++)
		   printf("%d",a[a[99]-i-1]);
	   printf("\n\n");
	   return;
   }
*/

int  cmp(int a1[RSA_MAX],int a2[RSA_MAX])
{   int l1, l2;
	int i;
	l1=a1[99];
	l2=a2[99];
	if (l1>l2)
     return 1;
    if (l1<l2)
       return -1;
    for(i=(l1-1);i>=0;i--)
   {
	   if (a1[i]>a2[i])
		   return 1 ;
	   if (a1[i]<a2[i])
		   return -1;
   }
    return 0;
}

void mov(int a[RSA_MAX],int *b)
{
	int j;
		for(j=0;j<RSA_MAX;j++)
			b[j]=a[j];
		
		return ;
}


void mul(int a1[RSA_MAX],int a2[RSA_MAX],int *c)
{
 int i,j;
 int y;
 int x;
 int z;
 int w;
 int l1, l2;
	l1=a1[RSA_MAX-1];
	l2=a2[RSA_MAX-1];
	if (a1[RSA_MAX-2]=='-'&& a2[RSA_MAX-2]=='-')
		c[RSA_MAX-2]=0;
	else if (a1[RSA_MAX-2]=='-')
		c[RSA_MAX-2]='-';
	else if (a2[RSA_MAX-2]=='-')
		c[RSA_MAX-2]='-';
 for(i=0;i<l1;i++)
 {
  for(j=0;j<l2;j++)
  {
     x=a1[i]*a2[j];
     y=x/10;
     z=x%10;
     w=i+j;
     c[w]=c[w]+z;
     c[w+1]=c[w+1]+y+c[w]/10;
     c[w]=c[w]%10;
  }
 }
 w=l1+l2;
 if(c[w-1]==0)w=w-1;
 c[RSA_MAX-1]=w;
 return;
} 

void add(int a1[RSA_MAX],int a2[RSA_MAX],int *c)
{

 int i,l1,l2;
 int len,temp[RSA_MAX];
 int k=0;
l1=a1[RSA_MAX-1];
l2=a2[RSA_MAX-1];
if((a1[RSA_MAX-2]=='-')&&(a2[RSA_MAX-2]=='-'))
{
	c[RSA_MAX-2]='-';
}
else if (a1[RSA_MAX-2]=='-')
{
	mov(a1,temp);
	temp[RSA_MAX-2]=0;
	sub(a2,temp,c);
	return;
}
else if (a2[RSA_MAX-2]=='-')
{
	mov(a2,temp);
	temp[98]=0;
	sub(a1,temp,c);
	return;
}

	if(l1<l2)len=l1;
 else len=l2;
 for(i=0;i<len;i++)
 {
  c[i]=(a1[i]+a2[i]+k)%10;
  k=(a1[i]+a2[i]+k)/10;
 }
 if(l1>len)
 {
  for(i=len;i<l1;i++)
  {
   c[i]=(a1[i]+k)%10;
      k=(a1[i]+k)/10;   
  }
  if(k!=0)
  {
   c[l1]=k;
   len=l1+1;
  }
  else len=l1;
 }
 else
 {
  for(i=len;i<l2;i++)
  {
   c[i]=(a2[i]+k)%10;
   k=(a2[i]+k)/10;   
  }
  if(k!=0)
  {
   c[l2]=k;
   len=l2+1;
  }
  else len=l2;
 }
  c[99]=len;
  return;
} 


void sub(int a1[RSA_MAX],int a2[RSA_MAX],int *c)
{
 int i,l1,l2;
 int t1[RSA_MAX],t2[RSA_MAX];
 int len=0;
 int k=0;
l1=a1[RSA_MAX-1];
l2=a2[RSA_MAX-1];
if ((a1[RSA_MAX-2]=='-') && (a2[RSA_MAX-2]=='-'))
{
	mov(a1,t1);
        mov(a2,t2);
	t1[RSA_MAX-2]=0;
        t2[RSA_MAX-2]=0;
	sub(t2,t1,c);
	return;
}
else if( a2[RSA_MAX-2]=='-')
{
	mov(a2,t2);
	t2[RSA_MAX-2]=0;
	add(a1,t2,c);
	return;
}
else if (a1[RSA_MAX-2]=='-')
{
	mov(a2,t2);
	t2[RSA_MAX-2]='-';
	add(a1,t2,c);
	return;
}

 if(cmp(a1,a2)==1)
 {
	 
	 len=l2;
 for(i=0;i<len;i++)
 {
	 if ((a1[i]-k-a2[i])<0)
 {
	 c[i]=(a1[i]-a2[i]-k+10)%10;
     k=1;
 }
     else 
	 {
		 c[i]=(a1[i]-a2[i]-k)%10;
	     k=0;
	 }
 }


  for(i=len;i<l1;i++)
  {
	  if ((a1[i]-k)<0)
 {
	 c[i]=(a1[i]-k+10)%10;
     k=1;
 }
     else 
	 {
		 c[i]=(a1[i]-k)%10;
	     k=0;
	 } 
  }
  if(c[l1-1]==0)/*使得数组C中的前面所以0字符不显示了，如1000-20=0980--->显示为980了*/
  {
	     len=l1-1;
		 i=2;
	  while (c[l1-i]==0)/*111456-111450=00006，消除0后变成了6；*/
	  {
	    len=l1-i;
			 i++;
	  }

  }

  else 
  {
	  len=l1;
  }
 }
else
if(cmp(a1,a2)==(-1))
 {
	 c[RSA_MAX-2]='-';
	 len=l1;
	 for(i=0;i<len;i++)
 {
	 if ((a2[i]-k-a1[i])<0)
 {
	 c[i]=(a2[i]-a1[i]-k+10)%10;
     k=1;
 }
     else 
	 {
		 c[i]=(a2[i]-a1[i]-k)%10;
	     k=0;
	 }
 }
  for(i=len;i<l2;i++)
  {
    if ((a2[i]-k)<0)
 {
	 c[i]=(a2[i]-k+10)%10;
     k=1;
 }
     else 
	 {
		 c[i]=(a2[i]-k)%10;
	     k=0;
	 }   
  }
  if(c[l2-1]==0)
  {  
   len=l2-1;
   i=2;
   while (c[l1-i]==0)
	  {
	    len=l1-i;
	    i++;
	  }

  }

  else len=l2;
 
 }

else if(cmp(a1,a2)==0)
   {
	   len=1;
	   c[len-1]=0;
   }
	c[RSA_MAX-1]=len;
return;
}

void  mod(int a[RSA_MAX],int b[RSA_MAX],int  *c)/*/c=a mod b//注意：经检验知道此处A和C的数组都改变了。*/
{	int d[RSA_MAX];
	mov (a,d);
	while (cmp(d,b)!=(-1))/*/c=a-b-b-b-b-b.......until(c<b)*/
	{
		sub(d,b,c);
		mov(c,d);/*/c复制给a*/
	}	
	return ;
}


void  divt(int t[RSA_MAX],int b[RSA_MAX],int  *c ,int *w)/*//试商法//调用以后w为a mod b, C为a  div b;*/
{

	int a1,b1,i,j,m;/*w用于暂时保存数据*/
	int d[RSA_MAX],e[RSA_MAX],f[RSA_MAX],g[RSA_MAX],a[RSA_MAX];
	
	mov(t,a);
		for(i=0;i<RSA_MAX;i++)
	   e[i]=0;
	for(i=0;i<RSA_MAX;i++)
	   d[i]=0;
	for(i=0;i<RSA_MAX;i++) g[i]=0;
	a1=a[RSA_MAX-1];
    b1=b[RSA_MAX-1];
	if (cmp(a,b)==(-1))
	{
		c[0]=0;
		c[RSA_MAX-1]=1;
		mov(t,w);
		return;
	}
	else if (cmp(a,b)==0)
	{
		c[0]=1;
        c[RSA_MAX-1]=1;
		w[0]=0;
		w[RSA_MAX-1]=1;
		return;
	}
		m=(a1-b1);
    for(i=m;i>=0;i--)/*341245/3=341245-300000*1--->41245-30000*1--->11245-3000*3--->2245-300*7--->145-30*4=25--->25-3*8=1*/
	{
      for(j=0;j<RSA_MAX;j++)
        d[j]=0;
		d[i]=1;
		d[RSA_MAX-1]=i+1;
		mov(b,g);
        mul(g,d,e);

	
		while (cmp(a,e)!=(-1))
		{
			c[i]++;
			sub(a,e,f);
		    mov(f,a);/*f复制给g*/
		}

     for(j=i;j<RSA_MAX;j++)/*高位清零*/
        e[j]=0;
	
	}
	mov(a,w);
	if (c[m]==0) c[RSA_MAX-1]=m;
	else c[RSA_MAX-1]=m+1;

	return;
}

void mulmod(int a[RSA_MAX] ,int b[RSA_MAX] ,int n[RSA_MAX],int *m)/*解决 了 m=a*b mod n;*/
{
	int c[RSA_MAX],d[RSA_MAX];
	int i;
	for(i=0;i<RSA_MAX;i++)
		d[i]=c[i]=0;
	mul(a,b,c);
   divt(c,n, d,m);
   //for(i=0;i<m[RSA_MAX-1];i++)
	 //printf("%d",m[m[RSA_MAX-1]-i-1]);
     //printf("\nm  length is :  %d \n",m[RSA_MAX-1]);
}

/*接下来的重点任务是要着手解决 m=a^p  mod n的函数问题。*/

void expmod(int a[RSA_MAX] ,int p[RSA_MAX] ,int n[RSA_MAX],int *m)
{
	int t[RSA_MAX],l[RSA_MAX],temp[RSA_MAX]; /*/t放入2，l放入1；*/
	int w[RSA_MAX],s[RSA_MAX],c[RSA_MAX],b[RSA_MAX],i;
	for(i=0;i<RSA_MAX-1;i++) {
		b[i]=l[i]=t[i]=w[i]=0;
	}
	t[0]=2;t[RSA_MAX-1]=1;
	l[0]=1;l[RSA_MAX-1]=1;
	mov(l,temp);
	mov(a,m);
	mov(p,b);

	while(cmp(b,l)!=0) {
		for(i=0;i<RSA_MAX;i++) {
			w[i]=c[i]=0;
		}
		divt(b,t,w,c);/*// c=p mod 2  w= p /2*/	
		mov(w,b);/*//p=p/2*/
		if (cmp(c,l)==0) { /*/余数c==1*/
			for(i=0;i<RSA_MAX;i++) {
				w[i]=0;
			}
			mul(temp,m,w);
			mov(w,temp);
			for (i=0;i<RSA_MAX;i++) {
				w[i]=c[i]=0;
			}
			divt(temp,n,w,c);/* /c为余c=temp % n，w为商w=temp/n */
			mov(c,temp);
		}
		for(i=0;i<RSA_MAX;i++) {
			s[i]=0;
		}
		mul(m,m,s);//s=a*a
		for(i=0;i<RSA_MAX;i++) {
			c[i]=0;
		}
		divt(s,n,w,c);/*/w=s/n;c=s mod n*/
		mov (c,m);
	}
	for (i=0;i<RSA_MAX;i++) {
		s[i]=0;
	}
	mul(m,temp,s);
	for (i=0;i<RSA_MAX;i++) {
		c[i]=0;
	}
	divt(s,n,w,c);
	mov (c,m);/*余数s给m*/
	m[RSA_MAX-2]=a[RSA_MAX-2];/*为后面的汉字显示需要，用第99位做为标记*/
	//return; /*/k=temp*k%n;*/
}


int   is_prime_san(int p[RSA_MAX] )
{
 
   int i,a[RSA_MAX],t[RSA_MAX],s[RSA_MAX],o[RSA_MAX]; 
   for(i=0;i<RSA_MAX;i++)
	   s[i]=o[i]=a[i]=t[i]=0;
   t[0]=1;
   t[RSA_MAX-1]=1;
   a[0]=2;// { 2,3,5,7 }
   a[RSA_MAX-1]=1;

   sub(p,t,s);

   expmod ( a, s, p ,o);
       if ( cmp(o,t) != 0 ) 
	   {
	  return 0;
	   }

   a[0]=3;
   for(i=0;i<RSA_MAX;i++)  o[i]=0;

   expmod ( a, s, p ,o);
       if ( cmp(o,t) != 0 ) 	   
	   {
	  return 0;
	   }

      a[0]=5;
   for(i=0;i<RSA_MAX;i++)  o[i]=0;

   expmod ( a, s, p ,o);
   if ( cmp(o,t) != 0 ) 
   {	   
	  return 0;
   }

     a[0]=7;
   for(i=0;i<RSA_MAX;i++)  o[i]=0;

   expmod ( a, s, p ,o);

  if ( cmp(o,t) != 0 ) 
  {

	  return 0;
  }

  return 1;
}


int coprime(int e[RSA_MAX],int s[RSA_MAX]) /*//// 求两个大数之间是否互质////*/

{
    int a[RSA_MAX],b[RSA_MAX],c[RSA_MAX],d[RSA_MAX],o[RSA_MAX],l[RSA_MAX];
    int i;
	for(i=0;i<RSA_MAX;i++)
		l[i]=o[i]=c[i]=d[i]=0;
	o[0]=0;o[RSA_MAX-1]=1;
	l[0]=1;l[RSA_MAX-1]=1;
	mov(e,b);
	mov(s,a);
do
{
if(cmp(b,l)==0)
{
    return 1;
}
for(i=0;i<RSA_MAX;i++)
	c[i]=0;
 divt(a,b,d,c);
 mov(b,a);/*b--->a*/
 mov(c,b);/*c--->b*/

}while(cmp(c,o)!=0);
/*	printf("Ihey are  not coprime!\n");*/
return 0;
}


void prime_random(int *p,int *q)
{
	printf("Generating big random number, please wait!!!\n");

	int i,k;
	time_t t; 
	 p[0]=1;
	 q[0]=3;
	 
//	p[19]=1;
//	q[18]=2;
	
	p[RSA_MAX-1]=10;
	q[RSA_MAX-1]=11;

 
	do
	{
			t=time(NULL);
    srand((unsigned int)t);
	for(i=1;i<p[RSA_MAX-1]-1;i++)
	{
	k=rand()%10;
	p[i]=k;
	}
	k=rand()%10;
	while (k==0)
	{
		k=rand()%10;
	}
	p[p[RSA_MAX-1]-1]=k;

	}while((is_prime_san(p))!=1);
		//printf("素数 p 为  : ");
    //for(i=0;i<p[RSA_MAX-1];i++)
	//{
	//printf("%d",p[p[RSA_MAX-1]-i-1]);
	//}
    //printf("\n\n");
		do
	{
			t=time(NULL);
    srand((unsigned int)t);
	for(i=1;i<q[RSA_MAX-1];i++)
	{
	k=rand()%10;
	q[i]=k;
	}

	}while((is_prime_san(q))!=1);
		//printf("素数 q 为 : ");
    //for(i=0;i<q[RSA_MAX-1];i++)
	//{
	//printf("%d",q[q[RSA_MAX-1]-i-1]);
	//}
    //printf("\n\n");
	return;
}
	
void  erand(int e[RSA_MAX],int m[RSA_MAX])
{
	int i,k;
	time_t t;
	e[RSA_MAX-1]=5;
	//printf("随机产生一个与(p-1)*(q-1)互素的 e :");
		do
	{
			t=time(NULL);
    srand((unsigned int)t);
	for(i=0;i<e[RSA_MAX-1]-1;i++)
	{
	k=rand()%10;
	e[i]=k;
	}
	while((k=rand()%10)==0)
		k=rand()%10;
	e[e[RSA_MAX-1]-1]=k;
	}while(coprime( e, m)!=1);
    //for(i=0;i<e[RSA_MAX-1];i++)
	//{
	//printf("%d",e[e[RSA_MAX-1]-i-1]);
	//}
    //printf("\n\n");
	return ;
}

void rsad(int e[RSA_MAX],int g[RSA_MAX],int *d)
{
	int   r[RSA_MAX],n1[RSA_MAX],n2[RSA_MAX],k[RSA_MAX],w[RSA_MAX];
	int     i,t[RSA_MAX],b1[RSA_MAX],b2[RSA_MAX],temp[RSA_MAX];
	mov(g,n1);
	mov(e,n2);
	for(i=0;i<RSA_MAX;i++)
		k[i]=w[i]=r[i]=temp[i]=b1[i]=b2[i]=t[i]=0;
	b1[RSA_MAX-1]=0;b1[0]=0;/*/b1=0;*/
	b2[RSA_MAX-1]=1;b2[0]=1;/*/b2=1;*/
	while(1)
	{

	 for(i=0;i<RSA_MAX;i++)
				k[i]=w[i]=0;
		divt(n1,n2,k,w);/*/k=n1/n2;*/
	 for(i=0;i<RSA_MAX;i++) {
		temp[i]=0;
	 }
	 mul(k,n2,temp);/*/temp=k*n2;*/
    for(i=0;i<RSA_MAX;i++)
		r[i]=0;
        sub(n1,temp,r);

		if((r[RSA_MAX-1]==1) && (r[0]==0))/*/r=0*/
		{
			break;
		}
		else
		{
			mov(n2,n1);/*/n1=n2;*/
			mov( r,n2);/*/n2=r;*/
			mov(b2, t);/*/t=b2;*/
			for(i=0;i<RSA_MAX;i++)
		      temp[i]=0;
			mul(k,b2,temp);/*/b2=b1-k*b2;*/
			for(i=0;i<RSA_MAX;i++)
		      b2[i]=0;
			sub(b1,temp,b2);
			mov(t,b1);
		}
	}

			for(i=0;i<RSA_MAX;i++)
				t[i]=0;
			add(b2,g,t);
			for(i=0;i<RSA_MAX;i++)
				temp[i]=d[i]=0;
	    	divt(t,g,temp,d);
    //printf("由以上的(p-1)*(q-1)和 e 计算得出的 d : ");
    //for(i=0;i<d[RSA_MAX-1];i++)
	//printf("%d",d[d[RSA_MAX-1]-i-1]);
    //printf("\n\n");
}


/*/求解密密钥d的函数(根据Euclid算法)96403770511368768000*/
unsigned int  rsa(unsigned int p,unsigned int q,unsigned int e)  /*/求解密密钥d的函数(根据Euclid算法)*/
{
unsigned int g,k,r,n1,n2,t;
unsigned int b1=0,b2=1;

 g=(p-1)*(q-1);
 n1=g;
 n2=e;
    
    while(1)
 {
        k=n1/n2;
        r=n1-k*n2;
  if(r!=0)
  {
     n1=n2;
     n2=r;
     t=b2;
     b2=b1-k*b2;
     b1=t;
  }

  else
  {
   break;
  }

 }

    return (g+b2)%g;
}

/*/------------------------------------------导入导出公钥和私钥------------------------------------/*/

/*
void loadskey(int d[RSA_MAX],int n[RSA_MAX]) //导入私钥
{
	{
	FILE *fp;
	char filename[25],str[RSA_MAX],ch;
	int i,k;
	for(i=0;i<RSA_MAX;i++)
		d[i]=n[i]=0;
	while(1)
	{
	printf("为导入(d,n)，请输入解密密钥对文件的路径: \n");
    scanf("%s",filename);
	 if((fp=fopen(filename,"r"))==NULL)
	 {
	   printf("输入的文件不存在，请重新输入!\n");   
	 }
	 else break;
	}
	 k=0;
	 while((ch=fgetc(fp))!=EOF)
	 {	
		if(ch!=' ')
		{
		str[k]=ch;
        k++;
		}
	    else
		{
	   for(i=0;i<k;i++)
	   {
		d[i]=str[k-i-1]-48;
	   }
	    d[RSA_MAX-1]=k;
	    k=0;
		}
	}
	 	for(i=0;i<k;i++)
	    	n[i]=str[k-i-1]-48;
		n[RSA_MAX-1]=k;
		printf("\n解密密钥 d : ");
		for(i=0;i<d[RSA_MAX-1];i++)
		    printf("%d",d[d[RSA_MAX-1]-i-1]);
		printf("\n");
		printf("\n    公钥 n : ");
		for(i=0;i<n[RSA_MAX-1];i++)
		    printf("%d",n[n[RSA_MAX-1]-i-1]);
		printf("\n");
		fclose(fp);
		printf("\n导入(d,n)成功!\n");
		getchar();
}
*/


#ifdef SERVER_EDITION

int p_global[RSA_MAX],q_global[RSA_MAX],n_global[RSA_MAX],d_global[RSA_MAX],e_global[RSA_MAX],m_global[RSA_MAX],p1_global[RSA_MAX],q1_global[RSA_MAX];

void rsa_init()
{
	int i;
	for (i=0;i<RSA_MAX;i++) {
		m_global[i]=p_global[i]=q_global[i]=n_global[i]=d_global[i]=e_global[i]=0;/*/简单初始化一下*/
	}
}

void rsa_gen_key(int RSA_Public_Key_d[RSA_MAX], int RSA_Public_Key_n[RSA_MAX])
{
	int i;
	for (i=0;i<RSA_MAX;i++) {
             m_global[i]=p_global[i]=q_global[i]=n_global[i]=d_global[i]=e_global[i]=0;
	}
	prime_random(p_global,q_global);/*/随机产生两个大素数*/
	mul(p_global,q_global,n_global); // Get n.
	mov(p_global,p1_global);
	p1_global[0]--;      
	mov(q_global,q1_global);
	q1_global[0]--;      /*/q-1;*/
	mul(p1_global,q1_global,m_global);//m=(p-1)*(q-1)
	erand(e_global,m_global);
	rsad(e_global,m_global,d_global); // Get e and d.

	for (i=0;i<RSA_MAX;i++) {
		RSA_Public_Key_d[i] = d_global[i];
		RSA_Public_Key_n[i] = n_global[i];
	}
}
#endif

#ifdef SERVER_EDITION
void rsa_get_public_key_str(std::string &public_key)
{
	public_key.clear();

	char ch;
	for (int i=0;i<e_global[RSA_MAX-1];i++) {
		ch=e_global[e_global[RSA_MAX-1]-i-1]+48;
		public_key += ch;
	}

	public_key += ' ';

	for(int i=0;i<n_global[RSA_MAX-1];i++) {
		ch=n_global[n_global[RSA_MAX-1]-i-1]+48;
		public_key += ch;
	}
}
#endif

#ifdef CLIENT_EDITION
void rsa_public_key_str_to_bin(std::string &public_key, int e[RSA_MAX], int n[RSA_MAX])
{
	char str[RSA_MAX],ch;
	int i,k;
	for(i=0;i<RSA_MAX;i++) {
		e[i]=n[i]=0;
	}

	k=0;
	size_t j=0;
	size_t len = public_key.length();

	while(j< len) {
		ch = public_key[j];
		j++;
		if(ch!=' ') {
			str[k]=ch;
			k++;
		} else {
			for(i=0;i<k;i++) {
				e[i]=str[k-i-1]-48;
			}
			e[RSA_MAX-1]=k; // Get e.
			k=0;
		}
	}
	for (i=0;i<k;i++) {
		n[i]=str[k-i-1]-48;
	}
	n[RSA_MAX-1]=k; // Get n.
}
#endif

#ifdef CLIENT_EDITION
void rsa_encrypt(std::vector<unsigned char> &src, std::vector<unsigned char> &dest, int *e, int *n)
{
	int i,k,count,temp,c;
	char ch;
	struct slink  *p,*p1,*p2;
	struct slink  *h;

	p=p1=p2=(struct slink * )malloc(LEN);
	for(i=0;i<RSA_MAX;i++) {
		p->bignum[i]=0;
	}
	h=NULL;

	count=0;
	size_t j=0;
	size_t len = src.size();
	while(j< len) {
		ch = src[j];
		j++;
		c=ch;
		k=0;
		if(c<0) {
			c=abs(c);/*/把负数取正并且做一个标记*/
			p1->bignum[RSA_MAX-2]='0';
		} else {
			p1->bignum[RSA_MAX-2]='1';
		}
		while(c/10!=0) {
			temp=c%10;
			c=c/10;
			p1->bignum[k]=temp;
			k++;
		}
		p1->bignum[k]=c;
		p1->bignum[RSA_MAX-1]=k+1;
		count=count+1;
		if(count==1) {
			h=p1;
		} else {
			p2->next=p1;
		}
		p2=p1;
		if (j < len -1) {
			p1=(struct slink * )malloc(LEN);
			for(i=0;i<RSA_MAX;i++) {
				p1->bignum[i]=0;
			}
		}
	}
	p2->next=NULL; 

	dest.clear();

	p=h;
	if(h!=NULL) {
		do {
			p1=(struct slink * )malloc(LEN);
			for(i=0;i<RSA_MAX;i++) {
				p1->bignum[i]=0;
			}
			expmod( p->bignum , e ,n ,p1->bignum);
			ch=p1->bignum[RSA_MAX-2];
			dest.push_back(ch);
			if ((p1->bignum[RSA_MAX-1]/10) ==0) { /*/判断p1->bignum[99]的是否大于十；*/
				ch=0+48;
				dest.push_back(ch);
				ch=p1->bignum[RSA_MAX-1]+48;
				dest.push_back(ch);
			} else {
				ch=p1->bignum[RSA_MAX-1]/10+48;
				dest.push_back(ch);
				ch=p1->bignum[RSA_MAX-1]%10+48;
				dest.push_back(ch);
			}
			for(i=0;i<p1->bignum[RSA_MAX-1];i++) {
				ch=p1->bignum[i]+48;
				dest.push_back(ch);
			}
			free(p1);
			p=p->next;
		} while(p!=NULL);
	}
	do_free_slink(h);
}
#endif

#ifdef SERVER_EDITION
void rsa_decrypt(std::vector<unsigned char> &src, std::vector<unsigned char> &dest,int *d, int *n)
{
	struct slink *h,*p1,*p2;
	char ch;
	int i,j,k,c,count,temp;

	i=0;
	j=3;
	count=0;
	h=p1=p2=(struct slink * )malloc(LEN);
	for(i=0;i<RSA_MAX;i++) {
		p1->bignum[i]=0;
	}

	size_t l =0;
	size_t len = src.size();
	while(l < len) {
		ch = src[l];
		l++;
		c=ch;
		if (j==3) {
			p1->bignum[RSA_MAX-2]=c;
			j--;
		} else if (j==2) {
			temp=c-48;
			j--;
		} else if(j==1) {
			p1->bignum[RSA_MAX-1]=temp*10+c-48;
			j--;
		} else if (j==0) {
			p1->bignum[i]=c-48;
			i++;
			if(i==p1->bignum[RSA_MAX-1]) { 
				i=0;
				j=3;
				count++;
				if (count==1) {
					h=p1;
				} else {
					p2->next=p1;
				}
				p2=p1;
				if (l < len -1) {
					p1=(struct slink * )malloc(LEN);
					for(i=0;i<RSA_MAX;i++) {
						p1->bignum[i]=0;
					}
				}
			}
		}
	}
	p2->next=NULL; 

	dest.clear();

	p1=h;
	k=0;
	if(h!=NULL) { /*/temp为暂存ASIIC码的int值*/
		do {
			p2=(struct slink * )malloc(LEN);
			for(i=0;i<RSA_MAX;i++) {
				p2->bignum[i]=0;
			}
			expmod( p1->bignum , d ,n ,p2->bignum);		  
			temp=p2->bignum[0]+p2->bignum[1]*10+p2->bignum[2]*100;
			if (( p2->bignum[RSA_MAX-2])=='0') {
				temp=0-temp;
			}/*/转化为正确的ASIIC码，如-78-96形成汉字	*/	 
			ch=temp;/*  str[k]--->ch */
			dest.push_back(ch);
			k++;
			p1=p1->next;
			free(p2);
		} while (p1!=NULL);
	}
	do_free_slink(h);
}
#endif

#ifdef SERVER_EDITION
void buffer_to_vector(unsigned char *buffer, size_t buffer_len, std::vector<unsigned char> &v)
{
	v.clear();
	for (size_t i = 0; i< buffer_len; i++) {
		v.push_back(buffer[i]);
	}
}
#endif

#ifdef CLIENT_EDITION
void string_to_vector(std::string &str, std::vector<unsigned char> &v)
{
	v.clear();
	size_t len = str.length();
	for (size_t i = 0; i< len; i++) {
		v.push_back(str[i]);
	}
}
#endif

void vector_to_string(std::vector<unsigned char> &v, std::string &str)
{
	str.clear();
	size_t len = v.size();
	for (size_t i = 0; i < len; i++) {
		str += v[i];
	}
}

