
#ifdef __cplusplus
extern "C" {
#endif
void np2_multithread_Suspend();
void np2_multithread_Resume();
void np2_multithread_EnterCriticalSection(); 
void np2_multithread_LeaveCriticalSection();
int np2_multithread_Enabled();
#ifdef __cplusplus
}
#endif