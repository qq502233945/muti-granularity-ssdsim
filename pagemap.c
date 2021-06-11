/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName�� pagemap.h
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
*����,�����ļ�ʧ��ʱ�������open �ļ��� error��
*************************************************/
void file_assert(int error,char *s)
{
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}

/*****************************************************
*����,�������ڴ�ռ�ʧ��ʱ�������malloc ������ error��
******************************************************/
void alloc_assert(void *p,char *s)//����
{
	if(p!=NULL) return;
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}

/*********************************************************************************
*����
*A��������time_t��device��lsn��size��ope��<0ʱ�������trace error:.....��
*B��������time_t��device��lsn��size��ope��=0ʱ�������probable read a blank line��
**********************************************************************************/
void trace_assert(_int64 time_t,int device,unsigned int lsn,int size,int ope)//����
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
*����
*�����ppn���й�񻯴���ÿ��channel��ppn������������������С�����Ⱦ�����
*������������ʵ�ʲ�������ô���ppn����˴���һ��������ppn������ʵӳ��
*���磺ĳppn = 1001001���������1K���������ǺϷ��ģ����Ǽ�����ӳ�䵽2K�ǲ��Ϸ��ġ�
*��Ϊ2kʱ���еĶ�ָ�������ĩβ����0
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
*�����Ĺ����Ǹ�������ҳ��ppn���Ҹ�����ҳ���ڵ�channel��chip��die��plane��block��page
*�õ���channel��chip��die��plane��block��page���ڽṹlocation�в���Ϊ����ֵ
*************************************************************************************/
struct local *find_location(struct ssd_info *ssd,unsigned int ppn)//��Ҫ�������Ƚ��е���
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
	*page_channel��һ��channel��page����Ŀ�� ppn/page_channel�͵õ������ĸ�channel��
	*��ͬ���İ취���Եõ�chip��die��plane��block��page
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
*��������Ĺ����Ǹ��ݲ���channel��chip��die��plane��block��page���ҵ�������ҳ��
*�����ķ���ֵ�����������ҳ��
******************************************************************************/
unsigned int find_ppn(struct ssd_info * ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
	unsigned int ppn=0;
	unsigned int i=0,j = 0,ch=0;
	int page_plane[4],page_die[4],page_chip[4];
	int page_channel[100];                  /*��������ŵ���ÿ��channel��page��Ŀ*/
	unsigned int channel_sum = 0;
	int page_block[4];

#ifdef DEBUG
	printf("enter find_psn, granularity:%d, channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n",granularity_num,channel,chip,die,plane,block,page);
#endif
    
	/*********************************************
	*�����plane��die��chip��channel�е�page����Ŀ
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
	*��������ҳ��ppn��ppn��channel��chip��die��plane��block��page��page�������ܺ�
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
*���������ǻ��һ�����������״̬
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
*������Ԥ����������������������ҳ����û������ʱ��
*��ҪԤ��������ҳ����д���ݣ��Ա�֤�ܶ�������
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

	if((err=fopen_s(&(ssd->tracefile),ssd->tracefilename,"r")) != 0 )      /*��trace�ļ����ж�ȡ����*/
	{
		printf("the trace file can't open\n");
		return NULL;
	}

	
	/*��������ssd������߼�������*/
	largest_lsn= (ssd->parameter->channel_number*ssd->parameter->chip_channel[0]*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*(ssd->parameter->block_capacity/512))*(1-ssd->parameter->overprovide);

	while(fgets(buffer_request,200,ssd->tracefile))
	{
		sscanf_s(buffer_request,"%I64u %d %d %d %d",&time,&device,&lsn,&size,&ope);
		fl++;
		trace_assert(time,device,lsn,size,ope);                         /*���ԣ���������time��device��lsn��size��ope���Ϸ�ʱ�ͻᴦ��*/

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
		add_size=0;                                                     /*add_size����������Ѿ�Ԥ����Ĵ�С*/
		lsn = lsn % largest_lsn;                                    /*��ֹ��õ�lsn������lsn����*/
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

		

		if(ope==1)                                                      /*����ֻ�Ƕ������Ԥ������Ҫ��ǰ����Ӧλ�õ���Ϣ������Ӧ�޸�*/
		{
			while(add_size<size)
			{				 

				if(add_size+sub_size>=size)                             /*ֻ�е�һ������Ĵ�СС��һ��page�Ĵ�Сʱ�����Ǵ���һ����������һ��pageʱ������������*/
				{		
					sub_size=size-add_size;		
					add_size+=sub_size;		
				}
				if ((sub_size > ssd->parameter->granularity_size[granularity] / 512) || (add_size > size))/*��Ԥ����һ���Ӵ�Сʱ�������С����һ��page�����Ѿ�����Ĵ�С����size�ͱ���*/
				{
					printf("pre_process sub_size:%d\n", sub_size);
				}

				if (((ssd->parameter->advanced_commands & AD_TWOPLANE) == AD_TWOPLANE)&&((size-add_size)>sub_size)&&((size - add_size)>=ssd->parameter->granularity_channel[granularity]*ssd->parameter->granularity_size[granularity]))//����߼�������ʣ���δ�����ҳ����һ��������ҳ
				{
					lsnn[0] = lsn;
					lpn1 = lsn / (ssd->parameter->granularity_size[0] / 512) - (lsn / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);
					lsn = lsn + sub_size;  /*�¸����������ʼλ��*/

					lsnn[1] = lsn;
					lpn2 = lsn / (ssd->parameter->granularity_size[0] / 512) - (lsn / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);
					if ((size - add_size) < 2 * sub_size)//���ʣ�������ҳ��һ����ҳ��һ����ҳ
					{
						size2 = size - add_size;
						add_size += sub_size;
					}
					else
					{
						size2 = sub_size;
					}
					if (ssd->dram->map->map_entry[lpn1].state == 0&& ssd->dram->map->map_entry[lpn2].state == 0)                 /*״̬Ϊ0�����*/
					{
						/**************************************************************
						*�������get_ppn_for_pre_process�������ppn���ٵõ�location
						*�޸�ssd����ز�����dram��ӳ���map���Լ�location�µ�page��״̬
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
					else if (ssd->dram->map->map_entry[lpn1].state > 0|| ssd->dram->map->map_entry[lpn2].state > 0)           /*״̬��Ϊ0�����*/
					{
						map_entry_new1 = set_entry_state(ssd, lsnn[0], sub_size);      /*�õ��µ�״̬������ԭ����״̬���ĵ�һ��״̬*/
						map_entry_old1 = ssd->dram->map->map_entry[lpn1].state;
						modify1 = map_entry_new1 | map_entry_old1;

						*ppn = ssd->dram->map->map_entry[lpn].pn;
						location1 = find_location(ssd, *ppn);

						map_entry_new2 = set_entry_state(ssd, lsnn[1], size2);      /*�õ��µ�״̬������ԭ����״̬���ĵ�һ��״̬*/
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
					lsn = lsn + sub_size;                                         /*�¸����������ʼλ��*/
					add_size += sub_size+size2;                                       /*�Ѿ������˵�add_size��С�仯*/
				}
				else
				{
					lpn = lsn / (ssd->parameter->granularity_size[0] / 512) - (lsn / (ssd->parameter->granularity_size[0] / 512)) % (ssd->parameter->granularity_size[granularity] / ssd->parameter->granularity_size[0]);
					if (ssd->dram->map->map_entry[lpn].state == 0)                 /*״̬Ϊ0�����*/
					{
						/**************************************************************
						*�������get_ppn_for_pre_process�������ppn���ٵõ�location
						*�޸�ssd����ز�����dram��ӳ���map���Լ�location�µ�page��״̬
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
					else if (ssd->dram->map->map_entry[lpn].state > 0)           /*״̬��Ϊ0�����*/
					{
						map_entry_new = set_entry_state(ssd, lsn, sub_size);      /*�õ��µ�״̬������ԭ����״̬���ĵ�һ��״̬*/
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
					lsn = lsn + sub_size;                                         /*�¸����������ʼλ��*/
					add_size += sub_size;                                       /*�Ѿ������˵�add_size��С�仯*/
				}


				
                /*******************************************************************************************************
				*�����߼�������lsn������߼�ҳ��lpn
				*�ж����dram��ӳ���map����lpnλ�õ�״̬
				*A�����״̬==0����ʾ��ǰû��д����������Ҫֱ�ӽ�ub_size��С����ҳд��ȥд��ȥ
				*B�����״̬>0����ʾ����ǰ��д��������Ҫ��һ���Ƚ�״̬����Ϊ��д��״̬��������ǰ��״̬���ص��������ĵط�
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
*����������ΪԤ��������ȡ����ҳ��ppn
*��ȡҳ�ŷ�Ϊ��̬��ȡ�;�̬��ȡ
**************************************/
unsigned int * get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int *lsn,unsigned int granularity,unsigned int command)     //�ܷ�ֱ����ȫ��channel���������е�channel֮��ת��
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
		if (ssd->parameter->allocation_scheme == 0)                           /*��̬��ʽ�»�ȡppn*/
		{
			if (ssd->parameter->dynamic_allocation == 0)                      /*��ʾȫ��̬��ʽ�£�Ҳ����channel��chip��die��plane��block�ȶ��Ƕ�̬����*/
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
	else if(command == AD_TWOPLANE)//�ҵ�һ������die�ֱ�
	{
		if (ssd->parameter->allocation_scheme == 0)                           /*��̬��ʽ�»�ȡppn*/
		{
			if (ssd->parameter->dynamic_allocation == 0)                      /*��ʾȫ��̬��ʽ�£�Ҳ����channel��chip��die��plane��block�ȶ��Ƕ�̬����*/
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
*������������������channel��chip��die��plane�����ҵ�һ��active_blockȻ���������block�����ҵ�һ��ҳ��
*������find_ppn�ҵ�ppn��
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
	*���ú���find_active_block��channel��chip��die��plane�ҵ���Ծblock
	*�����޸����channel��chip��die��plane��active_block�µ�last_write_page��free_page_num
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
	else                                                                            /*����߼�ҳ�����˸��£���Ҫ��ԭ����ҳ��ΪʧЧ*/
	{
		ppn=ssd->dram->map->map_entry[lpn].pn;
		location=find_location(ssd,ppn);
		if(	ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!=lpn)
		{
			printf("\n  Error in get_ppn()\n");
		}

		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;             /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;              /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
		ssd->granularity_head[location->granularity_num].channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		
		/*******************************************************************************************
		*��block��ȫ��invalid��ҳ������ֱ��ɾ�������ڴ���һ���ɲ����Ľڵ㣬����location�µ�plane����
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

	
	sub->ppn=ssd->dram->map->map_entry[lpn].pn;                                      /*�޸�sub�������ppn��location�ȱ���*/
	sub->location->granularity_num = granularity_num;
	sub->location->channel=channel;
	sub->location->chip=chip;
	sub->location->die=die;
	sub->location->plane=plane;
	sub->location->block=active_block;
	sub->location->page=page; 
	
	ssd->program_count_get++;
	ssd->program_count++;                                                           /*�޸�ssd��program_count,free_page�ȱ���*/
	ssd->granularity_head[granularity_num].progranm_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].program_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].program_count++;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=lpn;	
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;

	if (ssd->parameter->active_write==0)                                            /*���û���������ԣ�ֻ����gc_hard_threshold�������޷��ж�GC����*/
	{                                                                               /*���plane�е�free_page����Ŀ����gc_hard_threshold���趨����ֵ�Ͳ���gc����*/

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
*�������������Ϊgc����Ѱ���µ�ppn����Ϊ��gc��������Ҫ�ҵ��µ��������ԭ��������ϵ�����
*��gc��Ѱ���������ĺ�������������ѭ����gc����
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
* ��־�� ��2011��7��28���޸�   
*�����Ĺ��ܾ���erase_operation������������channel��chip��die��plane�µ�block������
*Ҳ���ǳ�ʼ�����block����ز�����eg��free_page_num=page_block��invalid_page_num=0��last_write_page=-1��erase_count++
*�������block�����ÿ��page����ز���ҲҪ�޸ġ�
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
*��������Ĺ����Ǵ���INTERLEAVE_TWO_PLANE��INTERLEAVE��TWO_PLANE��NORMAL�µĲ����Ĳ�����
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
		((command!=INTERLEAVE_TWO_PLANE)&&(command!=INTERLEAVE)&&(command!=TWO_PLANE)&&(command!=NORMAL)))     /*���û�в�������������command���ԣ����ش���*/           
	{
		return ERROR;
	}

	/************************************************************************************************************
	*�����������ʱ������Ҫ���Ͳ����������channel��chip���ڴ��������״̬����CHANNEL_TRANSFER��CHIP_ERASE_BUSY
	*��һ״̬��CHANNEL_IDLE��CHIP_IDLE��
	*************************************************************************************************************/
	block1=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;
	
	ssd->granularity_head[granularity_num].channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->granularity_head[granularity_num].channel_head[channel].current_time=ssd->current_time;										
	ssd->granularity_head[granularity_num].channel_head[channel].next_state=CHANNEL_IDLE;	

	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;										
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

	if(command==INTERLEAVE_TWO_PLANE)                                       /*�߼�����INTERLEAVE_TWO_PLANE�Ĵ���*/
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
					erase_operation(ssd,granularity_num,channel,chip,die,plane,block);     /*��ʵ�Ĳ��������Ĵ���*/
					free(direct_erase_node);                               
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
				}

			}
		}

		ssd->interleave_mplane_erase_count++;                             /*������һ��interleave two plane erase����,��������������ʱ�䣬�Լ���һ��״̬��ʱ��*/
		ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time=ssd->current_time+ (long long)18*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tWB;
		ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].next_state_predict_time- (long long)9*ssd->parameter->time_characteristics.tWC+ Bear_time(ssd, ssd->parameter->granularity_size[granularity_num]);

	}
	else if(command==INTERLEAVE)                                          /*�߼�����INTERLEAVE�Ĵ���*/
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
	else if(command==TWO_PLANE)                                          /*�߼�����TWO_PLANE�Ĵ���*/
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
	else if(command==NORMAL)                                             /*��ͨ����NORMAL�Ĵ���*/
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
*GC������ĳ��plane��free��������ֵ���д�������ĳ��plane������ʱ��GC����ռ�����plane���ڵ�die����Ϊdie��һ��������Ԫ��
*��һ��die��GC���������������ĸ�planeͬʱerase������interleave erase������GC����Ӧ������������ʱֹͣ���ƶ����ݺͲ���
*ʱ���У����Ǽ�϶ʱ�����ֹͣGC���������Է����µ�������󣬵��������������������϶ʱ�䣬����GC������������������
*GC��ֵ��һ������ֵ��һ��Ӳ��ֵ������ֵ��ʾ�������ֵ�󣬿��Կ�ʼ������GC���������ü�Ъʱ�䣬GC���Ա��µ��������жϣ�
*������Ӳ��ֵ��ǿ����ִ��GC�������Ҵ�GC�������ܱ��жϣ�ֱ���ص�Ӳ��ֵ���ϡ�
*������������棬�ҳ����die���е�plane�У���û�п���ֱ��ɾ����block��Ҫ���еĻ�������interleave two plane���ɾ��
*��Щblock�������ж���plane������ֱ��ɾ����block��ͬʱɾ�������еĻ��������ǵ������plane����ɾ��������Ҳ������Ļ���
*ֱ����������gc_parallelism�������н�һ��GC�������ú���Ѱ��ȫ��Ϊinvalid�Ŀ飬ֱ��ɾ�����ҵ���ֱ��ɾ���ķ���1��û����
*������-1��
*********************************************************************************************************************/
int gc_direct_erase(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	unsigned int lv_die=0,lv_plane=0;                                                           /*Ϊ����������ʹ�õľֲ����� Local variables*/
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
	*���ܴ���TWOPLANE�߼�����ʱ��������Ӧ��channel��chip��die��������ͬ��plane�ҵ�����ִ��TWOPLANE������block
	*����muilt_plane_flagΪTRUE
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
	*���ܴ���INTERLEAVE�߼�����ʱ��������Ӧ��channel��chip�ҵ�����ִ��INTERLEAVE������block
	*����interleaver_flagΪTRUE
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
	*A������ȿ���ִ��twoplane������block���п���ִ��interleaver������block����ô��ִ��INTERLEAVE_TWO_PLANE�ĸ߼������������
	*B�����ֻ����ִ��interleaver������block����ô��ִ��INTERLEAVE�߼�����Ĳ�������
	*C�����ֻ����ִ��TWO_PLANE������block����ô��ִ��TWO_PLANE�߼�����Ĳ�������
	*D��û��������Щ�������ô��ֻ�ܹ�ִ����ͨ�Ĳ���������
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

	if ((normal_erase_flag==TRUE))                              /*����ÿ��plane���п���ֱ��ɾ����block��ֻ�Ե�ǰplane������ͨ��erase����������ֻ��ִ����ͨ����*/
	{
		if (erase_planes(ssd,granularity_num,channel,chip,die,plane,NORMAL)==SUCCESS)
		{
			return SUCCESS;
		} 
		else
		{
			return FAILURE;                                     /*Ŀ���planeû�п���ֱ��ɾ����block����ҪѰ��Ŀ����������ʵʩ��������*/
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
	old_ppn=find_ppn(ssd,location->granularity_num,location->channel,location->chip,location->die,location->plane,location->block,location->page);      /*��¼�����Ч�ƶ�ҳ��ppn���Ա�map���߶���ӳ���ϵ�е�ppn������ɾ������Ӳ���*/
	ppn=get_ppn_for_gc(ssd,location->granularity_num,location->channel,location->chip,location->die,location->plane);                /*�ҳ�����ppnһ�����ڷ���gc������plane��,����ʹ��copyback������Ϊgc������ȡppn*/

	new_location=find_location(ssd,ppn);                                                                   /*�����»�õ�ppn��ȡnew_location*/

	if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
	{
		if (ssd->parameter->greed_CB_ad==1)                                                                /*̰����ʹ�ø߼�����*/
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

				ppn=get_ppn_for_gc(ssd,location->granularity_num,location->channel,location->chip,location->die,location->plane);    /*�ҳ�����ppnһ�����ڷ���gc������plane�У�����������ż��ַ����,����ʹ��copyback����*/
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

	if (old_ppn==ssd->dram->map->map_entry[lpn].pn)                                                     /*�޸�ӳ���*/
	{
		ssd->dram->map->map_entry[lpn].pn=ppn;
	}

	free(new_location);
	new_location=NULL;

	return SUCCESS;
}

/*******************************************************************************************************************************************
*Ŀ���planeû�п���ֱ��ɾ����block����ҪѰ��Ŀ����������ʵʩ�������������ڲ����жϵ�gc�����У��ɹ�ɾ��һ���飬����1��û��ɾ��һ���鷵��-1
*����������У����ÿ���Ŀ��channel,die�Ƿ��ǿ��е�,����invalid_page_num����block
********************************************************************************************************************************************/
int uninterrupt_gc(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)       
{

	unsigned int i=0,invalid_page=0;
	unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*��¼ʧЧҳ���Ŀ��*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;

	if(find_active_block(ssd,granularity_num,channel,chip,die,plane)!=SUCCESS)                                           /*��ȡ��Ծ��*/
	{
		printf("\n\n Error in uninterrupt_gc().\n");
		return ERROR;
	}
	active_block=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

	invalid_page=0;
	transfer_size=0;
	block=-1;
	for(i=0;i<ssd->parameter->block_plane;i++)                                                           /*�������invalid_page�Ŀ�ţ��Լ�����invalid_page_num*/
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
	for(i=0;i<(ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num]);i++)		                                                     /*������ÿ��page���������Ч���ݵ�page��Ҫ�ƶ��������ط��洢*/		
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
		if(ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*��ҳ����Чҳ����Ҫcopyback����*/		
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
			move_page( ssd, location, &transfer_size);                                                   /*��ʵ��move_page����*/
			page_move_count++;

			free(location);	
			location=NULL;
		}				
	}
	erase_operation(ssd ,granularity_num ,channel ,chip , die,plane ,block);	                                              /*ִ����move_page�����󣬾�����ִ��block�Ĳ�������*/

	ssd->granularity_head[granularity_num].channel_head[channel].current_state=CHANNEL_GC;									
	ssd->granularity_head[granularity_num].channel_head[channel].current_time=ssd->current_time;										
	ssd->granularity_head[granularity_num].channel_head[channel].next_state=CHANNEL_IDLE;	
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			
	
	/***************************************************************
	*�ڿ�ִ��COPYBACK�߼������벻��ִ��COPYBACK�߼���������������£�
	*channel�¸�״̬ʱ��ļ��㣬�Լ�chip�¸�״̬ʱ��ļ���
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
*Ŀ���planeû�п���ֱ��ɾ����block����ҪѰ��Ŀ����������ʵʩ�������������ڿ����жϵ�gc�����У��ɹ�ɾ��һ���飬����1��û��ɾ��һ���鷵��-1
*����������У����ÿ���Ŀ��channel,die�Ƿ��ǿ��е�
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

	if (ssd->granularity_head[granularity_num].channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num!=ssd->parameter->block_capacity/ssd->parameter->granularity_size[granularity_num])     /*����Ҫִ��copyback����*/
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

		return 1;                                                                      /*��gc������ɣ�����1�����Խ�channel�ϵ�gc����ڵ�ɾ��*/
	}

	printf("there is a problem in interrupt_gc\n");
	return 1;
}

/*************************************************************
*�����Ĺ����ǵ�������һ��gc����ʱ����Ҫ��gc���ϵ�gc_nodeɾ����
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
*��������Ĺ����Ǵ���channel��ÿ��gc����
****************************************/
Status gc_for_channel(struct ssd_info *ssd, unsigned int granularity_num,unsigned int channel)
{
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int chip,die,plane,flag_priority=0;
	unsigned int current_state=0, next_state=0;
	long long next_state_predict_time=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;
	
	/*******************************************************************************************
	*����ÿһ��gc_node����ȡgc_node���ڵ�chip�ĵ�ǰ״̬���¸�״̬���¸�״̬��Ԥ��ʱ��
	*�����ǰ״̬�ǿ��У������¸�״̬�ǿ��ж��¸�״̬��Ԥ��ʱ��С�ڵ�ǰʱ�䣬�����ǲ����жϵ�gc
	*��ô��flag_priority��Ϊ1������Ϊ0
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
			if (gc_node->priority==GC_UNINTERRUPT)                                     /*���gc�����ǲ����жϵģ����ȷ������gc����*/
			{
				
				flag_priority=1;
				break;
			}
		}
		gc_node=gc_node->next_node;
	}
	if (flag_priority!=1)                                                              /*û���ҵ������жϵ�gc��������ִ�ж��׵�gc����*/
	{
		
		gc_node=ssd->granularity_head[granularity_num].channel_head[channel].gc_command;
		while (gc_node!=NULL)
		{
	
			current_state=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].current_state;
			next_state=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].next_state;
			next_state_predict_time=ssd->granularity_head[granularity_num].channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
			 /**********************************************
			 *��Ҫgc������Ŀ��chip�ǿ��еģ��ſ��Խ���gc����
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
			
			flag_gc=uninterrupt_gc(ssd,granularity_num,channel,chip,die,plane);                         /*��һ��������gc�������ʱ���Ѿ�����һ���飬������һ��������flash�ռ䣩������1����channel����Ӧ��gc��������ڵ�ɾ��*/
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
	*���жϵ�gc������Ҫ����ȷ�ϸ�channel��û�������������ʱ����Ҫʹ�����channel��
	*û�еĻ�����ִ��gc�������еĻ�����ִ��gc����
	********************************************************************************/
	else        
	{
		flag_invoke_gc=decide_gc_invoke(ssd,granularity_num,channel);                                  /*�ж��Ƿ�����������Ҫchannel���������������Ҫ���channel����ô���gc�����ͱ��ж���*/

		if (flag_invoke_gc==1)
		{
			flag_direct_erase=gc_direct_erase(ssd,granularity_num,channel,chip,die,plane);
			if (flag_direct_erase==-1)
			{
				flag_gc=interrupt_gc(ssd,granularity_num,channel,chip,die,plane,gc_node);             /*��һ��������gc�������ʱ���Ѿ�����һ���飬������һ��������flash�ռ䣩������1����channel����Ӧ��gc��������ڵ�ɾ��*/
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
*flag�������gc��������ssd��������idle������±����õģ�1��������ȷ����channel��chip��die��plane�����ã�0��
*����gc��������Ҫ�ж��Ƿ��ǲ����жϵ�gc����������ǣ���Ҫ��һ����Ŀ��block��ȫ�����������ɣ�����ǿ��жϵģ�
*�ڽ���GC����ǰ����Ҫ�жϸ�channel��die�Ƿ����������ڵȴ����������û����ʼһ��һ���Ĳ������ҵ�Ŀ��
*���һ��ִ��һ��copyback����������gc��������ʱ����ǰ�ƽ���������һ��copyback����erase����
*����gc������һ����Ҫ����gc��������Ҫ����һ�����жϣ�������Ӳ��ֵ����ʱ���������gc����������������ֵ����ʱ��
*��Ҫ�жϣ������channel���Ƿ����������ڵȴ�(��д������ȴ��Ͳ��У�gc��Ŀ��die����busy״̬Ҳ����)�����
*�оͲ�ִ��gc���������������ִ��һ������
************************************************************************************************************/
unsigned int gc(struct ssd_info *ssd,unsigned int granularity_num,unsigned int channel, unsigned int flag)
{
	unsigned int i,j;
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int flag_priority=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;
	
	if (flag==1)                                                                       /*����ssd����IDEL�����*/
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
	else                                                                               /*ֻ�����ĳ���ض���channel��chip��die����gc����Ĳ���(ֻ���Ŀ��die�����ж������ǲ���idle��*/
	{
		if ((ssd->parameter->allocation_scheme==1)||((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==1)))
		{
			if ((ssd->granularity_head[granularity_num].channel_head[channel].subs_r_head!=NULL)||(ssd->granularity_head[granularity_num].channel_head[channel].subs_w_head!=NULL))    /*�������������ȷ�������*/
			{
				return 0;
			}
		}
		
		gc_for_channel(ssd,granularity_num,channel);
		return SUCCESS;
	}
}



/**********************************************************
*�ж��Ƿ���������Ѫҩchannel������û�з���1�Ϳ��Է���gc����
*����з���0���Ͳ���ִ��gc������gc�������ж�
***********************************************************/
int decide_gc_invoke(struct ssd_info *ssd, unsigned int granularity_num, unsigned int channel)      
{
	struct sub_request *sub;
	struct local *location;

	if ((ssd->granularity_head[granularity_num].channel_head[channel].subs_r_head==NULL)&&(ssd->granularity_head[granularity_num].channel_head[channel].subs_w_head==NULL))    /*������Ҷ�д�������Ƿ���Ҫռ�����channel�����õĻ�����ִ��GC����*/
	{
		return 1;                                                                        /*��ʾ��ǰʱ�����channelû����������Ҫռ��channel*/
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
				if (sub->current_state==SR_WAIT)                                         /*����������Ǵ��ڵȴ�״̬���������Ŀ��die����idle������ִ��gc����������0*/
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

