#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>
#include <string.h>
#include <pthread.h>

#include <sys/stat.h>
#include <curl/curl.h>
#ifdef WIN32
#include <io.h>
#define READ_3RD_ARG unsigned int
#else
#include <unistd.h>
#define READ_3RD_ARG size_t
#endif

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

struct fileInfo {
    char* watchDir;
    char* filename;
    char* url;
    char* credential;
};

/* read callback function, fread() look alike */
static size_t my_read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  ssize_t retcode;
  curl_off_t nread;

  int *fdp = (int *)stream;
  int fd = *fdp;

  retcode = read(fd, ptr, (READ_3RD_ARG)(size * nmemb));

  nread = (curl_off_t)retcode;

  fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
          " bytes from file\n", nread);

  return retcode;
}

int create_new_upload(int fd, int size, char *url, char *credentials, size_t read_callback(char *, size_t, size_t, void *))
{
  CURL *curl;
  CURLcode res;

  /* get a curl handle */
  curl = curl_easy_init();
  if(curl) {
    /* we want to use our own read function */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

    /* which file to upload */
    curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&fd);

    /* enable "uploading" (which means PUT when doing HTTP) */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* specify target URL, and note that this URL should also include a file
 *  *        name, not only a directory (as you can do with GTP uploads) */
    curl_easy_setopt(curl, CURLOPT_URL, url);
/*and give the size of the upload, this supports large file sizes
 *  *        on systems that have general support for it */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)size);

    /* tell libcurl we can use "any" auth, which lets the lib pick one, but it
 *  *        also costs one extra round-trip and possibly sending of all the PUT
 *   *               data twice!!! */
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_DIGEST);

    /* set user name and password for the authentication */
    curl_easy_setopt(curl, CURLOPT_USERPWD, credentials);

    /* Now run off and do what you've been told! */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
    return res;
  }
  return -1;
}

void *threadFunc(void *input){
	struct fileInfo* file = (struct fileInfo*)input;
	printf("thread id: %ld, file--> %s , url: %s , credential: %s \n", pthread_self(), file->filename, file->url, file->credential);

	int newbufSize = strlen(file->watchDir) + strlen(file->filename) + 1;
	char* filePath = (char*)malloc(newbufSize);
  	strcpy(filePath, file->watchDir);
 	strcat(filePath, file->filename);
	
	/* get the file size of the local file */
	int  hd = open(filePath, O_RDONLY);
	struct stat file_info;
	fstat(hd, &file_info);

	int newSize = strlen(file->url)  + strlen(file->filename) + 1; 

    char *fullUrlPath  = (char *)malloc(newSize);
    strcpy(fullUrlPath, file->url);
    strcat(fullUrlPath, file->filename); 

  	int err = create_new_upload(hd, file_info.st_size, fullUrlPath, file->credential, my_read_callback);
  	if(err)
   	 fprintf(stderr, "create_new_upload() failed: %d\n",err);

  	close(hd); /* close the local file */ 
//	printf("////////////////Exit thread : %ld , File: %s , Filesize: %d \n", pthread_self(), fullUrlPath, file_info.st_size); 
	free(file);
	free(filePath);
	free(fullUrlPath);
}

int main(int argc, char** argv )
{
  char *url;
  char *credentials;
  char *watchFolder;
  if(argc < 4) {
    printf("Usage\n%s <watchFolder> <url> <user:pwd> \n", argv[0]);
    return 1;
  }

  watchFolder = argv[1];
  url = argv[2];
  credentials = argv[3];

  int fd;
  int wd;
 
  /*creating the INOTIFY instance*/
  fd = inotify_init();

  /*checking for error*/
  if ( fd < 0 ) {
    perror( "inotify_init" );
  }

/* In windows, this will init the winsock stuff */
  curl_global_init(CURL_GLOBAL_ALL);


  /*adding the “watchFolder” directory into watch list.*/
  //wd = inotify_add_watch( fd, watchFolder, IN_CREATE );
  wd = inotify_add_watch(fd, watchFolder, IN_CLOSE_WRITE | IN_MOVED_TO);

while(1){
  int i=0, length;
  char buffer[EVENT_BUF_LEN];
  /*read to determine the event change happens on “watchFolder” directory.  blocking call*/
  length = read( fd, buffer, EVENT_BUF_LEN ); 

  /*checking for error*/
  if ( length < 0 ) {
    perror( "read" );
  }  

  /*Here, read the change event one by one and process it accordingly.*/
  while ( i < length ) {     
	struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];     if ( event->len ) {
      	if ( event->mask & IN_MOVED_TO || event->mask & IN_CLOSE_WRITE ) {
        	if ( event->mask & IN_ISDIR ) {
          		printf( "New directory %s created.\n", event->name );
        	}
        	else {
			char *ext;
			ext = strchr(event->name, '.');
			if (strcmp(ext, ".m3u8.tmp") != 0){
			  //  printf( "New file %s created, extension is : %s\n", event->name, ext );
			    struct fileInfo *info = (struct fileInfo *)malloc(sizeof(struct fileInfo));
			   
			    info->watchDir = watchFolder; 
			    info->filename = event->name;
			    info->url = url;
			    info->credential= credentials;

			    pthread_t tid;
			    pthread_create(&tid, NULL, threadFunc, (void *)info);
			}
       		 }
      	}
    }
    i += EVENT_SIZE + event->len;
  }
}
  
  curl_global_cleanup();
  /*removing the “watchFolder” directory from the watch list.*/
  inotify_rm_watch( fd, wd );
  /*closing the INOTIFY instance*/
  close( fd );

}
