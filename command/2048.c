#include "stdio.h"

//游戏运行库

unsigned int _seed2 = 0xDEADBEEF;

void srand(unsigned int seed){
    _seed2 = seed;
}

int rand() {
    unsigned int next = _seed2;
    unsigned int result;

    next *= 1103515245;
    next += 12345;
    result = ( unsigned int  ) ( next / 65536 ) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;

    _seed2 = next;

    return result;
}

int mat_init(int *mat);

int mat_left(int *mat);
int mat_right(int *mat);
int mat_up(int *mat);
int mat_down(int *mat);

int mat_reach(int *mat);
int mat_insert(int *mat);
void mat_print(int *mat);

int main(){
    int mat[16] = {0};
    int state = 0;
    char keys[128];
    printf("**************************************\n");
    printf("*               2048                 *\n");
    printf("**************************************\n");
    printf("*  1. Enter a to go left             *\n");
    printf("*  2. Enter w to go up               *\n");
    printf("*  3. Enter s to go down             *\n");
    printf("*  4. Enter d to go right            *\n");
    printf("*  5. Enter q to quit                *\n");
    printf("**************************************\n");
    while(1){
        printl("Init Matrix\n");
        mat_init(mat);

        while(1){
            printf("type in the direction(w a s d):");
            ClearArr(keys, 128);
            int r = read(0, keys, 128);

            if (strcmp(keys, "a") == 0)
            {
                state = mat_left(mat);
            }
            else if (strcmp(keys, "s") == 0)
            {
                state=mat_down(mat);
            }
            else if (strcmp(keys, "w") == 0)
            {
                state=mat_up(mat);
            }
            else if (strcmp(keys, "d") == 0)
            {
                state=mat_right(mat);
            }
            else if (strcmp(keys, "q") == 0)
            {
                return 0;
            }
            else
            {
                printl("Input Invalid, Please retry\n");
                continue;
            }

            if(state==0){
                printl("can't add,try again!\n");
                continue;
            }
            if(mat_reach(mat)){
                printf("You Win\n");
                break;
            }
            if(!mat_insert(mat)){
                printf("You Lose\n");
                break;
            }
            mat_print(mat);
        }

        printf("another one?(y or n):");

        ClearArr(keys, 128);
        int r = read(0, keys, 128);
        if (strcmp(keys, "n"))
        {
            break;
        }
    }
    return 0;
}

int mat_init(int *mat)
{
    int i, j;
    //给一个随机数
    /*srand(546852);*/
    mat_insert(mat);
    mat_insert(mat);
    mat_print(mat);
    return 0;
}

int mat_left(int *mat){
    printl("Left\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=0,k=0,last=0;j<4;j++){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=j+1;
                k++;
            }
        }
        if(k<last) flag=1;
        for(j=0;j<3;j++){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=0,k=0;k<4;k++){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                j++;
            }
        }
    }
    return flag;
}

int mat_right(int *mat){
    printl("Right\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=3,k=3,last=3;j>=0;j--){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=j-1;
                k--;
            }
        }
        if(k>last) flag=1;
        for(j=3;j>=0;j--){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=3,k=3;k>=0;k--){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                j--;
            }
        }
    }
    return flag;
}

int mat_up(int *mat){
    printl("Up\n");

    int i,j;
    int flag=0;

    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=0,k=0,last=0;j<4;j++){
            if(mat[j*4+i]!=0){
                temp[k]=mat[j*4+i];
                mat[j*4+i]=0;
                last=j+1;
                k++;
            }
        }
        if(k<last) flag=1;
        for(j=0;j<3;j++){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=0,k=0;k<4;k++){
            if(temp[k]!=0){
                mat[j*4+i]=temp[k];
                j++;
            }
        }
    }
    return flag;
}

int mat_down(int *mat){
    printl("Down\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(j=0;j<4;j++){
        memset(temp,0,sizeof(int)*4);
        for(i=3,k=3,last=3;i>=0;i--){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=i-1;
                k--;
            }
        }
        if(k>last) flag=1;
        for(i=3;i>0;i--){
            if(temp[i]>0&&temp[i]==temp[i-1]){
                temp[i]+=temp[i];
                temp[i-1]=0;
                flag=1;
            }
        }
        for(i=3,k=3;k>=0;k--){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                i--;
            }
        }
    }
    return flag;
}

int mat_reach(int *mat){
    int i, j;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            if(mat[i*4+j] == 2048)
                return 1;
        }
    }
    return 0;
}

int mat_insert(int *mat){
    char temp[16] = {0};
    int i, j, k = 0;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            if(mat[i*4+j] == 0){
                temp[k] = 4 * i + j;
                k++;
            }
        }
    }
    if(k == 0) return 0;
    k = temp[rand() % k];
    //随便给一个地方2或者4
    mat[((k-k%4)/4)*4+k%4]=2<<(rand()%2);
    return 1;
}

void mat_print(int *mat){
    int i, j;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            //这里需要规格化
            printf("%4d", mat[i*4+j]);
        }
        printf("\n");
    }
}

void ClearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

