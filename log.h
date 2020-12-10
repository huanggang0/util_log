/*
 * log.h
 *
 *  Created on: 2019年7月29日
 *      Author: Administrator
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

#define CONFIG_LOG_FILE 	1
#define CONFIG_LOG_TELNET	1
#define CONFIG_LOG_CONSOLE	1
#define CONFIG_LOG_DEBUG	1

#if CONFIG_LOG_FILE
#define CONFIG_FILE_MOUNT		"/"
#define CONFIG_FILE_PATH		"/home/work/log/"
#define CONFIG_FILE_PATTRN		"log_%d.txt"
#define CONFIG_FILE_FULLNAME	"/home/work/log/log_%d.txt"
#define CONFIG_FILE_NUM			5
#define CONFIG_FILE_FREE_MB		200
#endif 

#if CONFIG_LOG_TELNET
#define MAX_TELNET_BUFFER   	512
#define TELNET_PORT				8899
#endif

#define LOG_HEADER	"[ %-6s ]  %-6s %04d-%02d-%02d %02d:%02d:%02d %-8s  line:%-3d - "

void log_execute(char *level,char *tag,char *path,int line,char *fomart,...);

#define LOG_INFO(tag,foramt,...) \
	log_execute("info",tag,__FILE__,__LINE__,foramt,##__VA_ARGS__)

#define LOG_ERROR(tag,foramt,...) \
	log_execute("error",tag,__FILE__,__LINE__,foramt,##__VA_ARGS__)

#endif /* LOG_H_ */
