/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName?? flash.h
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
Zhiming Zhu     2012/07/19        2.1.1         Corr;ect erase_planes()   812839842@qq.com  
*****************************************************************************************************************************/

struct ssd_info *process(struct ssd_info * ssd);
struct ssd_info* insert2buffer(struct ssd_info* ssd, unsigned int lpn, unsigned long state, struct sub_request* sub, struct request* req);
int allocate_location(struct ssd_info* ssd, struct sub_request* sub_req, struct request* req);
struct ssd_info* flash_page_state_modify(struct ssd_info *ssd, struct sub_request* sub, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block, unsigned int page);
struct ssd_info* make_same_level(struct ssd_info* ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block, unsigned int aim_page);
int find_level_page(struct ssd_info* ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die, struct sub_request* subA, struct sub_request* subB);
unsigned int *find_level_page_pre(struct ssd_info* ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die);
int make_level_page(struct ssd_info * ssd, struct sub_request * sub0,struct sub_request * sub1);//Ϊsub1??????sub0??ͬλ?õ?ҳ
struct ssd_info *compute_serve_time(struct ssd_info *ssd, unsigned int granularity, unsigned int channel,unsigned int chip,unsigned int die,struct sub_request **subs,unsigned int subs_count,unsigned int command);
int get_ppn_for_advanced_commands(struct ssd_info *ssd, unsigned int granularity, unsigned int channel,unsigned int chip,struct sub_request * * subs ,unsigned int subs_count,unsigned int command);
int get_ppn_for_normal_command(struct ssd_info * ssd, unsigned int granularity, unsigned int channel,unsigned int chip,struct sub_request * sub);
struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd, unsigned int granularity, unsigned int channel,unsigned int chip);

//struct sub_request *find_two_plane_page(struct ssd_info *, struct sub_request *);
//struct sub_request *find_interleave_read_page(struct ssd_info *, struct sub_request *);
//int find_twoplane_write_sub_request(struct ssd_info * ssd, unsigned int granularity, unsigned int channel, struct sub_request * sub_twoplane_one,struct sub_request * sub_twoplane_two);
//int find_interleave_sub_request(struct ssd_info * ssd, unsigned int granularity, unsigned int channel, struct sub_request * sub_interleave_one,struct sub_request * sub_interleave_two);
struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die);
struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int granularity, unsigned int channel);
struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int ssize,unsigned long state,struct request * req,unsigned int operation, unsigned int granu, unsigned int balance, unsigned int aim_granu);

struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *onepage,unsigned int command);
struct sub_request_two *find_interleave_twoplane_sub_request(struct ssd_info * ssd, unsigned int granularity, unsigned int channel,struct sub_request * sub_request_one,unsigned int command);

struct ssd_info *delete_from_channel(struct ssd_info *ssd, unsigned int granularity, unsigned int channel,struct sub_request * sub_req);
struct ssd_info* un_greed_interleave_copyback(struct ssd_info* ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die, struct sub_request* sub1, struct sub_request* sub2);
struct ssd_info* un_greed_copyback(struct ssd_info* ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die, struct sub_request* sub1);
int  find_active_block(struct ssd_info *ssd, unsigned int granularity, unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);
int write_page(struct ssd_info *ssd, unsigned int granularity, unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn);


int go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command);
int services_2_r_cmd_trans_and_complete(struct ssd_info * ssd);
int services_2_r_wait(struct ssd_info * ssd, unsigned int granularity, unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);

int services_2_r_data_trans(struct ssd_info * ssd, unsigned int granularity, unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);
int services_2_write(struct ssd_info * ssd, unsigned int granularity, unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);
int delete_w_sub_request(struct ssd_info * ssd, unsigned int granularity, unsigned int channel, struct sub_request * sub );
int copy_back(struct ssd_info * ssd, unsigned int granularity, unsigned int channel, unsigned int chip, unsigned int die,struct sub_request * sub);
int static_write(struct ssd_info * ssd, unsigned int granularity, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);


