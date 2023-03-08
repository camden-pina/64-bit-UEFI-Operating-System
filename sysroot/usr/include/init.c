typedef unsigned long long size_t;

typedef struct
{
	unsigned long lock;
	unsigned long type;
	unsigned long owner;
	unsigned long recursion;
} pthread_mutex_t;

struct uthread
{
	struct uthread* uthread_pointer;
	size_t uthread_size;
	unsigned long uthread_flags;
	void* tls_master_mmap;
	size_t tls_master_size;
	size_t tls_master_align;
	void* tls_mmap;
	size_t tls_size;
	void* stack_mmap;
	size_t stack_size;
	void* arg_mmap;
	size_t arg_size;
	size_t __uthread_reserved[4];
};


struct pthread_t
{
	struct uthread uthread;
	pthread_mutex_t join_lock;
	pthread_mutex_t detach_lock;
	void* (*entry_function)(void*);
	void* entry_cookie;
	void* exit_result;
	void** keys;
	size_t keys_length;
	int detach_state;
};

struct pthread_t* pthread_self(void)
{
	struct pthread_t* current_thread;
	asm ("mov %%fs:0, %0" : "=r"(current_thread));
	return current_thread;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
	while (!__sync_bool_compare_and_swap(&mutex->lock, UNLOCKED_VALUE, LOCKED_VALUE))
	{
		if (mutex->type == PTHREAD_MUTEX_RECURSIVE &&
			(struct pthread_t*) mutex->owner == pthread_self())
				return mutex->recursion++, 0;
			sched_yield();
	}
	
	mutex->owner = (unsigned long)pthread_self();
	mutex->recursion = 0;
	return 0;
}

char* program_invocation_name;	// program we are running
char* program_invocation_short_name;

size_t strlen(const char* str)
{
	size_t ret = 0;
	while ( str[ret] )
		ret++;
	return ret;
}

static char* find_last_elem(char* str)
{
	size_t len = strlen(str);
	for (size_t i = len; i; i--)
		if (str[i-1] == '/')
			return str + i;
	return str;
}

#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_NORMAL_MUTEX_INITIALIZER_NP { 0, PTHREAD_MUTEX_NORMAL, 0, 0 }

#define PTHREAD_CREATE_JOINABLE 0

static void exit_file(FILE* fp)
{
	if ( !fp )
		return;
	flockfile(fp);
	if ( fp->fflush_indirect )
		fp->fflush_indirect(fp);
	if ( fp->close_func )
		fp->close_func(fp->user);
}

int exit_thread(int status, int flags, const struct exit_thread* extended)
{
	return sys_exit_thread(status, flags, extended);
}

void _exit(int status)
{
	int exit_code = 1;
	exit_thread(exit_code, EXIT_THREAD_PROCESS, NULL);
	__builtin_unreachable();
}

void exit(int status)
{
	pthread_mutex_lock(&exit_lock);
	
	if (currently_exiting)
		_exit(status);
	currently_exiting = true;
	
	while (__exit_handler_stack)
	{
		__exit_handler_stack->hook(status, __exit_handler_sack->param);
		__exit_handler_stack = __exit_handerl_stack->next;
	}
	
	pthread_mutex_lock(&__first_file_lock);
	
	exit_file(__stdin_used);
	exit_file(__stdout_used);
	
	for (FILE* fp = __first_file; fp; fp = fp->next)
		exit_file(fp);
	
	_exit(status);
}

void initialize_standard_library(int argc, char* argv[], int envc, char* envp[])
{
	program_invocation_name = argv[0];	// store invocated program name
	program_invocation_short_name = find_last_elem((char*)argv[0]);	// suffix any directories
	
	struct pthread_t* self = pthread_self();
	self->join_lock = (pthread_mutex_t) PTHREAD_NORMAL_MUTEX_INITIALIZER_NP;
	self->join_lock.lock = 1;
	self->join_lock.type = PTHREAD_MUTEX_NORMAL;
	self->join_lock.owner = (unsigned long)self;
	self->detach_lock = (pthread_mutex_t)PTHREAD_NORMAL_MUTEX_INITIALIZER_NP;
	self->detach_state = PTHREAD_CREATE_JOINABLE;
}
