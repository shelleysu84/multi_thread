#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include <vector>
#include <string>
#include <iostream>
using namespace std;

#define NUM_THREADS 10
pthread_mutex_t path_mutex;
vector<std::string> pathvector;

void *thread_func(void *dirpath);
void *thread_func1(char dirpath[256]);
int main(int argc,char *argv[])
{
   int res;
   pthread_t threads[NUM_THREADS];
   //void* thread_result;
   int lots_of_threads;
   
   //check the arguments
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

   DIR *dir;	//directory stream
   struct dirent *ent;	// directory entry structure
   //vector<std::string> pathvector;
   if ((dir = opendir (argv[1])) != NULL) 
   {   
      while((ent = readdir(dir)) != NULL/*&&ent->d_type == DT_REG*/)
      	pathvector.push_back(ent->d_name);
   }

   res = pthread_mutex_init(&path_mutex,NULL);
   for(lots_of_threads=0;lots_of_threads<NUM_THREADS;lots_of_threads++)
   {
	res = pthread_create(&threads[lots_of_threads],NULL,thread_func,(void*)argv[1]);
   }
   printf("waiting for thread to finish\n");
   for(lots_of_threads=0;lots_of_threads<NUM_THREADS;lots_of_threads++)
     /*lots_of_threads=NUM_THREADS-1;lots_of_threads>=0;lots_of_threads--*/
   {
	res = pthread_join(threads[lots_of_threads],NULL);
	if(res==0)
	    printf("pick up a thread\n");
	else
	    perror("pthread_join failed\n");
   }
   printf("all done\n");
   return 0;
}

void *thread_func(void *dirpath)
{
   return thread_func1((char*)dirpath);
}
void *thread_func1(char dirpath[256])
{
   //DIR *dir;	//directory stream
   FILE *file;	//file stream	
   //struct dirent *ent;	// directory entry structure
   char *line = NULL;	// pointer to 
   size_t len = 1000;	//the length of bytes getline will allocate
   size_t read;
   char full_filename[256];
   pthread_mutex_lock(&path_mutex);
   //if ((dir = opendir (dirpath)) != NULL) 
   //{   
      //while((ent = readdir(dir)) != NULL/*&&ent->d_type == DT_REG*/)
      //{
      while(!pathvector.empty())
      {
	//printf ("%s\n", ent->d_name);
	//pthread_mutex_unlock(&path_mutex);//realse mutex
	// Check if the list is a regular file
	//if(ent->d_type == DT_REG)
	//{
		// Create the absolute path of the filename
		int lastindex = pathvector.size()-1;
		snprintf(full_filename, sizeof full_filename,"./%s/%s\0",dirpath,pathvector[lastindex].c_str()/*ent->d_name*/);	
                printf("%s\n", full_filename);
		pathvector.pop_back();	
		//pthread_mutex_unlock(&path_mutex);//realse mutex		
		// open the file
		file = fopen(full_filename, "r");
		// file was able to be open
		if (file != NULL)
		{
			// Print out each line in the file
			while ((read = getline(&line, &len, file)) != -1) 				
			{
    				printf("Retrieved line of length %d:\n", read);
    				printf("%s", line);
			}
			fclose(file);
			pthread_mutex_unlock(&path_mutex);//realse mutex			
		}
							
	//}
        pthread_mutex_lock(&path_mutex);
     }
   pthread_mutex_unlock(&path_mutex);
   printf("bye~\n");
   
   pthread_exit(NULL);
   }
//}

