#include "stdio.h"
  
//使用二维数组实现 扫雷  
int main()
{  
  
   char ui[8][8]={  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+',  
                  '+','+','+','+','+','+','+','+'  
                 };  
   int map[8][8]={  
                     0,0,0,0,0,0,0,0,  
                     0,0,1,0,0,1,0,0,  
                     0,0,0,0,1,0,0,0,  
                     0,0,0,0,0,1,0,0,  
                     0,0,1,0,0,0,0,0,  
                     0,0,1,0,0,0,0,0,  
                     0,1,0,1,1,0,0,0,  
                     1,0,0,0,0,0,0,0  
                     };  
   int p[8][2]={{-1,-1} ,{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};  
   int i=0,j=0;   
   int h=0,l=0; 
   int h1=0,l1=0;  
   int n=0;//用来保存 雷的数量 计数  
   int win=0;  
   char x[2]; char y[2];

   printf("**************************************\n");
   printf("*              saolei                *\n");
   printf("**************************************\n");
   printf("*  1. Enter 1-8 row number           *\n");
   printf("*  2. Enter 1-8 col number           *\n");
   printf("*  3. Enter q to quit                *\n");
   printf("**************************************\n\n");
 while(1)  
  {  
       printf("SAO LEI!!\n");  
         
          for(i=0;i<8;i++)  
            {  
              for(j=0;j<8;j++)  
                {  
                  printf("%c ",ui[i][j]);  
                }  
               printf("\n");  
           }  
  
         
         printf("please enter the row number:");  
         ClearArr(x, 2);
         int r = read(0, x, 2);
	 h = x[0]-49+1;
	 if(x[0] == 'q') break;
	 while(!(h >=0 && h <= 7)){printf("Wrong input!\nplease enter the row number:");r = read(0, x, 2);h = x[0]-49+1;}
         printf("please enter the col number:");
	 ClearArr(x, 2);
         int r1 = read(0, y, 2);
	 l = y[0]-49+1;
	 if(y[0] == 'q') break;
	 while(!(l >=0 && l <= 7)){printf("Wrong input!\nplease enter the col number:");r1 = read(0, y, 2);l = y[0]-49+1;}
         if(map[h-1][l-1]==1)  
          {  
              printf("BOOM SHAKALAKA! GAME OVER!\n");  
                break;  
          }  
         h=h-1;  
         l=l-1;   
           
          
           
         //没有踩到雷的情况  判断周围有几个雷 并把数字显示在 界面上  
         //-1 -1    -1, 0   -1,+1   0 ,-1   0 ,+1   +1 ,-1    +1,0   +1 ,+1   
        
        //n=map[h-1][l-1]+map[h-1][l]+map[h-1][l+1]+map[h][l-1]+map[h][l+1]+map[h+1][l-1]+map[h+1][l+1]+map[h+1][l];  
        i=0;  
        
        while(i<8)  
         {  
             n=0;  
             h1=h;  
             l1=l;   
             h1= h1+p[i][0];  
             l1=l1+p[i][1];  
             if(h1>=0&&h1<8&&l1>=0&&l1<8)  
               {  
                    if(map[h1][l1]==1)  
                        {  
                           n++;  
                        }    
               }  
          
             i++;  
          }  
             
         //把得到的数字显示到 界面上 ui[h][l];  
         //把int数字转换成 字符  
         switch(n)  
           {  
                case 0:  
                    ui[h][l]='0';  
                       break;  
                case 1:  
                    ui[h][l]='1';  
                       break;  
                case 2:  
                    ui[h][l]='2';  
                       break;  
                case 3:  
                    ui[h][l]='3';  
                       break;  
                case 4:  
                    ui[h][l]='4';  
                       break;  
                case 5:  
                    ui[h][l]='5';  
                       break;  
                case 6:  
                    ui[h][l]='6';  
                       break;  
                case 7:  
                    ui[h][l]='7';  
                       break;  
                case 8:  
                    ui[h][l]='8';  
                       break;  
           }  
           win++;  
           if(win==54)  
             {  
                printf("You win!\n");  
                   break;  
             }   
   }  
  return 0;  
} 

void ClearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}
