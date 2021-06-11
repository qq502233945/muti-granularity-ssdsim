/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º ssd.h
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
Zhiming Zhu     2012/07/19        2.1.1         Correct erase_planes()   812839842@qq.com  
*****************************************************************************************************************************/

int main();
struct ssd_info *simulate(struct ssd_info * ssd);
int get_requests(struct ssd_info * ssd);
struct ssd_info *buffer_management(struct ssd_info * ssd);
unsigned int lpn2ppn(struct ssd_info * ssd,unsigned int lsn);
struct ssd_info *distribute(struct ssd_info * ssd);
void trace_output(struct ssd_info* ssd);
void statistic_output(struct ssd_info * ssd);
unsigned int size(unsigned int ssd);
unsigned int transfer_size(struct ssd_info* ssd, unsigned long need_distribute, unsigned int lpn, struct request* req);
__int64 find_nearest_event(struct ssd_info * ssd);
void free_all_node(struct ssd_info * ssd);
struct ssd_info *make_aged(struct ssd_info * ssd);
struct ssd_info *no_buffer_distribute(struct ssd_info * ssd);
struct request* request_granularity(struct ssd_info* ssd, struct request* new_request);
unsigned int request_granularity_sub(struct ssd_info* ssd, struct sub_request* new_request);
unsigned int req_grannnum(struct ssd_info* ssd, unsigned int granu);
unsigned int easre_least_channle(struct ssd_info* ssd, unsigned int granu);
unsigned int easre_most_channle(struct ssd_info* ssd, unsigned int granu);
unsigned int easre_least_chip(struct ssd_info* ssd, unsigned int granu, unsigned int channel);
unsigned int balance_granularity(struct ssd_info* ssd);
unsigned int easre_least_granularity(struct ssd_info* ssd);
unsigned int easre_underleast_granularity(struct ssd_info* ssd, unsigned int granu);
unsigned int pre_least_channel(struct ssd_info* ssd, unsigned int granu);
unsigned int pre_least_granu(struct ssd_info* ssd);
unsigned int pre_avg_granu(struct ssd_info* ssd);
unsigned int need_map_size(struct ssd_info* ssd);