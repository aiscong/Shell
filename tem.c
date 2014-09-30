#include<stdio.h>
void test(int *a);
int main(){
  int * a;
  int b = 3;
  a = &b;
  printf("%d\n", *a);
  test(a);
  printf("%d\n", *a);

}

void test(int * a){
  *a = 5;
}
