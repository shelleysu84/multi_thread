#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include <vector>
#include <set>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h> 
#include <memory.h> 
#include <regex.h>
#include <iostream>

using namespace std;
#define return_if_fail(p)  if((p) == 0){printf ("[%s]:func error!\n", __func__);return;}  
#define NUM_THREADS 2
#define SUBSLEN 1000 	        /* 匹配子串的数量 length of sub_string */
#define EBUFLEN 128             /* 错误消息buffer长度 length of error buffer */
#define BUFLEN 1024             /* 匹配到的字符串buffer长度 length of string buffer */
#define IP "60.18.93.11"		/* 特定IP specific IP */
static int count = 0;			/* 统计特定IP的登陆次数 counter for specific IP */
static int count_total = 0;		/* 统计整个文件出现的所有IP登陆次数用于测试 for test total IP appeared*/
set<std::string> collection_of_IP;

int IP_retrieve(char argv[1024])
{
	size_t len;					/* 匹配上的字符串长度*/
	regex_t re;             	/* 存储编译好的正则表达式，正则表达式在使用之前要经过编译 */
	regmatch_t subs[SUBSLEN];     	/* 存储匹配到的字符串位置 */
	char matched[BUFLEN];	  	/* 存储匹配到的字符串 */
	char errbuf[EBUFLEN];  		/* 存储错误消息 */
	int err,i;
	char *src = argv; 			/* 被匹配的字符串 */
	char pattern[] = "([0-9]{1,3}[.]){3}[0-9]{1,3}";/* IP字符串的正则形式 */

	/* 编译正则表达式 */
	err = regcomp(&re, pattern, REG_EXTENDED);

	if (err) {
		len = regerror(err, &re, errbuf, sizeof(errbuf));
		printf("error: regcomp: %s\n", errbuf);
		regfree(&re); 			/*释放中间编译模式*/
		return 1;
	}
	/* 执行模式匹配 */
	err = regexec(&re, src, (size_t) SUBSLEN, subs, 0);

	if (err == REG_NOMATCH) { 	/* 没有匹配成功 */
		printf("Sorry, no match ...\n");
		regfree(&re);			/*释放中间编译模式*/
		return 0;
	} else if (err) {  			/* 其它错误 */
		len = regerror(err, &re, errbuf, sizeof(errbuf));
		printf("error: regexec: %s\n", errbuf);
		regfree(&re);			/*释放中间编译模式*/
		return 1;
	}

	/* 如果不是REG_NOMATCH并且没有其它错误，则模式匹配上 */
	
	else 
	{
		for(i=0;i<re.re_nsub; i++) 
		{
			len = subs[i].rm_eo - subs[i].rm_so; 			/* 长度 */
			memcpy(matched,src + subs[i].rm_so,len);		/* 拷贝src里面的位置到matched里 */
			matched[len] = '\0';
			count_total = count_total+1;
			//printf("count_total: %d\n", count_total); 	/* 测试用 */
			//if(!strcmp(matched,IP))						/* 比较IP是否相同，相同strcmp为0 */
			//{
			//	count = count+1;
			//	printf("match IP: %s\t",matched);
			//	printf("count: %d\t", count);
			//}
			printf("match IP pattern: %s\t",matched);
			printf("count: %d\t", count_total);		
			collection_of_IP.insert(matched);				/* 这里不需要用str(matched) */
			printf("set size: %d\n ",(int)collection_of_IP.size());
		}
		regfree(&re); /* 释放中间编译模式 */
		return 0;
	}  	
}
  
typedef struct _SemaphoreComb 
{  
	sem_t s1;      
	sem_t s2;
    	char *dirpath;
}SemaphoreComb;

static void Semaphore_init(SemaphoreComb* thiz,char _dirpath[])
{
    
    	return_if_fail(thiz!=NULL);
    	thiz->dirpath = _dirpath;
    	sem_init(&thiz->s1,0,1);
    	sem_init(&thiz->s2,0,0); 
    	return;
}
static void Semaphore_destroy(SemaphoreComb* thiz)
{
    	return_if_fail(thiz!=NULL);
    	sem_destroy(&thiz->s1);
    	sem_destroy(&thiz->s2);
    	//delete []thiz->dirpath; 					/* 千万不要手贱在没有用new的时候用delete */
    	printf("destroy the semaphores\n");
    	free(thiz);
    	thiz =NULL;
    	return;
}

//create a file path name vector to elimate different thread enter into the same file
/* 设置一个保存所有文件名字的向量 */
vector<std::string> pathvector; 
//define thread function wrapper and inside func for *.cpp, if used in *.c no need for wrapper
/* 定义包裹函数和内部函数 只在cpp下面这样定义 */
static void* thread_func1_wrapper(void* thiz);
static void* thread_func1(SemaphoreComb* thiz);
static void* thread_func2_wrapper(void* thiz);
static void* thread_func2(SemaphoreComb* thiz);
/* 打印每一行出现的IP */
void print_ip(vector<std::string>& _pathvector,char _full_filename[],char _dirpath[],char _line[],size_t _len,size_t _read);
//argv[1] represent dirpath
/* argv[1]是输入目录的路径 */
int main(int argc,char *argv[])
{
	int res;
	pthread_t threads[NUM_THREADS];
	   
	//index of threads
	/* 设置线程个数 */
	int lots_of_threads;
	   
	//check the arguments
	/* 检查参数个数 */
	if(argc<2)
	{
		printf("not enough arguments\n");
		return -1;
	}
	if(argc>2)
	{
		printf("too many arguments\n");
		return -1;
	}
	//push all files' name into pathvector
	/* 定义目录路径和内部文件流 */
	DIR *dir;	//directory stream
	struct dirent *ent;	// directory entry structure
	   
	/* 目录是否能打开 */
	if ((dir = opendir (argv[1])) != NULL) 
	{
		/* 如果文件流能不断打开 */
		while((ent = readdir(dir)) != NULL) 
			//add file name into pathvector  
			/* 添加文件到向量 */ 	
			pathvector.push_back(ent->d_name);
	}

        //set semaphoneComb
	/* 开辟一个semaphore组合的结构体并检查*/
	SemaphoreComb* thiz = NULL;
   	thiz = (SemaphoreComb*)malloc(sizeof(SemaphoreComb)); 
   	if(thiz==NULL)
   	{
	    printf("failed to malloc sempahore combination!");
	    return -1;
   	}
	//initialize Semaphore 
	/*初始化结构体*/ 
	Semaphore_init(thiz,argv[1]);
	/*建立两个线程*/
	res = pthread_create(&threads[0],NULL,thread_func1_wrapper,thiz);
	if(res!=0)
        {
	    perror("pthread 0 not create");
	}
        res = pthread_create(&threads[1],NULL,thread_func2_wrapper,thiz);
	if(res!=0)
        {
	    perror("pthread 1 not create");
	}
	
   	printf("waiting for thread to finish\n");
   	/*线程退出*/
   	pthread_join(threads[0],NULL);
   	pthread_join(threads[1],NULL);
   
   	Semaphore_destroy(thiz);
   	printf("all done\n");
   	return (0);
}
//thread fuction wrapper
static void *thread_func1_wrapper(void* thiz)
{
   	return thread_func1((SemaphoreComb*) thiz);
}
//ture thread function
static void *thread_func1(SemaphoreComb* thiz)
{	
   	char *line =NULL; 
   	int len = 1000;	//the length of bytes getline will allocate
   	int read = 0;
   	char full_filename[256];

   	while(!pathvector.empty())
   	{
		//signal s2 - 1, s2 enter critical region
		sem_wait(&thiz->s2);
		printf("thread1: s2 get the lock.\n");
        	print_ip(pathvector,full_filename,thiz->dirpath,line,len,read);
		//signal s1 + 1, s1 left critical region 		
		sem_post(&thiz->s1);
		printf("thread1: s1 unlock\n");
		print_ip(pathvector,full_filename,thiz->dirpath,line,len,read);
		sleep (1);
   	}
}

static void *thread_func2_wrapper(void* thiz)
{
   	return thread_func2((SemaphoreComb*) thiz);
}
static void *thread_func2(SemaphoreComb* thiz)
{	
   	char *line = NULL;// pointer to each line  
   	size_t len = 1000;	//the length of bytes getline will allocate
   	size_t read;
   	char full_filename[256];
   
   	while(!pathvector.empty())
   	{
		//signal s1 - 1, s1 enter critical region
        	sem_wait(&thiz->s1);
		printf("thread2: s1 get the lock.\n");        	
		print_ip(pathvector,full_filename,thiz->dirpath,line,len,read);
		//signal s2 + 1, s2 left critical region		
		sem_post(&thiz->s2);
		printf("thread2: s2 unlock\n");
		print_ip(pathvector,full_filename,thiz->dirpath,line,len,read);
		sleep (1);
   	}
}

void print_ip(vector<std::string>& _pathvector,char _full_filename[],char *_dirpath,char _line[],size_t _len,size_t _read)
{	
	int lastindex = _pathvector.size()-1;

	//create full path name for each file
	sprintf(_full_filename,"./%s/%s\0",_dirpath,_pathvector[lastindex].c_str());		
        printf("%s\n",_full_filename);

	//handle over the filename then pop the filename to avoid repeatation
	_pathvector.pop_back();	

	//open the file
	FILE* _file;
        _file = fopen(_full_filename, "r");
	//file was able to be open
	if (_file != NULL)
	{
		// Print out each line in the file
		while ((_read = getline(&_line, &_len, _file)) != -1) 				
		{
			
			IP_retrieve(_line);
			//printf("Retrieved line of length %d:\n", _read);
    			//printf("%s", _line);
		}
		fclose(_file);			
	}
}
