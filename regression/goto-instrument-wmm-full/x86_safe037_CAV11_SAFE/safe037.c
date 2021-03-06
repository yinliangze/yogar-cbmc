void fence() { asm("sync"); }
void lwfence() { asm("lwsync"); }
void isync() { asm("isync"); }

int __unbuffered_cnt=0;
int __unbuffered_p0_EAX=0;
int __unbuffered_p1_EAX=0;
int __unbuffered_p2_EAX=0;
int x=0;
int y=0;
int z=0;

void * P0(void * arg) {
  z = 1;
  fence();
  __unbuffered_p0_EAX = x;
  // Instrumentation for CPROVER
  fence();
  __unbuffered_cnt++;
}

void * P1(void * arg) {
  x = 1;
  fence();
  __unbuffered_p1_EAX = y;
  // Instrumentation for CPROVER
  fence();
  __unbuffered_cnt++;
}

void * P2(void * arg) {
  y = 1;
  fence();
  __unbuffered_p2_EAX = z;
  // Instrumentation for CPROVER
  fence();
  __unbuffered_cnt++;
}

int main() {
  __CPROVER_ASYNC_0: P0(0);
  __CPROVER_ASYNC_1: P1(0);
  __CPROVER_ASYNC_2: P2(0);
  __CPROVER_assume(__unbuffered_cnt==3);
  fence();
  // EXPECT:exists
  __CPROVER_assert(!(__unbuffered_p0_EAX==0 && __unbuffered_p1_EAX==0 && __unbuffered_p2_EAX==0), "Program was expected to be safe for X86, model checker should have said NO.\nThis likely is a bug in the tool chain.");
  return 0;
}
