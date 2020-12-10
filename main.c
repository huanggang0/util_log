#include "log.h"

int main(void){
    int cnt = 0;
    while(1) {
        LOG_INFO("hehe","ni hao %d\n",cnt);
        cnt++;
        sleep(1);
    }
}
