/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName： pagemap.h
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

#define _CRTDBG_MAP_ALLOC
//#define DEBUG
#include <stdlib.h>
#include <crtdbg.h>

#include "initialize.h"
#include "pagemap.h"
#include "ssd.h"
#include "flash.h"

/************************************************
*断言,当打开文件失败时，输出“open 文件名 error”
*************************************************/
void file_assert(int error,char *s)
{
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}

/*****************************************************
*断言,当申请内存空间失败时，输出“malloc 变量名 error”
******************************************************/
void alloc_assert(void *p,char *s)//断言
{
	if(p!=NULL) return;
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}

/*********************************************************************************
*断言
*A，读到的time_t，device，lsn，size，ope都<0时，输出“trace error:.....”
*B，读到的time_t，device，lsn，size，ope都=0时，输出“probable read a blank line”
**********************************************************************************/
void trace_assert(_int64 time_t,int device,unsigned int lsn,int size,int ope)//断言
{
	if(time_t <0 || device < 0 || lsn < 0 || size < 0 || ope < 0)
	{
		printf("trace error:%I64u %d %d %d %d\n",time_t,device,lsn,size,ope);
		getchar();
		exit(-1);
	}
	if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
	{
		printf("probable read a blank line\n");
		getchar();
	}
}

/*********************************************************************************
*断言
*假设对ppn进行规格化处理，每个channel的ppn数量由所有粒度域最小的粒度决定。
*大粒度域由于实际不存在这么多的ppn，因此存在一定数量的ppn并无真实映射
*例如：某ppn = 1001001，其存在于1K的粒度中是合法的，但是假如其映射到2K是不合法的。
*因为2k时所有的都指向二进制末尾都是0
**********************************************************************************/
void ppn_assert(int ppn, struct ssd_info *ssd)
{
	int ppn_value = ppn;
	int max_page_num = ssd->page[0];
	unsigned int channel_num = ppn_value / max_page_num;
	unsigned int i = 0, sum = 0, j = 0;
	int size = 0;
	int divsor = 2;


	for ( i = 0; i < ssd->parameter->granularity_num; i++)
	{	
		sum += ssd->parameter->granularity_channel[i];
		if (channel_num < sum)
		{
			size = ssd->parameter->granularity_size[i];
			break;
		}
	}

	if ((size/ssd->parameter->granularity_size[0] - 1) >= 0)
	{
		return ;
	}
	else
	{
		for (size_t i = 0; i < size/ssd->parameter->granularity_size[0]/2 - 2; i++)
		{
			printf("ppn error");
			getchar();
			exit(-1);
		}
	}
}




/************************************************************************************
*函数的功能是根据物理页号ppn查找该物理页所在的channel，chip，die，plane，block，page
*得到的channel，chip，die，plane，block，page放在结构location中并作为返回值
*************************************************************************************/
struct local *find_location(struct ssd_info *ssd,unsigned int ppn)//需要根据粒度进行调整
{
	struct local *location=NULL;
	int pn,ppn_value=ppn;
	int i=0,sum=0,mid = 0,j = 0,temp =0,ch=0;
    int granularity_num=0;
	int page_block[4],page_plane[4], page_die[4], page_chip[4],page_channel[32];
	pn = ppn;
	//ppn_assert(pn,ssd);
#ifdef DEBUG
	printf("enter find_location\n");
#endif

	location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
	memset(location,0, sizeof(struct local));
	for (i = 0;i < ssd->parameter->granularity_num;i++)
	{
		page_block[i] = (ssd->parameter->block_capacity / ssd->parameter->granularity_size[i]);
		page_plane[i] = page_block[i] * ssd->parameter->block_plane;
		page_die[i] = page_plane[i] * ssd->parameter->plane_die;
		page_chip[i] = page_die[i] * ssd->parameter->die_chip;
		for (j = 0;j < ssd->parameter->granularity_channel[i];j++)
		{
			
			page_channel[ch] = ssd->parameter->chip_channel[0] * page_chip[i];
			ch++;
		}
	}
	/*******************************************************************************
	*page_channel是一个channel中page的数目， ppn/page_channel就得到了在哪个channel中
	*用同样的办法可以得到chip，die，plane，block，page
	********************************************************************************/
	temp = ppn;
	int tempp = 0;
	for (i = 0; i < ssd->parameter->channel_number;i++)
	{
		tempp = temp;
		temp -= page_channel[i];
		if (temp < 0)
		{
			mid = i;
			
			break;
		}
		
	}
	


		for ( i = 0; i < ssd->parameter->granularity_num; i++)
	{
			if (mid >= ssd->parameter->granularity_channel[i])
			{
				mid -= ssd->parameter->granularity_channel[i];
				granularity_num++;
			}
			else
			{
				location->granularity_num = i;
				break;
			}
		
	}
	
	location->channel = mid;
	location->chip = (tempp)/page_chip[granularity_num];
	location->die = ((tempp)%page_chip[granularity_num])/page_die[granularity_num];
	location->plane = (((tempp)%page_chip[granularity_num])%page_die[granularity_num])/page_plane[granularity_num];
	location->block = ((((tempp)%page_chip[granularity_num])%page_die[granularity_num])%page_plane[granularity_num])/ page_block[granularity_num];
	location->page = (((((tempp)%page_chip[granularity_num])%page_die[granularity_num])%page_plane[granularity_num])% page_block[granularity_num]);

	return location;
}


/*****************************************************************************
*这个函数的功能是根据参数channel，chip，die，plane，block，page，找到该物理页号
*函数的返回值就是这个物理页号
******************************************************************************/
unsigned int find_ppn(struct ssd_info * ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
	unsigned int ppn=0;
	unsigned int i=0,j = 0,ch=0;
	int page_plane[4],page_die[4],page_chip[4];
	int page_channel[100];                  /*这个数组存放的是每个channel的page数目*/
	unsigned int channel_sum = 0;
	int page_block[4];

#ifdef DEBUG
	printf("enter find_psn, granularity:%d, channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n",granularity_num,channel,chip,die,plane,block,page);
#endif
    
	/*********************************************
	*计算出plane，die，chip，channel中的page的数目
	**********************************************/
	for ( i = 0; i < granularity_num; i++)
	{
		channel_sum += ssd->parameter->granularity_channel[i];
	}
	channel_sum += channel; 
	


	i = 0;
	for (i = 0;i <= granularity_num;i++)
	{
		page_block[i] = (ssd->parameter->block_capacity / ssd->parameter->granularity_size[i]);
		page_plane[i] = page_block[i] * ssd->parameter->block_plane;
		page_die[i] = page_plane[i] * ssd->parameter->plane_die;
		page_chip[i] = page_die[i] * ssd->parameter->die_chip;
		for (j = 0;j < ssd->parameter->granularity_channel[i];j++)
		{
			page_channel[ch] = ssd->parameter->chip_channel[0] * page_chip[i];
			ch++;
		}
	}


    /****************************************************************************
	*计算物理页号ppn，ppn是channel，chip，die，plane，block，page中page个数的总和
	*****************************************************************************/
	i=0;
	while(i<channel_sum)
	{
		ppn=ppn+page_channel[i];
		i++;
	}
	ppn=ppn+page_chip[granularity_num]*chip+page_die[granularity_num]*die+page_plane[granularity_num]*plane+block* page_block[granularity_num]+page;
	//ppn_assert(ppn,ssd);
	return ppn;
}

/********************************
*函数功能是获得一个读子请求的状态
*********************************/
int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size)
{	

	unsigned long temp,state,move;
	unsigned int channel, granularity,i=0;
	channel = lsn /  (ssd->parameter->chip_channel[0] * ssd->parameter->die_chip * ssd->parameter->plane_die * ssd->parameter->block_plane * (ssd->parameter->block_capacity / 512));
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		if (channel < ssd->parameter->granularity_channel[i])
		{
			
			granularity = i;
			break;
		}
		channel -= ssd->parameter->granularity_channel[i];

	}
	if (size == 32)
	{
		temp = 0xffffffff;
	}
	else
	{
		temp = ~(0xffffffff << size);
	}
	
	move=lsn%(ssd->parameter->granularity_size[granularity]/512);
	state=temp<<move;

	return state;
}

/**************************************************
*读请求预处理函数，当读请求所读得页里面没有数据时，
*需要预处理网该页里面写数据，以保证能读到数据
***************************************************/
struct ssd_info *pre_process_page(struct ssd_info *ssd)
{
	int fl=0;
	unsigned int device,lsn,size,ope,lpn,lpn1,lpn2,size2;
	unsigned long full_page;
	unsigned int granularity,channel,largest_lsn,sub_size,add_size=0;
	unsigned int i=0,j,k,r,m;
	unsigned long map_entry_new,map_entry_old,modify;
	unsigned long map_entry_new1, map_entry_old1, modify1;
	unsigned long map_entry_new2, map_entry_old2, modify2;
	int flag=0;
	char buffer_request[200];
	struct local *location;
	struct local *location1,*location2;
	__int64 time;
	errno_t err;
	unsigned int *ppn;
	ppn = (unsigned int*)malloc(2 * sizeof(unsigned int));
	unsigned int* lsnn;
	lsnn = (unsigned int*)malloc(2 * sizeof(unsigned int));
	printf("\n");
	printf("begin pre_process_page.................\n");

	if((err=fopen_s(&(ssd->tracefile),ssd->tracefilename,"r")) != 0 )      /*打开trace文件从中读取请求*/
	{
		printf("the trace file can't open\n");
		return NULL;
	}

	
	/*计算出这个ssd的最大逻辑扇区号*/
	largest_lsn= (ssd->parameter->channel_number*ssd->parameter->chip_channel[0]*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*(ssd->parameter->block_capacity/512))*(1-ssd->parameter->overprovide);

	while(fgets(buffer_request,200,ssd->tracefile))
	{
		sscanf_s(buffer_request,"%I64u %d %d %d %d",&time,&device,&lsn,&size,&ope);
		fl++;
		trace_assert(time,device,lsn,size,ope);                         /*断言，当读到的time，device，lsn，size，ope不合法时就会处理*/

		//printf("%I64u %d %d %d %d\n", time,device,lsn,size,ope);
		if (size == 583)
		{
			printf("ddd");
		}
		if (device == 1)
		{
			continue;
		}
		pre_avg_granu(ssd);
		add_size=0;                                                     /*add_size是这个请求已经预处理的大小*/
		lsn = lsn % largest_lsn;                                    /*防止获得的lsn比最大的lsn还大*/
		channel = lsn / (ssd->parameter->chip_channel[0] * ssd->parameter->die_chip * (ssd->parameter->plane_die * ssd->parameter->block_plane) * (ssd->parameter->block_capacity / 512));

		for (i = 0; i < ssd->parameter->granularity_num; i++)
		{
			if (channel < ssd->parameter->granularity_channel[i])
			{

				granularity = i;
				break;
			}
			channel -= ssd->parameter->granularity_channel[i];
		}
		sub_size = ssd->parameter->granularity_size[granularity] / 512;

		

		if(ope==1)                                                      /*这里只是读请求的预处理，需要提前将相应位置的信息进行相应修改*/
		{
			while(add_size<size)
			{				 

				if(add_size+sub_size>=size)                             /*只有当一个请求的大小小于一个page的大小时或者是处理一个请求的最后一个page时会出现这种情况*/
				{		
					sub_size=size-add_size;		
					add_size+=sub_size;		
				}
				if ((sub_size > ssd->parameter->granularity_size[granularity] / 512) || (add_size > size))/*当预处理一个子大小时，这个大小大于一个page或是已经处理的大小大于size就报错*/
				{
					printf("pre_process sub_size:%d\n", sub_size);
				}

				if (((ssd->parameter->advanced_commands & AD_TWOPLANE) == AD_TWOPLANE)&&((size-add_size)>sub_size)&&((size - add_size)>=ssd->parameter->granularity_channel[granularity]*ssd->parameter->granularity_size[granularity]))//满足高级命令且剩余的未分配的页大于一整个闪存页
				{
					lsnn[0] = lsn;
					lpn1 = lsn / (ssd->parameter->granularity_size[0] / 512) - (lsn / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);
					lsn = lsn + sub_size;  /*下个子请求的起始位置*/

					lsnn[1] = lsn;
					lpn2 = lsn / (ssd->parameter->granularity_size[0] / 512) - (lsn / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);
					if ((size - add_size) < 2 * sub_size)//如果剩余的两个页是一个整页加一个残页
					{
						size2 = size - add_size;
						add_size += sub_size;
					}
					else
					{
						size2 = sub_size;
					}
					if (ssd->dram->map->map_entry[lpn1].state == 0&& ssd->dram->map->map_entry[lpn2].state == 0)                 /*状态为0的情况*/
					{
						/**************************************************************
						*获得利用get_ppn_for_pre_process函数获得ppn，再得到location
						*修改ssd的相关参数，dram的映射表map，以及location下的page的状态
						***************************************************************/
						ppn = get_ppn_for_pre_process(ssd, lsnn, granularity, AD_TWOPLANE);

						location1 = find_location(ssd, *ppn);
						location2 = find_location(ssd, *(ppn+1));

						if (location1->granularity_num != 3)
							full_page = ~(0xffffffff << (ssd->parameter->granularity_size[location1->granularity_num] / 512));
						else
						{
							full_page = 0xffffffff;
						}
						ssd->program_count++;
						ssd->program_count_pre++;
						ssd->granularity_head[location1->granularity_num].progranm_count+=2;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].program_count+=2;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].program_count+=2;
						ssd->dram->map->map_entry[lpn1].pn = *ppn;
						ssd->dram->map->map_entry[lpn2].pn = *(ppn + 1);

						ssd->dram->map->map_entry[lpn1].state = set_entry_state(ssd, lsnn[0], sub_size);   //0001
						ssd->dram->map->map_entry[lpn2].state = set_entry_state(ssd, lsnn[1], size2);   //0001
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].die_head[location1->die].plane_head[location1->plane].blk_head[location1->block].page_head[location1->page].lpn = lpn1;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].die_head[location1->die].plane_head[location1->plane].blk_head[location1->block].page_head[location1->page].valid_state = ssd->dram->map->map_entry[lpn1].state;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].die_head[location1->die].plane_head[location1->plane].blk_head[location1->block].page_head[location1->page].free_state = ((~ssd->dram->map->map_entry[lpn1].state) & full_page);

						ssd->granularity_head[location2->granularity_num].channel_head[location2->channel].chip_head[location2->chip].die_head[location2->die].plane_head[location2->plane].blk_head[location2->block].page_head[location2->page].lpn = lpn2;
						ssd->granularity_head[location2->granularity_num].channel_head[location2->channel].chip_head[location2->chip].die_head[location2->die].plane_head[location2->plane].blk_head[location2->block].page_head[location2->page].valid_state = ssd->dram->map->map_entry[lpn2].state;
						ssd->granularity_head[location2->granularity_num].channel_head[location2->channel].chip_head[location2->chip].die_head[location2->die].plane_head[location2->plane].blk_head[location2->block].page_head[location2->page].free_state = ((~ssd->dram->map->map_entry[lpn2].state) & full_page);
						free(location1);
						location1 = NULL;

						free(location2);
						location2 = NULL;
					}
					else if (ssd->dram->map->map_entry[lpn1].state > 0|| ssd->dram->map->map_entry[lpn2].state > 0)           /*状态不为0的情况*/
					{
						map_entry_new1 = set_entry_state(ssd, lsnn[0], sub_size);      /*得到新的状态，并与原来的状态相或的到一个状态*/
						map_entry_old1 = ssd->dram->map->map_entry[lpn1].state;
						modify1 = map_entry_new1 | map_entry_old1;

						*ppn = ssd->dram->map->map_entry[lpn].pn;
						location1 = find_location(ssd, *ppn);

						map_entry_new2 = set_entry_state(ssd, lsnn[1], size2);      /*得到新的状态，并与原来的状态相或的到一个状态*/
						map_entry_old2 = ssd->dram->map->map_entry[lpn2].state;
						modify2 = map_entry_new2 | map_entry_old2;

						*(ppn + 1) = ssd->dram->map->map_entry[lpn2].pn;
						location2 = find_location(ssd, *(ppn + 1));

						if (location1->granularity_num != 3)
							full_page = ~(0xffffffff << (ssd->parameter->granularity_size[location1->granularity_num] / 512));
						else
						{
							full_page = 0xffffffff;
						}
						ssd->program_count+=2;
						ssd->program_count_pre+=2;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].program_count+=2;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].program_count+=2;
						ssd->dram->map->map_entry[lsnn[0] / (ssd->parameter->granularity_size[0] / 512)].state = modify1;
						ssd->dram->map->map_entry[lsnn[1] / (ssd->parameter->granularity_size[0] / 512)].state = modify2;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].die_head[location1->die].plane_head[location1->plane].blk_head[location1->block].page_head[location1->page].valid_state = modify1;
						ssd->granularity_head[location1->granularity_num].channel_head[location1->channel].chip_head[location1->chip].die_head[location1->die].plane_head[location1->plane].blk_head[location1->block].page_head[location1->page].free_state = ((~modify1) & full_page);

						ssd->granularity_head[location2->granularity_num].channel_head[location2->channel].chip_head[location2->chip].die_head[location2->die].plane_head[location2->plane].blk_head[location2->block].page_head[location2->page].valid_state = modify2;
						ssd->granularity_head[location2->granularity_num].channel_head[location2->channel].chip_head[location2->chip].die_head[location2->die].plane_head[location2->plane].blk_head[location2->block].page_head[location2->page].free_state = ((~modify2) & full_page);
						free(location1);
						location1 = NULL;

						free(location2);
						location2 = NULL;
					}//else if(ssd->dram->map->map_entry[lpn].state>0)
					lsn = lsn + sub_size;                                         /*下个子请求的起始位置*/
					add_size += sub_size+size2;                                       /*已经处理了的add_size大小变化*/
				}
				else
				{
					lpn = lsn / (ssd->parameter->granularity_size[0] / 512) - (lsn / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);
					if (ssd->dram->map->map_entry[lpn].state == 0)                 /*状态为0的情况*/
					{
						/**************************************************************
						*获得利用get_ppn_for_pre_process函数获得ppn，再得到location
						*修改ssd的相关参数，dram的映射表map，以及location下的page的状态
						***************************************************************/
						lsnn[0] = lsn;
						*ppn = get_ppn_for_pre_process(ssd, lsnn, granularity, NORMAL);

						location = find_location(ssd, *ppn);
						if (location->granularity_num != 3)
							full_page = ~(0xffffffff << (ssd->parameter->granularity_size[location->granularity_num] / 512));
						else
						{
							full_page = 0xffffffff;
						}
						ssd->program_count++;
						ssd->program_count_pre++;
						ssd->granularity_head[location->granularity_num].progranm_count++;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].program_count++;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].program_count++;
						ssd->dram->map->map_entry[lpn].pn = *ppn;

						ssd->dram->map->map_entry[lpn].state = set_entry_state(ssd, lsn, sub_size);   //0001
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn = lpn;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state = ssd->dram->map->map_entry[lpn].state;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state = ((~ssd->dram->map->map_entry[lpn].state) & full_page);

						free(location);
						location = NULL;
					}
					else if (ssd->dram->map->map_entry[lpn].state > 0)           /*状态不为0的情况*/
					{
						map_entry_new = set_entry_state(ssd, lsn, sub_size);      /*得到新的状态，并与原来的状态相或的到一个状态*/
						map_entry_old = ssd->dram->map->map_entry[lpn].state;
						modify = map_entry_new | map_entry_old;
						*ppn = ssd->dram->map->map_entry[lpn].pn;
						location = find_location(ssd, *ppn);
						if (location->granularity_num != 3)
							full_page = ~(0xffffffff << (ssd->parameter->granularity_size[location->granularity_num] / 512));
						else
						{
							full_page = 0xffffffff;
						}
						ssd->program_count++;
						ssd->program_count_pre++;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].program_count++;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].program_count++;
						ssd->dram->map->map_entry[lsn / (ssd->parameter->granularity_size[0] / 512)].state = modify;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state = modify;
						ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state = ((~modify) & full_page);

						free(location);
						location = NULL;
					}//else if(ssd->dram->map->map_entry[lpn].state>0)
					lsn = lsn + sub_size;                                         /*下个子请求的起始位置*/
					add_size += sub_size;                                       /*已经处理了的add_size大小变化*/
				}


				
                /*******************************************************************************************************
				*利用逻辑扇区号lsn计算出逻辑页号lpn
				*判断这个dram中映射表map中在lpn位置的状态
				*A，这个状态==0，表示以前没有写过，现在需要直接将ub_size大小的子页写进去写进去
				*B，这个状态>0，表示，以前有写过，这需要进一步比较状态，因为新写的状态可以与以前的状态有重叠的扇区的地方
				********************************************************************************************************/

			}//while(add_size<size)
		}//if(ope==1) 
	}	

	printf("\n");
	printf("pre_process is complete!\n");

	fclose(ssd->tracefile);
	for (r = 0; r < ssd->parameter->granularity_num; r++)
	for(i=0;i<ssd->parameter->granularity_channel[r];i++)
	for (m = 0; m < ssd->parameter->die_chip; m++)
    for(j=0;j<ssd->parameter->die_chip;j++)
	for(k=0;k<ssd->parameter->plane_die;k++)
	{
		fprintf(ssd->outputfile,"granularity:%d,channel:%d,chip:%d,die:%d,plane:%d have free page: %d\n",r,i,m,j,k,ssd->granularity_head[r].channel_head[i].chip_head[m].die_head[j].plane_head[k].free_page);				
		fflush(ssd->outputfile);
	}
	
	
	
	return ssd;
}

/**************************************
*函数功能是为预处理函数获取物理页号ppn
*获取页号分为动态获取和静态获取
**************************************/
unsigned int * get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int *lsn,unsigned int granularity,unsigned int command)     //能否直接用全局channel与粒度域中的channel之间转换
{

	unsigned int i=0;
	
	unsigned int pppn,channel=0,chip=0,die=0,plane=0; 
	unsigned int lpn;
	unsigned int active_block;
	unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;

	unsigned int *ppn;
	ppn = (unsigned int*)malloc(2 * sizeof(unsigned int*));
#ifdef DEBUG
	printf("enter get_psn_for_pre_process\n"); 
#endif
	lpn = (lsn[0] / (ssd->parameter->granularity_size[0] / 512)) - (lsn[0] / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);

	chip_num=ssd->parameter->chip_channel[0];
	die_num=ssd->parameter->die_chip;
	plane_num=ssd->parameter->plane_die;
	if (command == NORMAL)
	{
		if (ssd->parameter->allocation_scheme == 0)                           /*动态方式下获取ppn*/
		{
			if (ssd->parameter->dynamic_allocation == 0)                      /*表示全动态方式下，也就是channel，chip，die，plane，block等都是动态分配*/
			{
				channel = ssd->granularity_head[granularity].token;
				ssd->granularity_head[granularity].token = (ssd->granularity_head[granularity].token + 1) % ssd->parameter->granularity_channel[granularity];
				//channel = pre_least_channel(ssd,granularity);
				chip = ssd->granularity_head[granularity].channel_head[channel].token;
				ssd->granularity_head[granularity].channel_head[channel].token = (chip + 1) % ssd->parameter->chip_channel[0];
				die = ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].token;
				ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].token = (die + 1) % ssd->parameter->die_chip;
				plane = ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].die_head[die].token;
				ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].die_head[die].token = (plane + 1) % ssd->parameter->plane_die;
			}
		}

		if (find_active_block(ssd, granularity, channel, chip, die, plane) == FAILURE)
		{
			printf("the read operation is expand the capacity of SSD\n");
			return NULL;
		}
		active_block = ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
		if (write_page(ssd, granularity, channel, chip, die, plane, active_block, &pppn) == ERROR)
		{
			return NULL;
		}

		return pppn;
	}
	else if(command == AD_TWOPLANE)//找到一个空闲die分别
	{
		if (ssd->parameter->allocation_scheme == 0)                           /*动态方式下获取ppn*/
		{
			if (ssd->parameter->dynamic_allocation == 0)                      /*表示全动态方式下，也就是channel，chip，die，plane，block等都是动态分配*/
			{
				channel = ssd->granularity_head[granularity].token;
				ssd->granularity_head[granularity].token = (ssd->granularity_head[granularity].token + 1) % ssd->parameter->granularity_channel[granularity];
				//channel = pre_least_channel(ssd,granularity);
				chip = ssd->granularity_head[granularity].channel_head[channel].token;
				ssd->granularity_head[granularity].channel_head[channel].token = (chip + 1) % ssd->parameter->chip_channel[0];
				die = ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].token;
				ssd->granularity_head[granularity].channel_head[channel].chip_head[chip].token = (die + 1) % ssd->parameter->die_chip;

			}
		}

		ppn = find_level_page_pre(ssd, granularity,  channel,  chip,  die);

		return ppn;
	}

}


/***************************************************************************************************
*函数功能是在所给的channel，chip，die，plane里面找到一个active_block然后再在这个block里面找到一个页，
*再利用find_ppn找到ppn。
****************************************************************************************************/
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)
{
	int old_ppn=-1;
	unsigned int ppn,lpn;
	unsigned long full_page;
	unsigned int active_block;
	unsigned int block;
   	unsigned int page,flag=0,flag1=0;
	unsigned int old_state=0,state=0;
	struct local *location;
	struct direct_erase *direct_erase_node,*new_direct_erase;
	struct gc_operation *gc_node;

	unsigned int i=0,j=0,k=0,l=0,m=0,n=0;

#ifdef DEBUG
	printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
	if (ssd->parameter->granularity_size[granularity_num] == 16384)
	{
		full_page = 0xffffffff;
	}
	else
	{
		full_page = ~(0xffffffff << (ssd->parameter->granularity_size[granularity_num] / 512));
	}
	
	lpn=sub->lpn;

	/*************************************************************************************
	*利用函数find_active_block在channel，chip，die，plane找到活跃block
	*并且修改这个channel，chip，die，plane，active_block下的last_write_page和free_page_num
	**************************************************************************************/
	if(find_active_block(ssd,granularity_num,channel,chip,die,plane)==FAILURE)                      
	{
		printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);	
		return ssd;
	}

	active_block=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

	if(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]))
	{
		printf("error! the last write page larger than 64!!\n");
		while(1){}
		//exit(-1);
	}

	block=active_block;	
	page=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;	

	if(ssd->dram->map->map_entry[lpn].state==0)                                       /*this is the first logical page*/
	{
		if(ssd->dram->map->map_entry[lpn].pn!=0)
		{
			printf(" Error in get_ppn()\n");
		}
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,granularity_num,channel,chip,die,plane,block,page);

		ssd->dram->map->map_entry[lpn].state=sub->state;
	}
	else                                                                            /*这个逻辑页进行了更新，需要将原来的页置为失效*/
	{
		ppn=ssd->dram->map->map_entry[lpn].pn;
		location=find_location(ssd,ppn);
		if(	ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!=lpn)
		{
			printf("\n  Error in get_ppn()\n");
		}

		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;             /*表示某一页失效，同时标记valid和free状态都为0*/
		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;              /*表示某一页失效，同时标记valid和free状态都为0*/
		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		
		/*******************************************************************************************
		*该block中全是invalid的页，可以直接删除，就在创建一个可擦除的节点，挂在location下的plane下面
		********************************************************************************************/
		if (ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]))    
		{
			new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
            alloc_assert(new_direct_erase,"new_direct_erase");
			memset(new_direct_erase,0, sizeof(struct direct_erase));

			new_direct_erase->block=location->block;
			new_direct_erase->next_node=NULL;
			direct_erase_node=ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
			if (direct_erase_node==NULL)
			{
				ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			} 
			else
			{
				new_direct_erase->next_node=ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
				ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			}
		}

		free(location);
		location=NULL;
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,granularity_num,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[lpn].state|sub->state);
	}

	
	sub->ppn=ssd->dram->map->map_entry[lpn].pn;                                      /*修改sub子请求的ppn，location等变量*/
	sub->location->granularity_num = granularity_num;
	sub->location->channel=channel;
	sub->location->chip=chip;
	sub->location->die=die;
	sub->location->plane=plane;
	sub->location->block=active_block;
	sub->location->page=page; 
	
	ssd->program_count_get++;
	ssd->program_count++;                                                           /*修改ssd的program_count,free_page等变量*/
	ssd->granularity_head[granularity_num].progranm_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].program_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].program_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=lpn;	
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;

	if (ssd->parameter->active_write==0)                                            /*如果没有主动策略，只采用gc_hard_threshold，并且无法中断GC过程*/
	{                                                                               /*如果plane中的free_page的数目少于gc_hard_threshold所设定的阈值就产生gc操作*/

		if (ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page<((ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num])*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
		{
			gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
			alloc_assert(gc_node,"gc_node");
			memset(gc_node,0, sizeof(struct gc_operation));

			gc_node->next_node=NULL;
			gc_node->chip=chip;
			gc_node->die=die;
			gc_node->plane=plane;
			gc_node->block=0xffffffff;
			gc_node->page=0;
			gc_node->state=GC_WAIT;
			gc_node->priority=GC_UNINTERRUPT;
			gc_node->next_node=ssd->granularity_head[granularity_num].channel_head[channel].gc_command;
			ssd->granularity_head[granularity_num].channel_head[channel].gc_command=gc_node;
			ssd->gc_request++;
			ssd->gc_num++;
		}
	} 

	return ssd;
}
/*****************************************************************************************
*这个函数功能是为gc操作寻找新的ppn，因为在gc操作中需要找到新的物理块存放原来物理块上的数据
*在gc中寻找新物理块的函数，不会引起循环的gc操作
******************************************************************************************/
 unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	unsigned int ppn;
	unsigned int active_block,block,page;

#ifdef DEBUG
	printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif

	if(find_active_block(ssd,granularity_num,channel,chip,die,plane)!=SUCCESS)
	{
		printf("\n\n Error int get_ppn_for_gc().\n");
		return 0xffffffff;
	}
    
	active_block=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

	if(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page> (ssd->parameter->block_capacity / ssd->parameter->granularity_size[granularity_num]))
	{
		printf("error! the last write page larger than 64!!\n");
		while(1){}
	}

	block=active_block;	
	page=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;	

	ppn=find_ppn(ssd,granularity_num,channel,chip,die,plane,block,page);
	ssd->program_count_gc++;
	ssd->program_count++;
	ssd->granularity_head[granularity_num].progranm_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].program_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].program_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;

	return ppn;

}

/*********************************************************************************************************************
* 朱志明 于2011年7月28日修改   
*函数的功能就是erase_operation擦除操作，把channel，chip，die，plane下的block擦除掉
*也就是初始化这个block的相关参数，eg：free_page_num=page_block，invalid_page_num=0，last_write_page=-1，erase_count++
*还有这个block下面的每个page的相关参数也要修改。
*********************************************************************************************************************/

Status erase_operation(struct ssd_info * ssd,unsigned int granularity_num,unsigned int channel ,unsigned int chip ,unsigned int die ,unsigned int plane ,unsigned int block)
{

	unsigned int i=0;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num=(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]);
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num=0;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=-1;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;

	for (i=0;i<(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]);i++)
	{
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state=PG_SUB;
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state=0;
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn=-1;
	}
	ssd->erase_count++;
	ssd->granularity_head[granularity_num].erase_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].erase_count++;			
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].erase_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page+=(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]);

	return SUCCESS;

}

/**************************************************************************************
*这个函数的功能是处理INTERLEAVE_TWO_PLANE，INTERLEAVE，TWO_PLANE，NORMAL下的擦除的操作。
***************************************************************************************/
Status erase_planes(struct ssd_info * ssd, unsigned int granularity_num,unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
	unsigned int die=0;
	unsigned int plane=0;
	unsigned int block=0;
	struct direct_erase *direct_erase_node=NULL;
	unsigned int block0=0xffffffff;
	unsigned int block1=0;

	if((ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node==NULL)||               
		((command!=INTERLEAVE_TWO_PLANE)&&(command!=INTERLEAVE)&&(command!=TWO_PLANE)&&(command!=NORMAL)))     /*如果没有擦除操作，或者command不对，返回错误*/           
	{
		return ERROR;
	}

	/************************************************************************************************************
	*处理擦除操作时，首先要传送擦除命令，这是channel，chip处于传送命令的状态，即CHANNEL_TRANSFER，CHIP_ERASE_BUSY
	*下一状态是CHANNEL_IDLE，CHIP_IDLE。
	*************************************************************************************************************/
	block1=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;
	
	ssd->granularity_head[granularity_num].channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->granularity_head[granularity_num].channel_head[channel].current_time=ssd->current_time;										
	ssd->granularity_head[granularity_num].channel_head[channel].next_state=CHANNEL_IDLE;	

	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;										
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

	if(command==INTERLEAVE_TWO_PLANE)                                       /*高级命令INTERLEAVE_TWO_PLANE的处理*/
	{
		for(die=0;die<ssd->parameter->die_chip;die++)
		{
			block0=0xffffffff;
			for (plane=0;plane<ssd->parameter->plane_die;plane++)
			{
				direct_erase_node=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
				if(direct_erase_node!=NULL)
				{
					
					block=direct_erase_node->block; 
					
					if(block0==0xffffffff)
					{
						block0=block;
					}
					else
					{
						if(block!=block0)
						{
							continue;
						}

					}
					ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
					erase_operation(ssd,granularity_num,channel,chip,die,plane,block);     /*真实的擦除操作的处理*/
					free(direct_erase_node);                               
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
				}

			}
		}

		ssd->interleave_mplane_erase_count++;                             /*发送了一个interleave two plane erase命令,并计算这个处理的时间，以及下一个状态的时间*/
		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+ (long long)18*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tWB;
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time- (long long)9*ssd->parameter->time_characteristics.tWC+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);

	}
	else if(command==INTERLEAVE)                                          /*高级命令INTERLEAVE的处理*/
	{
		for(die=0;die<ssd->parameter->die_chip;die++)
		{
			for (plane=0;plane<ssd->parameter->plane_die;plane++)
			{
				if(die==die1)
				{
					plane=plane1;
				}
				direct_erase_node=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
				if(direct_erase_node!=NULL)
				{
					block=direct_erase_node->block;
					ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
					erase_operation(ssd,granularity_num,channel,chip,die,plane,block);
					free(direct_erase_node);
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
					break;
				}	
			}
		}
		ssd->interleave_erase_count++;
		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;       
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);
	}
	else if(command==TWO_PLANE)                                          /*高级命令TWO_PLANE的处理*/
	{

		for(plane=0;plane<ssd->parameter->plane_die;plane++)
		{
			direct_erase_node=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node;
			if((direct_erase_node!=NULL))
			{
				block=direct_erase_node->block;
				if(block==block1)
				{
					ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node=direct_erase_node->next_node;
					erase_operation(ssd,channel,granularity_num,chip,die1,plane,block);
					free(direct_erase_node);
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
				}
			}
		}

		ssd->mplane_erase_conut++;
		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;      
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);
	}
	else if(command==NORMAL)                                             /*普通命令NORMAL的处理*/
	{
		direct_erase_node=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;
		block=direct_erase_node->block;
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node=direct_erase_node->next_node;
		free(direct_erase_node);
		direct_erase_node=NULL;
		erase_operation(ssd,granularity_num,channel,chip,die1,plane1,block);

		ssd->direct_erase_count++;
		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;       								
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);
	}
	else
	{
		return ERROR;
	}

	direct_erase_node=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;

	if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
	{
		return FAILURE; 
	}
	else
	{
		return SUCCESS;
	}
}



/*******************************************************************************************************************
*GC操作由某个plane的free块少于阈值进行触发，当某个plane被触发时，GC操作占据这个plane所在的die，因为die是一个独立单元。
*对一个die的GC操作，尽量做到四个plane同时erase，利用interleave erase操作。GC操作应该做到可以随时停止（移动数据和擦除
*时不行，但是间隙时间可以停止GC操作），以服务新到达的请求，当请求服务完后，利用请求间隙时间，继续GC操作。可以设置两个
*GC阈值，一个软阈值，一个硬阈值。软阈值表示到达该阈值后，可以开始主动的GC操作，利用间歇时间，GC可以被新到的请求中断；
*当到达硬阈值后，强制性执行GC操作，且此GC操作不能被中断，直到回到硬阈值以上。
*在这个函数里面，找出这个die所有的plane中，有没有可以直接删除的block，要是有的话，利用interleave two plane命令，删除
*这些block，否则有多少plane有这种直接删除的block就同时删除，不行的话，最差就是单独这个plane进行删除，连这也不满足的话，
*直接跳出，到gc_parallelism函数进行进一步GC操作。该函数寻找全部为invalid的块，直接删除，找到可直接删除的返回1，没有找
*到返回-1。
*********************************************************************************************************************/
int gc_direct_erase(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	unsigned int lv_die=0,lv_plane=0;                                                           /*为避免重名而使用的局部变量 Local variables*/
	unsigned int interleaver_flag=FALSE,muilt_plane_flag=FALSE;
	unsigned int normal_erase_flag=TRUE;

	struct direct_erase * direct_erase_node1=NULL;
	struct direct_erase * direct_erase_node2=NULL;

	direct_erase_node1=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
	if (direct_erase_node1==NULL)
	{
		return FAILURE;
	}
    
	/********************************************************************************************************
	*当能处理TWOPLANE高级命令时，就在相应的channel，chip，die中两个不同的plane找到可以执行TWOPLANE操作的block
	*并置muilt_plane_flag为TRUE
	*********************************************************************************************************/
	if((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)
	{	
		for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
		{
			direct_erase_node2=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
			if((lv_plane!=plane)&&(direct_erase_node2!=NULL))
			{
				if((direct_erase_node1->block)==(direct_erase_node2->block))
				{
					muilt_plane_flag=TRUE;
					break;
				}
			}
		}
	}

	/***************************************************************************************
	*当能处理INTERLEAVE高级命令时，就在相应的channel，chip找到可以执行INTERLEAVE的两个block
	*并置interleaver_flag为TRUE
	****************************************************************************************/
	if((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
	{
		for(lv_die=0;lv_die<ssd->parameter->die_chip;lv_die++)
		{
			if(lv_die!=die)
			{
				for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
				{
					if(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node!=NULL)
					{
						interleaver_flag=TRUE;
						break;
					}
				}
			}
			if(interleaver_flag==TRUE)
			{
				break;
			}
		}
	}
    
	/************************************************************************************************************************
	*A，如果既可以执行twoplane的两个block又有可以执行interleaver的两个block，那么就执行INTERLEAVE_TWO_PLANE的高级命令擦除操作
	*B，如果只有能执行interleaver的两个block，那么就执行INTERLEAVE高级命令的擦除操作
	*C，如果只有能执行TWO_PLANE的两个block，那么就执行TWO_PLANE高级命令的擦除操作
	*D，没有上述这些情况，那么就只能够执行普通的擦除操作了
	*************************************************************************************************************************/
	if ((muilt_plane_flag==TRUE)&&(interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
	{
		if(erase_planes(ssd,granularity_num,channel,chip,die,plane,INTERLEAVE_TWO_PLANE)==SUCCESS)
		{
			return SUCCESS;
		}
	} 
	else if ((interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
	{
		if(erase_planes(ssd,granularity_num,channel,chip,die,plane,INTERLEAVE)==SUCCESS)
		{
			return SUCCESS;
		}
	}
	else if ((muilt_plane_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
	{
		if(erase_planes(ssd,granularity_num,channel,chip,die,plane,TWO_PLANE)==SUCCESS)
		{
			return SUCCESS;
		}
	}

	if ((normal_erase_flag==TRUE))                              /*不是每个plane都有可以直接删除的block，只对当前plane进行普通的erase操作，或者只能执行普通命令*/
	{
		if (erase_planes(ssd,granularity_num,channel,chip,die,plane,NORMAL)==SUCCESS)
		{
			return SUCCESS;
		} 
		else
		{
			return FAILURE;                                     /*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作*/
		}
	}
	return SUCCESS;
}


Status move_page(struct ssd_info * ssd, struct local *location, unsigned int * transfer_size)
{
	struct local *new_location=NULL;
	unsigned long free_state=0,valid_state=0;
	unsigned int lpn=0,old_ppn=0,ppn=0;

	lpn=ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
	valid_state=ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
	free_state=ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
	old_ppn=find_ppn(ssd,location->granularity_num,location->channel,location->chip,location->die,location->plane,location->block,location->page);      /*记录这个有效移动页的ppn，对比map或者额外映射关系中的ppn，进行删除和添加操作*/
	ppn=get_ppn_for_gc(ssd,location->granularity_num,location->channel,location->chip,location->die,location->plane);                /*找出来的ppn一定是在发生gc操作的plane中,才能使用copyback操作，为gc操作获取ppn*/

	new_location=find_location(ssd,ppn);                                                                   /*根据新获得的ppn获取new_location*/

	if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
	{
		if (ssd->parameter->greed_CB_ad==1)                                                                /*贪婪地使用高级命令*/
		{
			ssd->copy_back_count++;
			ssd->gc_copy_back++;
			while (old_ppn%2!=ppn%2)
			{
				ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=0;
				ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=0;
				ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=0;
				ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].invalid_page_num++;
				
				free(new_location);
				new_location=NULL;

				ppn=get_ppn_for_gc(ssd,location->granularity_num,location->channel,location->chip,location->die,location->plane);    /*找出来的ppn一定是在发生gc操作的plane中，并且满足奇偶地址限制,才能使用copyback操作*/
				ssd->program_count--;
				ssd->write_flash_count--;
				ssd->waste_page_count++;
			}
			if(new_location==NULL)
			{
				new_location=find_location(ssd,ppn);
			}
			
			ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
			ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
			ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;
		} 
		else
		{
			if (old_ppn%2!=ppn%2)
			{
				(* transfer_size)+=size(valid_state);
			}
			else
			{

				ssd->copy_back_count++;
				ssd->gc_copy_back++;
			}
		}	
	} 
	else
	{
		(* transfer_size)+=size(valid_state);
	}
	ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
	ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
	ssd->granularity_head[new_location->granularity_num].channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;


	ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
	ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
	ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
	ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;

	if (old_ppn==ssd->dram->map->map_entry[lpn].pn)                                                     /*修改映射表*/
	{
		ssd->dram->map->map_entry[lpn].pn=ppn;
	}

	free(new_location);
	new_location=NULL;

	return SUCCESS;
}

/*******************************************************************************************************************************************
*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作，用在不能中断的gc操作中，成功删除一个块，返回1，没有删除一个块返回-1
*在这个函数中，不用考虑目标channel,die是否是空闲的,擦除invalid_page_num最多的block
********************************************************************************************************************************************/
int uninterrupt_gc(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)       
{

	unsigned int i=0,invalid_page=0;
	unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*记录失效页最多的块号*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;

	if(find_active_block(ssd,granularity_num,channel,chip,die,plane)!=SUCCESS)                                           /*获取活跃块*/
	{
		printf("\n\n Error in uninterrupt_gc().\n");
		return ERROR;
	}
	active_block=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

	invalid_page=0;
	transfer_size=0;
	block=-1;
	for(i=0;i<ssd->parameter->block_plane;i++)                                                           /*查找最多invalid_page的块号，以及最大的invalid_page_num*/
	{	

		if((active_block!=i)&&(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
		{				
			invalid_page=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
			block=i;						
		}
	}
	if (block==-1)
	{
		return 1;
	}

	//if(invalid_page<5)
	//{
		//printf("\ntoo less invalid page. \t %d\t %d\t%d\t%d\t%d\t%d\t\n",invalid_page,channel,chip,die,plane,block);
	//}
	unsigned int xxx = 0;
	if (ssd->parameter->granularity_size[granularity_num] != (16384))
		xxx = ~(0xffffffff << (ssd->parameter->granularity_size[granularity_num] / 512));
	else
	{
		xxx = 0xffffffff;
	}
	free_page=0;
	for(i=0;i<(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]);i++)		                                                     /*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/		
	{		
		if ((ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state & PG_SUB) == xxx)
		{

			free_page++;
		}
		if(free_page!=0)
		{
			printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t%d\t\n",free_page, granularity_num,channel,chip,die,plane);
		}
		if (granularity_num == 3 && channel == 3 && chip == 1 && die == 0 && plane == 1 && block == 17&&i==23)
		{
			printf("sss");
		}
		if(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/		
		{	
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));
			
			location->granularity_num=granularity_num;
			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block;
			location->page=i;
			move_page( ssd, location, &transfer_size);                                                   /*真实的move_page操作*/
			page_move_count++;

			free(location);	
			location=NULL;
		}				
	}
	erase_operation(ssd ,granularity_num ,channel ,chip , die,plane ,block);	                                              /*执行完move_page操作后，就立即执行block的擦除操作*/

	ssd->granularity_head[granularity_num].channel_head[channel].current_state=CHANNEL_GC;									
	ssd->granularity_head[granularity_num].channel_head[channel].current_time=ssd->current_time;										
	ssd->granularity_head[granularity_num].channel_head[channel].next_state=CHANNEL_IDLE;	
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			
	
	/***************************************************************
	*在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
	*channel下个状态时间的计算，以及chip下个状态时间的计算
	***************************************************************/
	if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
	{
		if (ssd->parameter->greed_CB_ad==1)
		{

			ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ Read_time(ssd, ssd->parameter->granularity_size[granularity_num]) +7*ssd->parameter->time_characteristics.tWC+Prog_time(ssd, ssd->parameter->granularity_size[granularity_num]));
			ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);
		} 
	} 
	else
	{

		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ Read_time(ssd, ssd->parameter->granularity_size[granularity_num]) +7*ssd->parameter->time_characteristics.tWC+Prog_time(ssd, ssd->parameter->granularity_size[granularity_num]))+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);

	}

	return 1;
}


/*******************************************************************************************************************************************
*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作，用在可以中断的gc操作中，成功删除一个块，返回1，没有删除一个块返回-1
*在这个函数中，不用考虑目标channel,die是否是空闲的
********************************************************************************************************************************************/
int interrupt_gc(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node)        
{
	unsigned int i,block,active_block,transfer_size,invalid_page=0;
	struct local *location;

	active_block=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	transfer_size=0;
	if (gc_node->block>=ssd->parameter->block_plane)
	{
		for(i=0;i<ssd->parameter->block_plane;i++)
		{			
			if((active_block!=i)&&(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
			{				
				invalid_page=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
				block=i;						
			}
		}
		gc_node->block=block;
	}

	if (ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num!=ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num])     /*还需要执行copyback操作*/
	{
		for (i=gc_node->page;i<ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num];i++)
		{
			if (ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].page_head[i].valid_state>0)
			{
				location=(struct local * )malloc(sizeof(struct local ));
				alloc_assert(location,"location");
				memset(location,0, sizeof(struct local));
			
				location->granularity_num=granularity_num;
				location->channel=channel;
				location->chip=chip;
				location->die=die;
				location->plane=plane;
				location->block=block;
				location->page=i;
				transfer_size=0;

				move_page( ssd, location, &transfer_size);

				free(location);
				location=NULL;

				gc_node->page=i+1;
				ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num++;
				ssd->granularity_head[granularity_num].channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->granularity_head[granularity_num].channel_head[channel].current_time=ssd->current_time;										
				ssd->granularity_head[granularity_num].channel_head[channel].next_state=CHANNEL_IDLE;	
				ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_state=CHIP_COPYBACK_BUSY;								
				ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
				ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;		

				if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
				{					
					ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ Read_time(ssd, granularity_num) +7*ssd->parameter->time_characteristics.tWC;
					ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+Prog_time(ssd, ssd->parameter->granularity_size[granularity_num]);
				} 
				else
				{	
					ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+((long long)7+ (long long)transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC+ Read_time(ssd, granularity_num) +(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC;
					ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+Prog_time(ssd, ssd->parameter->granularity_size[granularity_num]);
				}
				return 0;    
			}
		}
	}
	else
	{
		erase_operation(ssd,granularity_num ,channel ,chip, die,plane,gc_node->block);	

		ssd->granularity_head[granularity_num].channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
		ssd->granularity_head[granularity_num].channel_head[channel].current_time=ssd->current_time;										
		ssd->granularity_head[granularity_num].channel_head[channel].next_state=CHANNEL_IDLE;								
		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;

		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;							
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);

		return 1;                                                                      /*该gc操作完成，返回1，可以将channel上的gc请求节点删除*/
	}

	printf("there is a problem in interrupt_gc\n");
	return 1;
}

/*************************************************************
*函数的功能是当处理完一个gc操作时，需要把gc链上的gc_node删除掉
**************************************************************/
int delete_gc_node(struct ssd_info *ssd, unsigned int granularity_num ,unsigned int channel,struct gc_operation *gc_node)
{
	struct gc_operation *gc_pre=NULL;
	if(gc_node==NULL)                                                                  
	{
		return ERROR;
	}

	if (gc_node==ssd->granularity_head[granularity_num].channel_head[channel].gc_command)
	{
		ssd->granularity_head[granularity_num].channel_head[channel].gc_command=gc_node->next_node;
	}
	else
	{
		gc_pre=ssd->granularity_head[granularity_num].channel_head[channel].gc_command;
		while (gc_pre->next_node!=NULL)
		{
			if (gc_pre->next_node==gc_node)
			{
				gc_pre->next_node=gc_node->next_node;
				break;
			}
			gc_pre=gc_pre->next_node;
		}
	}
	free(gc_node);
	gc_node=NULL;
	ssd->gc_request--;
	return SUCCESS;
}

/***************************************
*这个函数的功能是处理channel的每个gc操作
****************************************/
Status gc_for_channel(struct ssd_info *ssd, unsigned int granularity_num,unsigned int channel)
{
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int chip,die,plane,flag_priority=0;
	unsigned int current_state=0, next_state=0;
	long long next_state_predict_time=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;
	
	/*******************************************************************************************
	*查找每一个gc_node，获取gc_node所在的chip的当前状态，下个状态，下个状态的预计时间
	*如果当前状态是空闲，或是下个状态是空闲而下个状态的预计时间小于当前时间，并且是不可中断的gc
	*那么就flag_priority令为1，否则为0
	********************************************************************************************/
	gc_node=ssd->granularity_head[granularity_num].channel_head[channel].gc_command;
	
	while (gc_node!=NULL)
	{
		
		current_state=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].current_state;
		next_state=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].next_state;
		next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
		if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))
		{
			ssd->count_valid++;
			if (gc_node->priority==GC_UNINTERRUPT)                                     /*这个gc请求是不可中断的，优先服务这个gc操作*/
			{
				
				flag_priority=1;
				break;
			}
		}
		gc_node=gc_node->next_node;
	}
	if (flag_priority!=1)                                                              /*没有找到不可中断的gc请求，首先执行队首的gc请求*/
	{
		
		gc_node=ssd->granularity_head[granularity_num].channel_head[channel].gc_command;
		while (gc_node!=NULL)
		{
	
			current_state=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].current_state;
			next_state=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].next_state;
			next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
			 /**********************************************
			 *需要gc操作的目标chip是空闲的，才可以进行gc操作
			 ***********************************************/
			if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))   
			{
				
				break;
			}
			gc_node=gc_node->next_node;
		}

	}
	if(gc_node==NULL)
	{
		
		return FAILURE;
	}

	chip=gc_node->chip;
	die=gc_node->die;
	plane=gc_node->plane;

	if (gc_node->priority==GC_UNINTERRUPT)
	{
		
		flag_direct_erase=gc_direct_erase(ssd,granularity_num,channel,chip,die,plane);
		if (flag_direct_erase!=SUCCESS)
		{
			
			flag_gc=uninterrupt_gc(ssd,granularity_num,channel,chip,die,plane);                         /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
			if (flag_gc==1)
			{
				delete_gc_node(ssd,granularity_num,channel,gc_node);
			}
		}
		else
		{
			delete_gc_node(ssd,granularity_num,channel,gc_node);
		}
		return SUCCESS;
	}
	/*******************************************************************************
	*可中断的gc请求，需要首先确认该channel上没有子请求在这个时刻需要使用这个channel，
	*没有的话，在执行gc操作，有的话，不执行gc操作
	********************************************************************************/
	else        
	{
		flag_invoke_gc=decide_gc_invoke(ssd,granularity_num,channel);                                  /*判断是否有子请求需要channel，如果有子请求需要这个channel，那么这个gc操作就被中断了*/

		if (flag_invoke_gc==1)
		{
			flag_direct_erase=gc_direct_erase(ssd,granularity_num,channel,chip,die,plane);
			if (flag_direct_erase==-1)
			{
				flag_gc=interrupt_gc(ssd,granularity_num,channel,chip,die,plane,gc_node);             /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
				if (flag_gc==1)
				{
					delete_gc_node(ssd,granularity_num,channel,gc_node);
				}
			}
			else if (flag_direct_erase==1)
			{
				delete_gc_node(ssd,granularity_num,channel,gc_node);
			}
			return SUCCESS;
		} 
		else
		{
			return FAILURE;
		}		
	}
}



/************************************************************************************************************
*flag用来标记gc函数是在ssd整个都是idle的情况下被调用的（1），还是确定了channel，chip，die，plane被调用（0）
*进入gc函数，需要判断是否是不可中断的gc操作，如果是，需要将一整块目标block完全擦除后才算完成；如果是可中断的，
*在进行GC操作前，需要判断该channel，die是否有子请求在等待操作，如果没有则开始一步一步的操作，找到目标
*块后，一次执行一个copyback操作，跳出gc函数，待时间向前推进后，再做下一个copyback或者erase操作
*进入gc函数不一定需要进行gc操作，需要进行一定的判断，当处于硬阈值以下时，必须进行gc操作；当处于软阈值以下时，
*需要判断，看这个channel上是否有子请求在等待(有写子请求等待就不行，gc的目标die处于busy状态也不行)，如果
*有就不执行gc，跳出，否则可以执行一步操作
************************************************************************************************************/
unsigned int gc(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel, unsigned int flag)
{
	unsigned int i,j;
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int flag_priority=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;
	
	if (flag==1)                                                                       /*整个ssd都是IDEL的情况*/
	{
		for (j = 0; j < ssd->parameter->granularity_num; j++)
		for (i=0;i<ssd->parameter->granularity_channel[j];i++)
		{
			flag_priority=0;
			flag_direct_erase=1;
			flag_gc=1;
			flag_invoke_gc=1;
			gc_node=NULL;
			gc_p=NULL;
			if((ssd->granularity_head[j].channel_head[i].current_state==CHANNEL_IDLE)||(ssd->granularity_head[j].channel_head[i].next_state==CHANNEL_IDLE&&ssd->granularity_head[j].channel_head[i].next_state_predict_time<=ssd->current_time))
			{
				if (ssd->granularity_head[j].channel_head[i].gc_command!=NULL)
				{
					gc_for_channel(ssd, j, i);
				}
			}
		}
		return SUCCESS;

	} 
	else                                                                               /*只需针对某个特定的channel，chip，die进行gc请求的操作(只需对目标die进行判定，看是不是idle）*/
	{
		if ((ssd->parameter->allocation_scheme==1)||((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==1)))
		{
			if ((ssd->granularity_head[granularity_num].channel_head[channel].subs_r_head!=NULL)||(ssd->granularity_head[granularity_num].channel_head[channel].subs_w_head!=NULL))    /*队列上有请求，先服务请求*/
			{
				return 0;
			}
		}
		
		gc_for_channel(ssd,granularity_num,channel);
		return SUCCESS;
	}
}



/**********************************************************
*判断是否有子请求血药channel，若果没有返回1就可以发送gc操作
*如果有返回0，就不能执行gc操作，gc操作被中断
***********************************************************/
int decide_gc_invoke(struct ssd_info *ssd, unsigned int granularity_num, unsigned int channel)      
{
	struct sub_request *sub;
	struct local *location;

	if ((ssd->granularity_head[granularity_num].channel_head[channel].subs_r_head==NULL)&&(ssd->granularity_head[granularity_num].channel_head[channel].subs_w_head==NULL))    /*这里查找读写子请求是否需要占用这个channel，不用的话才能执行GC操作*/
	{
		return 1;                                                                        /*表示当前时间这个channel没有子请求需要占用channel*/
	}
	else
	{
		if (ssd->granularity_head[granularity_num].channel_head[channel].subs_w_head!=NULL)
		{
			return 0;
		}
		else if (ssd->granularity_head[granularity_num].channel_head[channel].subs_r_head!=NULL)
		{
			sub=ssd->granularity_head[granularity_num].channel_head[channel].subs_r_head;
			while (sub!=NULL)
			{
				if (sub->current_state==SR_WAIT)                                         /*这个读请求是处于等待状态，如果他的目标die处于idle，则不能执行gc操作，返回0*/
				{
					location=find_location(ssd,sub->ppn);
					if ((ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].current_state==CHIP_IDLE)||((ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].next_state==CHIP_IDLE)&&
						(ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)))
					{
						free(location);
						location=NULL;
						return 0;
					}
					free(location);
					location=NULL;
				}
				else if (sub->next_state==SR_R_DATA_TRANSFER)
				{
					location=find_location(ssd,sub->ppn);
					if (ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)
					{
						free(location);
						location=NULL;
						return 0;
					}
					free(location);
					location=NULL;
				}
				sub=sub->next_node;
			}
		}
		return 1;
	}
}

