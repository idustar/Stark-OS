#include "stdio.h"

int main(int argc, char * argv[])
{
	int i, num1 = 0, num2 = 0, flag = 1, res = 0;
	char bufr[128];
	printf("***************************************************\n");
	printf("*               My Calculator                     *\n");
	printf("***************************************************\n");
	printf("*  1. Only two pararmeters                        *\n");
	printf("*  2. Only for integer                            *\n");
	printf("*  3. Enter q to quit                             *\n");
	printf("***************************************************\n\n");
	while(flag == 1){	
		printf("Please input num1:");
		i = read(0, bufr, 128);
		if (bufr[0] == 'q')
			break;
		num1 = getNum(bufr);
		printf("num1: %d\n", num1);
		printf("Please input num2:");
		i = read(0, bufr, 128);	
		if (bufr[0] == 'q')
			break;
		num2 = getNum(bufr);
		printf("num1: %d\n", num2);
		printf("Please input op( + - * / ):");
		i = read(0, bufr, 1);
		switch(bufr[0])
		{
			case '+':
				res = num1 + num2;
				printf("%d + %d = %d\n", num1, num2, res);
				break;
			case '-':
				res = num1 - num2;
				printf("%d - %d = %d\n", num1, num2, res);
				break;
			case '*':
				res = num1 * num2;
				break;
			case '/':
				if(num2 <= 0)
				{
					printf("Num2 = Zero!\n");
					break;
				}
				res = num1 / num2;
				printf("%d / %d = %d\n", num1, num2, res);
			case 'q':
				flag = 0;
				break;
			default:
				printf("No such command!\n");
		}
	}
	return 0;
}

int getNum(char * bufr)
{
	int ten = 1, i = 0, res = 0;
	for (i = 0; i < strlen(bufr) - 1; i++)
	{
		ten *= 10;
	}
	for (i = 0; i < strlen(bufr); i++)
	{
		res += (bufr[i] - '0') * ten;
		ten /= 10;
	}
	return res;
}
