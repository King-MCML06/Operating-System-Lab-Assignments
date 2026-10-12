#include "types.h"
#include "stat.h"
#include "user.h"
#define N 3
#define M 2
int alloc[N][M], req[N][M], avail[M];
int detect_deadlock(int n,int m,int a[N][M],int r[N][M],int v[M],int dead[N]){
  int work[M],finish[N],i,j,progress;
  for(j=0;j<m;j++) work[j]=v[j];
  for(i=0;i<n;i++) finish[i]=0;
  do { progress=0; for(i=0;i<n;i++) if(!finish[i]) {
    for(j=0;j<m && r[i][j]<=work[j];j++);
    if(j==m){ for(j=0;j<m;j++) work[j]+=a[i][j]; finish[i]=1; progress=1; }
  }} while(progress);
  {int count=0; for(i=0;i<n;i++){ dead[i]=!finish[i]; if(dead[i])count++; } return count;}
}
/* Banker safety check: Need=Max-Allocation; caller supplies a hypothetical
   request vector and requester. Grant only if request<=Need, request<=Available,
   and the resulting state admits a complete finish sequence. */
int is_safe(int n,int m,int a[N][M],int max[N][M],int v[M]){
  int work[M],done[N],i,j,progress;
  for(j=0;j<m;j++)work[j]=v[j]; for(i=0;i<n;i++)done[i]=0;
  do{progress=0;for(i=0;i<n;i++)if(!done[i]){for(j=0;j<m && max[i][j]-a[i][j]<=work[j];j++);
  if(j==m){for(j=0;j<m;j++)work[j]+=a[i][j];done[i]=1;progress=1;}}}while(progress);
  for(i=0;i<n;i++)if(!done[i])return 0; return 1;
}
void run(char *name,int a[N][M],int r[N][M],int v[M]){
  int dead[N],i,j,count; printf(1,"%s Available=[%d,%d]\n",name,v[0],v[1]);
  for(i=0;i<N;i++)printf(1,"P%d Allocation=[%d,%d] Request=[%d,%d]\n",i,a[i][0],a[i][1],r[i][0],r[i][1]);
  count=detect_deadlock(N,M,a,r,v,dead);printf(1,"unfinished/deadlocked:");
  if(!count)printf(1," none");for(i=0;i<N;i++)if(dead[i])printf(1," P%d",i);printf(1,"\n");
}
int main(void){int a[N][M],r[N][M],v[M];
  /* Deadlock: each process holds one of each resource class and requests the other. */
  a[0][0]=1;a[0][1]=0;r[0][0]=0;r[0][1]=1; a[1][0]=0;a[1][1]=1;r[1][0]=1;r[1][1]=0;a[2][0]=0;a[2][1]=0;r[2][0]=0;r[2][1]=0;v[0]=0;v[1]=0;run("cycle",a,r,v);
  /* Safe: one unit of each resource is free, so both allocations can finish. */
  a[0][0]=1;a[0][1]=0;r[0][0]=0;r[0][1]=1;a[1][0]=0;a[1][1]=1;r[1][0]=1;r[1][1]=0;a[2][0]=0;a[2][1]=0;r[2][0]=0;r[2][1]=0;v[0]=1;v[1]=1;run("safe",a,r,v);
  /* Unsafe after allocation: no resource remains, two processes each need the other. */
  v[0]=0;v[1]=0;run("unsafe after request",a,r,v);
  printf(1,"Banker safety for safe state: %s\n",is_safe(N,M,a,(int[N][M]){{2,1},{1,2},{0,0}},(int[M]){1,1})?"safe":"unsafe");
  exit();}
