/*
 * log_execute.c
 *
 *  Created on: 2019年7月29日
 *      Author: huanggang
 */

#include "log.h"

static pthread_mutex_t g_usart_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 *	@berif	make log header
 *  @param[out]	header	save the header
 *	@param[in]	level	debug level
 *	@param[in]	tag		log tag
 *	@param[in]	path	file path
 *  @param[in]	line	file line
 *	@return		none
 */
static void log_mk_header(char *header,char *level,char *tag,char *path,int line){
    char *filename;
    time_t nSeconds;
    struct tm *pTM;

    time(&nSeconds);
    pTM = localtime(&nSeconds);

    filename = basename(path);
    sprintf(header,LOG_HEADER,
            level,
            tag,
            (pTM->tm_year + 1900),
            pTM->tm_mon,
            pTM->tm_mday,
            pTM->tm_hour,
            pTM->tm_min,
            pTM->tm_sec,
            filename,
            line);
}

#if CONFIG_LOG_FILE
/**
 *	@berif	get the free size of log partition
 *  @param[out]	free	save the free size,unit MB
 *	@return		int		operate state
 *	@retval		0		success
 *  @retval		-1		fail
 */
static int log_fs_free_size(unsigned int *free){
    int ret;
    struct statfs diskInfo; 
    ret = statfs(CONFIG_FILE_MOUNT, &diskInfo);   
    if(ret < 0){
        return -1;
    } 
    unsigned long long blocksize = diskInfo.f_bsize; 
    unsigned long long freeDisk = diskInfo.f_bfree * blocksize; 
    *free = freeDisk >> 20;
    return 0;
}

/**
 *	@berif		get log filename minmum and maximum (log_1.txt,log_2.txt) 
 *  @param[out]	min_id	save the minmum of filename
 *  @param[out]	max_id	save the maxmum of filename
 *	@return		none
 */
static void log_filename_scan(int *min_id,int *max_id){
    DIR *dir;
    struct dirent *ptr;
    int id,cnt = 0;
    char *filename;

    *min_id = 0x7fffffff;
    *max_id = 0;

    dir = opendir(CONFIG_FILE_PATH);
    if (dir) {
        while ((ptr = readdir(dir)) != NULL) {
            if(ptr->d_type == 8){ //file
                filename = basename(ptr->d_name);
                if(filename[0] == '.'){
                    continue;
                }
                sscanf(filename,CONFIG_FILE_PATTRN,&id);
                if(id > *max_id){
                    *max_id = id;
                }

                if(id < *min_id){
                    *min_id = id;
                }
                cnt++;
            }
        }
    }

    /** log path is empty */
    if(cnt == 0){
        *min_id = 0;
        *max_id = 0;
    }
#if CONFIG_LOG_DEBUG
    printf("debug : filename id (min,max) = (%d,%d)\n",*min_id,*max_id);
#endif
    closedir(dir);
}

/**
 *	@berif		get file size,unit KB
 *  @param[in]	filename	the path of file
 *	@return		int
 *	@retval		>0	the size of file
 *	@retval		0	fail
 */
static int log_file_size(char *filename) {
    struct stat buf;  
    if(stat(filename, &buf)<0) {  
        return 0;  
    }  
    return (unsigned long)buf.st_size;
}

/**
 *	@berif		delete log file from index min_id
 *  @param[in]	min_id	the start index of log file
 *  @param[in]	mb		the size that want to delete
 *	@return		none
 */
static void log_delete_file(int min_id,int mb) {
    char filename[128];
    int i = min_id,ret;
    unsigned int total;
    while(total < mb * 1024 * 1024) {
        sprintf(filename,CONFIG_FILE_FULLNAME,i);
        ret = log_file_size(filename);
        if(ret == 0){
            return;
        }
        remove(filename);
        total += ret;
        i++;
    }
    return;
}

/**
 *	@berif		get log file file descriptor
 *	@return		int	
 *	@retval		>0	log file fd
 *	@retval		<0	fail
 */
static int log_get_file_fd(void){
    int fd = -1;
    char filename[128]={0};
    int min_id,max_id;
    unsigned int free;

    log_filename_scan(&min_id,&max_id);

    log_fs_free_size(&free);
    if(free < CONFIG_FILE_FREE_MB){
        log_delete_file(min_id,CONFIG_FILE_FREE_MB);
    }
#if CONFIG_FILE_NUM > 0
    /** when the number of log file reach the CONFIG_FILE_NUM */
    else if((max_id-min_id+1) >= CONFIG_FILE_NUM) {
        /** delete the oldest log file */
        sprintf(filename,CONFIG_FILE_FULLNAME,min_id);
        remove(filename);
    }
#endif
    /** construct the new log filename */
    sprintf(filename,CONFIG_FILE_FULLNAME,max_id+1);
    /* open log file */
    if(fd < 0) {
        fd = open(filename,O_RDWR | O_CREAT , 0777);
        if(fd < 0){
            return -1;
        }
    }
    return fd;
}

/**
 *	@berif		output user log to file
 *  @param[in]	buffer	the start address of user log
 *  @param[in]	len		the length of user log
 *	@return		int	
 *	@retval		>0	success
 *	@retval		<0	fail
 */
static int log_write_file(char *buffer,int len){
    static int fd = 0;
    if(fd == 0){
        fd = log_get_file_fd();
        if(fd == 0){
            return -1;
        }
    }
    return write(fd,buffer,len);
}
#endif

#if CONFIG_LOG_TELNET
/** save telnet socket that can communicate with peer client */
static int g_telnet_sock = -1;
static pthread_t telnet_server; 

/**
 *	@berif		telnet server function
 *  @param[in]	arg		pthread argument
 *	@return		pointer	
 */
static void *log_telnet_server_task(void *arg) {
    static int sock = -1;
    int ret;
    int optval = 1;
    struct sockaddr_in name;
    char buf[4] = {0};

    if(sock > 0){
        return NULL;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("opening socket");
        return NULL;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(TELNET_PORT);
    if(bind(sock, (void*) &name, sizeof(name))) {
        perror("binding tcp socket");
        return NULL;
    }
    if(listen(sock, 1) == -1) {
        perror("listen");
        return NULL;
    }

    struct sockaddr cli_addr;
    int cli_len = sizeof(cli_addr);

    while(1) {
        /** wait for client to connect */
        g_telnet_sock = accept(sock, &cli_addr, &cli_len);
#if CONFIG_LOG_DEBUG
				printf("debug : peer client connect!\n");
#endif	
        while(1) {
            /** detect client whether disconnect ? */
            ret = read(g_telnet_sock,buf,sizeof(buf));
            if(ret <= 0){
#if CONFIG_LOG_DEBUG
				printf("debug : peer client close!\n");
#endif		
                close(g_telnet_sock);
                g_telnet_sock = -1;
                break;
            }
        }

    }
    return NULL;
}

/**
 *	@berif		create telnet server thread
 *	@return		none
 */
static int log_telnet_server_task_create(void){
    if(g_telnet_sock < 0) {
        pthread_create(&telnet_server,NULL,log_telnet_server_task,NULL);
    }
}
#endif

/**
 *	@berif		prepare for logging
 *	@return		none
 */
static void log_init(void){
    static int inited = 0;
    if(inited == 0) {
        if(access(CONFIG_FILE_PATH, 0)!=0)  {  
            if(mkdir(CONFIG_FILE_PATH, 0755)==-1) {   
                printf("mkdir %s error\n",CONFIG_FILE_PATH);   
                return;   
            }  
        }  
#if CONFIG_LOG_TELNET
        log_telnet_server_task_create();
#endif

		inited = 1;
    }
}

/**
 *	@berif		log user interface
 *	@param[in]	level	debug level
 *	@param[in]	tag		log tag
 *	@param[in]	path	file path
 *  @param[in]	line	file line
 *  @param[in]	format	print format
 *	@return		none
 */
void log_execute(char *level,char *tag,char *path,int line,char *format,...){
    char header[256] = {0};
    char body[256] = {0};
    char total[2048] = {0};

	log_init();

    pthread_mutex_lock(&g_usart_mutex);

    log_mk_header(header,level,tag,path,line);

    va_list ap;
    va_start(ap, format);
    vsprintf(body, format, ap);
    sprintf(total,"%s %s",header,body);
    va_end(ap);

#if CONFIG_LOG_FILE
    log_write_file(total,strlen(total));
#endif

#if CONFIG_LOG_TELNET
    if(g_telnet_sock > 0) {
        write(g_telnet_sock,total,strlen(total));
    }
#endif

#if CONFIG_LOG_CONSOLE
    puts(total);
#endif

    pthread_mutex_unlock(&g_usart_mutex);
}
